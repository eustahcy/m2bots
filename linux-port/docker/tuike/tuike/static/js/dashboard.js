/* The dashboard's cards other than the live map (live-map.js) and the event
 * list (news.js): the ranking carousel, the bots/stalls-by-map tile, the
 * day's activity chart, the host gauges and the recent logins.
 *
 * Colours come from the CSS tokens, so every chart follows the theme.
 */
(() => {
  const $ = (id) => document.getElementById(id);
  const styles = () => getComputedStyle(document.documentElement);
  const token = (name, fallback) => (styles().getPropertyValue(name).trim() || fallback);
  const escape = (value) => String(value).replace(/[&<>"']/g, (character) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[character]));
  const thousands = (value) => Number(value || 0).toLocaleString('pl-PL');
  const ROTATE = 8000;
  const SERIES = ['#4f8cff', '#36d399', '#f6c85f', '#a78bfa', '#fb7185', '#22d3ee',
                  '#f97316', '#a3e635', '#e879f9', '#facc15', '#14b8a6', '#818cf8'];

  /* --- ranking carousel ---------------------------------------------------
   * Turns every eight seconds. Any click on an arrow or a dot restarts the
   * count, so a slide the operator chose is not swept away a second later. */
  (() => {
    const slides = [...document.querySelectorAll('.carousel-slide')];
    const dots = [...document.querySelectorAll('.carousel-dots button')];
    if (slides.length < 2) return;
    const title = $('carousel-title');
    const subtitle = $('carousel-subtitle');
    let current = 0;
    let timer = null;

    function show(index) {
      current = (index + slides.length) % slides.length;
      slides.forEach((slide, position) => {
        slide.hidden = position !== current;
        slide.classList.toggle('entering', position === current);
      });
      dots.forEach((dot, position) => dot.classList.toggle('active', position === current));
      title.textContent = slides[current].dataset.title;
      subtitle.textContent = slides[current].dataset.subtitle;
    }
    function restart() {
      clearInterval(timer);
      timer = setInterval(() => show(current + 1), ROTATE);
    }
    $('carousel-prev').addEventListener('click', () => { show(current - 1); restart(); });
    $('carousel-next').addEventListener('click', () => { show(current + 1); restart(); });
    dots.forEach((dot) => dot.addEventListener('click', () => { show(Number(dot.dataset.slide)); restart(); }));
    restart();
  })();

  /* --- bots / stalls by map ------------------------------------------------
   * One tile, two views: a donut of where the bots are, and bars of where the
   * stalls stand. It alternates on its own; picking a view by hand stops the
   * alternation, because the operator has said which one they want. */
  (() => {
    const canvas = $('distribution-canvas');
    if (!canvas || typeof Chart === 'undefined') return;
    const legend = $('distribution-legend');
    const titleText = document.querySelector('#distribution-title span');
    const buttons = [...document.querySelectorAll('[data-distribution]')];
    let bots = [];
    try { bots = JSON.parse($('map-load-data').textContent) || []; } catch (_) { bots = []; }
    bots = bots.filter((row) => Number(row.character_count) > 0);
    let shops = [];
    let chart = null;
    let view = 'bots';
    let timer = null;

    function draw() {
      const rows = view === 'bots'
        ? bots.map((row) => ({ name: row.name, value: Number(row.character_count) }))
        : shops.map((row) => ({ name: row.name, value: Number(row.shop_count) }));
      const total = rows.reduce((sum, row) => sum + row.value, 0) || 1;
      const ink = token('--text-dim', '#a9bed8');
      const grid = token('--grid', '#16253d');
      if (chart) chart.destroy();
      titleText.textContent = view === 'bots' ? 'Boty według map' : 'Stragany według map';
      buttons.forEach((button) => button.classList.toggle('active', button.dataset.distribution === view));
      canvas.parentElement.classList.toggle('is-bars', view !== 'bots');

      if (view === 'bots') {
        chart = new Chart(canvas, {
          type: 'doughnut',
          data: {
            labels: rows.map((row) => row.name),
            datasets: [{ data: rows.map((row) => row.value),
                         backgroundColor: rows.map((_row, index) => SERIES[index % SERIES.length]),
                         borderWidth: 2, borderColor: token('--surface', '#0f1a2c') }],
          },
          options: { cutout: '64%', responsive: true, maintainAspectRatio: false,
                     plugins: { legend: { display: false } } },
        });
      } else {
        chart = new Chart(canvas, {
          type: 'bar',
          data: {
            labels: rows.map((row) => row.name.split(' — ')[0]),
            datasets: [{ data: rows.map((row) => row.value), borderRadius: 6,
                         backgroundColor: rows.map((_row, index) => SERIES[index % SERIES.length]) }],
          },
          options: {
            indexAxis: 'y', responsive: true, maintainAspectRatio: false,
            plugins: { legend: { display: false } },
            scales: { x: { ticks: { color: ink, precision: 0 }, grid: { color: grid }, beginAtZero: true },
                      y: { ticks: { color: ink }, grid: { display: false } } },
          },
        });
      }
      legend.innerHTML = rows.slice(0, 8).map((row, index) => `
        <li><i style="background:${SERIES[index % SERIES.length]}"></i>
          <span>${escape(row.name)}</span><b>${Math.round(row.value * 100 / total)}%</b></li>`).join('')
        || `<li class="muted">${view === 'bots' ? 'Żaden bot nie jest teraz w świecie.' : 'Nikt nie prowadzi straganu.'}</li>`;
    }

    function rotate() {
      clearInterval(timer);
      timer = setInterval(() => { view = view === 'bots' ? 'shops' : 'bots'; draw(); }, ROTATE);
    }
    buttons.forEach((button) => button.addEventListener('click', () => {
      clearInterval(timer);
      view = button.dataset.distribution;
      draw();
    }));
    window.addEventListener('tuike:dashboard', (event) => { shops = event.detail.shops_by_map || []; });
    draw();
    rotate();
  })();

  /* --- a day of activity -------------------------------------------------- */
  let activityChart = null;
  function drawActivity(activity) {
    const canvas = $('activity-canvas');
    if (!canvas || typeof Chart === 'undefined' || !activity) return;
    const ink = token('--text-dim', '#a9bed8');
    const grid = token('--grid', '#16253d');
    const line = (colour, fill) => ({ borderColor: colour, backgroundColor: `${colour}22`, fill,
                                      tension: .35, pointRadius: 0, pointHoverRadius: 4, borderWidth: 2 });
    const data = {
      labels: activity.labels,
      datasets: [
        { label: 'Aktywne boty', data: activity.bots, yAxisID: 'bots', ...line(token('--accent', '#5b9bff'), true) },
        { label: 'Logowania', data: activity.logins, yAxisID: 'events', ...line('#36d399', false) },
        { label: 'Sprzedaże na straganach', data: activity.sales, yAxisID: 'events', ...line('#f6c85f', false) },
      ],
    };
    if (activityChart) { activityChart.data = data; activityChart.update('none'); return; }
    activityChart = new Chart(canvas, {
      type: 'line',
      data,
      options: {
        responsive: true, maintainAspectRatio: false,
        interaction: { mode: 'index', intersect: false },
        plugins: { legend: { position: 'bottom', labels: { color: ink, boxWidth: 10, boxHeight: 10, usePointStyle: true } } },
        scales: {
          x: { ticks: { color: ink, maxTicksLimit: 8 }, grid: { color: grid } },
          bots: { position: 'left', beginAtZero: true, ticks: { color: ink, precision: 0 }, grid: { color: grid } },
          events: { position: 'right', beginAtZero: true, ticks: { color: ink, precision: 0 }, grid: { display: false } },
        },
      },
    });
  }

  /* --- recent logins ------------------------------------------------------ */
  function drawLogins(logins) {
    const list = $('recent-logins');
    if (!list || !logins) return;
    list.innerHTML = logins.map((row) => `
      <li>
        <img class="portrait portrait-tiny" src="${row.portrait}" alt="" aria-hidden="true" loading="lazy">
        <a href="${row.url}">${escape(row.name)}</a>
        <span class="tag ${row.is_bot ? 'tag-bot' : 'tag-live'}">${row.is_bot ? 'bot' : 'gracz'}</span>
        <small>Lv ${Number(row.level || 0)}</small>
        <b class="muted">${escape(row.time)}</b>
      </li>`).join('') || '<li class="muted">Dziennik logowań jest pusty.</li>';
  }

  async function refreshSlow() {
    try {
      const data = await fetch('/api/dashboard', { cache: 'no-store' }).then((response) => response.json());
      if (!data.ok) return;
      window.dispatchEvent(new CustomEvent('tuike:dashboard', { detail: data }));
      drawActivity(data.activity);
      drawLogins(data.logins);
    } catch (_) { /* the cards keep what they were rendered with */ }
  }

  /* --- host gauges -------------------------------------------------------- */
  async function refreshHost() {
    const rings = $('host-rings');
    if (!rings) return;
    try {
      const data = await fetch('/api/system', { cache: 'no-store' }).then((response) => response.json());
      const sample = (data && data.system) || {};
      rings.querySelectorAll('[data-ring]').forEach((ring) => {
        const value = Math.round(Number(sample[`${ring.dataset.ring}_percent`] || 0));
        ring.style.setProperty('--value', value);
        ring.querySelector('b span').textContent = value;
      });
      const ram = $('readout-ram');
      if (ram && sample.ram_total_mb) ram.textContent = `${thousands(sample.ram_used_mb)} / ${thousands(sample.ram_total_mb)} MB`;
      const disk = $('readout-disk');
      if (disk && sample.disk_total_mb) {
        disk.textContent = `${(sample.disk_used_mb / 1024).toFixed(1)} / ${(sample.disk_total_mb / 1024).toFixed(1)} GB`;
      }
    } catch (_) { /* keep the rendered reading */ }
  }

  refreshSlow();
  setInterval(refreshSlow, 120000);
  setInterval(refreshHost, 30000);
})();
