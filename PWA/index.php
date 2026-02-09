<?php
// ---- Force trailing slash so relative URLs don't break ----
$uri = $_SERVER['REQUEST_URI'] ?? '';
if (preg_match('#^/keymbo$#', $uri)) {
  header("Location: /keymbo/", true, 301);
  exit;
}
$BASE = "/keymbo/";
?><!doctype html>
<html lang="en">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" />
<meta name="theme-color" content="#0f172a" />
<title>KeyMBO BLE</title>

<!-- Make ALL relative URLs resolve under /keymbo/ -->
<base href="<?php echo htmlspecialchars($BASE, ENT_QUOTES); ?>">

<link rel="manifest" href="<?php echo $BASE; ?>manifest.webmanifest">

<style>
* { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
:root{
  --primary:#2563eb; --primary-dark:#1e40af;
  --success:#10b981; --danger:#ef4444; --warn:#f59e0b;
  --bg:#0f172a; --surface:#1e293b; --surface-light:#334155;
  --border:#475569; --text:#f1f5f9; --text-dim:#94a3b8;
}
html, body { height: 100%; }
body{
  font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',system-ui,sans-serif;
  margin:0; padding:0;
  background:var(--bg); color:var(--text);
  overflow:hidden;
  height:100dvh;
  display:flex; flex-direction:column;
}
header{
  background:var(--surface);
  padding:12px 16px;
  display:flex;
  align-items:center;
  justify-content:space-between;
  border-bottom:1px solid var(--border);
  flex-shrink:0;
}
.header-left{
  display:flex;
  align-items:center;
  gap:12px;
  min-width:0;
}
h1{
  margin:0;
  font-size:18px;
  font-weight:700;
  white-space:nowrap;
  overflow:hidden;
  text-overflow:ellipsis;
}
.status-badge{
  display:inline-flex;
  align-items:center;
  gap:6px;
  padding:4px 10px;
  border-radius:12px;
  font-size:12px;
  font-weight:700;
  background:var(--surface-light);
  flex-shrink:0;
}
.status-dot{
  width:8px;
  height:8px;
  border-radius:50%;
  background:var(--text-dim);
}
.status-badge.connected .status-dot{
  background:var(--success);
  animation:pulse 2s ease-in-out infinite;
}
.status-badge.warn .status-dot{ background:var(--warn); }
.status-badge.bad .status-dot{ background:var(--danger); }
@keyframes pulse{ 0%,100%{opacity:1} 50%{opacity:.5} }

.menu-btn{
  background:none;
  border:none;
  color:var(--text);
  font-size:24px;
  cursor:pointer;
  padding:4px 8px;
  display:flex;
  align-items:center;
}

main{
  flex:1;
  display:flex;
  flex-direction:column;
  overflow:hidden;
  min-height:0;
}
.touchpad-container{
  flex:1;
  padding:16px;
  display:flex;
  flex-direction:column;
  min-height:0;
  overflow:hidden;
}
#touchpad{
  flex:1 1 auto;
  background:var(--surface);
  border:2px solid var(--border);
  border-radius:16px;
  touch-action:none;
  display:grid;
  place-items:center;
  user-select:none;
  position:relative;
  overflow:hidden;
  min-height:160px;
  max-height:clamp(240px, 46vh, 520px);
}
#touchpad::before{
  content:'';
  position:absolute;
  inset:0;
  background: radial-gradient(circle at var(--mouse-x, 50%) var(--mouse-y, 50%), rgba(37,99,235,0.12) 0%, transparent 52%);
  pointer-events:none;
  opacity:0;
  transition:opacity .15s;
}
#touchpad.active::before{ opacity:1; }

.touchpad-hint{
  color:var(--text-dim);
  font-size:14px;
  text-align:center;
  pointer-events:none;
  padding:0 12px;
  line-height:1.25;
}

.quick-actions{
  padding:8px 16px;
  background:var(--bg);
  border-top:1px solid var(--border);
  display:flex;
  gap:6px;
  overflow-x:auto;
  -webkit-overflow-scrolling:touch;
  scrollbar-width:none;
  flex-shrink:0;
}
.quick-actions::-webkit-scrollbar{ display:none; }

