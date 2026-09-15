/* The standalone heat map on the map-activity page.
 *
 * Same data as the dashboard's heat mode, but its own stage: here the operator
 * is comparing maps deliberately rather than watching one.
 */
(() => {
  const stage = document.getElementById('heat-map-stage');
  if (!stage) return;

  const mapSelect = document.getElementById('heat-map');
  const kindSelect = document.getElementById('heat-kind');
  const caption = document.getElementById('heat-caption');
  const count = document.getElementById('heat-count');

  const place = (value) => `${Math.max(1, Math.min(99, value))}%`;

  async function render() {
    const option = mapSelect.options[mapSelect.selectedIndex];
    stage.dataset.mapIndex = mapSelect.value;
    stage.style.setProperty('--aspect', option.dataset.aspect || '1 / 1');
    stage.style.backgroundImage = option.dataset.background
      ? `linear-gradient(#00000030, #00000030), url('${option.dataset.background}')`
      : 'none';
    caption.textContent = option.textContent.trim();
    stage.querySelectorAll('.heat-point').forEach((node) => node.remove());

    const response = await fetch(`/api/heat-events?type=${encodeURIComponent(kindSelect.value)}`,
                                 { cache: 'no-store' });
    const data = await response.json();
    if (!data.ok) return;

    const index = Number(mapSelect.value);
    const bound = data.bounds[String(index)] || data.bounds[index];
    const events = data.events.filter((event) => event.map_index === index);

    const fragment = document.createDocumentFragment();
    events.forEach((event) => {
      const dot = document.createElement('i');
      dot.className = 'heat-point';
      dot.style.left = place((event.x - bound[0]) / bound[2] * 100);
      dot.style.top = place((event.y - bound[1]) / bound[3] * 100);
      dot.title = `${event.name || 'Zdarzenie'} · ${String(event.time).slice(11, 16)}`;
      fragment.append(dot);
    });
    stage.append(fragment);
    count.textContent = `${events.length} ${data.label} / 24 h`;
  }

  [mapSelect, kindSelect].forEach((node) => {
    node.addEventListener('input', () => render().catch(() => {}));
  });
  render().catch(() => { count.textContent = 'Brak danych'; });
})();
