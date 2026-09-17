/* The server card at the foot of the sidebar, on every page.
 *
 * Two ports decide the state, and they come up in order - the auth server
 * accepts logins before the world is ready - so a restart shows as "podnosi
 * się" rather than flipping straight from off to on. Uptime counts from when
 * the running game cores started, whoever restarted them.
 */
(() => {
  const badge = document.getElementById('server-status');
  if (!badge) return;

  const $ = (id) => document.getElementById(id);
  const REFRESH = 15000;
  let startedAt = 0;

  const NOTES = {
    up: () => 'wszystkie usługi odpowiadają',
    partial: (data) => (data.auth ? 'logowanie działa, świat wstaje' : 'trwa uruchamianie'),
    down: () => 'gra nie odpowiada',
  };

  function uptime() {
    const node = $('server-status-uptime');
    if (!node) return;
    if (!startedAt || badge.dataset.state !== 'up') { node.textContent = '—'; return; }
    const seconds = Math.max(0, Math.floor(Date.now() / 1000 - startedAt));
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    node.textContent = days ? `${days} d ${hours} h` : `${hours} h ${String(minutes).padStart(2, '0')} min`;
  }

  async function refresh() {
    try {
      const response = await fetch('/api/status', { cache: 'no-store' });
      const data = await response.json();
      if (!data.ok) return;
      badge.dataset.state = data.state;
      $('server-status-label').textContent = data.label;
      $('server-status-note').textContent = (NOTES[data.state] || NOTES.down)(data);
      $('server-status-bots').textContent = Number(data.bots || 0).toLocaleString('pl-PL');
      // Anything else on the page that shows the live bot count follows along.
      document.querySelectorAll('[data-live-bots]').forEach((node) => {
        node.textContent = Number(data.bots || 0).toLocaleString('pl-PL');
      });
      startedAt = Number(data.started_at || 0);
      uptime();
    } catch (_) {
      badge.dataset.state = 'unknown';
      $('server-status-label').textContent = 'Brak połączenia';
      $('server-status-note').textContent = 'panel nie dosięga gry';
    }
  }

  refresh();
  setInterval(refresh, REFRESH);
  setInterval(uptime, 30000);
})();