.quick-btn{
  padding:8px 14px;
  background:var(--surface);
  border:1px solid var(--border);
  border-radius:10px;
  color:var(--text);
  font-size:12px;
  font-weight:800;
  white-space:nowrap;
  cursor:pointer;
  flex-shrink:0;
  user-select:none;
  touch-action: manipulation;
}
.quick-btn:active{ background:var(--border); transform:scale(.98); }

.controls{
  padding:12px 16px calc(12px + env(safe-area-inset-bottom));
  background:var(--surface);
  border-top:1px solid var(--border);
  display:flex;
  gap:8px;
  flex-shrink:0;
}
.btn{
  flex:1;
  padding:14px;
  border:none;
  border-radius:12px;
  font-size:14px;
  font-weight:900;
  cursor:pointer;
  transition:transform .08s, background .15s;
  display:flex;
  align-items:center;
  justify-content:center;
  gap:6px;
  user-select:none;
  touch-action:none;
}
.btn-primary{ background:var(--primary); color:white; }
.btn-primary:active{ background:var(--primary-dark); transform:scale(.97); }
.btn-secondary{ background:var(--surface-light); color:var(--text); }
.btn-secondary:active{ background:var(--border); transform:scale(.97); }
.btn-danger{ background:var(--danger); color:white; }
.btn-danger:active{ transform:scale(.97); }
.btn-full{ width:100%; margin-top:8px; }

.panel-overlay{
  position:fixed;
  inset:0;
  background:rgba(0,0,0,0.5);
  z-index:100;
  opacity:0;
  pointer-events:none;
  transition:opacity .25s;
}
.panel-overlay.active{ opacity:1; pointer-events:all; }

.slide-panel{
  position:fixed;
  top:0;
  right:-100%;
  width:min(340px, 88vw);
  height:100dvh;
  background:var(--surface);
  z-index:101;
  transition:right .25s ease-out;
  overflow-y:auto;
  overflow-x:hidden;
  overscroll-behavior:contain;
  box-shadow:-4px 0 12px rgba(0,0,0,.3);
  padding-bottom: env(safe-area-inset-bottom);
}
.slide-panel.active{ right:0; }

.panel-header{
  padding:16px;
  border-bottom:1px solid var(--border);
  display:flex;
  align-items:center;
  justify-content:space-between;
  position:sticky;
  top:0;
  background:var(--surface);
  z-index:1;
}
.panel-header h2{ margin:0; font-size:18px; font-weight:900; }

.close-btn{
  background:none;
  border:none;
  color:var(--text);
  font-size:28px;
  cursor:pointer;
  padding:0;
  width:32px;
  height:32px;
  display:flex;
  align-items:center;
  justify-content:center;
}

.panel-section{ padding:16px; border-bottom:1px solid var(--border); }
.panel-section:last-child{ padding-bottom: calc(24px + env(safe-area-inset-bottom)); }
.panel-section h3{
  margin:0 0 12px 0;
  font-size:13px;
  font-weight:900;
  color:var(--text-dim);
  text-transform:uppercase;
  letter-spacing:.6px;
}

.kv{
  display:grid;
  grid-template-columns: 92px 1fr;
  gap: 8px 10px;
  align-items:center;
  font-size: 13px;
  color: var(--text-dim);
  margin-bottom: 10px;
}
.kv code{
  color: var(--text);
  background: var(--bg);
  border: 1px solid var(--border);
  padding: 2px 6px;
  border-radius: 8px;
  font-size: 12px;
}

.input-group{ margin-bottom:12px; }
.input-group label{
  display:block;
  font-size:13px;
  margin-bottom:6px;
  color:var(--text-dim);
}
input[type="number"], input[type="text"], select, textarea{
  width:100%;
  padding:12px;
  background:var(--bg);
  border:1px solid var(--border);
  border-radius:10px;
  color:var(--text);
  font-size:16px;
  outline:none;
  font-family:inherit;
}
input:focus, textarea:focus, select:focus{ border-color:var(--primary); }

