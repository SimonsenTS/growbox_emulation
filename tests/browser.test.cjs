// Run with: node --test tests/browser.test.cjs
// Uses the real web assets against a mock firmware transport, not physical GPIO.
const { test } = require('node:test');
const assert = require('node:assert/strict');
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
let playwright;
try { playwright = require('playwright'); }
catch { playwright = require('../.pio/browser-tests/node_modules/playwright'); }

test('GrowBox live dashboard and PWA', {timeout: 90000}, async t => {
  let state = {revision:1,fresh:true,temperature:25.5,humidity:50,soil:30,water:70,lux:null,pump:false,growLed:false,boost:false,rgb:true,brightness:0,soilColor:0xffa500,waterColor:0x00ff00};
  const clients = new Set();
  const posts = [];
  const sessions = new Map();
  const loginSource = fs.readFileSync(path.join(__dirname,'../src/AuthManager.cpp'),'utf8');
  const loginHTML = loginSource.match(/R"delimiter\(([\s\S]*?)\)delimiter"/)[1].replace('%NETWORK_LIST%','').replace('%DASHBOARD_URL%','http://growbox-dashboard.local');
  const authenticated = req => sessions.has((req.headers.cookie || '').match(/(?:^|;\s*)GrowBoxSession=([^;]+)/)?.[1]);
  let controlStatus = 202;
  const emit = () => { for (const res of clients) res.write(`event: state\nid: ${state.revision}\ndata: ${JSON.stringify(state)}\n\n`); };
  const update = changes => { state = {...state,...changes,revision:state.revision+1}; emit(); };
  let network = {connected:false,connecting:false,saved:false,saveFailed:false,failed:false,mdns:false,ssid:'',ip:'192.168.4.1',apIp:'192.168.4.1',hostname:'growbox-dashboard.local',dashboardUrl:'http://growbox-dashboard.local/dashboard',ipUrl:'http://192.168.4.1/dashboard'};
  const updateNetwork = changes => {
    network = {...network,...changes};
    for (const client of clients) client.write('event: network\ndata: ' + JSON.stringify(network) + '\n\n');
  };
  const assets = new Map([['/dashboard','app-shell.html'],['/app-shell.html','app-shell.html'],['/app.js','app.js'],['/app.css','app.css'],['/sw.js','sw.js'],['/manifest.webmanifest','manifest.webmanifest'],['/icon-192.png','icon-192.png'],['/icon-512.png','icon-512.png']]);
  const server = http.createServer(async (req,res) => {
    const url = new URL(req.url, 'http://localhost');
    if (url.pathname === '/login' || url.pathname === '/') {
      if (url.pathname === '/' && authenticated(req)) { res.writeHead(303,{Location:'/dashboard'}); res.end(); return; }
      res.writeHead(200,{'Content-Type':'text/html; charset=utf-8','Cache-Control':'no-store'});
      res.end(loginHTML); return;
    }
    if (url.pathname === '/api/networks') { res.setHeader('Content-Type','application/json'); res.end('{"scanning":false,"options":""}'); return; }
    if (url.pathname === '/dashboard' && !authenticated(req)) { res.writeHead(303,{Location:'/login'}); res.end(); return; }
    if (url.pathname === '/events' && !authenticated(req)) { res.writeHead(403); res.end(); return; }
    if (url.pathname.startsWith('/api/') && !authenticated(req)) { res.writeHead(401); res.end('{"error":"Login required"}'); return; }
    if (url.pathname === '/events') {
      res.writeHead(200, {'Content-Type':'text/event-stream','Cache-Control':'no-store'});
      clients.add(res); res.on('close', () => clients.delete(res));
      res.write(`retry: 150\nevent: state\ndata: ${JSON.stringify(state)}\n\n`);
      res.write('event: network\ndata: ' + JSON.stringify(network) + '\n\n');
      return;
    }
    if (url.pathname === '/api/config') { res.setHeader('Content-Type','application/json'); res.end('{"simulation":true}'); return; }
    if (url.pathname === '/api/network') { res.setHeader('Content-Type','application/json'); res.end(JSON.stringify(network)); return; }
    if (req.method === 'POST') {
      let body = ''; for await (const chunk of req) body += chunk;
      const values = Object.fromEntries(new URLSearchParams(body));
      if (url.pathname === '/connect') {
        if (values.username !== 'admin' || values.password !== 'password123') { res.writeHead(401); res.end('Invalid credentials'); return; }
        const token = require('node:crypto').randomBytes(32).toString('hex');
        sessions.set(token,{remember:values.remember==='1'});
        res.writeHead(303,{Location:'/dashboard','Set-Cookie':'GrowBoxSession='+token+'; Path=/; HttpOnly; SameSite=Lax'+(values.remember==='1'?'; Max-Age=2592000':'')});
        res.end(); return;
      }
      if (url.pathname === '/logout') {
        res.writeHead(303,{Location:'/login','Set-Cookie':'GrowBoxSession=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0'}); res.end(); return;
      }
      if (!authenticated(req)) { res.writeHead(401); res.end('{"error":"Login required"}'); return; }
      posts.push({path:url.pathname,values});
      res.writeHead(controlStatus, {'Content-Type':'application/json','Cache-Control':'no-store'});
      res.end(controlStatus === 202 ? '{"queued":true}' : '{"error":"Control queue full; retry"}');
      if (controlStatus !== 202) return;
      if (url.pathname === '/simulation') update(Object.fromEntries(Object.entries(values).map(([k,v]) => [k,Number(v)])));
      else if (values.device === 'brightness') update({brightness:Number(values.value),growLed:Number(values.value)>0});
      else update({[values.device]:!state[values.device]});
      return;
    }
    if (assets.has(url.pathname)) {
      const file = assets.get(url.pathname);
      const ext = path.extname(file);
      const type = {'.html':'text/html; charset=utf-8','.js':'application/javascript','.css':'text/css','.png':'image/png','.webmanifest':'application/manifest+json'}[ext];
      res.writeHead(200, {'Content-Type':type,'Cache-Control':'no-cache'});
      res.end(fs.readFileSync(path.join(__dirname,'../web',file))); return;
    }
    res.writeHead(404); res.end();
  });
  await new Promise(resolve => server.listen(0,'127.0.0.1',resolve));
  const {createProxy} = require('../scripts/local_proxy.cjs');
  const proxy = createProxy('http://127.0.0.1:' + server.address().port);
  await new Promise(resolve => proxy.listen(0,'127.0.0.1',resolve));
  const base = 'http://localhost:' + proxy.address().port;
  const browser = await playwright.chromium.launch({channel:'msedge',headless:true});
  const context = await browser.newContext();
  const page = await context.newPage();
  page.setDefaultTimeout(8000);
  page.setDefaultNavigationTimeout(10000);
  const errors = [];
  let navigations = 0;
  page.on('pageerror', error => errors.push(error.message));
  page.on('framenavigated', frame => { if (frame === page.mainFrame()) navigations++; });
  const waitText = (id,value) => page.waitForFunction(([id,value]) => document.getElementById(id).textContent === value, [id,value]);
  try {
    await page.goto(base+'/dashboard');
    await t.test('login supports password managers and remembered sign-in', async () => {
      assert.equal(await page.locator('#username').getAttribute('autocomplete'),'username');
      assert.equal(await page.locator('#password').getAttribute('autocomplete'),'current-password');
      assert.equal(await page.locator('[name="remember"]').isChecked(),true);
      await page.fill('#username','admin');
      await page.fill('#password','password123');
      await page.click('input[type="submit"]');
      await waitText('temperature','25.5 \u00b0C');
      const cookie=(await context.cookies()).find(c=>c.name==='GrowBoxSession');
      assert.equal(cookie.httpOnly,true); assert.ok(cookie.expires > Date.now()/1000+29*86400);
      assert.equal(await page.evaluate(()=>document.cookie.includes('GrowBoxSession')),false);
      // A second browser session retains a remembered cookie without submitting credentials.
      const returning=await browser.newContext({storageState:await context.storageState()});
      const returningPage=await returning.newPage();
      await returningPage.goto(base+'/dashboard');
      await returningPage.waitForSelector('#temperature');
      assert.equal(new URL(returningPage.url()).pathname,'/dashboard');
      await returning.close();
    });
    await waitText('temperature','25.5 \u00b0C');
    await page.waitForFunction(() => !document.getElementById('simulation').hidden);
    const initialNavigations = navigations;
    await t.test('home WiFi handoff provides a stable dashboard name and IP fallback', async () => {
      assert.equal(await page.locator('#home-network').isHidden(),true);
      updateNetwork({connecting:true});
      await page.waitForFunction(()=>document.getElementById('network').textContent.includes('Connecting to home WiFi'));
      updateNetwork({connected:true,connecting:false,saved:true,mdns:true,ssid:'Home 2.4 GHz',ip:'192.168.1.50',ipUrl:'http://192.168.1.50/dashboard'});
      await page.waitForSelector('#home-network',{state:'visible'});
      assert.equal(await page.locator('#home-dashboard').getAttribute('href'),'http://growbox-dashboard.local/dashboard');
      assert.equal(await page.locator('#home-ip').getAttribute('href'),'http://192.168.1.50/dashboard');
      assert.match(await page.locator('#home-network').textContent(),/Return this phone or computer to your normal home WiFi/);
      assert.match(await page.locator('#network-save').textContent(),/reconnect automatically after a restart/);
      assert.equal(navigations,initialNavigations);
    });
    await t.test('WiFi failure keeps setup guidance and missing mDNS uses the home IP', async () => {
      updateNetwork({connected:false,failed:true,mdns:false});
      await page.waitForFunction(()=>document.getElementById('network').textContent.includes('connection failed'));
      assert.equal(await page.locator('#home-network').isHidden(),true);
      updateNetwork({connected:true,failed:false,mdns:false,saveFailed:true});
      await page.waitForSelector('#home-network',{state:'visible'});
      assert.equal(await page.locator('#home-dashboard').getAttribute('href'),'http://192.168.1.50/dashboard');
      assert.match(await page.locator('#network-save').textContent(),/could not be saved/);
      updateNetwork({mdns:true,saveFailed:false});
    });
    await t.test('updates changed DOM values only, without navigation', async () => {
      await page.evaluate(() => {
        window.mutations={temperature:0,humidity:0};
        for (const id of Object.keys(window.mutations)) new MutationObserver(records => { window.mutations[id]+=records.length; }).observe(document.getElementById(id), {childList:true,subtree:true,characterData:true});
      });
      update({temperature:26.1});
      await waitText('temperature','26.1 \u00b0C');
      const mutations=await page.evaluate(() => window.mutations);
      assert.ok(mutations.temperature > 0); assert.equal(mutations.humidity,0);
      assert.equal(navigations,initialNavigations);
    });
    await t.test('toggles and brightness use POST and apply live state', async () => {
      await page.click('[data-device="growLed"]'); await waitText('growLed','ON');
      await page.locator('#brightness').fill('72'); await page.locator('#brightness').dispatchEvent('change');
      await waitText('brightness-value','72%');
      await page.waitForFunction(() => !document.getElementById('brightness').disabled);
      assert.ok(posts.some(p=>p.path==='/api/control' && p.values.device==='brightness' && p.values.value==='72'));
      assert.equal(navigations,initialNavigations);
    });
    await t.test('simulation updates values without reloading', async () => {
      await page.fill('[name="temperature"]','29.2');
      await page.fill('[name="lux"]','615');
      await page.click('#simulation-submit');
      await waitText('temperature','29.2 \u00b0C'); await waitText('lux','615 lx');
      assert.equal(navigations,initialNavigations);
    });
    await t.test('missing and stale sensors lock pump; critical water color persists', async () => {
      update({water:null,pump:false,waterColor:0xff0000,rgb:false});
      await waitText('water','Unavailable');
      assert.equal(await page.locator('[data-device="pump"]').isDisabled(),true);
      assert.equal(await page.locator('#waterColor').evaluate(e=>e.style.backgroundColor),'rgb(255, 0, 0)');
      update({fresh:false,soil:null});
      await waitText('soil','Unavailable');
      assert.match(await page.locator('#connection').textContent(),/pump protected/);
      update({fresh:true,soil:30,water:70,rgb:true});
      await waitText('water','70%');
    });
    await t.test('queue failures are visible and do not fabricate device state', async () => {
      controlStatus=503;
      await page.click('[data-device="growLed"]');
      await page.waitForFunction(()=>document.getElementById('message').textContent.includes('Control queue full'));
      assert.equal(await page.locator('#growLed').textContent(),'ON');
      controlStatus=202;
    });
    await t.test('SSE reconnect receives the latest snapshot', async () => {
      for (const client of clients) client.end();
      state={...state,temperature:31.3,revision:state.revision+1};
      await waitText('temperature','31.3 \u00b0C');
      assert.equal(navigations,initialNavigations);
    });
    await t.test('mobile layout has no horizontal overflow', async () => {
      await page.setViewportSize({width:390,height:844});
      assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth <= innerWidth),true);
      await page.evaluate(() => document.getElementById('message').textContent = '');
      await page.screenshot({path:path.join(__dirname,'../.pio/dashboard-mobile.png'),fullPage:true});
    });
    await t.test('manifest, service worker, offline shell and API cache isolation', {timeout:20000}, async () => {
      await page.waitForFunction(async()=>!!(await navigator.serviceWorker.getRegistration())?.active);
      await page.waitForFunction(()=>!!navigator.serviceWorker.controller);
      const manifest=await (await context.request.get(base+'/manifest.webmanifest')).json();
      assert.equal(manifest.display,'standalone'); assert.equal(manifest.icons.length,2);
      await context.setOffline(true);
      await page.reload({waitUntil:'domcontentloaded'});
      await page.waitForFunction(()=>document.body.dataset.live==='false');
      assert.equal(await page.locator('[data-device="pump"]').isDisabled(),true);
      assert.equal(await page.locator('#temperature').textContent(),'\u2014');
      const keys=await page.evaluate(async()=>{const name=(await caches.keys()).find(key=>key.startsWith('growbox-shell-'));const cache=await caches.open(name);return (await cache.keys()).map(r=>new URL(r.url).pathname);});
      assert.ok(keys.includes('/app-shell.html')); assert.ok(keys.every(key=>!key.startsWith('/api/') && key!=='/events'));
      const apiWorked=await page.evaluate(async()=>{try{return (await fetch('/api/state')).ok;}catch{return false;}});
      assert.equal(apiWorked,false);
      await context.setOffline(false);
      await waitText('temperature','31.3 \u00b0C');
    });
    await t.test('sign out clears remembered login and APIs require authentication', async () => {
      await page.click('form[action="/logout"] button');
      await page.waitForSelector('#username');
      assert.equal((await context.cookies()).some(c=>c.name==='GrowBoxSession'),false);
      assert.equal((await context.request.get(base+'/api/config')).status(),401);
      const blank=await browser.newContext();
      await blank.addCookies([{name:'GrowBoxSession',value:'tampered-token',url:base}]);
      assert.equal((await blank.request.get(base+'/api/config')).status(),401);
      assert.equal((await blank.request.get(base+'/events')).status(),403);
      const blankPage=await blank.newPage();
      await blankPage.goto(base+'/dashboard');
      assert.equal(new URL(blankPage.url()).pathname,'/login');
      await blank.close();
      await page.fill('#username','admin');
      await page.fill('#password','password123');
      await page.uncheck('[name="remember"]');
      await page.click('input[type="submit"]');
      await waitText('temperature','31.3 \u00b0C');
      const cookie=(await context.cookies()).find(c=>c.name==='GrowBoxSession');
      assert.equal(cookie.expires,-1);
    });
    assert.deepEqual(errors,[]);
  } finally {
    await browser.close();
    for (const client of clients) client.end();
    proxy.closeAllConnections();
    await new Promise(resolve=>proxy.close(resolve));
    server.closeAllConnections();
    await new Promise(resolve=>server.close(resolve));
  }
});
