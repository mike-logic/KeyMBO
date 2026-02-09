/* sw-20260207-01.js — DROP-IN (fixed asset list)
   Network-first for navigations, cache-first for assets.
*/
const VERSION = "20260208-02";
const CACHE = `keymbo-${VERSION}`;
const ASSETS = [
  "./",
  "./index.php",
  "./app.js",
  "./manifest.webmanifest",
  "./icon-192.png",
  "./icon-512.png"
];

self.addEventListener("install", (event) => {
  self.skipWaiting();
  event.waitUntil(
    caches.open(CACHE).then((cache) => cache.addAll(ASSETS)).catch(() => {})
  );
});

self.addEventListener("activate", (event) => {
  event.waitUntil((async () => {
    const keys = await caches.keys();
    await Promise.all(keys.map((k) => (k === CACHE ? null : caches.delete(k))));
    await self.clients.claim();
  })());
});

self.addEventListener("fetch", (event) => {
  const req = event.request;
  const url = new URL(req.url);
  if (url.origin !== location.origin) return;

  if (req.mode === "navigate" || req.destination === "document") {
    event.respondWith((async () => {
      try {
        const net = await fetch(req, { cache: "no-store" });
        const cache = await caches.open(CACHE);
        cache.put(req, net.clone()).catch(() => {});
        return net;
      } catch {
        const cached = await caches.match("./index.php");
        return cached || caches.match("./") || Response.error();
      }
    })());
    return;
  }

  event.respondWith((async () => {
    const cached = await caches.match(req);
    if (cached) return cached;
    try {
      const net = await fetch(req);
      const cache = await caches.open(CACHE);
      cache.put(req, net.clone()).catch(() => {});
      return net;
    } catch {
      return cached || Response.error();
    }
  })());
});
