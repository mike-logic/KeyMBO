# KeyMBO

**KeyMBO** is a USB HID + BLE input device built specifically for the **M5Stack AtomS3**. It combines native USB HID (keyboard + mouse), BLE control, and a lightweight web interface, using the AtomS3’s screen as the primary on-device UI.

The project is intentionally focused and opinionated:
- One target device (AtomS3)
- Native USB first
- BLE as a secondary control path
- Simple web UI where it makes sense

---

## 🧠 Architecture

KeyMBO combines **native USB HID**, **BLE**, and **web-based control surfaces** into a single device focused on flexible input.

At a high level:

- **USB HID** is the primary output path — KeyMBO appears as a standard keyboard and mouse to the host computer.
- **BLE** provides a wireless control channel (typically from a phone or tablet).
- **Web apps (PWA / local UI)** provide higher-level input tools and scripting.

There is a single firmware image and no role-based modes.

---


## 🚀 Getting Started

### Flashing the Firmware

KeyMBO is currently intended **only** for the **M5Stack AtomS3**.

You can flash the device in two ways:

#### Option 1: Flash from Your Browser (Recommended)

KeyMBO can be flashed directly from your browser using **Chrome or Edge** — no IDE required.

👉 **https://mike-logic.github.io/KeyMBO/**

Plug in your AtomS3 via USB-C, click **Install**, and follow the prompts.

#### Option 2: Flash via PlatformIO

```bash
cd firmware
pio run -t upload
pio run -t uploadfs
```

After flashing, the device will enumerate as a USB HID keyboard and mouse when plugged into a host.

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

BLE provides a wireless control path to KeyMBO and is typically used by the web apps.

Through BLE, clients can:
- Send keyboard input
- Send mouse movement / clicks
- Trigger stored macros or scripts

BLE does **not** require Wi-Fi and is active whenever the device is powered.

---


## 🖥 Web Apps (Local + Hosted)

KeyMBO includes multiple web-based control surfaces designed to work both **locally** (served by the device) and **remotely** (hosted PWA).

### Local Web UI (on-device)

When KeyMBO is connected to Wi-Fi, it can serve a lightweight local interface directly from the device.

This UI is intended for:
- Basic device interaction
- Debugging and development
- Direct control without installing anything

### Hosted PWA

A full-featured Progressive Web App is hosted externally and intended for daily use from mobile devices:

👉 **https://mybad.website/keymbo**

The PWA provides higher-level input tools, including:

- **Virtual mousepad** — touch-based mouse movement and clicking
- **Virtual keyboard** — send key presses wirelessly
- **Ducky-style scripting** — define and trigger scripted input sequences
- **Locally stored scripts** — scripts are stored in the browser and sent to the device on demand

This allows complex input behavior without reflashing the device.

Wi-Fi is optional for the PWA — BLE is the primary communication channel.

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

