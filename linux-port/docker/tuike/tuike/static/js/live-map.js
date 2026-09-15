/* The live world map on the dashboard.
 *
 * Two modes share one stage: bot positions, refreshed every one and a half
 * seconds, and a 24-hour heat map of logged events. Everything the page needs
 * to draw a map - its picture and its aspect ratio - is rendered into the
 * <option> by the server, so this file holds no table of maps to fall out of
 * step with the one in Python.
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

  const LIVE_INTERVAL = 1500;
  const STATUS_INTERVAL = 30000;
  const AUTOPLAY_INTERVAL = 8000;
  const IDLE_BEFORE_AUTOPLAY = 15000;
  const RANKING_SIZE = 10;
  const ACTIVITY_ROWS = 6;

  let snapshot = [];
  let leaderId = null;
  let levelFilter = 'all';
  let lastInteraction = Date.now();

  /* --- helpers ----------------------------------------------------------- */
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
    stage.querySelectorAll('.bot-point, .heat-point').forEach((node) => node.remove());
  }

  // Percentages, clamped so a point on a map edge stays visible.
  const place = (value) => `${Math.max(1, Math.min(99, value))}%`;

  /* --- what a bot is busy with ------------------------------------------- */
  // The goal says what a bot is trying to achieve and the action what it is
  // doing this second; the goal is the better summary when it has one.
  const GOAL_SUMMARY = {
    2: 'Wybiera profesję', 3: 'Zdobywa ekwipunek', 4: 'Uzupełnia zapasy',
    5: 'Ulepsza ekwipunek', 6: 'Rozwija umiejętności', 7: 'Poluje na Metiny',
    8: 'Gra w grupie', 9: 'Robi Biologa', 10: 'Misje polowania', 11: 'Rozwija konia',
  };
  const ACTION_SUMMARY = {
    2: 'Walczy', 3: 'Zbiera łup', 4: 'Regeneruje się', 6: 'Handluje',
    7: 'Ulepsza ekwipunek', 8: 'Rozwija umiejętności', 9: 'Ulepsza ekwipunek',
    10: 'Gra w grupie', 11: 'Robi Biologa', 12: 'Rozwija konia',
    13: 'Prowadzi stragan', 14: 'Łowi ryby', 15: 'Przegląda stragany',
    17: 'Odpoczywa w mieście', 18: 'Kopie rudę',
  };

  const activityOf = (bot) => GOAL_SUMMARY[bot.goal]
    || ACTION_SUMMARY[bot.action]
    || (bot.action === 1 ? 'Przemieszcza się' : 'Walczy');

  function renderActivities(bots) {
    const box = $('live-activity');
    if (!box) return;
    const grouped = bots.reduce((all, bot) => {
      const label = activityOf(bot);
      all[label] = (all[label] || 0) + 1;
      return all;
    }, {});
    let entries = Object.entries(grouped).sort((a, b) => b[1] - a[1]);
    if (entries.length > ACTIVITY_ROWS) {
      const rest = entries.slice(ACTIVITY_ROWS - 1).reduce((sum, entry) => sum + entry[1], 0);
      entries = entries.slice(0, ACTIVITY_ROWS - 1);
      if (rest) entries.push(['Pozostałe', rest]);
    }
    const total = bots.length || 1;
    box.innerHTML = entries.map(([label, number]) => `
      <div class="activity-row">
        <span>${escape(label)}</span><b>${number}</b>
        <i style="--share:${Math.max(4, Math.round(number / total * 100))}%"></i>
      </div>`).join('') || '<p class="muted">Brak aktywnych botów na tej mapie.</p>';
  }

  /* --- live positions ----------------------------------------------------- */
  function renderLive() {
    if (mode.value !== 'live') return;
    dressStage();
    const mapIndex = Number(mapSelect.value);
    const needle = search.value.trim().toLowerCase();
    const bots = snapshot.filter((bot) => bot.map_index === mapIndex
      && inLevelBand(bot.level)
      && (!partyOnly.checked || bot.in_party)
      && (!needle || bot.name.toLowerCase().includes(needle)));

    const fragment = document.createDocumentFragment();
    bots.forEach((bot) => {
      const point = document.createElement('a');
      point.className = 'bot-point'
        + (bot.in_party ? ' is-party' : '')
        + (bot.stuck ? ' is-stuck' : '')
        + (bot.fighting_metin ? ' is-metin' : '');
      point.href = `/player/${bot.id}`;
      point.style.left = place(bot.px);
      point.style.top = place(bot.py);
      point.title = `${bot.name} · poziom ${bot.level}`
        + (bot.in_party ? ' · w grupie' : '')
        + (bot.stuck ? ' · możliwie zawieszony' : '')
        + (bot.fighting_metin ? ' · walczy z Metinem' : '');
      if (showNames.checked) point.innerHTML = `<em>${escape(bot.name)} (${bot.level})</em>`;
      fragment.append(point);
    });
    stage.append(fragment);

    const average = bots.length
      ? (bots.reduce((sum, bot) => sum + bot.level, 0) / bots.length).toFixed(1) : '—';
    $('stat-visible').textContent = bots.length;
    $('stat-party').textContent = bots.filter((bot) => bot.in_party).length;
    $('stat-average').textContent = average;
    $('stat-max').textContent = bots.length ? Math.max(...bots.map((bot) => bot.level)) : '—';
    count.textContent = `${bots.length} botów na mapie`;

    const ranked = [...bots]
      .sort((a, b) => b.level - a.level || a.name.localeCompare(b.name, 'pl'))
      .slice(0, RANKING_SIZE);
    $('live-ranking').innerHTML = ranked.map((bot, index) => `
      <a class="${bot.id === leaderId ? 'is-leader' : ''}" href="/player/${bot.id}">
        <b>#${index + 1}</b>${escape(bot.name)}
        ${bot.in_party ? '<mark>PT</mark>' : ''}${bot.stuck ? '<mark>⚠</mark>' : ''}
        <span>Lv ${bot.level}</span>
      </a>`).join('') || '<p class="muted">Brak botów spełniających filtr.</p>';

    renderActivities(bots);
  }

  function renderOverviewMaps() {
    const box = $('overview-maps');
    if (!box) return;
    const counts = new Map();
    [...mapSelect.options].forEach((option) => counts.set(Number(option.value), 0));
    snapshot.forEach((bot) => {
      if (counts.has(bot.map_index)) counts.set(bot.map_index, counts.get(bot.map_index) + 1);
    });
    const names = new Map([...mapSelect.options].map((o) => [Number(o.value), o.textContent.trim()]));
    const rows = [...counts.entries()].sort((a, b) => b[1] - a[1]);
    box.innerHTML = '<h4>Boty na mapach</h4>' + rows.map(([index, number]) =>
      `<div><span>${escape(names.get(index) || `Mapa #${index}`)}</span><b>${number}</b></div>`)
      .join('');
  }

  async function loadLive() {
    try {
      const response = await fetch('/api/live-bots', { cache: 'no-store' });
      const data = await response.json();
      if (!data.ok) return;
      leaderId = data.leader_id;
      snapshot = data.bots.map((bot) => {
        const bound = data.bounds[String(bot.map_index)] || data.bounds[bot.map_index];
        if (!bound) return bot;
        return { ...bot,
                 px: (bot.x - bound[0]) / bound[2] * 100,
                 py: (bot.y - bound[1]) / bound[3] * 100 };
      });

      const total = snapshot.length;
      const average = total
        ? (snapshot.reduce((sum, bot) => sum + bot.level, 0) / total).toFixed(1) : '0';
      $('overview-bots').textContent = total;
      $('overview-average').textContent = average;
      $('overview-party').textContent = snapshot.filter((bot) => bot.in_party).length;
      $('overview-max').textContent = total ? Math.max(...snapshot.map((bot) => bot.level)) : '0';
      $('overview-stuck').textContent = snapshot.filter((bot) => bot.stuck).length;
      renderOverviewMaps();
      renderLive();
    } catch (_) {
      count.textContent = 'Brak danych na żywo';
    }
  }

  /* --- heat maps ---------------------------------------------------------- */
  async function loadHeat() {
    if (mode.value === 'live') return;
    dressStage();
    try {
      const response = await fetch(`/api/heat-events?type=${encodeURIComponent(mode.value)}`,
                                   { cache: 'no-store' });
      const data = await response.json();
      if (!data.ok) return;
      const mapIndex = Number(mapSelect.value);
      const bound = data.bounds[String(mapIndex)] || data.bounds[mapIndex];
      const events = data.events.filter((event) => event.map_index === mapIndex);

      const fragment = document.createDocumentFragment();
      events.forEach((event) => {
        const dot = document.createElement('i');
        dot.className = 'heat-point';
        dot.style.left = place((event.x - bound[0]) / bound[2] * 100);
        dot.style.top = place((event.y - bound[1]) / bound[3] * 100);
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

  const refresh = () => (mode.value === 'live' ? renderLive() : loadHeat());

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

  [mode, mapSelect, search, showNames, partyOnly].forEach((node) => {
    node.addEventListener('input', () => { noteInteraction(); refresh(); });
  });
  autoplay.addEventListener('input', noteInteraction);
  window.addEventListener('pointerdown', noteInteraction, { passive: true });

  setInterval(() => {
    if (!autoplay.checked || Date.now() - lastInteraction < IDLE_BEFORE_AUTOPLAY) return;
    mapSelect.selectedIndex = (mapSelect.selectedIndex + 1) % mapSelect.options.length;
    refresh();
  }, AUTOPLAY_INTERVAL);

  dressStage();
  loadLive();
  refreshStatus();
  setInterval(() => { if (mode.value === 'live') loadLive(); }, LIVE_INTERVAL);
  setInterval(refreshStatus, STATUS_INTERVAL);
})();
