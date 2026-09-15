/* A live item lookup against /api/items, wired into any container marked
 * data-item-search. Used on a character's page (the give-item tool) and on
 * the mass-grant page - one lookup, so a fix or a wording change lands in
 * both places at once instead of drifting apart.
 *
 * Expected children, found by data attribute rather than id so a page can
 * hold more than one instance:
 *   [data-item-lookup]  the text input the operator types into
 *   [data-item-results] the <ul> the matches are drawn into
 *   [data-item-vnum]    the hidden/number input a match's vnum is written to
 *   [data-item-chosen]  optional: a summary line shown once something is picked
 */
(() => {
  const DEBOUNCE = 220;

  const escape = (value) => String(value).replace(/[&<>"']/g, (character) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[character]));

  function wire(root) {
    const lookup = root.querySelector('[data-item-lookup]');
    const results = root.querySelector('[data-item-results]');
    const vnum = root.querySelector('[data-item-vnum]');
    const chosen = root.querySelector('[data-item-chosen]');
    if (!lookup || !results || !vnum) return;

    let timer = null;
    let controller = null;

    function close() {
      results.hidden = true;
      results.innerHTML = '';
      lookup.setAttribute('aria-expanded', 'false');
    }

    function choose(item) {
      vnum.value = item.vnum;
      lookup.value = item.name;
      if (chosen) {
        chosen.hidden = false;
        chosen.innerHTML = (item.icon ? `<img class="item-icon" src="${item.icon}" alt="">` : '')
          + `<b>${escape(item.name)}</b>`
          + `<span class="muted">VNUM ${item.vnum} · ${escape(item.type_label || '')}</span>`;
      }
      close();
      vnum.dispatchEvent(new Event('change', { bubbles: true }));
    }

    async function search() {
      const query = lookup.value.trim();
      if (query.length < 2 && !/^\d+$/.test(query)) { close(); return; }
      // A new keystroke makes the previous request pointless; dropping it
      // keeps answers from arriving out of order.
      if (controller) controller.abort();
      controller = new AbortController();
      try {
        const response = await fetch(`/api/items?q=${encodeURIComponent(query)}`,
                                     { cache: 'no-store', signal: controller.signal });
        const data = await response.json();
        if (!data.ok || !data.items.length) {
          results.innerHTML = '<li class="muted">Nic nie pasuje.</li>';
          results.hidden = false;
          lookup.setAttribute('aria-expanded', 'true');
          return;
        }
        results.innerHTML = data.items.map((item) => `
          <li role="option" data-vnum="${item.vnum}">
            ${item.icon ? `<img class="item-icon" src="${item.icon}" alt="">` : ''}
            <span class="lookup-name">${escape(item.name)}</span>
            <span class="muted">${item.vnum}</span>
          </li>`).join('');
        results.hidden = false;
        lookup.setAttribute('aria-expanded', 'true');
        [...results.querySelectorAll('li[data-vnum]')].forEach((row, index) => {
          row.addEventListener('click', () => choose(data.items[index]));
        });
      } catch (error) {
        if (error.name !== 'AbortError') close();
      }
    }

    lookup.addEventListener('input', () => {
      clearTimeout(timer);
      timer = setTimeout(search, DEBOUNCE);
    });
    lookup.addEventListener('keydown', (event) => {
      if (event.key === 'Escape') close();
    });
    document.addEventListener('click', (event) => {
      if (!results.contains(event.target) && event.target !== lookup) close();
    });
  }

  document.querySelectorAll('[data-item-search]').forEach(wire);
})();
