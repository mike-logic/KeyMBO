# KeyMBO

**KeyMBO** is a USB HID + BLE input device built specifically for the **M5Stack AtomS3**. It combines native USB HID (keyboard + mouse), BLE control, and a lightweight web interface, using the AtomS3’s screen as the primary on-device UI.

The project is intentionally focused and opinionated:
- One target device (AtomS3)
- Native USB first
- BLE as a secondary control path
- Simple web UI where it makes sense

---

## 🧠 Architecture

KeyMBO operates in two parallel control paths:

- **USB HID** — The device presents itself as a standard USB keyboard and mouse when plugged into a host.
- **BLE** — Used for remote input/control (typically from a phone or PWA).

Optionally, the device can connect to **Wi-Fi** to serve a small web UI and expose WebSocket endpoints for interaction.

There is no external configuration system and no role-based firmware — everything is handled by a single firmware image.

---

## 🚀 Getting Started

### 1. Flash the Firmware

KeyMBO is currently intended **only** for the **M5Stack AtomS3**, due to its screen size, button layout, and native USB support.

```bash
cd firmware
pio run -t upload
pio run -t uploadfs
```

After flashing:
- Plug the AtomS3 into a host computer via USB-C
- It will enumerate as a USB HID device

---

## 🔌 USB HID Mode

When connected over USB, KeyMBO acts as:

- USB Keyboard
- USB Mouse
- (Optional) Consumer control device

No drivers are required on the host system.

USB HID is always available when the device is plugged in, regardless of BLE or Wi-Fi state.

---

## 📡 BLE Mode

BLE provides a wireless control path to KeyMBO.

Typical uses:
- Sending key presses or mouse movement from a phone
- Remote interaction without touching the device

BLE is active whenever the device is powered.

The PWA or other BLE clients can:
- Scan for the KeyMBO device
- Connect and send input commands

BLE does **not** require Wi-Fi.

---

## 📶 Wi-Fi + Web UI

Wi-Fi is optional and used only for:
- Hosting a small web UI
- WebSocket-based control and debugging

### Connecting to Wi-Fi

On first boot (or when Wi-Fi is not configured):

1. The device creates a temporary access point
2. Connect to the AP from your phone or computer
3. Select your local Wi-Fi network

Once connected:
- The device will display its IP address
- Open that address in a browser to access the web UI

Wi-Fi is **not required** for USB HID or BLE operation.

---

## 🖥 On-Device UI (AtomS3)

The AtomS3 screen is used for:
- Connection status (USB / BLE / Wi-Fi)
- Basic state feedback
- Debug visibility during development

This is why the firmware is currently AtomS3-only.

---

## 📁 Repository Layout

```
KeyMBO/
├─ firmware/
│  ├─ src/
│  │  └─ main.cpp
│  ├─ data/
│  │  └─ index.html
│  ├─ include/
│  ├─ lib/
│  ├─ test/
│  └─ platformio.ini
│
└─ PWA/
   ├─ index.php
   ├─ app.js
   ├─ sw-*.js
   ├─ manifest.webmanifest
   ├─ icon-192.svg
   └─ icon-512.svg
```

---

## ⚠️ Project Status

This project is **active and experimental**.

Expect:
- Breaking changes
- Iterative refactors
- Firmware behavior to evolve alongside hardware ideas

The focus is flexibility and experimentation, not long-term stability.

---

## 📜 License

MIT License.

