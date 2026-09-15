/* The game-master fields only make sense for a non-PLAYER account kind. */
(() => {
  const chips = document.getElementById('account-kind-chips');
  const fields = document.querySelector('.gm-fields');
  if (!chips || !fields) return;

  const sync = () => {
    const chosen = chips.querySelector('input[name="authority"]:checked');
    fields.hidden = !chosen || chosen.value === 'PLAYER';
  };
  chips.addEventListener('change', sync);
  sync();
})();
