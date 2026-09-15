/* The server badge in the sidebar, on every page.
 *
 * Two ports decide it, and they come up in order - the auth server accepts
 * logins before the world is ready - so a restart shows as "podnosi się"
 * rather than flipping straight from off to on.
 */
(() => {
  const badge = document.getElementById('server-status');
  if (!badge) return;

  const label = document.getElementById('server-status-label');
  const note = document.getElementById('server-status-note');
  const REFRESH = 15000;

  const NOTES = {
    up: (data) => `${data.bots} botów w świecie`,
    partial: (data) => (data.auth ? 'logowanie działa, świat wstaje' : 'trwa uruchamianie'),
    down: () => 'gra nie odpowiada',
  };

  async function refresh() {
    try {
      const response = await fetch('/api/status', { cache: 'no-store' });
      const data = await response.json();
      if (!data.ok) return;
      badge.dataset.state = data.state;
      label.textContent = data.label;
      note.textContent = (NOTES[data.state] || NOTES.down)(data);
    } catch (_) {
      badge.dataset.state = 'unknown';
      label.textContent = 'Brak połączenia';
      note.textContent = 'panel nie dosięga gry';
    }
  }

  refresh();
  setInterval(refresh, REFRESH);
})();
