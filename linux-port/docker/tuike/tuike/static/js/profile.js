/* A character profile's live parts: the core log and the stall teleport.
 *
 * The core log starts by itself and accumulates. Each poll returns the newest
 * matching tail of the syslogs, which overlaps the previous one; only lines
 * not already on screen are appended, so a busy log does not flicker or lose
 * the lines the operator was reading between two refreshes.
 */
(() => {
  const escape = (value) => String(value).replace(/[&<>"']/g, (character) => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;',
  }[character]));

  /* --- the core log ------------------------------------------------------- */
  const section = document.getElementById('core-log');
  const output = document.getElementById('core-log-lines');
  if (section && output) {
    const REFRESH = 4000;
    const KEEP = 400;
    const pauseButton = document.getElementById('core-log-pause');
    const copyButton = document.getElementById('core-log-copy');
    let lines = [];
    let paused = false;
    let started = false;

    // The same colour cues the classic panel uses: the AI's own reasoning in
    // the accent, trouble in red, loot and money in green, fights in amber.
    const tone = (line) => {
      if (/error|fail|nie uda|stuck|zawies/i.test(line)) return 'bad';
      if (/PLAYERBOT_AI|PLAYERBOT/.test(line)) return 'ai';
      if (/DROP_ITEM|GET_GOLD|PICK|SHOP|SELL|BUY/i.test(line)) return 'loot';
      if (/ATTACK|KILL|DEAD|DAMAGE|USE_SKILL/i.test(line)) return 'fight';
      return '';
    };

    function paint() {
      const nearBottom = output.scrollHeight - output.scrollTop - output.clientHeight < 40;
      output.innerHTML = lines.length
        ? lines.map((line) => `<span class="line ${tone(line)}">${escape(line)}</span>`).join('\n')
        : '<span class="muted">Rdzeń nie wspomniał tej postaci w ostatnich wpisach syslogu.</span>';
      if (nearBottom || !started) output.scrollTop = output.scrollHeight;
      started = true;
    }

    async function poll() {
      if (paused || document.hidden) return;
      try {
        const response = await fetch(section.dataset.url, { cache: 'no-store' });
        const data = await response.json();
        if (!data.ok) return;
        const fresh = data.logs || [];
        // Find where the new tail picks up after what is already shown.
        let overlap = 0;
        const last = lines[lines.length - 1];
        if (last !== undefined) {
          const at = fresh.lastIndexOf(last);
          overlap = at === -1 ? 0 : at + 1;
        }
        const added = fresh.slice(overlap);
        if (added.length || !started) {
          lines = [...lines, ...added].slice(-KEEP);
          paint();
        }
      } catch (_) {
        if (!started) output.innerHTML = '<span class="muted">Nie udało się odczytać dziennika rdzenia.</span>';
      }
    }

    pauseButton.addEventListener('click', () => {
      paused = !paused;
      pauseButton.textContent = paused ? 'Wznów' : 'Wstrzymaj';
      if (!paused) poll();
    });
    copyButton.addEventListener('click', async () => {
      const text = lines.join('\n');
      try {
        await navigator.clipboard.writeText(text);
        copyButton.classList.add('done');
        setTimeout(() => copyButton.classList.remove('done'), 1500);
      } catch (_) {
        // No clipboard permission (plain http): select the text instead.
        const range = document.createRange();
        range.selectNodeContents(output);
        const selection = window.getSelection();
        selection.removeAllRanges();
        selection.addRange(range);
      }
    });

    poll();
    setInterval(poll, REFRESH);
  }

  /* --- teleport to a stall ------------------------------------------------ */
  document.querySelectorAll('[data-warp]').forEach((button) => {
    const status = document.getElementById(button.dataset.status);
    button.addEventListener('click', async () => {
      button.disabled = true;
      status.hidden = false;
      status.className = 'warp-me-status';
      status.textContent = '⏳ Szukam Twojej postaci w grze…';
      try {
        const response = await fetch(button.dataset.action, {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: `csrf=${encodeURIComponent(button.dataset.csrf)}`,
        });
        const data = await response.json();
        status.classList.add(data.ok ? 'ok' : 'warn');
        status.textContent = data.ok ? `✅ Przeniesiono ${data.name} pod stragan.`
          : `⚠️ ${data.error || 'Nie udało się teleportować.'}`;
      } catch (_) {
        status.classList.add('warn');
        status.textContent = '⚠️ Brak połączenia z panelem.';
      } finally {
        button.disabled = false;
      }
    });
  });
})();
