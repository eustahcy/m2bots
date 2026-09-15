/* The management console: live progress, and the small form conveniences.
 *
 * Restart and update progress are polled rather than pushed, because both are
 * carried out by another container through a file spool - there is nothing to
 * open a socket to.
 */
(() => {
  const $ = (id) => document.getElementById(id);
  const POLL_INTERVAL = 2000;

  // Who asked for the last restart. The helper reports a key; the operator
  // needs a sentence.
  const SOURCES = {
    panel: 'Na zlecenie z tego panelu',
    classic_panel: 'Na zlecenie z panelu klasycznego lub starszej kolejki',
    deployment: 'Podczas wdrożenia aktualizacji',
    external_unknown: 'Uruchomienie spoza panelu — źródła nie ustalono',
  };

  const stamp = (value) => (Number(value) > 0
    ? new Date(Number(value) * 1000).toLocaleString('pl-PL')
    : 'brak zarejestrowanych danych');

  function showRestart(restart) {
    if (!restart || !$('restart-fill')) return;
    $('restart-fill').style.width = `${restart.percent}%`;
    const bar = document.querySelector('.progress[aria-label="Postęp restartu"]');
    if (bar) bar.setAttribute('aria-valuenow', restart.percent);
    $('restart-stage').textContent = `${restart.percent}% · ${restart.stage}`;
    $('restart-last').textContent = stamp(restart.last_restart_time);
    $('restart-origin').textContent = restart.last_restart_time
      ? (SOURCES[restart.last_restart_source] || 'Źródła nie ustalono')
      : '';
    $('restart-core').textContent = restart.last_core_time
      ? `Ostatnie automatyczne podniesienie rdzenia ${restart.last_core_name}: ${stamp(restart.last_core_time)}`
      : '';
    document.querySelectorAll('.restart-buttons button').forEach((button) => {
      button.disabled = restart.state === 'running';
    });
  }

  function showUpdater(update) {
    if (!update || !$('updater-fill')) return;
    $('updater-fill').style.width = `${update.percent}%`;
    $('updater-state').textContent = update.state;
    $('updater-percent').textContent = `${update.percent}%`;
    $('updater-worker').textContent = update.watcher_ready ? 'gotowa' : 'nie uruchomiona';
    $('updater-message').textContent = update.message;
    $('updater-log').textContent = (update.log && update.log.length)
      ? update.log.join('\n') : 'Brak wpisów.';
    const submit = $('updater-submit');
    if (submit) submit.disabled = !update.watcher_ready || update.state === 'running';
  }

  const initial = (id) => {
    const node = $(id);
    if (!node) return null;
    try { return JSON.parse(node.textContent); } catch (_) { return null; }
  };

  showRestart(initial('restart-state'));
  showUpdater(initial('updater-state-data'));

  async function poll() {
    try {
      const response = await fetch('/api/manage-status', { cache: 'no-store' });
      if (response.ok) {
        const data = await response.json();
        if (data.ok) {
          showRestart(data.restart);
          showUpdater(data.updater);
          if ($('bot-live-count')) $('bot-live-count').textContent = data.bots;
          const rows = $('map-load-rows');
          if (rows && data.maps) {
            rows.innerHTML = data.maps.length
              ? data.maps.map((row) => `<tr><td>${row.name}</td>`
                  + `<td class="numeric">${row.character_count}</td></tr>`).join('')
              : '<tr><td colspan="2" class="muted empty">Żaden rdzeń nie zgłasza teraz botów.</td></tr>';
          }
        }
      }
    } catch (_) { /* a poll that fails simply leaves the last state on screen */ }
    finally {
      setTimeout(poll, POLL_INTERVAL);
    }
  }
  setTimeout(poll, POLL_INTERVAL);

  /* --- form conveniences -------------------------------------------------- */
  // "Domyślne" next to a respawn field clears it, which is how the helper is
  // told to restore the shipped value.
  document.querySelectorAll('[data-clear]').forEach((button) => {
    button.addEventListener('click', () => {
      const input = button.parentElement.querySelector('input');
      if (input) input.value = '';
    });
  });

  // Every slider shows its own value.
  document.querySelectorAll('[data-weight]').forEach((slider) => {
    const output = slider.parentElement.querySelector('output');
    if (!output) return;
    slider.addEventListener('input', () => { output.value = slider.value; });
  });

  // A rate preset fills the three fields; saving is still a deliberate click.
  document.querySelectorAll('[data-rates]').forEach((button) => {
    button.addEventListener('click', () => {
      const [exp, drop, yang] = button.dataset.rates.split(' ');
      const form = button.closest('form');
      Object.entries({ exp, drop, yang }).forEach(([name, value]) => {
        const field = form.querySelector(`input[name="${name}"]`);
        if (field) field.value = value;
      });
      document.querySelectorAll('[data-rates]').forEach((other) => {
        other.classList.toggle('active', other === button);
      });
    });
  });

  /* --- tabs ---------------------------------------------------------- */
  // Six tabs, switched client-side - every panel is already in the page from
  // the server, this only shows one and hides the rest. Old anchors
  // (#behavior, #rates, ...) from bookmarks or other pages still work: they
  // are mapped onto whichever tab now holds that section.
  const tabButtons = [...document.querySelectorAll('#manage-tabs [data-tab]')];
  const tabPanels = [...document.querySelectorAll('[data-tab-panel]')];
  const ANCHOR_TAB = {
    rates: 'world', restart: 'world', language: 'world',
    updater: 'updates', behavior: 'behavior',
    grants: 'tools', 'related-panels': 'tools',
    appearance: 'appearance', population: 'population',
  };

  function activateTab(name) {
    if (!tabButtons.some((button) => button.dataset.tab === name)) return;
    tabButtons.forEach((button) => button.classList.toggle('active', button.dataset.tab === name));
    tabPanels.forEach((panel) => { panel.hidden = panel.dataset.tabPanel !== name; });
  }

  if (tabButtons.length) {
    tabButtons.forEach((button) => {
      button.addEventListener('click', () => {
        activateTab(button.dataset.tab);
        history.replaceState(null, '', `#${button.dataset.tab}`);
      });
    });
    const requested = location.hash.replace('#', '');
    activateTab(ANCHOR_TAB[requested] || requested || tabButtons[0].dataset.tab);
  }

  const reset = $('reset-weights');
  if (reset) {
    reset.addEventListener('click', () => {
      // Only the 25-250 goal weights go back to neutral. SCRAP and REST are
      // 0-100 settings that mean something else entirely at 100.
      document.querySelectorAll('.weight-grid [data-weight]').forEach((slider) => {
        slider.value = 100;
        const output = slider.parentElement.querySelector('output');
        if (output) output.value = 100;
      });
    });
  }
})();
