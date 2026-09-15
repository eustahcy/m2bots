/* The admin tools on a character's page: tab switching and the Yang field
 * that appears only for a custom amount. The item lookup itself lives in
 * item-search.js, shared with the mass-grant page. */
(() => {
  const tabs = [...document.querySelectorAll('.tools .tabs button')];
  const panes = [...document.querySelectorAll('.tool-pane')];
  if (!tabs.length) return;

  tabs.forEach((tab) => {
    tab.addEventListener('click', () => {
      tabs.forEach((other) => other.classList.toggle('active', other === tab));
      panes.forEach((pane) => { pane.hidden = pane.dataset.tool !== tab.dataset.tool; });
    });
  });

  /* --- teleport the operator's own character here ------------------------ */
  const warpButton = document.getElementById('warp-me-button');
  const warpStatus = document.getElementById('warp-me-status');
  if (warpButton && warpStatus) {
    warpButton.addEventListener('click', async () => {
      warpButton.disabled = true;
      warpStatus.hidden = false;
      warpStatus.className = 'warp-me-status';
      warpStatus.textContent = '⏳ Szukam Twojej postaci w grze…';
      try {
        const response = await fetch(warpButton.dataset.action, {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: `csrf=${encodeURIComponent(warpButton.dataset.csrf)}`,
        });
        const data = await response.json();
        if (data.ok) {
          warpStatus.classList.add('ok');
          warpStatus.textContent = `✅ Przeniesiono ${data.name}.`;
        } else {
          warpStatus.classList.add('warn');
          warpStatus.textContent = `⚠️ ${data.error || 'Nie udało się teleportować.'}`;
        }
      } catch (_) {
        warpStatus.classList.add('warn');
        warpStatus.textContent = '⚠️ Brak połączenia z panelem.';
      } finally {
        warpButton.disabled = false;
      }
    });
  }

  /* --- a custom Yang amount ---------------------------------------------- */
  const preset = document.getElementById('gold-preset');
  const custom = document.getElementById('gold-custom');
  if (preset && custom) {
    const sync = () => {
      custom.hidden = preset.value !== 'custom';
      const field = custom.querySelector('input');
      // Required only while it is the field being used, or the hidden input
      // would block the form.
      if (field) field.required = !custom.hidden;
    };
    preset.addEventListener('change', sync);
    sync();
  }
})();
