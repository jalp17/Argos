#ifndef WEBPAGE_H
#define WEBPAGE_H

/* Página web embebida en español para el panel de control de Argos */
static const char ARGOS_WEBPAGE_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Argos - Panel de Control</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;background:#0d1117;color:#c9d1d9;min-height:100vh}
header{background:#161b22;padding:15px 20px;border-bottom:2px solid #30363d;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap}
header h1{color:#58a6ff;font-size:1.5em;display:flex;align-items:center;gap:10px}
header h1 span{color:#8b949e;font-size:0.6em}
#status{font-size:0.85em;padding:6px 14px;border-radius:12px;font-weight:600}
.status-online{background:#1a3a2a;color:#3fb950;border:1px solid #3fb950}
.status-offline{background:#3a1a1a;color:#f85149;border:1px solid #f85149}
.container{max-width:1400px;margin:0 auto;padding:20px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));gap:20px;margin-bottom:20px}
.card{background:#161b22;border:1px solid #30363d;border-radius:12px;padding:20px}
.card h2{color:#58a6ff;font-size:1.1em;margin-bottom:15px;padding-bottom:8px;border-bottom:1px solid #30363d}
.card h3{color:#8b949e;font-size:0.9em;margin:10px 0 5px}
.valor-medicion{font-size:2em;font-weight:700;color:#f0f6fc;font-variant-numeric:tabular-nums}
.unidad{font-size:0.5em;color:#8b949e;margin-left:4px}
.canal-label{font-size:0.8em;color:#8b949e;margin-bottom:2px}
.medicion-item{margin-bottom:12px;padding:8px;background:#0d1117;border-radius:8px}
.barra-progreso{height:4px;background:#21262d;border-radius:2px;margin-top:6px;overflow:hidden}
.barra-progreso-inner{height:100%;border-radius:2px;transition:width 0.3s}
.barra-progreso-inner.verde{background:#3fb950}
.barra-progreso-inner.amarillo{background:#d29922}
.barra-progreso-inner.rojo{background:#f85149}
.btn{background:#21262d;color:#c9d1d9;border:1px solid #30363d;padding:8px 16px;border-radius:6px;cursor:pointer;font-size:0.85em;transition:all 0.2s}
.btn:hover{background:#30363d;border-color:#8b949e}
.btn-primario{background:#238636;color:#fff;border-color:#2ea043}
.btn-primario:hover{background:#2ea043}
.btn-peligro{background:#da3633;color:#fff;border-color:#f85149}
.btn-peligro:hover{background:#f85149}
.btn-descarga{background:#1f6feb;color:#fff;border-color:#388bfd}
.btn-descarga:hover{background:#388bfd}
.controles{display:flex;gap:10px;flex-wrap:wrap;margin-top:10px}
.control-group{margin-bottom:12px}
.control-group label{display:block;margin-bottom:4px;font-size:0.85em;color:#8b949e}
.control-group input[type="range"]{width:100%;accent-color:#58a6ff}
.control-group input[type="number"]{background:#0d1117;border:1px solid #30363d;color:#c9d1d9;padding:6px 10px;border-radius:4px;width:80px}
.control-group select{background:#0d1117;border:1px solid #30363d;color:#c9d1d9;padding:6px 10px;border-radius:4px}
.tabla{width:100%;border-collapse:collapse;font-size:0.85em}
.tabla th{text-align:left;padding:8px 10px;background:#0d1117;color:#8b949e;border-bottom:2px solid #30363d}
.tabla td{padding:8px 10px;border-bottom:1px solid #21262d}
.tabla tr:hover{background:#0d1117}
.chart-container{width:100%;height:250px;position:relative;background:#0d1117;border-radius:8px;overflow:hidden}
canvas{width:100%!important;height:100%!important}
.info-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;font-size:0.85em}
.info-item{padding:6px 0;display:flex;justify-content:space-between;border-bottom:1px solid #21262d}
.info-label{color:#8b949e}
.info-value{color:#f0f6fc;font-weight:600}
.clientes-list{max-height:200px;overflow-y:auto}
.toast{position:fixed;bottom:20px;right:20px;padding:12px 24px;border-radius:8px;color:#fff;font-size:0.9em;z-index:1000;opacity:0;transition:opacity 0.3s;pointer-events:none}
.toast.show{opacity:1}
.toast-exito{background:#238636}
.toast-error{background:#da3633}
@media(max-width:600px){header h1{font-size:1.2em}.grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<header>
<h1>&#9670; Argos <span>Panel de Control</span></h1>
<div id="status" class="status-offline">Desconectado</div>
</header>
<div class="container">
<div class="grid">
<div class="card">
<h2>&#9881; Canales ADC</h2>
<div id="adc-readings">
<div class="medicion-item"><div class="canal-label">Canal 0 (GPIO 36)</div><div class="valor-medicion" id="adc0">---<span class="unidad">mV</span></div><div class="barra-progreso"><div class="barra-progreso-inner verde" id="bar0" style="width:0%"></div></div></div>
<div class="medicion-item"><div class="canal-label">Canal 1 (GPIO 39)</div><div class="valor-medicion" id="adc1">---<span class="unidad">mV</span></div><div class="barra-progreso"><div class="barra-progreso-inner verde" id="bar1" style="width:0%"></div></div></div>
<div class="medicion-item"><div class="canal-label">Canal 2 (GPIO 32)</div><div class="valor-medicion" id="adc2">---<span class="unidad">mV</span></div><div class="barra-progreso"><div class="barra-progreso-inner verde" id="bar2" style="width:0%"></div></div></div>
<div class="medicion-item"><div class="canal-label">Canal 3 (GPIO 33)</div><div class="valor-medicion" id="adc3">---<span class="unidad">mV</span></div><div class="barra-progreso"><div class="barra-progreso-inner verde" id="bar3" style="width:0%"></div></div></div>
</div>
</div>
<div class="card">
<h2>&#128200; Gráfico en Tiempo Real</h2>
<div class="chart-container">
<canvas id="chart"></canvas>
</div>
</div>
<div class="card">
<h2>&#128451; Control de Salidas</h2>
<div class="control-group">
<label>DAC 0 (GPIO 25) - Voltaje: <span id="dac0-val">0</span> mV</label>
<input type="range" id="dac0" min="0" max="3300" value="0" step="10">
</div>
<div class="control-group">
<label>DAC 1 (GPIO 26) - Voltaje: <span id="dac1-val">0</span> mV</label>
<input type="range" id="dac1" min="0" max="3300" value="0" step="10">
</div>
<div class="control-group">
<label>PWM Canal 0 (GPIO 18) - Ciclo de Trabajo: <span id="pwm0-val">0</span>%</label>
<input type="range" id="pwm0" min="0" max="100" value="0">
</div>
<div class="control-group">
<label>PWM Canal 1 (GPIO 19) - Ciclo de Trabajo: <span id="pwm1-val">0</span>%</label>
<input type="range" id="pwm1" min="0" max="100" value="0">
</div>
</div>
<div class="card">
<h2>&#128194; Archivos de Registro</h2>
<table class="tabla">
<thead><tr><th>Archivo</th><th>Tamaño</th><th>Acción</th></tr></thead>
<tbody id="log-files">
<tr><td colspan="3" style="color:#8b949e;text-align:center">Cargando...</td></tr>
</tbody>
</table>
</div>
<div class="card">
<h2>&#128268; Estado del Sistema</h2>
<div class="info-grid">
<div class="info-item"><span class="info-label">SSID</span><span class="info-value" id="sys-ssid">---</span></div>
<div class="info-item"><span class="info-label">IP</span><span class="info-value" id="sys-ip">---</span></div>
<div class="info-item"><span class="info-label">Clientes Conectados</span><span class="info-value" id="sys-clients">0</span></div>
<div class="info-item"><span class="info-label">Almacenamiento</span><span class="info-value" id="sys-storage">---</span></div>
<div class="info-item"><span class="info-label">Mediciones</span><span class="info-value" id="sys-measurements">0</span></div>
<div class="info-item"><span class="info-label">Uptime</span><span class="info-value" id="sys-uptime">0s</span></div>
</div>
</div>
</div>
</div>
<div class="toast" id="toast"></div>
<script>
var ws=null,chart=null,chartData=[],maxPoints=50,measurementCount=0,startTime=Date.now();
function conectarWS(){var p=window.location.protocol==='https:'?'wss:':'ws:';var u=p+'//'+window.location.hostname+':81/ws';ws=new WebSocket(u);
ws.onopen=function(){document.getElementById('status').textContent='Conectado';document.getElementById('status').className='status status-online';mostrarToast('Conectado al dispositivo Argos','exito')};
ws.onclose=function(){document.getElementById('status').textContent='Desconectado';document.getElementById('status').className='status status-offline';setTimeout(conectarWS,2000)};
ws.onmessage=function(e){procesarMensaje(JSON.parse(e.data))};}
function procesarMensaje(d){if(d.tipo==='medicion'){actualizarADC(d);agregarPuntoGrafico(d);measurementCount++;document.getElementById('sys-measurements').textContent=measurementCount}
else if(d.tipo==='estado'){document.getElementById('sys-ssid').textContent=d.ssid;document.getElementById('sys-ip').textContent=d.ip;document.getElementById('sys-clients').textContent=d.clientes;document.getElementById('sys-storage').textContent=d.almacenamiento;document.getElementById('sys-uptime').textContent=d.uptime}
else if(d.tipo==='archivos'){actualizarArchivos(d.archivos)}}
function actualizarADC(d){for(var i=0;i<4;i++){var v=d.canales[i]||0;var el=document.getElementById('adc'+i);if(el){el.innerHTML=v+'<span class="unidad">mV</span>'}
var bar=document.getElementById('bar'+i);if(bar){var pct=Math.min(100,(v/3300)*100);bar.style.width=pct+'%';bar.className='barra-progreso-inner '+(pct>85?'rojo':pct>60?'amarillo':'verde')}}}
function agregarPuntoGrafico(d){var t=new Date();var label=t.getHours().toString().padStart(2,'0')+':'+t.getMinutes().toString().padStart(2,'0')+':'+t.getSeconds().toString().padStart(2,'0');chartData.push({tiempo:label,valores:d.canales||[]});if(chartData.length>maxPoints)chartData.splice(0,chartData.length-maxPoints);actualizarGrafico()}
function actualizarGrafico(){if(!chart||!chartData.length)return;var labels=chartData.map(function(p){return p.tiempo});var datasets=[];var colores=['#3fb950','#58a6ff','#d29922','#f85149'];for(var i=0;i<4;i++){var data=chartData.map(function(p){return p.valores[i]||0});datasets.push({label:'CH'+i,data:data,borderColor:colores[i],backgroundColor:colores[i]+'33',borderWidth:2,fill:true,tension:0.3,pointRadius:1})}
chart.data={labels:labels,datasets:datasets};chart.update('none')}
function actualizarArchivos(archivos){var tbody=document.getElementById('log-files');if(!archivos||!archivos.length){tbody.innerHTML='<tr><td colspan="3" style="color:#8b949e;text-align:center">Sin archivos</td></tr>';return}
var h='';archivos.forEach(function(f){var nom=f.nombre,bytes=f.tamano;var tam=bytes>1024?(bytes/1024).toFixed(1)+' KB':bytes+' B';h+='<tr><td>'+nom+'</td><td>'+tam+'</td><td><button class="btn btn-descarga" onclick="descargar(\''+nom+'\')">Descargar</button></td></tr>'});tbody.innerHTML=h}
function descargar(nom){window.location.href='/api/descargar?archivo='+encodeURIComponent(nom)}
function mostrarToast(msg,tipo){var t=document.getElementById('toast');t.textContent=msg;t.className='toast toast-'+(tipo||'exito')+' show';setTimeout(function(){t.className='toast'},3000)}
function enviarControl(d){if(ws&&ws.readyState===WebSocket.OPEN){ws.send(JSON.stringify(d))}}
document.getElementById('dac0').addEventListener('input',function(){var v=parseInt(this.value);document.getElementById('dac0-val').textContent=v;enviarControl({tipo:'dac',canal:0,valor:v})});
document.getElementById('dac1').addEventListener('input',function(){var v=parseInt(this.value);document.getElementById('dac1-val').textContent=v;enviarControl({tipo:'dac',canal:1,valor:v})});
document.getElementById('pwm0').addEventListener('input',function(){var v=parseInt(this.value);document.getElementById('pwm0-val').textContent=v;enviarControl({tipo:'pwm',canal:0,valor:v})});
document.getElementById('pwm1').addEventListener('input',function(){var v=parseInt(this.value);document.getElementById('pwm1-val').textContent=v;enviarControl({tipo:'pwm',canal:1,valor:v})});
function iniciarGrafico(){var ctx=document.getElementById('chart').getContext('2d');chart=new Chart(ctx,{type:'line',data:{labels:[],datasets:[]},options:{responsive:true,maintainAspectRatio:false,animation:false,plugins:{legend:{labels:{color:'#8b949e',boxWidth:12}}},scales:{x:{ticks:{color:'#8b949e',maxTicksLimit:8,font:{size:10}},grid:{color:'#21262d'}},y:{beginAtZero:true,max:3300,ticks:{color:'#8b949e',font:{size:10}},grid:{color:'#21262d',drawBorder:false}}}}})}
var script=document.createElement('script');script.src='https://cdn.jsdelivr.net/npm/chart.js@4';script.onload=function(){iniciarGrafico();conectarWS()};document.head.appendChild(script);
</script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
