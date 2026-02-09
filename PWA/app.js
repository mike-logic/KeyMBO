(() => {
  const SVC_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
  const RX_UUID  = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
  const TX_UUID  = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

  const $ = (id) => document.getElementById(id);
  const setText = (el, v) => { if (el) el.textContent = (v ?? "").toString(); };
  const setShow = (el, show) => { if (el) el.style.display = show ? "" : "none"; };

  const statusBadge = $('statusBadge');
  const statusText  = $('statusText');

  const devName   = $('devName');
  const devPinReq = $('devPinReq');
  const devMedia  = $('devMedia');
  const devHID    = $('devHID');
  const devMode   = $('devMode');

  const debugText = $('debugText');
  const pinInput  = $('pinInput');

  const connectBtn    = $('connectBtn');
  const disconnectBtn = $('disconnectBtn');

  const menuBtn      = $('menuBtn');
  const slidePanel   = $('slidePanel');
  const panelOverlay = $('panelOverlay');
  const closePanel   = $('closePanel');

  const hidResetBtn  = $('hidResetBtn');

  const touchpad = $('touchpad');

  // Install
  const installBtn  = $('installBtn');
  const installHint = $('installHint');
  let deferredInstallPrompt = null;

  // Type modal
  const typeModal  = $('typeModal');
  const typeBtn    = $('typeBtn');
  const cancelType = $('cancelType');
  const sendType   = $('sendType');
  const typeInput  = $('typeInput');

  // Macros
  const macroSelect = $('macroSelect');
  const macroName   = $('macroName');
  const macroBody   = $('macroBody');
  const macroNew    = $('macroNew');
  const macroDelete = $('macroDelete');
  const macroSave   = $('macroSave');
  const macroRun    = $('macroRun');

  let bleDevice = null;
  let bleServer = null;
  let bleSvc = null;
  let bleRx = null;
  let bleTx = null;

  // default to non-blocking
  let pinRequired = false;
  let pinOk = true;

  let writeChain = Promise.resolve();

  function setStatus(text, state) {
    setText(statusText, text);
    if (!statusBadge) return;
    statusBadge.classList.remove('connected','warn','bad');
    if (state === 'ok') statusBadge.classList.add('connected');
    else if (state === 'bad') statusBadge.classList.add('bad');
    else statusBadge.classList.add('warn');
  }

  function setDebug(s) { setText(debugText, s); }
  function currentPin() { return ((pinInput && pinInput.value) || "").trim(); }

  // Only attach pin if device requires it OR user entered one
  function mk(payload) {
    const out = Object.assign({}, payload);
    const p = currentPin();
    if (pinRequired || p.length) out.pin = p;
    return out;
  }

  function enc(obj) { return new TextEncoder().encode(JSON.stringify(obj)); }

  async function bleWrite(bytes) {
    if (!bleRx) throw new Error("Not connected");
    writeChain = writeChain.then(async () => {
      if (bleRx.writeValueWithoutResponse) await bleRx.writeValueWithoutResponse(bytes);
      else await bleRx.writeValue(bytes);
    });
    return writeChain;
  }

  async function send(payload) {
    if (!bleRx) { setStatus("Disconnected", "warn"); return false; }
    try {
      await bleWrite(enc(mk(payload)));
      return true;
    } catch (e) {
      setDebug(`TX error: ${e?.message || e}`);
      setStatus("BLE write failed", "bad");
      return false;
    }
  }

  function tapKey(k) {
    send({ t:"k", k, d:1 });
    setTimeout(() => send({ t:"k", k, d:0 }), 25);
  }

  // ---- PWA install ----
  window.addEventListener('beforeinstallprompt', (e) => {
    e.preventDefault();
    deferredInstallPrompt = e;
    setShow(installBtn, true);
  });
  if (installBtn) {
    installBtn.addEventListener('click', async () => {
      if (!deferredInstallPrompt) { setShow(installHint, true); return; }
      deferredInstallPrompt.prompt();
      await deferredInstallPrompt.userChoice.catch(()=>{});
      deferredInstallPrompt = null;
      setShow(installBtn, false);
    });
  }
  window.addEventListener('appinstalled', () => {
    deferredInstallPrompt = null;
    setShow(installBtn, false);
  });

  // ---- Panel controls ----
  function openPanel(){
    if (slidePanel) slidePanel.classList.add('active');
    if (panelOverlay) panelOverlay.classList.add('active');
  }
  function closePanelFn(){
    if (slidePanel) slidePanel.classList.remove('active');
    if (panelOverlay) panelOverlay.classList.remove('active');
  }
  if (menuBtn) menuBtn.addEventListener('click', openPanel);
  if (closePanel) closePanel.addEventListener('click', closePanelFn);
  if (panelOverlay) panelOverlay.addEventListener('click', closePanelFn);

  // ---- Type modal ----
  if (typeBtn) typeBtn.addEventListener('click', () => {
    if (!typeModal) return;
    typeModal.classList.add('active');
    setTimeout(() => typeInput && typeInput.focus(), 80);
  });
  if (cancelType) cancelType.addEventListener('click', () => {
    if (!typeModal) return;
    typeModal.classList.remove('active');
    if (typeInput) typeInput.value='';
  });
  if (sendType) sendType.addEventListener('click', async () => {
    const text = (typeInput && typeInput.value) || '';
    if (!text.trim()) return;
    if (await send({ t:"txt", s:text })) {
      if (typeModal) typeModal.classList.remove('active');
      if (typeInput) typeInput.value='';
    }
  });
  if (typeModal) typeModal.addEventListener('click', (e) => {
    if (e.target === typeModal) typeModal.classList.remove('active');
  });

  // ---- Hold buttons ----
  function bindHoldButton(el, msgDown, msgUp) {
    if (!el) return;
    el.addEventListener('pointerdown', async (e) => {
      el.setPointerCapture(e.pointerId);
      await send(msgDown);
    });
    const up = () => send(msgUp);
    el.addEventListener('pointerup', up);
    el.addEventListener('pointercancel', up);
    el.addEventListener('lostpointercapture', up);
    el.addEventListener('contextmenu', (e) => e.preventDefault());
  }
  bindHoldButton($('leftClick'),  { t:"mc", b:"l", d:1 }, { t:"mc", b:"l", d:0 });
  bindHoldButton($('rightClick'), { t:"mc", b:"r", d:1 }, { t:"mc", b:"r", d:0 });

  // ---- Volume / Scroll ----
  const volDown = $('volDown'); if (volDown) volDown.addEventListener('click', () => tapKey("VOLDN"));
  const volUp   = $('volUp');   if (volUp)   volUp.addEventListener('click',   () => tapKey("VOLUP"));
  const muteBtn = $('muteBtn'); if (muteBtn) muteBtn.addEventListener('click', () => tapKey("MUTE"));

  const scrollUp = $('scrollUp');     if (scrollUp) scrollUp.addEventListener('click',   () => send({ t:"ms", v:-8 }));
  const scrollDn = $('scrollDown');   if (scrollDn) scrollDn.addEventListener('click',   () => send({ t:"ms", v: 8 }));

  // ---- Shortcuts ----
  const shortcuts = {
    altTab: { t:"combo", mods:["ALT"], k:"TAB" },
    ctrlC:  { t:"combo", mods:["CTRL"], k:"c" },
    ctrlV:  { t:"combo", mods:["CTRL"], k:"v" },
    ctrlL:  { t:"combo", mods:["CTRL"], k:"l" },

    ctrlW:  { t:"combo", mods:["CTRL"], k:"w" },
    ctrlT:  { t:"combo", mods:["CTRL"], k:"t" },
    ctrlZ:  { t:"combo", mods:["CTRL"], k:"z" },
    ctrlY:  { t:"combo", mods:["CTRL"], k:"y" },
    ctrlF:  { t:"combo", mods:["CTRL"], k:"f" },
    altF4:  { t:"combo", mods:["ALT"],  k:"F4" },
    winD:   { t:"combo", mods:["GUI"],  k:"d" },

    tab:    { tap:"TAB" },
    enter:  { tap:"ENTER" },
    esc:    { tap:"ESC" },
    del:    { tap:"DELETE" },
    prtsc:  { tap:"PRINTSCREEN" },
    volup:  { tap:"VOLUP" },
    voldn:  { tap:"VOLDN" },
    mute:   { tap:"MUTE" }
  };

  document.querySelectorAll('[data-action]').forEach(btn => {
    btn.addEventListener('click', () => {
      const s = shortcuts[btn.dataset.action];
      if (!s) return;
      if (s.tap) tapKey(s.tap);
      else send(s);
    });
  });
  document.querySelectorAll('[data-shortcut]').forEach(btn => {
    btn.addEventListener('click', () => {
      const s = shortcuts[btn.dataset.shortcut];
      if (!s) return;
      if (s.tap) tapKey(s.tap);
      else send(s);
    });
  });
  document.querySelectorAll('[data-key]').forEach(btn => {
    btn.addEventListener('click', () => tapKey(btn.dataset.key));
  });

  // ---- Touchpad ----
  if (touchpad) {
    let lastPos = null;
    let activePointerId = null;
    let downAt = 0;
    let moved = false;

    function updateGlow(e){
      const rect = touchpad.getBoundingClientRect();
      const x = ((e.clientX - rect.left) / rect.width) * 100;
      const y = ((e.clientY - rect.top)  / rect.height) * 100;
      touchpad.style.setProperty('--mouse-x', `${x}%`);
      touchpad.style.setProperty('--mouse-y', `${y}%`);
    }

    touchpad.addEventListener('pointerdown', (e) => {
      touchpad.setPointerCapture(e.pointerId);
      activePointerId = e.pointerId;
      lastPos = { x: e.clientX, y: e.clientY };
      downAt = performance.now();
      moved = false;
      touchpad.classList.add('active');
      updateGlow(e);
    });

    touchpad.addEventListener('pointermove', (e) => {
      if (activePointerId !== e.pointerId || !lastPos) return;
      const dx = e.clientX - lastPos.x;
      const dy = e.clientY - lastPos.y;
      if (Math.abs(dx) + Math.abs(dy) > 2) moved = true;

      const speed = 1.25;
      if (dx || dy) send({ t:"mm", dx: Math.round(dx * speed), dy: Math.round(dy * speed) });

      lastPos = { x: e.clientX, y: e.clientY };
      updateGlow(e);
    });

    function endPointer(e){
      if (activePointerId !== e.pointerId) return;
      const elapsed = performance.now() - downAt;
      const wasTap = !moved && elapsed < 220;

      lastPos = null;
      activePointerId = null;
      touchpad.classList.remove('active');

      if (wasTap) {
        send({ t:"mc", b:"l", d:1 });
        setTimeout(() => send({ t:"mc", b:"l", d:0 }), 20);
      }
    }

    touchpad.addEventListener('pointerup', endPointer);
    touchpad.addEventListener('pointercancel', () => {
      lastPos = null; activePointerId = null;
      touchpad.classList.remove('active');
    });
  }

  // Prevent zoom on double-tap
  let lastTouchEnd = 0;
  document.addEventListener('touchend', (e) => {
    const now = Date.now();
    if (now - lastTouchEnd <= 300) e.preventDefault();
    lastTouchEnd = now;
  }, { passive:false });

  // ---- PIN persistence ----
  const savedPin = localStorage.getItem("keymbo_pin");
  if (savedPin && pinInput) pinInput.value = savedPin;
  if (pinInput) {
    pinInput.addEventListener('input', () => {
      const p = currentPin();
      if (p.length === 6) localStorage.setItem("keymbo_pin", p);
      pinOk = false;
    });
  }

  // ---- BLE notify handler ----
  function safeJsonParse(s) { try { return JSON.parse(s); } catch { return null; } }

  function onNotify(ev){
    const bytes = new Uint8Array(ev.target.value.buffer);
    const text = new TextDecoder().decode(bytes);
    const j = safeJsonParse(text);
    if (!j) { setDebug(`<= ${text}`); return; }

    if (j.t === "hello") {
      setText(devName, j.name ?? (bleDevice?.name || "?"));
      setText(devMode, j.mode ?? "ble");
      setText(devMedia, String(!!j.hasConsumer));
      setText(devHID, String(!!j.hasHID));

      pinRequired = !!j.pinRequired;
      setText(devPinReq, String(pinRequired));
      if (!pinRequired) pinOk = true;

      setStatus("Connected", "ok");
      setDebug("Hello received.");
      return;
    }

    if (j.t === "result" && j.status === "err") {
      if (j.msg === "bad_pin") {
        pinOk = false;
        setStatus("Bad PIN", "bad");
        setDebug("Device rejected PIN. Enter the correct 6-digit PIN.");
      } else {
        setDebug(`Device error: ${j.msg || "err"}`);
      }
      return;
    }

    setDebug(`<= ${text}`);
  }

  async function connectBLE(){
    if (!navigator.bluetooth) {
      setStatus("Web Bluetooth unavailable", "bad");
      setDebug("This browser/context does not support Web Bluetooth.");
      return;
    }

    setStatus("Choosing device…", "warn");
    setDebug("Requesting device…");

    try{
      bleDevice = await navigator.bluetooth.requestDevice({
        filters: [{ services: [SVC_UUID] }, { namePrefix: "KeyMBO" }],
        optionalServices: [SVC_UUID]
      });

      bleDevice.addEventListener('gattserverdisconnected', () => {
        setStatus("Disconnected", "warn");
        setDebug("GATT disconnected.");
        bleServer = bleSvc = bleRx = bleTx = null;
        setShow(connectBtn, true);
        setShow(disconnectBtn, false);
      });

      setStatus("Connecting…", "warn");
      setDebug(`Connecting to ${bleDevice.name || "device"}…`);

      bleServer = await bleDevice.gatt.connect();
      bleSvc = await bleServer.getPrimaryService(SVC_UUID);
      bleRx  = await bleSvc.getCharacteristic(RX_UUID);
      bleTx  = await bleSvc.getCharacteristic(TX_UUID);

      await bleTx.startNotifications();
      bleTx.addEventListener('characteristicvaluechanged', onNotify);

      setShow(connectBtn, false);
      setShow(disconnectBtn, true);

      // reset non-blocking defaults
      pinRequired = false;
      pinOk = true;

      setText(devName, bleDevice.name || "?");
      setText(devMode, "ble");
      setText(devMedia, "?");
      setText(devHID, "?");
      setText(devPinReq, "?");

      setStatus("Connected", "ok");
      setDebug("Connected. Sending hello…");

      await bleWrite(enc(mk({ t:"hello" })));

      // close panel after connect
      if (slidePanel) slidePanel.classList.remove('active');
      if (panelOverlay) panelOverlay.classList.remove('active');

    } catch (e) {
      setStatus("Connect failed", "bad");
      setDebug(`Connect error: ${e?.message || e}`);
    }
  }

  function disconnectBLE(){
    try { if (bleDevice?.gatt?.connected) bleDevice.gatt.disconnect(); } catch {}
  }

  if (connectBtn) connectBtn.addEventListener('click', connectBLE);
  if (disconnectBtn) disconnectBtn.addEventListener('click', disconnectBLE);

  if (hidResetBtn) {
    hidResetBtn.addEventListener('click', async () => {
      if (!bleRx) { setStatus("Connect first", "warn"); return; }
      setStatus("HID reset…", "warn");
      const ok = await send({ t:"hidreset" });
      if (ok) setStatus("HID reset sent", "ok");
    });
  }

  // --------------------------
  // ✅ Macros / Ducky support
  // --------------------------
  const LS_KEY = "keymbo_macros_v1";

  function loadMacros() {
    try {
      const raw = localStorage.getItem(LS_KEY);
      const arr = raw ? JSON.parse(raw) : [];
      return Array.isArray(arr) ? arr : [];
    } catch {
      return [];
    }
  }

  function saveMacros(arr) {
    localStorage.setItem(LS_KEY, JSON.stringify(arr));
  }

  function refreshMacroSelect(selectName = null) {
    if (!macroSelect) return;
    const macros = loadMacros();
    macroSelect.innerHTML = "";
    const opt0 = document.createElement("option");
    opt0.value = "";
    opt0.textContent = macros.length ? "— Select a macro —" : "— No macros saved —";
    macroSelect.appendChild(opt0);

    for (const m of macros) {
      const opt = document.createElement("option");
      opt.value = m.name;
      opt.textContent = m.name;
      macroSelect.appendChild(opt);
    }

    if (selectName) macroSelect.value = selectName;
  }

  function getSelectedMacro() {
    if (!macroSelect) return null;
    const name = macroSelect.value;
    if (!name) return null;
    const macros = loadMacros();
    return macros.find(m => m.name === name) || null;
  }

  function upsertMacro(name, body) {
    const macros = loadMacros();
    const trimmedName = (name || "").trim();
    if (!trimmedName) return { ok:false, err:"Name required" };

    const idx = macros.findIndex(m => m.name === trimmedName);
    const entry = { name: trimmedName, body: String(body || "") };

    if (idx >= 0) macros[idx] = entry;
    else macros.push(entry);

    macros.sort((a,b) => a.name.localeCompare(b.name));
    saveMacros(macros);
    return { ok:true };
  }

  function deleteMacro(name) {
    const macros = loadMacros().filter(m => m.name !== name);
    saveMacros(macros);
  }

  if (macroSelect) {
    macroSelect.addEventListener("change", () => {
      const m = getSelectedMacro();
      if (!m) return;
      if (macroName) macroName.value = m.name;
      if (macroBody) macroBody.value = m.body;
    });
  }

  if (macroNew) {
    macroNew.addEventListener("click", () => {
      if (macroSelect) macroSelect.value = "";
      if (macroName) macroName.value = "";
      if (macroBody) macroBody.value = "";
      macroName && macroName.focus();
    });
  }

  if (macroSave) {
    macroSave.addEventListener("click", () => {
      const res = upsertMacro(macroName?.value, macroBody?.value);
      if (!res.ok) {
        setStatus(res.err || "Save failed", "bad");
        return;
      }
      refreshMacroSelect((macroName?.value || "").trim());
      setStatus("Saved macro", "ok");
    });
  }

  if (macroDelete) {
    macroDelete.addEventListener("click", () => {
      const name = (macroName?.value || "").trim();
      if (!name) return;
      deleteMacro(name);
      refreshMacroSelect();
      if (macroSelect) macroSelect.value = "";
      if (macroName) macroName.value = "";
      if (macroBody) macroBody.value = "";
      setStatus("Deleted macro", "ok");
    });
  }

  function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

  function normKeyToken(tok) {
    const t = tok.toUpperCase();
    if (t === "CONTROL") return "CTRL";
    if (t === "WINDOWS") return "GUI";
    if (t === "WIN") return "GUI";
    if (t === "RETURN") return "ENTER";
    if (t === "ESCAPE") return "ESC";
    if (t === "DEL") return "DELETE";
    if (t === "PGUP") return "PAGE_UP";
    if (t === "PAGEUP") return "PAGE_UP";
    if (t === "PGDN") return "PAGE_DOWN";
    if (t === "PAGEDOWN") return "PAGE_DOWN";
    if (t === "PRTSCR") return "PRINTSCREEN";
    if (t === "PRTSC") return "PRINTSCREEN";
    if (t === "PRINT") return "PRINTSCREEN";
    return t;
  }

  async function playComboTokens(tokens) {
    if (!tokens.length) return;

    if (tokens.length === 1) {
      const k = tokens[0];
      if (k.length === 1) {
        send({ t:"k", k, d:1 });
        await sleep(10);
        send({ t:"k", k, d:0 });
      } else {
        tapKey(k);
      }
      return;
    }

    const mods = tokens.slice(0, -1);
    const last = tokens[tokens.length - 1];
    send({ t:"combo", mods, k:last });
  }

  let macroRunning = false;

  async function runDucky(scriptText) {
    if (macroRunning) return;
    macroRunning = true;

    try {
      if (!bleRx) {
        setStatus("Connect first", "warn");
        return;
      }

      setStatus("Running macro…", "warn");

      const lines = String(scriptText || "").replace(/\r\n/g, "\n").split("\n");
      for (let i = 0; i < lines.length; i++) {
        let line = lines[i].trim();
        if (!line) continue;

        if (/^(REM|#|\/\/)/i.test(line)) continue;

        const mDelay = line.match(/^DELAY\s+(\d+)/i);
        if (mDelay) {
          await sleep(parseInt(mDelay[1], 10) || 0);
          continue;
        }

        const mStr = line.match(/^STRING\s+(.*)$/i);
        if (mStr) {
          const s = mStr[1] ?? "";
          if (s.length) await send({ t:"txt", s });
          await sleep(20);
          continue;
        }

        const tokens = line.split(/\s+/).map(normKeyToken).filter(Boolean);

        const looksLikeText =
          tokens.length > 1 &&
          tokens.every(t => !["CTRL","ALT","SHIFT","GUI","ENTER","TAB","ESC","SPACE","BACKSPACE","DELETE","UP","DOWN","LEFT","RIGHT","HOME","END","PAGE_UP","PAGE_DOWN","PRINTSCREEN"].includes(t)) &&
          tokens.every(t => !(t.length >= 2 && t[0] === "F" && Number.isFinite(parseInt(t.slice(1),10))));

        if (looksLikeText) {
          await send({ t:"txt", s: line });
          await sleep(20);
          continue;
        }

        await playComboTokens(tokens);
        await sleep(20);
      }

      setStatus("Macro done", "ok");
    } catch (e) {
      setDebug(`Macro error: ${e?.message || e}`);
      setStatus("Macro failed", "bad");
    } finally {
      macroRunning = false;
    }
  }

  if (macroRun) {
    macroRun.addEventListener("click", async () => {
      const body = macroBody?.value || "";
      if (!body.trim()) return;
      await runDucky(body);
    });
  }

  refreshMacroSelect();

  // Initial state
  setStatus("Disconnected", "warn");
  setDebug("Ready. Tap ☰ → Connect BLE.");
})();
