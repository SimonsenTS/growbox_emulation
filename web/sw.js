/* Cache public shell assets only. Never cache measurements, credentials or commands. */
'use strict';
const CACHE = 'growbox-shell-v1';
const SHELL = ['/app-shell.html', '/app.css', '/app.js', '/manifest.webmanifest', '/icon-192.png', '/icon-512.png'];
self.addEventListener('install', event => {
  event.waitUntil(caches.open(CACHE).then(cache => cache.addAll(SHELL)).then(() => self.skipWaiting()));
});
self.addEventListener('activate', event => {
  event.waitUntil(caches.keys().then(keys => Promise.all(keys.filter(key => key.startsWith('growbox-shell-') && key !== CACHE).map(key => caches.delete(key)))).then(() => self.clients.claim()));
});
self.addEventListener('fetch', event => {
  const url = new URL(event.request.url);
  if (url.origin !== self.location.origin || event.request.method !== 'GET') return;
  // These always go to the ESP32. Offline commands are never queued for replay.
  if (url.pathname.startsWith('/api/') || ['/events', '/login', '/logout', '/connect', '/simulation', '/scan-networks'].includes(url.pathname)) return;
  if (event.request.mode === 'navigate') {
    event.respondWith(fetch(event.request).then(response => response.status >= 500 ? caches.match('/app-shell.html').then(cached => cached || response) : response).catch(() => caches.match('/app-shell.html')));
    return;
  }
  if (SHELL.includes(url.pathname)) {
    // Revalidate online; cached files permit offline opening.
    event.respondWith(fetch(event.request).then(response => {
      if (!response.ok) return caches.match(url.pathname).then(cached => cached || response);
      if (response.ok) {
        const copy = response.clone();
        event.waitUntil(caches.open(CACHE).then(cache => cache.put(url.pathname, copy)));
      }
      return response;
    }).catch(() => caches.match(url.pathname)));
  }
});
