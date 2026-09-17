/* The stalls page charts: sales tempo with its price line, and stalls per map.
 * The live counters and the recent-sales table come from economy.js, which
 * this page shares with Gospodarka. */
(() => {
  if (typeof Chart === 'undefined') return;
  const pulseCanvas = document.getElementById('pulse-canvas');
  const mapCanvas = document.getElementById('shops-map-canvas');
  if (!pulseCanvas && !mapCanvas) return;

  const styles = getComputedStyle(document.documentElement);
  const token = (name, fallback) => (styles.getPropertyValue(name).trim() || fallback);
  const INK = token('--text-dim', '#a9bed8');
  const GRID = token('--grid', '#16253d');
  const ACCENT = token('--accent', '#5b9bff');
  const WARN = token('--warn', '#f5c451');
  const SERIES = ['#4f8cff', '#36d399', '#f6c85f', '#a78bfa', '#fb7185', '#22d3ee'];
  let pulseChart = null;
  let mapChart = null;

  function drawPulse(pulse) {
    const data = {
      labels: pulse.labels,
      datasets: [
        { type: 'bar', label: 'Sprzedaże', data: pulse.sales, yAxisID: 'sales',
          backgroundColor: `${ACCENT}99`, borderRadius: 4 },
        { type: 'line', label: 'Śr. cena za sztukę', data: pulse.avg_price, yAxisID: 'price',
          borderColor: WARN, backgroundColor: WARN, tension: .35, pointRadius: 0, borderWidth: 2, spanGaps: true },
      ],
    };
    if (pulseChart) { pulseChart.data = data; pulseChart.update('none'); return; }
    pulseChart = new Chart(pulseCanvas, {
      data,
      options: {
        responsive: true, maintainAspectRatio: false,
        interaction: { mode: 'index', intersect: false },
        plugins: { legend: { position: 'bottom', labels: { color: INK, boxWidth: 10 } } },
        scales: {
          x: { ticks: { color: INK, maxTicksLimit: 8 }, grid: { color: GRID } },
          sales: { position: 'left', beginAtZero: true, ticks: { color: INK, precision: 0 }, grid: { color: GRID } },
          price: { position: 'right', beginAtZero: true, grid: { display: false },
                   ticks: { color: INK, callback: (value) => Number(value).toLocaleString('pl-PL') } },
        },
      },
    });
  }

  function drawMaps(rows) {
    const data = {
      labels: rows.map((row) => row.name),
      datasets: [{ data: rows.map((row) => row.shop_count), borderRadius: 6,
                   backgroundColor: rows.map((_row, index) => SERIES[index % SERIES.length]) }],
    };
    if (mapChart) { mapChart.data = data; mapChart.update('none'); return; }
    mapChart = new Chart(mapCanvas, {
      type: 'bar',
      data,
      options: {
        indexAxis: 'y', responsive: true, maintainAspectRatio: false,
        plugins: { legend: { display: false } },
        scales: { x: { beginAtZero: true, ticks: { color: INK, precision: 0 }, grid: { color: GRID } },
                  y: { ticks: { color: INK }, grid: { display: false } } },
      },
    });
  }

  async function refresh() {
    try {
      const data = await fetch('/api/shops/pulse', { cache: 'no-store' }).then((response) => response.json());
      if (!data.ok) return;
      if (pulseCanvas) drawPulse(data.pulse);
      if (mapCanvas) drawMaps(data.by_map || []);
    } catch (_) { /* the next tick tries again */ }
  }

  refresh();
  setInterval(refresh, 120000);
})();
