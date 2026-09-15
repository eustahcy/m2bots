/* Gospodarka, na żywo: refreshes the market without a page reload.
 *
 * Split into two polls on purpose. /api/economy is indexed, cheap lookups
 * (id-ordered LIMIT queries) and can refresh often. /api/economy/market
 * groups the whole trade log to find what actually sells, which is far
 * heavier on an unbounded table - it refreshes much less often, and the
 * active-shops list it carries barely changes second to second anyway.
 */
(() => {
  const root = document.getElementById('economy-live');
  if (!root) return;

  const FAST_REFRESH = 6000;
  const SLOW_REFRESH = 45000;

  const escape = (value) => String(value).replace(/[&<>"']/g, (character) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[character]));

  const thousands = (value) => Number(value || 0).toLocaleString('pl-PL');

  function characterCell(name, url, portrait) {
    if (!name) return '<span class="muted">—</span>';
    return `<a class="character-link" href="${url}">`
      + (portrait ? `<img class="portrait portrait-tiny" src="${portrait}" alt="" aria-hidden="true" loading="lazy">` : '')
      + `${escape(name)}</a>`;
  }

  function renderTrades(trades) {
    const body = document.getElementById('recent-trades-body');
    if (!body) return;
    if (!trades.length) {
      body.innerHTML = '<tr><td colspan="5" class="muted empty">Żaden stragan jeszcze niczego nie sprzedał.</td></tr>';
      return;
    }
    body.innerHTML = trades.map((trade) => `
      <tr>
        <td class="item-cell">
          <a href="${trade.item_url}">
            ${trade.icon ? `<img class="item-icon" src="${trade.icon}" alt="" loading="lazy">` : ''}
            ${escape(trade.item_name)}${trade.count > 1 ? ` ×${trade.count}` : ''}</a>
        </td>
        <td>${characterCell(trade.buyer_name, trade.buyer_url, trade.buyer_portrait)}</td>
        <td>${characterCell(trade.seller_name, trade.seller_url, trade.seller_portrait)}</td>
        <td class="numeric">${thousands(trade.yang)}</td>
        <td class="numeric muted">${escape(trade.time)}</td>
      </tr>`).join('');
  }

  function renderTopItems(items) {
    const body = document.getElementById('top-items-body');
    if (!body) return;
    if (!items.length) {
      body.innerHTML = '<tr><td colspan="4" class="muted empty">Brak sprzedaży w tym okresie.</td></tr>';
      return;
    }
    body.innerHTML = items.map((item) => `
      <tr>
        <td class="item-cell">
          <a href="${item.item_url}">
            ${item.icon ? `<img class="item-icon" src="${item.icon}" alt="" loading="lazy">` : ''}
            ${escape(item.item_name)}</a>
        </td>
        <td class="numeric">${item.sales}</td>
        <td class="numeric">${thousands(item.avg_price)}</td>
        <td class="numeric">${thousands(item.turnover)}</td>
      </tr>`).join('');
  }

  function renderShops(shops) {
    const grid = document.getElementById('active-shops-grid');
    if (!grid) return;
    if (!shops.length) {
      grid.innerHTML = '<p class="muted">Nikt aktualnie nie prowadzi straganu.</p>';
      return;
    }
    grid.innerHTML = shops.map((shop) => `
      <a class="item-card" href="${shop.shop_url}">
        <div>
          <b>${escape(shop.shop_name)}</b>
          <small>${escape(shop.owner_name)} · ${shop.item_count} przedmiotów${shop.is_premium ? ' · premium' : ''}</small>
        </div>
      </a>`).join('');
  }

  async function refreshFast() {
    try {
      const response = await fetch('/api/economy', { cache: 'no-store' });
      const data = await response.json();
      if (!data.ok) return;
      const set = (id, value) => { const el = document.getElementById(id); if (el) el.textContent = value; };
      set('stat-shops', thousands(data.overview.shops));
      set('stat-listed', thousands(data.overview.listed));
      set('stat-trades', thousands(data.overview.trades_24h));
      set('stat-turnover', thousands(data.overview.turnover_24h));
      renderTrades(data.trades);
    } catch (_) { /* the next tick tries again */ }
  }

  async function refreshSlow() {
    try {
      const response = await fetch('/api/economy/market', { cache: 'no-store' });
      const data = await response.json();
      if (!data.ok) return;
      renderTopItems(data.top_items);
      renderShops(data.shops);
    } catch (_) { /* the next tick tries again */ }
  }

  refreshFast();
  refreshSlow();
  setInterval(refreshFast, FAST_REFRESH);
  setInterval(refreshSlow, SLOW_REFRESH);
})();
