/* State arrives via SSE; only changed fields are written to the DOM. */
'use strict';
const byId = id => document.getElementById(id);
const buttons = [...document.querySelectorAll('[data-device]')];
let current = null;
let live = false;
let lastHeard = 0;
let stream;
let sliderBusy = false;
const pending = new Set();
const units = {temperature: ' \u00b0C', humidity: '%', soil: '%', water: '%', lux: ' lx'};

function text(id, value) {
  const node = byId(id);
  if (node.textContent !== value) node.textContent = value;
}
function controls() {
  for (const button of buttons) {
    const device = button.dataset.device;
    button.disabled = !live || !current || pending.has(device) ||
      (device === 'boost' && !current.growLed) ||
      (device === 'pump' && !current.pump && (!current.fresh || current.water === null || current.soil === null || current.water <= 10 || current.soil >= 60));
  }
  byId('brightness').disabled = !live || sliderBusy;
  byId('simulation-submit').disabled = !live;
}
function connection(connected) {
  live = connected;
  document.body.dataset.live = String(live);
  let status = 'Disconnected \u2014 displayed values may be stale. Reconnecting\u2026';
  if (live) {
    status = current?.fresh ? 'Live \u00b7 updates arrive when values change' : 'Live \u00b7 waiting for fresh sensors; pump protected';
    if (current?.fresh && (current.water === null || current.soil === null)) status = 'Live \u00b7 sensor unavailable; pump protected';
  }
  text('connection', status);
  controls();
}
function render(state) {
  const previous = current;
  current = state;
  for (const [key, unit] of Object.entries(units)) {
    if (previous && previous[key] === state[key]) continue;
    text(key, state[key] === null ? 'Unavailable' : String(state[key]) + unit);
    const meter = byId(key + '-meter');
    if (meter) {
      meter.hidden = state[key] === null;
      if (state[key] !== null && meter.value !== state[key]) meter.value = state[key];
    }
  }
  for (const button of buttons) {
    const key = button.dataset.device;
    if (previous && previous[key] === state[key]) continue;
    text(key, state[key] ? 'ON' : 'OFF');
    button.setAttribute('aria-pressed', String(state[key]));
  }
  if (!sliderBusy && document.activeElement !== byId('brightness')) {
    const value = String(state.brightness);
    if (byId('brightness').value !== value) byId('brightness').value = value;
    text('brightness-value', value + '%');
  }
  for (const key of ['soilColor', 'waterColor']) {
    const visible = key === 'waterColor' && state.waterColor === 0xff0000 || state.rgb;
    const oldVisible = previous && (key === 'waterColor' && previous.waterColor === 0xff0000 || previous.rgb);
    if (previous && previous[key] === state[key] && visible === oldVisible) continue;
    byId(key).style.backgroundColor = visible ? '#' + state[key].toString(16).padStart(6, '0') : '#dce5dc';
  }
  lastHeard = Date.now();
  connection(true);
}
async function post(path, values) {
  const response = await fetch(path, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: new URLSearchParams(values), cache: 'no-store', signal: AbortSignal.timeout(5000)});
  if (response.status === 401) throw new Error('Login required. Open WiFi setup & login below.');
  if (!response.ok) {
    let error = 'Command failed (' + response.status + ')';
    try { error = (await response.json()).error || error; } catch (_) { /* non-JSON response */ }
    throw new Error(error);
  }
}
for (const button of buttons) button.addEventListener('click', async () => {
  if (!live) return;
  const device = button.dataset.device;
  pending.add(device);
  controls();
  text('message', '');
  try { await post('/api/control', {device}); }
  catch (error) { text('message', error.message + ' Check device state before retrying.'); }
  finally { pending.delete(device); controls(); }
});
byId('brightness').addEventListener('input', event => text('brightness-value', event.target.value + '%'));
byId('brightness').addEventListener('change', async event => {
  if (!live) return;
  sliderBusy = true;
  controls();
  try { await post('/api/control', {device: 'brightness', value: event.target.value}); text('message', ''); }
  catch (error) { text('message', error.message); }
  finally { sliderBusy = false; controls(); }
});
byId('brightness').addEventListener('blur', () => {
  if (current && !sliderBusy) { byId('brightness').value = current.brightness; text('brightness-value', current.brightness + '%'); }
});
byId('simulation-form').addEventListener('submit', async event => {
  event.preventDefault();
  if (!live) return;
  try { await post('/simulation', new FormData(event.target)); text('message', 'Simulation queued for the next sample.'); }
  catch (error) { text('message', error.message); }
});
async function config() {
  try {
    const response = await fetch('/api/config', {cache: 'no-store'});
    if (response.status === 401) { text('message', 'Login required. Open WiFi setup & login below.'); return; }
    if (response.ok) byId('simulation').hidden = !(await response.json()).simulation;
  } catch (_) { /* reconnect will retry */ }
}
function renderNetwork(network) {
  if (network.connected) {
    text('network', 'Home WiFi connected: ' + network.ssid + ' | ' + network.ip);
    text('home-ssid', network.ssid);
    const hostLink = byId('home-dashboard');
    hostLink.href = network.mdns ? network.dashboardUrl : network.ipUrl;
    hostLink.textContent = network.mdns ? network.dashboardUrl : network.ipUrl;
    byId('home-ip').href = network.ipUrl;
    text('home-ip', network.ipUrl);
    text('network-save', network.saveFailed ? 'Connected, but WiFi settings could not be saved. Configure WiFi again after a restart.' : network.saved ? 'WiFi settings saved. GrowBox will reconnect automatically after a restart.' : 'WiFi settings have not been saved.');
    // Show the handoff when viewing setup/proxy; it is unnecessary on the home address.
    byId('home-network').hidden = location.hostname === network.hostname || location.hostname === network.ip;
  } else {
    byId('home-network').hidden = true;
    text('network', network.connecting ? 'Connecting to home WiFi... setup access remains available.' : network.failed ? 'Home WiFi connection failed. Check WiFi details in setup. The setup access point remains available.' : network.saved ? 'Home WiFi disconnected. GrowBox will retry automatically; setup access remains available.' : 'Setup WiFi only: join your home 2.4 GHz WiFi in setup to use GrowBox and internet together.');
  }
}
function connect() {
  stream = new EventSource('/events');
  stream.addEventListener('state', event => {
    try { render(JSON.parse(event.data)); } catch (_) { connection(false); }
  });
  stream.addEventListener('open', () => { lastHeard = Date.now(); config(); });
  stream.addEventListener('ping', () => { lastHeard = Date.now(); if (current) connection(true); });
  stream.addEventListener('network', event => {
    lastHeard = Date.now();
    const network = JSON.parse(event.data);
    renderNetwork(network);
  });
  stream.addEventListener('error', () => connection(false)); // EventSource reconnects automatically
}
connection(false);
config();
connect();
setInterval(() => {
  if (live && Date.now() - lastHeard > 35000) { connection(false); stream.close(); connect(); }
}, 5000);
window.addEventListener('offline', () => connection(false));
window.addEventListener('online', () => { stream.close(); connect(); });

let installPrompt;
window.addEventListener('beforeinstallprompt', event => {
  event.preventDefault(); installPrompt = event; byId('install').hidden = false;
});
byId('install').addEventListener('click', async () => {
  if (!installPrompt) return;
  await installPrompt.prompt();
  installPrompt = null; byId('install').hidden = true;
});
window.addEventListener('appinstalled', () => { byId('install').hidden = true; });
if (window.isSecureContext && 'serviceWorker' in navigator) {
  navigator.serviceWorker.register('/sw.js').then(() => {
    text('pwa-note', 'App supports offline opening. Live readings and controls require a connection to GrowBox.');
  }).catch(() => text('pwa-note', 'Offline setup failed. Live monitoring remains available.'));
} else {
  text('pwa-note', 'Live updates work here. Install and offline mode require HTTPS or localhost.');
}
