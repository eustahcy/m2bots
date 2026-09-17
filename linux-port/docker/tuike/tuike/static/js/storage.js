/* Inventory and safebox pages, and the switch between them, on a character
 * profile. Each storage window pages on its own: turning the backpack to
 * page II must not hide the safebox's page I. */
(() => {
  document.querySelectorAll('[data-storage-pane]').forEach((pane) => {
    const pageButtons = [...pane.querySelectorAll('.inventory-pages button')];
    const slots = [...pane.querySelectorAll('.grid-slot[data-page]')];

    function showPage(page) {
      pageButtons.forEach((button) => {
        const active = button.dataset.page === String(page);
        button.classList.toggle('active', active);
        // Pages 1 and 2 use the client's own button art, swapped between its
        // active and inactive state; later pages fall back to a plain label
        // and have nothing to swap.
        const image = button.querySelector('img');
        if (image) image.src = active ? image.dataset.active : image.dataset.inactive;
      });
      slots.forEach((slot) => { slot.hidden = slot.dataset.page !== String(page); });
    }

    pageButtons.forEach((button) => {
      button.addEventListener('click', () => showPage(Number(button.dataset.page)));
    });
    showPage(0);
  });

  const tabs = [...document.querySelectorAll('.storage .tabs button')];
  const panes = {
    inventory: document.getElementById('storage-inventory'),
    safebox: document.getElementById('storage-safebox'),
  };
  tabs.forEach((tab) => {
    tab.addEventListener('click', () => {
      tabs.forEach((other) => other.classList.toggle('active', other === tab));
      Object.entries(panes).forEach(([name, pane]) => {
        if (pane) pane.hidden = name !== tab.dataset.storage;
      });
    });
  });
})();
