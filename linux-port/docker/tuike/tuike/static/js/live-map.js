/* The live world map on the dashboard.
 *
 * The map is continuous, not a slideshow. Each bot keeps its own element for
 * as long as it is on screen and is moved with a transform that eases over the
 * gap between two ticks, so a bot walking across Yongbi glides instead of
 * jumping every second. Rebuilding the whole layer each tick - which is what
 * this did before - threw away that motion, discarded hover and cost a full
 * relayout for a thousand dots.
 *
 * The feed matches: /api/live-map returns one map's positions as percentages
 * plus the world's totals, a few kilobytes, rather than every bot in the world
 * with all its labels.
 */
(() => {
  const stage = document.getElementById('world-map');
  if (!stage) return;

  const $ = (id) => document.getElementById(id);
  const mode = $('live-mode');
  const mapSelect = $('live-map');
  const search = $('bot-search');
  const showNames = $('show-names');
  const partyOnly = $('party-only');
  const autoplay = $('map-autoplay');
  const caption = $('map-caption');
  const count = $('live-count');

  const TICK = 1000;
  const STATUS_INTERVAL = 30000;
  const AUTOPLAY_INTERVAL = 8000;
  const IDLE_BEFORE_AUTOPLAY = 15000;
  const RANKING_SIZE = 10;
  const ACTIVITY_ROWS = 6;
  const FLAG_PARTY = 1;
  const FLAG_STUCK = 2;
  const FLAG_METIN = 4;

  // id -> {node, label, x, y, flags, level, name}
  const points = new Map();
  let snapshot = [];
  let leaderId = null;
  let levelFilter = 'all';
  let lastInteraction = Date.now();
  let missing = 0;

  const escape = (value) => String(value).replace(/[&<>"']/g, (character) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[character]));

  const inLevelBand = (level) => {
    if (levelFilter === 'all') return true;
    if (levelFilter.endsWith('+')) return level >= Number(levelFilter.slice(0, -1));
    const [low, high] = levelFilter.split('-').map(Number);
    return level >= low && level <= high;
  };

  const selectedOption = () => mapSelect.options[mapSelect.selectedIndex];

  function dressStage() {
    const option = selectedOption();
    stage.dataset.mapIndex = mapSelect.value;
    stage.style.setProperty('--aspect', option.dataset.aspect || '1 / 1');
    stage.style.backgroundImage = option.dataset.background
      ? `linear-gradient(#00000014, #00000014), url('${option.dataset.background}')`
      : 'none';
    caption.textContent = option.textContent.trim();
  }

  function clearPoints() {
    points.forEach((point) => point.node.remove());
    points.clear();
  }

  function clearHeat() {
    stage.querySelectorAll('.heat-point').forEach((node) => node.remove());
  }

  /* --- what a bot is busy with -------------------------------------------
   * The map feed carries positions, not goals, so activity is summarised from
   * what the map itself can see: fighting a Metin, in a party, stuck, or
   * simply out in the world. */
  function renderActivities(bots) {
    const box = $('live-activity');
    if (!box) return;
    const groups = [
      ['Walczy z Metinem', bots.filter((bot) => bot.flags & FLAG_METIN).length],
      ['Gra w grupie', bots.filter((bot) => (bot.flags & FLAG_PARTY) && !(bot.flags & FLAG_METIN)).length],
      ['Możliwie zawieszone', bots.filter((bot) => bot.flags & FLAG_STUCK).length],
      ['W drodze przez świat', bots.filter((bot) => !bot.flags).length],
    ].filter(([, number]) => number > 0).slice(0, ACTIVITY_ROWS);
    const total = bots.length || 1;
    box.innerHTML = groups.map(([label, number]) => `
      <div class="activity-row">
        <span>${escape(label)}</span><b>${number}</b>
        <i style="--share:${Math.max(4, Math.round(number / total * 100))}%"></i>
      </div>`).join('') || '<p class="muted">Brak aktywnych botów na tej mapie.</p>';
  }

  /* --- the moving layer --------------------------------------------------- */
  function pointClass(flags) {
    return 'bot-point'
      + ((flags & FLAG_PARTY) ? ' is-party' : '')
      + ((flags & FLAG_STUCK) ? ' is-stuck' : '')
      + ((flags & FLAG_METIN) ? ' is-metin' : '');
  }

  function pointTitle(bot) {
    return `${bot.name} · poziom ${bot.level}`
      + ((bot.flags & FLAG_PARTY) ? ' · w grupie' : '')
      + ((bot.flags & FLAG_STUCK) ? ' · możliwie zawieszony' : '')
      + ((bot.flags & FLAG_METIN) ? ' · walczy z Metinem' : '');
  }

  function place(point, bot, animate) {
    // The transition is set per move: a bot that has just appeared, or one the
    // feed skipped, must not slide in from wherever the last bot stood.
    point.node.style.transitionDuration = animate ? `${TICK + 120}ms` : '0ms';
    point.node.style.left = `${bot.x}%`;
    point.node.style.top = `${bot.y}%`;
  }

  function renderLive() {
    if (mode.value !== 'live') return;
    const needle = search.value.trim().toLowerCase();
    const visible = snapshot.filter((bot) => inLevelBand(bot.level)
      && (!partyOnly.checked || (bot.flags & FLAG_PARTY))
      && (!needle || bot.name.toLowerCase().includes(needle)));
    const seen = new Set();

    visible.forEach((bot) => {
      seen.add(bot.id);
      let point = points.get(bot.id);
      if (!point) {
        const node = document.createElement('a');
        node.className = pointClass(bot.flags);
        node.href = `/player/${bot.id}`;
        point = { node, flags: bot.flags, named: false };
        points.set(bot.id, point);
        stage.append(node);
        place(point, bot, false);
        // One frame later, so the browser has the starting position before the
        // transition is allowed to matter.
        requestAnimationFrame(() => place(point, bot, true));
      } else {
        place(point, bot, true);
      }
      if (point.flags !== bot.flags) {
        point.node.className = pointClass(bot.flags);
        point.flags = bot.flags;
      }
      point.node.title = pointTitle(bot);
      const wantsName = showNames.checked;
      if (wantsName !== point.named || (wantsName && point.name !== bot.name)) {
        point.node.innerHTML = wantsName ? `<em>${escape(bot.name)} (${bot.level})</em>` : '';
        point.named = wantsName;
        point.name = bot.name;
      }
    });

    points.forEach((point, id) => {
      if (seen.has(id)) return;
      point.node.remove();
      points.delete(id);
    });

    const average = visible.length
      ? (visible.reduce((sum, bot) => sum + bot.level, 0) / visible.length).toFixed(1) : '—';
    $('stat-visible').textContent = visible.length;
    $('stat-party').textContent = visible.filter((bot) => bot.flags & FLAG_PARTY).length;
    $('stat-average').textContent = average;
    $('stat-max').textContent = visible.length ? Math.max(...visible.map((bot) => bot.level)) : '—';
    count.textContent = `${visible.length} botów na mapie`;

    const ranked = [...visible]
      .sort((a, b) => b.level - a.level || a.name.localeCompare(b.name, 'pl'))
      .slice(0, RANKING_SIZE);
    $('live-ranking').innerHTML = ranked.map((bot, index) => `
      <a class="${bot.id === leaderId ? 'is-leader' : ''}" href="/player/${bot.id}">
        <b>#${index + 1}</b>${escape(bot.name)}
        ${(bot.flags & FLAG_PARTY) ? '<mark>PT</mark>' : ''}${(bot.flags & FLAG_STUCK) ? '<mark>⚠</mark>' : ''}
        <span>Lv ${bot.level}</span>
      </a>`).join('') || '<p class="muted">Brak botów spełniających filtr.</p>';

    renderActivities(visible);
  }

  function renderOverview(summary) {
    const set = (id, value) => { const node = $(id); if (node) node.textContent = value; };
    set('overview-bots', summary.bots);
    set('overview-average', summary.average_level);
    set('overview-party', summary.party_bots);
    set('overview-max', summary.max_level);
    set('overview-stuck', summary.stuck_bots);
    const box = $('overview-maps');
    if (!box) return;
    box.innerHTML = '<h4>Boty na mapach</h4>' + summary.maps.map((row) =>
      `<div><span>${escape(row.name)}</span><b>${row.count}</b></div>`).join('');
  }

  async function tick() {
    if (mode.value !== 'live' || document.hidden) return;
    try {
      const response = await fetch(`/api/live-map?map=${encodeURIComponent(mapSelect.value)}`,
                                   { cache: 'no-store' });
      const data = await response.json();
      if (!data.ok) return;
      missing = 0;
      stage.classList.remove('is-stale');
      leaderId = data.leader_id;
      // A tick for another map is a reply that overtook a map change.
      if (Number(data.map) !== Number(mapSelect.value)) return;
      snapshot = data.bots.map(([id, x, y, level, flags, name]) => ({ id, x, y, level, flags, name }));
      renderOverview(data.summary);
      renderLive();
    } catch (_) {
      // One missed tick is a hiccup; several mean the panel lost the world.
      missing += 1;
      if (missing >= 3) {
        stage.classList.add('is-stale');
        count.textContent = 'Brak danych na żywo';
      }
    }
  }

  /* --- heat maps ---------------------------------------------------------- */
  async function loadHeat() {
    if (mode.value === 'live') return;
    dressStage();
    clearPoints();
    clearHeat();
    try {
      const response = await fetch(`/api/heat-events?type=${encodeURIComponent(mode.value)}`,
                                   { cache: 'no-store' });
      const data = await response.json();
      if (!data.ok) return;
      const mapIndex = Number(mapSelect.value);
      const bound = data.bounds[String(mapIndex)] || data.bounds[mapIndex];
      const events = data.events.filter((event) => event.map_index === mapIndex);

      const fragment = document.createDocumentFragment();
      const clamp = (value) => `${Math.max(1, Math.min(99, value))}%`;
      events.forEach((event) => {
        const dot = document.createElement('i');
        dot.className = 'heat-point';
        dot.style.left = clamp((event.x - bound[0]) / bound[2] * 100);
        dot.style.top = clamp((event.y - bound[1]) / bound[3] * 100);
        dot.title = `${event.name || 'Zdarzenie'} · ${String(event.time).slice(11, 16)}`;
        fragment.append(dot);
      });
      stage.append(fragment);

      count.textContent = `${events.length} ${data.label} / 24 h`;
      caption.textContent = `${selectedOption().textContent.trim()} · ${data.label}`;
      $('stat-visible').textContent = events.length;
      $('stat-party').textContent = '—';
      $('stat-average').textContent = '24 h';
      $('stat-max').textContent = '●';
      $('live-ranking').innerHTML = events.slice(0, 15).map((event, index) => `
        <a href="#"><b>#${index + 1}</b>${escape(event.name || 'Zdarzenie')}
          <span>${String(event.time).slice(11, 16)}</span></a>`).join('')
        || '<p class="muted">Brak zdarzeń na tej mapie.</p>';
      $('live-activity').innerHTML =
        '<p class="muted">W trybie mapy cieplnej aktywności nie są wyświetlane.</p>';
    } catch (_) {
      count.textContent = 'Brak danych mapy cieplnej';
    }
  }

  function switchMap() {
    clearPoints();
    clearHeat();
    snapshot = [];
    dressStage();
    if (mode.value === 'live') tick(); else loadHeat();
  }

  /* --- the restart line and the rate readouts ----------------------------- */
  async function refreshStatus() {
    try {
      const data = await fetch('/api/manage-status', { cache: 'no-store' }).then((r) => r.json());
      const stamp = Number(data.last_restart_time || 0);
      const line = $('live-restart');
      if (line && stamp) {
        line.textContent = `Ostatni restart: ${new Date(stamp * 1000).toLocaleString('pl-PL')}`;
      }
      Object.entries(data.rates || {}).forEach(([name, value]) => {
        const node = $(`rate-${name}`);
        if (node) node.textContent = `${value}%`;
      });
    } catch (_) { /* the page keeps the values it was rendered with */ }
  }

  /* --- wiring ------------------------------------------------------------- */
  const noteInteraction = () => { lastInteraction = Date.now(); };

  document.querySelectorAll('[data-level]').forEach((button) => {
    button.addEventListener('click', () => {
      noteInteraction();
      levelFilter = button.dataset.level;
      document.querySelectorAll('[data-level]').forEach((other) => {
        other.classList.toggle('active', other === button);
      });
      renderLive();
    });
  });

  mapSelect.addEventListener('input', () => { noteInteraction(); switchMap(); });
  mode.addEventListener('input', () => { noteInteraction(); switchMap(); });
  [search, showNames, partyOnly].forEach((node) => {
    node.addEventListener('input', () => { noteInteraction(); renderLive(); });
  });
  autoplay.addEventListener('input', noteInteraction);
  window.addEventListener('pointerdown', noteInteraction, { passive: true });
  // Coming back to a tab that has been hidden for a while: no sliding in from
  // wherever every bot stood when the tab went to sleep.
  document.addEventListener('visibilitychange', () => {
    if (document.hidden) return;
    points.forEach((point) => { point.node.style.transitionDuration = '0ms'; });
    tick();
  });

  setInterval(() => {
    if (!autoplay.checked || Date.now() - lastInteraction < IDLE_BEFORE_AUTOPLAY) return;
    mapSelect.selectedIndex = (mapSelect.selectedIndex + 1) % mapSelect.options.length;
    switchMap();
  }, AUTOPLAY_INTERVAL);

  dressStage();
  tick();
  refreshStatus();
  setInterval(tick, TICK);
  setInterval(refreshStatus, STATUS_INTERVAL);
})();
