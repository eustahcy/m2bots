/* The top bar: clock, light/dark switch, Ctrl+K search and the bell.
 *
 * Everything here is a per-browser convenience. The theme saved in
 * Zarządzanie stays the panel's theme; the switch only lays a light or dark
 * variant over it in this browser, and forgets itself if storage is blocked.
 */
(() => {
  const root = document.documentElement;
  const store = {
    get(key) { try { return localStorage.getItem(key); } catch (_) { return null; } },
    set(key, value) { try { localStorage.setItem(key, value); } catch (_) { /* private mode */ } },
    drop(key) { try { localStorage.removeItem(key); } catch (_) { /* private mode */ } },
  };
  const escape = (value) => String(value).replace(/[&<>"']/g, (character) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[character]));

  /* --- clock ------------------------------------------------------------- */
  const clockTime = document.getElementById('clock-time');
  const clockDate = document.getElementById('clock-date');
  function tick() {
    const now = new Date();
    if (clockTime) clockTime.textContent = now.toLocaleTimeString('pl-PL', { hour: '2-digit', minute: '2-digit' });
    if (clockDate) {
      clockDate.textContent = now.toLocaleDateString('pl-PL', { weekday: 'short', day: 'numeric', month: 'long' });
    }
  }
  tick();
  setInterval(tick, 10000);

  /* --- light / dark ------------------------------------------------------ */
  const LIGHT = 'dawn';
  const saved = root.dataset.serverTheme || 'midnight';
  const toggle = document.getElementById('theme-toggle');
  if (toggle) {
    toggle.addEventListener('click', () => {
      const next = root.dataset.theme === LIGHT ? (saved === LIGHT ? 'midnight' : saved) : LIGHT;
      root.dataset.theme = next;
      // Back on the saved theme means there is nothing left to override.
      if (next === saved) store.drop('tuike.theme.override');
      else store.set('tuike.theme.override', next);
    });
  }

  /* --- Ctrl+K ------------------------------------------------------------ */
  const search = document.getElementById('global-search');
  document.addEventListener('keydown', (event) => {
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'k' && search) {
      event.preventDefault();
      search.focus();
      search.select();
    }
    if (event.key === 'Escape') closeMenu();
  });

  /* --- the bell ---------------------------------------------------------- */
  const button = document.getElementById('notify-button');
  const menu = document.getElementById('notify-menu');
  const list = document.getElementById('notify-list');
  const count = document.getElementById('notify-count');
  const SEEN_KEY = 'tuike.notify.seen';
  const REFRESH = 60000;
  const SHOW = 8;
  let events = [];

  function closeMenu() {
    if (!menu || menu.hidden) return;
    menu.hidden = true;
    button.setAttribute('aria-expanded', 'false');
  }

  function render() {
    const seen = store.get(SEEN_KEY) || '';
    // The feed runs oldest to newest; the menu reads newest first.
    const newest = events.slice(-SHOW).reverse();
    // Everything newer than the last entry the operator opened the menu on.
    const cut = newest.findIndex((event) => event.key === seen);
    const unseen = cut === -1 ? newest.length : cut;
    count.hidden = !unseen;
    count.textContent = unseen > 9 ? '9+' : String(unseen);
    list.innerHTML = newest.map((event) => `
      <li class="${Number(event.refine_tier) >= 8 ? 'rare' : ''}">
        <i aria-hidden="true"></i><span>${escape(event.message)}</span><time>${escape(event.time)}</time>
      </li>`).join('') || '<li class="muted">Na razie cisza - nic rzadkiego się nie wydarzyło.</li>';
  }

  async function refresh() {
    if (!button) return;
    try {
      const response = await fetch('/api/news', { cache: 'no-store' });
      const data = await response.json();
      if (!data.ok) return;
      events = data.events || [];
      render();
    } catch (_) { /* the bell keeps its last state */ }
  }

  if (button && menu) {
    button.addEventListener('click', (event) => {
      event.stopPropagation();
      menu.hidden = !menu.hidden;
      button.setAttribute('aria-expanded', String(!menu.hidden));
      if (!menu.hidden && events.length) {
        store.set(SEEN_KEY, events[events.length - 1].key);
        count.hidden = true;
      }
    });
    document.addEventListener('click', (event) => {
      if (!menu.contains(event.target)) closeMenu();
    });
    refresh();
    setInterval(refresh, REFRESH);
  }
})();
