// Hand colours: Pebble palette byte (0xC0 | rrggbb) and the hex shown on the page.
var COLORS = [
  { key: 'white',  name: 'White',         argb: 0xFF, hex: '#ffffff' },
  { key: 'gray',   name: 'Light grey',    argb: 0xEA, hex: '#aaaaaa' },
  { key: 'peach',  name: 'Chrome yellow', argb: 0xF8, hex: '#ffaa00' },
  { key: 'orange', name: 'Orange',        argb: 0xF4, hex: '#ff5500' },
  { key: 'red',    name: 'Red',           argb: 0xF0, hex: '#ff0000' },
  { key: 'pink',   name: 'Magenta',       argb: 0xF3, hex: '#ff00ff' },
  { key: 'cyan',   name: 'Cyan',          argb: 0xCF, hex: '#00ffff' },
  { key: 'green',  name: 'Green',         argb: 0xCC, hex: '#00ff00' }
];

function colorByKey(key) {
  for (var i = 0; i < COLORS.length; i++) if (COLORS[i].key === key) return COLORS[i];
  return COLORS[0];
}

function currentSettings() {
  return { handColor: localStorage.getItem('handColor') || 'white' };
}

function sendSettings(cfg) {
  Pebble.sendAppMessage(
    { 'HAND_COLOR': colorByKey(cfg.handColor).argb },
    function() { console.log('Settings sent: ' + JSON.stringify(cfg)); },
    function(err) { console.log('Failed to send settings: ' + JSON.stringify(err)); }
  );
}

function configHtml() {
  var rows = '';
  for (var i = 0; i < COLORS.length; i++) {
    var c = COLORS[i];
    rows += '<div class="row"><input type="radio" name="hc" id="' + c.key + '" value="' + c.key + '">' +
            '<label for="' + c.key + '">' + c.name + '</label>' +
            '<span class="swatch" style="background:' + c.hex + '"></span></div>';
  }
  return '<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width">' +
    '<style>body{font-family:-apple-system,sans-serif;margin:20px;background:#f5f5f5;color:#111}' +
    '.card{background:#fff;border-radius:8px;padding:16px;margin-bottom:12px;box-shadow:0 1px 3px rgba(0,0,0,.12)}' +
    '.row{display:flex;align-items:center;padding:8px 0}' +
    'label{font-size:16px;margin-left:10px}input[type=radio]{width:22px;height:22px}' +
    '.swatch{display:inline-block;width:22px;height:22px;border-radius:50%;border:1px solid #999;margin-left:auto}' +
    'button{width:100%;padding:14px;background:#007aff;color:#fff;border:none;border-radius:8px;font-size:16px;margin-top:16px}' +
    '</style></head><body><h2>Varv</h2>' +
    '<div class="card"><div style="font-size:14px;color:#666;margin-bottom:4px">Hand colour</div>' + rows + '</div>' +
    '<button onclick="submit()">Save</button>' +
    '<script>' +
    'var o=JSON.parse(decodeURIComponent(location.hash.substring(1))||"{}");' +
    '(document.getElementById(o.handColor)||document.getElementById("white")).checked=true;' +
    'function submit(){var r={handColor:document.querySelector("input[name=hc]:checked").value};' +
    'document.location="pebblejs://close#"+encodeURIComponent(JSON.stringify(r))}' +
    '</script></body></html>';
}

Pebble.addEventListener('ready', function() {
  sendSettings(currentSettings());
});

Pebble.addEventListener('showConfiguration', function() {
  var hash = encodeURIComponent(JSON.stringify(currentSettings()));
  Pebble.openURL('data:text/html,' + encodeURIComponent(configHtml()) + '#' + hash);
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) return;
  var cfg = JSON.parse(decodeURIComponent(e.response));
  localStorage.setItem('handColor', colorByKey(cfg.handColor).key);
  sendSettings(cfg);
});
