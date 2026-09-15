/* The world ticker.
 *
 * The API only reports rare achievements from the last twelve hours, so a
 * quiet window returns nothing. That is not an empty feed, and a reload must
 * not wipe what the operator already saw - hence the local cache, keyed per
 * browser and capped so it cannot grow without bound.
 */
(() => {
  const list = document.getElementById('news-feed');
  if (!list) return;

  const ticker = list.closest('.ticker');
  const toggle = document.getElementById('ticker-toggle');
  const CACHE_KEY = 'tuike.news';
  const HIDDEN_KEY = 'tuike.news.hidden';
  const KEEP = 40;
  const SHOW = 8;
  const REFRESH = 30000;
  const RARE_TIER = 8;

  const store = {
    read(key, fallback) {
      try { return JSON.parse(localStorage.getItem(key)) ?? fallback; }
      catch (_) { return fallback; }
    },
    write(key, value) {
      try { localStorage.setItem(key, JSON.stringify(value)); } catch (_) { /* private mode */ }
    },
  };

  const escape = (value) => String(value).replace(/[&<>"']/g, (character) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[character]));

  function setHidden(hidden) {
    ticker.classList.toggle('is-hidden', hidden);
    toggle.textContent = hidden ? 'Pokaż' : 'Ukryj';
    toggle.setAttribute('aria-expanded', String(!hidden));
    store.write(HIDDEN_KEY, hidden);
  }

  if (toggle) {
    setHidden(store.read(HIDDEN_KEY, false) === true);
    toggle.addEventListener('click', () => setHidden(!ticker.classList.contains('is-hidden')));
  }

  let cached = store.read(CACHE_KEY, []);
  if (!Array.isArray(cached)) cached = [];
  cached = cached.filter((event) => event && event.key && event.message);
  const seen = new Set(cached.map((event) => event.key));

  function render(events) {
    if (!events.length) {
      list.innerHTML = '<li class="muted">Oczekiwanie na nowe wydarzenia ze świata…</li>';
      return;
    }
    list.innerHTML = events.slice(-SHOW).map((event) => `
      <li${Number(event.refine_tier) >= RARE_TIER ? ' class="rare"' : ''}>
        <time>${escape(event.time)}</time><span>${escape(event.message)}</span>
      </li>`).join('');
  }

  render(cached);

  async function refresh() {
    try {
      const response = await fetch('/api/news', { cache: 'no-store' });
      if (!response.ok) throw new Error('news');
      const data = await response.json();
      if (!data.ok) throw new Error('news');
      const fresh = data.events.filter((event) => !seen.has(event.key));
      if (fresh.length) {
        fresh.forEach((event) => seen.add(event.key));
        cached = [...cached, ...fresh].slice(-KEEP);
        store.write(CACHE_KEY, cached);
      }
      render(cached);
    } catch (_) {
      if (!cached.length) {
        list.innerHTML = '<li class="muted">Feed wydarzeń jest chwilowo niedostępny.</li>';
      }
    }
  }

  refresh();
  setInterval(refresh, REFRESH);
})();