.key-grid{
  display:grid;
  grid-template-columns:repeat(2, 1fr);
  gap:8px;
}
.key-btn{
  padding:12px;
  background:var(--bg);
  border:1px solid var(--border);
  border-radius:10px;
  color:var(--text);
  font-size:13px;
  font-weight:900;
  cursor:pointer;
  user-select:none;
  touch-action: manipulation;
}
.key-btn:active{ background:var(--border); transform:scale(.97); }

.scroll-controls{ display:flex; gap:8px; }
.scroll-controls .btn{ padding:12px; }

.modal-overlay{
  position:fixed;
  inset:0;
  background:rgba(0,0,0,0.7);
  z-index:200;
  display:none;
  align-items:center;
  justify-content:center;
  padding:20px;
}
.modal-overlay.active{ display:flex; }
.modal{
  background:var(--surface);
  border-radius:16px;
  width:min(420px, 100%);
  max-height:90dvh;
  overflow:hidden;
  animation: modalIn .22s ease-out;
}
@keyframes modalIn{
  from{ opacity:0; transform:translateY(16px) scale(.97);}
  to{ opacity:1; transform:translateY(0) scale(1);}
}
.modal-header{ padding:18px 18px 12px; border-bottom:1px solid var(--border); }
.modal-header h2{ margin:0; font-size:18px; font-weight:900; }
.modal-body{ padding:16px 18px; max-height:62dvh; overflow-y:auto; }
.modal-footer{
  padding:14px 18px calc(14px + env(safe-area-inset-bottom));
  border-top:1px solid var(--border);
  display:flex;
  gap:10px;
  justify-content:flex-end;
}
.btn-modal{
  padding:10px 16px;
  border:none;
  border-radius:10px;
  font-size:14px;
  font-weight:900;
  cursor:pointer;
  color:var(--text);
  background:var(--surface-light);
}
.btn-confirm{ background:var(--primary); color:white; }

.small{
  font-size:12px;
  color:var(--text-dim);
  line-height:1.35;
}
.row{ display:flex; gap:8px; }
.row > * { flex:1; }

details.macro-help{
  margin-top:10px;
  border:1px solid var(--border);
  border-radius:12px;
  background:rgba(15,23,42,0.55);
  overflow:hidden;
}
details.macro-help summary{
  cursor:pointer;
  user-select:none;
  padding:10px 12px;
  font-weight:900;
  font-size:12px;
  color:var(--text);
  background:rgba(51,65,85,0.35);
  list-style:none;
}
details.macro-help summary::-webkit-details-marker{ display:none; }
details.macro-help .help-body{
  padding:10px 12px 12px;
  color:var(--text-dim);
  font-size:12px;
  line-height:1.4;
}
details.macro-help code{
  color:var(--text);
  background: var(--bg);
  border:1px solid var(--border);
  padding:1px 6px;
  border-radius:8px;
  font-size:12px;
}

@media (max-height: 700px){
  #touchpad{ max-height: clamp(200px, 40vh, 420px); min-height: 140px; }
  .btn{ padding:12px; }
}
</style>
</head>
<body>

<header>
  <div class="header-left">
    <h1>KeyMBO BLE</h1>
    <div class="status-badge warn" id="statusBadge">
      <span class="status-dot"></span>
      <span id="statusText">Disconnected</span>
    </div>
  </div>
  <button class="menu-btn" id="menuBtn">☰</button>
</header>

