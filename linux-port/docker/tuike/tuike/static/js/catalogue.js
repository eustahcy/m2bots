/* Live filtering of the item catalogue.
 *
 * The whole catalogue is rendered at once and filtered here, so a typo costs
 * nothing and there are no pages to lose your place in.
 */
(() => {
  const search = document.getElementById('catalogue-search');
  const counter = document.getElementById('catalogue-count');
  if (!search || !counter) return;

  const cards = [...document.querySelectorAll('#catalogue .item-card')];

  function filter() {
    const needle = search.value.trim().toLocaleLowerCase('pl-PL');
    let visible = 0;
    cards.forEach((card) => {
      const match = !needle || card.dataset.search.includes(needle);
      card.hidden = !match;
      if (match) visible += 1;
    });
    counter.textContent = needle
      ? `${visible} z ${cards.length} przedmiotów pasuje do wyszukiwania`
      : `${cards.length} przedmiotów w tej kategorii`;
  }

  search.addEventListener('input', filter);
  if (search.value) filter();
})();
