/* Every chart in the panel.
 *
 * One file, loaded on any page that needs Chart.js; each block below draws
 * only if its canvas is actually on the page. Colours are read from the CSS
 * tokens, so a chart matches whichever theme the operator chose rather than
 * carrying a second, hard-coded palette that drifts from the first.
 */
(() => {
  if (typeof Chart === 'undefined') return;

  const styles = getComputedStyle(document.documentElement);
  const token = (name, fallback) => (styles.getPropertyValue(name).trim() || fallback);

  const INK = token('--text-dim', '#a8bcd6');
  const GRID = token('--grid', '#1d3048');
  const ACCENT = token('--accent', '#5795ff');
  const OK = token('--ok', '#3ed39b');
  const WARN = token('--warn', '#f2c34d');
  const SURFACE = token('--surface-3', '#1d2f49');

  // Enough hues to keep twenty map series apart without repeating early.
  const SERIES = ['#4ea5ff', '#36d399', '#f6c85f', '#c084fc', '#fb7185', '#22d3ee',
                  '#f97316', '#a3e635', '#e879f9', '#facc15', '#14b8a6', '#ef4444',
                  '#818cf8', '#84cc16', '#f472b6', '#38bdf8', '#fca5a5', '#4ade80',
                  '#fbbf24', '#a78bfa'];

  Chart.defaults.color = INK;
  Chart.defaults.font.family = styles.getPropertyValue('--font').trim() || 'system-ui, sans-serif';

  const json = (id) => {
    const node = document.getElementById(id);
    if (!node) return null;
    try { return JSON.parse(node.textContent); } catch (_) { return null; }
  };

  const axes = (extra = {}) => ({
    x: { ticks: { color: INK, maxTicksLimit: 12 }, grid: { color: GRID }, ...(extra.x || {}) },
    y: { ticks: { color: INK, precision: 0 }, grid: { color: GRID }, beginAtZero: true, ...(extra.y || {}) },
  });

  const line = (canvas, labels, datasets, scales) => new Chart(canvas, {
    type: 'line',
    data: { labels, datasets },
    options: {
      responsive: true, maintainAspectRatio: false,
      interaction: { mode: 'index', intersect: false },
      plugins: { legend: { display: datasets.length > 1, labels: { color: INK, boxWidth: 10 } } },
      scales: scales || axes(),
    },
  });

  const area = (colour) => ({
    borderColor: colour, backgroundColor: colour + '22',
    fill: true, tension: .28, pointRadius: 0, pointHoverRadius: 4, borderWidth: 2,
  });

  /* --- donuts: one value against its remainder --------------------------- */
  const donut = (canvas, value, label, colour) => new Chart(canvas, {
    type: 'doughnut',
    data: {
      labels: [label, 'Wolne'],
      datasets: [{
        data: [Math.max(0, value), Math.max(0, 100 - value)],
        backgroundColor: [colour, SURFACE], borderWidth: 0,
      }],
    },
    options: { cutout: '73%', responsive: true, maintainAspectRatio: false,
               plugins: { legend: { display: false } } },
  });

  async function drawLoadDonuts() {
    const cpu = document.getElementById('cpu-donut');
    if (!cpu) return;
    let sample = {};
    try {
      const response = await fetch('/api/system', { cache: 'no-store' });
      sample = (await response.json()).system || {};
    } catch (_) { /* an empty sample draws empty rings, which is honest */ }
    donut(cpu, Number(sample.cpu_percent || 0), 'CPU', ACCENT);
    const ram = document.getElementById('ram-donut');
    if (ram) donut(ram, Number(sample.ram_percent || 0), 'RAM', OK);
    const disk = document.getElementById('disk-donut');
    if (disk) donut(disk, Number(sample.disk_percent || 0), 'Dysk', WARN);
  }
  drawLoadDonuts();

  /* --- map distribution --------------------------------------------------- */
  const mapRows = json('map-data');
  const mapDonut = document.getElementById('map-donut');
  if (mapRows && mapDonut) {
    const populated = mapRows.filter((row) => row.character_count > 0);
    new Chart(mapDonut, {
      type: 'doughnut',
      data: {
        labels: populated.map((row) => row.name),
        datasets: [{
          data: populated.map((row) => row.character_count),
          backgroundColor: populated.map((_row, index) => SERIES[index % SERIES.length]),
          borderWidth: 2, borderColor: token('--surface', '#111c2e'),
        }],
      },
      options: {
        cutout: '58%', responsive: true, maintainAspectRatio: false,
        plugins: { legend: { position: 'right',
                             labels: { color: INK, boxWidth: 9, font: { size: 10 } } } },
      },
    });
  }

  /* --- economy ------------------------------------------------------------ */
  const yang = json('yang-data');
  if (yang && document.getElementById('yang-chart')) {
    line(document.getElementById('yang-chart'),
         yang.map((row) => row.captured_at),
         [{ label: 'Yang w obiegu', data: yang.map((row) => row.value), ...area(ACCENT) }]);
  }

  const itemHistory = json('item-data');
  if (itemHistory && document.getElementById('item-chart')) {
    line(document.getElementById('item-chart'),
         itemHistory.map((row) => row.captured_at),
         [{ label: 'Sztuk na serwerze', data: itemHistory.map((row) => row.amount), ...area(WARN) }]);
  }

  /* --- host telemetry ----------------------------------------------------- */
  const samples = json('system-data');
  if (samples && document.getElementById('system-chart')) {
    line(document.getElementById('system-chart'),
         samples.map((row) => row.label),
         [
           { label: 'CPU %', data: samples.map((row) => row.cpu_percent), ...area(ACCENT), fill: false },
           { label: 'RAM %', data: samples.map((row) => row.ram_percent), ...area(OK), fill: false },
           { label: 'Dysk %', data: samples.map((row) => row.disk_percent), ...area(WARN), fill: false },
         ],
         axes({ y: { min: 0, max: 100 } }));
  }

  /* --- map population over time ------------------------------------------ */
  const history = json('maps-data');
  const mapsCanvas = document.getElementById('maps-chart');
  if (history && mapsCanvas) {
    const shown = new Set(history.series.map((series) => series.id));
    const filters = document.getElementById('map-filters');
    const rows = [...document.querySelectorAll('.map-table tr[data-map-index]')];

    const dataset = (series) => {
      const colour = SERIES[history.series.indexOf(series) % SERIES.length];
      return {
        label: series.name, data: series.data,
        borderColor: colour, backgroundColor: colour + '22',
        borderWidth: 2, tension: .25, pointRadius: 0, pointHoverRadius: 4,
      };
    };

    const chart = new Chart(mapsCanvas, {
      type: 'line',
      data: { labels: history.labels, datasets: [] },
      options: {
        responsive: true, maintainAspectRatio: false,
        interaction: { mode: 'index', intersect: false },
        plugins: {
          legend: { display: false },
          tooltip: {
            callbacks: {
              title: (items) => (items.length ? `Godzina: ${items[0].label}` : ''),
              label: (item) => `${item.dataset.label}: ${item.formattedValue} postaci`,
            },
          },
        },
        scales: axes(),
      },
    });

    const render = () => {
      chart.data.datasets = history.series.filter((series) => shown.has(series.id)).map(dataset);
      chart.update();
      filters.querySelectorAll('input').forEach((input) => {
        input.checked = shown.has(Number(input.value));
      });
      rows.forEach((row) => {
        row.classList.toggle('selected',
          shown.size === 1 && shown.has(Number(row.dataset.mapIndex)));
      });
    };

    history.series.forEach((series, index) => {
      const label = document.createElement('label');
      const input = document.createElement('input');
      input.type = 'checkbox';
      input.value = series.id;
      input.checked = true;
      const swatch = document.createElement('i');
      swatch.style.setProperty('--map-color', SERIES[index % SERIES.length]);
      const name = document.createElement('span');
      name.textContent = series.name;
      input.addEventListener('change', () => {
        const id = Number(input.value);
        if (input.checked) shown.add(id); else shown.delete(id);
        render();
      });
      label.append(input, swatch, name);
      filters.append(label);
    });

    const only = (id) => {
      shown.clear();
      shown.add(Number(id));
      render();
      mapsCanvas.scrollIntoView({ behavior: 'smooth', block: 'center' });
    };
    rows.forEach((row) => {
      row.addEventListener('click', () => only(row.dataset.mapIndex));
      row.addEventListener('keydown', (event) => {
        if (event.key === 'Enter' || event.key === ' ') {
          event.preventDefault();
          only(row.dataset.mapIndex);
        }
      });
    });
    document.getElementById('maps-all').addEventListener('click', () => {
      history.series.forEach((series) => shown.add(series.id));
      render();
    });
    render();
  }
})();
