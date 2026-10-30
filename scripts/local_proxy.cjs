// node scripts/local_proxy.cjs http://192.168.4.1 [8180]
'use strict';
const http = require('node:http');
const https = require('node:https');
function createProxy(targetURL) {
  const target = new URL(targetURL);
  if (!['http:', 'https:'].includes(target.protocol)) throw new Error('Target must use HTTP or HTTPS');
  return http.createServer((req, res) => {
    const transport = target.protocol === 'https:' ? https : http;
    const upstream = transport.request(new URL(req.url, target), {
      method: req.method, headers: {...req.headers, host: target.host},
    }, response => {
      const headers = {...response.headers};
      if (headers.location && /^https?:\/\//.test(headers.location)) {
        const location = new URL(headers.location);
        if (location.host === target.host || location.hostname === '192.168.4.1')
          headers.location = location.pathname + location.search;
      }
      res.writeHead(response.statusCode, headers);
      response.pipe(res); // SSE streams without buffering
    });
    upstream.on('error', () => {
      if (!res.headersSent) res.writeHead(502, {'Content-Type':'text/plain','Cache-Control':'no-store'});
      res.end('GrowBox is unreachable. Check the device address and WiFi connection.');
    });
    res.on('close', () => upstream.destroy());
    req.pipe(upstream);
  });
}
module.exports = {createProxy};
if (require.main === module) {
  if (!process.argv[2]) { console.error('Usage: node scripts/local_proxy.cjs http://DEVICE_IP [8180]'); process.exit(1); }
  const port = Number(process.argv[3] || 8180);
  if (!Number.isInteger(port) || port < 1 || port > 65535) throw new Error('Invalid port');
  const proxy = createProxy(process.argv[2]);
  proxy.listen(port, '127.0.0.1', () => console.log('GrowBox PWA: http://localhost:' + port));
}