<main>
  <div class="touchpad-container">
    <div id="touchpad">
      <div class="touchpad-hint">
        <div style="font-size: 40px; margin-bottom: 8px;">👆</div>
        <div>Tap to click • Drag to move</div>
        <div style="margin-top:6px; font-size:12px;">Use ☰ for connect + controls</div>
      </div>
    </div>
  </div>

  <div class="quick-actions">
    <button class="quick-btn" data-action="altTab">Alt+Tab</button>
    <button class="quick-btn" data-action="ctrlC">Ctrl+C</button>
    <button class="quick-btn" data-action="ctrlV">Ctrl+V</button>
    <button class="quick-btn" data-action="ctrlL">Ctrl+L</button>
    <button class="quick-btn" data-action="enter">Enter</button>
    <button class="quick-btn" data-action="esc">Esc</button>
    <button class="quick-btn" data-action="del">Delete</button>
    <button class="quick-btn" data-action="prtsc">PrtSc</button>
    <button class="quick-btn" data-action="mute">Mute</button>
    <button class="quick-btn" data-action="voldn">Vol−</button>
    <button class="quick-btn" data-action="volup">Vol+</button>
  </div>

  <div class="controls">
    <button class="btn btn-secondary" id="leftClick">Left (hold)</button>
    <button class="btn btn-secondary" id="rightClick">Right (hold)</button>
    <button class="btn btn-primary" id="typeBtn">⌨️ Type</button>
  </div>
</main>

<div class="panel-overlay" id="panelOverlay"></div>

<div class="slide-panel" id="slidePanel">
  <div class="panel-header">
    <h2>BLE Controls</h2>
    <button class="close-btn" id="closePanel">&times;</button>
  </div>

  <div class="panel-section">
    <h3>Connection</h3>

    <div class="kv">
      <div>Name</div><div><code id="devName">?</code></div>
      <div>Mode</div><div><code id="devMode">ble</code></div>
      <div>PIN Req</div><div><code id="devPinReq">?</code></div>
      <div>Media</div><div><code id="devMedia">?</code></div>
      <div>HID</div><div><code id="devHID">?</code></div>
    </div>

    <div class="input-group">
      <label>Device PIN (optional unless required)</label>
      <input type="number" id="pinInput" placeholder="6-digit PIN (if enabled on device)" inputmode="numeric" />
    </div>

    <div class="row">
      <button class="btn btn-primary" id="connectBtn">Connect BLE</button>
      <button class="btn btn-secondary" id="disconnectBtn" style="display:none;">Disconnect</button>
    </div>

    <button class="btn btn-danger btn-full" id="hidResetBtn">🔌 HID Reset</button>
    <div class="small" style="margin-top:10px;">
      If the host slept and dropped the USB HID device, tap this to re-enumerate.
    </div>

    <button class="btn btn-secondary btn-full" id="installBtn" style="display:none;">⬇️ Install App</button>
    <div class="small" id="installHint" style="display:none; margin-top:8px;">
      Install isn’t available right now (already installed or browser doesn’t support prompt).
    </div>

    <div class="small" style="margin-top:10px;">
      <strong>Debug:</strong> <span id="debugText">Ready.</span>
    </div>
  </div>

  <div class="panel-section">
    <h3>Volume</h3>
    <div class="scroll-controls">
      <button class="btn btn-secondary btn-full" id="volDown">Vol −</button>
      <button class="btn btn-secondary btn-full" id="volUp">Vol +</button>
    </div>
    <button class="btn btn-secondary btn-full" id="muteBtn" style="margin-top:8px;">Mute</button>
  </div>

  <div class="panel-section">
    <h3>Keyboard Shortcuts</h3>
    <div class="key-grid">
      <button class="key-btn" data-shortcut="altF4">Alt+F4</button>
      <button class="key-btn" data-shortcut="ctrlW">Ctrl+W</button>
      <button class="key-btn" data-shortcut="ctrlT">Ctrl+T</button>
      <button class="key-btn" data-shortcut="ctrlZ">Ctrl+Z</button>
      <button class="key-btn" data-shortcut="ctrlY">Ctrl+Y</button>
      <button class="key-btn" data-shortcut="ctrlF">Ctrl+F</button>
      <button class="key-btn" data-shortcut="winD">Win+D</button>
      <button class="key-btn" data-shortcut="tab">Tab</button>
    </div>
  </div>

  <div class="panel-section">
    <h3>Scroll</h3>
    <div class="scroll-controls">
      <button class="btn btn-secondary btn-full" id="scrollUp">Scroll Up</button>
      <button class="btn btn-secondary btn-full" id="scrollDown">Scroll Down</button>
    </div>
  </div>

  <div class="panel-section">
    <h3>Special Keys</h3>
    <div class="key-grid">
      <button class="key-btn" data-key="BACKSPACE">Backspace</button>
      <button class="key-btn" data-key="DELETE">Delete</button>
      <button class="key-btn" data-key="HOME">Home</button>
      <button class="key-btn" data-key="END">End</button>
      <button class="key-btn" data-key="PAGE_UP">Page Up</button>
      <button class="key-btn" data-key="PAGE_DOWN">Page Dn</button>
      <button class="key-btn" data-key="PRINTSCREEN">Print Screen</button>
      <button class="key-btn" data-key="TAB">Tab</button>
    </div>
  </div>

  <div class="panel-section">
    <h3>Macros / Ducky</h3>

    <div class="input-group">
      <label>Saved Macros</label>
      <select id="macroSelect"></select>
    </div>

    <div class="row">
      <button class="btn btn-secondary" id="macroNew">New</button>
      <button class="btn btn-secondary" id="macroDelete">Delete</button>
    </div>

    <div class="input-group" style="margin-top:12px;">
      <label>Name</label>
      <input type="text" id="macroName" placeholder="e.g. Login / Open Terminal" />
    </div>

    <div class="input-group">
      <label>Script (Ducky-ish)</label>
      <textarea id="macroBody" rows="10" placeholder="Example:
REM Open Run dialog on Windows
GUI r
DELAY 200
STRING notepad
ENTER
DELAY 300
STRING hello from KeyMBO
ENTER"></textarea>
    </div>

    <div class="row">
      <button class="btn btn-secondary" id="macroSave">Save</button>
      <button class="btn btn-primary" id="macroRun">Run</button>
    </div>

    <!-- ✅ no tooltip; uses a dropdown (details) that won't clip -->
    <details class="macro-help">
      <summary>How to write macros (tap to expand)</summary>
      <div class="help-body">
        <div style="margin-bottom:8px;">
          Lines are executed top → bottom. Empty lines are ignored.
        </div>

        <div style="margin-bottom:8px;">
          <code>REM</code> comment<br>
          <code>DELAY 250</code> wait milliseconds<br>
          <code>STRING hello</code> types text
        </div>

        <div style="margin-bottom:8px;">
          Single keys: <code>ENTER</code>, <code>TAB</code>, <code>ESC</code>, <code>SPACE</code>, <code>BACKSPACE</code>, <code>DELETE</code>,
          arrows <code>UP</code>/<code>DOWN</code>/<code>LEFT</code>/<code>RIGHT</code>,
          <code>HOME</code>/<code>END</code>/<code>PAGEUP</code>/<code>PAGEDOWN</code>, <code>PRINTSCREEN</code>, <code>F1</code>..<code>F12</code>
        </div>

        <div style="margin-bottom:8px;">
          Combos: list modifiers then final key on one line:
          <br><code>CTRL ALT DEL</code>
          <br><code>GUI r</code> (Win+R)
          <br><code>CTRL SHIFT ESC</code>
        </div>

        <div>
          Saved macros live in this browser’s <code>localStorage</code> (per device/browser).
        </div>
      </div>
    </details>
  </div>
</div>

<div class="modal-overlay" id="typeModal">
  <div class="modal">
    <div class="modal-header"><h2>Type Text</h2></div>
    <div class="modal-body">
      <textarea id="typeInput" style="width:100%; min-height:120px; padding:12px; background:var(--bg); border:1px solid var(--border); border-radius:10px; color:var(--text); font-size:16px; resize:vertical; font-family:inherit;" placeholder="Enter text to send…"></textarea>
    </div>
    <div class="modal-footer">
      <button class="btn-modal" id="cancelType">Cancel</button>
      <button class="btn-modal btn-confirm" id="sendType">Send</button>
    </div>
  </div>
</div>

<script defer src="/keymbo/app.js?v=20260208-ble-2"></script>
<script>
  if ("serviceWorker" in navigator) {
    window.addEventListener("load", () => {
      navigator.serviceWorker.register("/keymbo/sw-20260207-01.js?v=20260208-ble-2").catch(()=>{});
    });
  }
</script>
</body>
</html>
