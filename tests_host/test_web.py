#!/usr/bin/env python3
"""
Simulador del backend Argos para probar la página web sin ESP32.

Sirve la página web embebida, un WebSocket que transmite datos falsos
en tiempo real, y los endpoints REST. Útil para validar la interfaz
web, el streaming de datos y la descarga de archivos.

Uso:
  python3 tests_host/test_web.py
  # Luego abrir http://localhost:8080

Dependencias: python3 -m pip install websockets
"""
import asyncio
import json
import time
import math
import struct
import hashlib
import base64
import os
import signal
import mimetypes
from datetime import datetime

try:
    import websockets
except ImportError:
    print("ERROR: Necesitas websockets. Instala con:")
    print("  pip install websockets")
    exit(1)

HOST = "0.0.0.0"
HTTP_PORT = 8080
WS_PORT = 8080  # El ESP32 real sirve WS en el mismo puerto que HTTP (80)

# ============================================================
# SIMULACIÓN DE DATOS
# ============================================================
class Simulador:
    def __init__(self):
        self.mediciones = 0
        self.t_inicio = time.time()
        self.tiempo = 0.0
        self.canales = {
            0: {"offset": 1000, "frecuencia": 0.5, "amplitud": 800, "ruido": 20},
            1: {"offset": 2000, "frecuencia": 0.3, "amplitud": 500, "ruido": 15},
            2: {"offset": 500,  "frecuencia": 0.8, "amplitud": 300, "ruido": 10},
            3: {"offset": 1500, "frecuencia": 0.1, "amplitud": 1000, "ruido": 25},
        }
        self.archivos = []
        for i in range(3):
            ts = datetime.now().strftime("%Y%m%d_%H%M%S")
            self.archivos.append({
                "nombre": f"argos_log_{ts}.csv",
                "tamano": 1024 + i * 512
            })

    def medicion(self):
        """Genera una medición simulada para los 4 canales"""
        self.tiempo += 0.1  # ~100ms entre mediciones
        t = self.tiempo
        canales = []
        for ch in range(4):
            cfg = self.canales[ch]
            seno = cfg["amplitud"] * math.sin(2 * math.pi * cfg["frecuencia"] * t)
            ruido = cfg["ruido"] * (2 * math.random() - 1) if hasattr(math, 'random') else 0
            valor = int(cfg["offset"] + seno + ruido)
            canales.append(max(0, min(3300, valor)))
        self.mediciones += 1
        return {"tipo": "medicion", "canales": canales}

    def estado(self):
        uptime = int(time.time() - self.t_inicio)
        uso_almacenamiento = 25 + int(15 * math.sin(self.tiempo * 0.01))
        return {
            "tipo": "estado",
            "ssid": "Argos-AP",
            "ip": "192.168.4.1",
            "clientes": 1,
            "almacenamiento": f"{uso_almacenamiento}% usado",
            "uptime": f"{uptime}s"
        }

    def lista_archivos(self):
        return {
            "tipo": "archivos",
            "archivos": self.archivos
        }

    def agregar_archivo(self):
        """Simula la creación de un archivo nuevo (rotación)"""
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.archivos.insert(0, {
            "nombre": f"argos_log_{ts}.csv",
            "tamano": 2048 + int(1024 * math.sin(self.tiempo * 0.05))
        })
        if len(self.archivos) > 10:
            self.archivos.pop()

    def csv_content(self, nombre):
        """Genera contenido CSV simulado para descarga"""
        lines = ["tiempo_ms,canal,valor_crudo,voltaje_mV\n"]
        for i in range(100):
            t = i * 100
            v = 1500 + 500 * math.sin(i * 0.1)
            lines.append(f"{t},0,{int(v * 1000 / 3.3)},{v:.2f}\n")
        return "".join(lines)


sim = Simulador()

# ============================================================
# SERVIDOR HTTP (REST + página web)
# ============================================================
# Extraer la página web del HTML embebido (la misma que usa el ESP32)
HTML_PAGE = """<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Argos - Panel de Control (Simulación)</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;background:#0d1117;color:#c9d1d9;min-height:100vh}
header{background:#161b22;padding:15px 20px;border-bottom:2px solid #30363d;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap}
header h1{color:#58a6ff;font-size:1.5em;display:flex;align-items:center;gap:10px}
header h1 span{color:#8b949e;font-size:0.6em}
header .nota-sim{color:#d29922;font-size:0.7em;font-style:italic}
#status{font-size:0.85em;padding:6px 14px;border-radius:12px;font-weight:600}
.status-online{background:#1a3a2a;color:#3fb950;border:1px solid #3fb950}
.status-offline{background:#3a1a1a;color:#f85149;border:1px solid #f85149}
.container{max-width:1400px;margin:0 auto;padding:20px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));gap:20px;margin-bottom:20px}
.card{background:#161b22;border:1px solid #30363d;border-radius:12px;padding:20px}
.card h2{color:#58a6ff;font-size:1.1em;margin-bottom:15px;padding-bottom:8px;border-bottom:1px solid #30363d}
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
.tabla{width:100%;border-collapse:collapse;font-size:0.85em}
.tabla th{text-align:left;padding:8px 10px;background:#0d1117;color:#8b949e;border-bottom:2px solid #30363d}
.tabla td{padding:8px 10px;border-bottom:1px solid #21262d}
.tabla tr:hover{background:#0d1117}
#chart-container{width:100%;height:250px;background:#0d1117;border-radius:8px;overflow:hidden}
canvas{width:100%!important;height:100%!important}
.info-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;font-size:0.85em}
.info-item{padding:6px 0;display:flex;justify-content:space-between;border-bottom:1px solid #21262d}
.info-label{color:#8b949e}
.info-value{color:#f0f6fc;font-weight:600}
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
<span class="nota-sim">&#9881; MODO SIMULACIÓN</span>
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
<div id="chart-container"><canvas id="chart"></canvas></div>
</div>
<div class="card">
<h2>&#128451; Control de Salidas (simulado)</h2>
<div class="control-group"><label>DAC 0 (GPIO 25): <span id="dac0-val">0</span> mV</label><input type="range" id="dac0" min="0" max="3300" value="0" step="10"></div>
<div class="control-group"><label>DAC 1 (GPIO 26): <span id="dac1-val">0</span> mV</label><input type="range" id="dac1" min="0" max="3300" value="0" step="10"></div>
<div class="control-group"><label>PWM Canal 0 (GPIO 18): <span id="pwm0-val">0</span>%</label><input type="range" id="pwm0" min="0" max="100" value="0"></div>
<div class="control-group"><label>PWM Canal 1 (GPIO 19): <span id="pwm1-val">0</span>%</label><input type="range" id="pwm1" min="0" max="100" value="0"></div>
<p style="color:#8b949e;font-size:0.75em;margin-top:10px">Los comandos DAC/PWM se muestran en la consola del servidor.</p>
</div>
<div class="card">
<h2>&#128194; Archivos de Registro</h2>
<table class="tabla">
<thead><tr><th>Archivo</th><th>Tamaño</th><th>Acción</th></tr></thead>
<tbody id="log-files"><tr><td colspan="3" style="color:#8b949e;text-align:center">Cargando...</td></tr></tbody>
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
function conectarWS(){
  var p=window.location.protocol==='https:'?'wss:':'ws:';
  var u=p+'//'+window.location.hostname+':'+window.location.port+'/ws';
  ws=new WebSocket(u);
  ws.onopen=function(){document.getElementById('status').textContent='Conectado';document.getElementById('status').className='status status-online';mostrarToast('Conectado al simulador Argos','exito')};
  ws.onclose=function(){document.getElementById('status').textContent='Desconectado';document.getElementById('status').className='status status-offline';setTimeout(conectarWS,2000)};
  ws.onmessage=function(e){procesarMensaje(JSON.parse(e.data))};}
function procesarMensaje(d){
  if(d.tipo==='medicion'){actualizarADC(d);agregarPuntoGrafico(d);measurementCount++;document.getElementById('sys-measurements').textContent=measurementCount}
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
function enviarControl(d){if(ws&&ws.readyState===WebSocket.OPEN){console.log('Comando enviado:',JSON.stringify(d));ws.send(JSON.stringify(d))}}
document.getElementById('dac0').addEventListener('input',function(){var v=parseInt(this.value);document.getElementById('dac0-val').textContent=v;enviarControl({tipo:'dac',canal:0,valor:v})});
document.getElementById('dac1').addEventListener('input',function(){var v=parseInt(this.value);document.getElementById('dac1-val').textContent=v;enviarControl({tipo:'dac',canal:1,valor:v})});
document.getElementById('pwm0').addEventListener('input',function(){var v=parseInt(this.value);document.getElementById('pwm0-val').textContent=v;enviarControl({tipo:'pwm',canal:0,valor:v})});
document.getElementById('pwm1').addEventListener('input',function(){var v=parseInt(this.value);document.getElementById('pwm1-val').textContent=v;enviarControl({tipo:'pwm',canal:1,valor:v})});
function iniciarGrafico(){var ctx=document.getElementById('chart').getContext('2d');chart=new Chart(ctx,{type:'line',data:{labels:[],datasets:[]},options:{responsive:true,maintainAspectRatio:false,animation:false,plugins:{legend:{labels:{color:'#8b949e',boxWidth:12}}},scales:{x:{ticks:{color:'#8b949e',maxTicksLimit:8,font:{size:10}},grid:{color:'#21262d'}},y:{beginAtZero:true,max:3300,ticks:{color:'#8b949e',font:{size:10}},grid:{color:'#21262d',drawBorder:false}}}}})}
var script=document.createElement('script');script.src='https://cdn.jsdelivr.net/npm/chart.js@4';script.onload=function(){iniciarGrafico();conectarWS()};document.head.appendChild(script);
</script>
</body>
</html>"""


# ============================================================
# MANEJADOR HTTP
# ============================================================
async def manejar_http(reader, writer):
    try:
        request = b""
        while b"\r\n\r\n" not in request:
            chunk = await reader.read(4096)
            if not chunk:
                break
            request += chunk
            if len(request) > 65536:
                break

        if not request:
            writer.close()
            return

        linea = request.split(b"\r\n")[0].decode("utf-8", errors="replace")
        parts = linea.split(" ")
        if len(parts) < 2:
            writer.close()
            return

        metodo = parts[0]
        ruta = parts[1].split("?")[0]
        query = parts[1].split("?")[1] if "?" in parts[1] else ""

        # Parse query params
        params = {}
        if query:
            for p in query.split("&"):
                if "=" in p:
                    k, v = p.split("=", 1)
                    params[k] = v

        if metodo == "GET":
            if ruta == "/":
                await responder_html(writer, HTML_PAGE)
            elif ruta == "/api/estado":
                await responder_json(writer, sim.estado())
            elif ruta == "/api/archivos":
                await responder_json(writer, sim.lista_archivos())
            elif ruta == "/api/descargar":
                archivo = params.get("archivo", "")
                if not archivo:
                    await responder_error(writer, 400, "Falta parámetro 'archivo'")
                else:
                    contenido = sim.csv_content(archivo)
                    await responder_csv(writer, archivo, contenido)
            else:
                await responder_error(writer, 404, "No encontrado")
        else:
            await responder_error(writer, 405, "Método no permitido")

    except Exception as e:
        print(f"  ERROR HTTP: {e}")
    finally:
        try:
            writer.close()
        except:
            pass


async def responder_html(writer, html):
    data = html.encode("utf-8")
    resp = (
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        f"Content-Length: {len(data)}\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n"
    ).encode() + data
    writer.write(resp)
    await writer.drain()


async def responder_json(writer, obj):
    data = json.dumps(obj, ensure_ascii=False).encode("utf-8")
    resp = (
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        f"Content-Length: {len(data)}\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n"
    ).encode() + data
    writer.write(resp)
    await writer.drain()


async def responder_csv(writer, nombre, contenido):
    data = contenido.encode("utf-8")
    resp = (
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/csv; charset=utf-8\r\n"
        f"Content-Disposition: attachment; filename=\"{nombre}\"\r\n"
        f"Content-Length: {len(data)}\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n"
    ).encode() + data
    writer.write(resp)
    await writer.drain()


async def responder_error(writer, code, msg):
    data = json.dumps({"error": msg}).encode("utf-8")
    status = {400: "Bad Request", 404: "Not Found", 405: "Method Not Allowed"}
    resp = (
        f"HTTP/1.1 {code} {status.get(code, 'Error')}\r\n"
        "Content-Type: application/json\r\n"
        f"Content-Length: {len(data)}\r\n"
        "Connection: close\r\n\r\n"
    ).encode() + data
    writer.write(resp)
    await writer.drain()


# ============================================================
# MANEJADOR WEBSOCKET
# ============================================================
class WebSocketCliente:
    """Maneja una conexión WebSocket individual"""
    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer
        self.abierto = True

    async def enviar(self, mensaje):
        if not self.abierto:
            return
        try:
            data = json.dumps(mensaje).encode("utf-8")
            frame = self._enmarcar(data)
            self.writer.write(frame)
            await self.writer.drain()
        except:
            self.abierto = False

    def _enmarcar(self, payload):
        """Crea un frame WebSocket (sin enmascarar, del servidor al cliente)"""
        frame = bytearray()
        frame.append(0x81)  # FIN + texto
        length = len(payload)
        if length < 126:
            frame.append(length)
        elif length < 65536:
            frame.append(126)
            frame.extend(struct.pack(">H", length))
        else:
            frame.append(127)
            frame.extend(struct.pack(">Q", length))
        frame.extend(payload)
        return bytes(frame)

    async def escuchar(self):
        """Escucha mensajes entrantes del cliente"""
        while self.abierto:
            try:
                b = await self.reader.read(4096)
                if not b:
                    break
                # Decodificar frame WebSocket enmascarado (del cliente)
                if len(b) < 2:
                    continue
                opcode = b[0] & 0x0F
                if opcode == 0x8:  # Close
                    break
                if opcode == 0x9:  # Ping
                    continue
                if opcode != 0x1:  # No es texto
                    continue

                mask = b[1] & 0x80
                length = b[1] & 0x7F
                offset = 2
                if length == 126:
                    length = struct.unpack(">H", b[2:4])[0]
                    offset = 4
                elif length == 127:
                    length = struct.unpack(">Q", b[2:10])[0]
                    offset = 10

                if mask:
                    masking_key = b[offset:offset + 4]
                    offset += 4
                    payload = bytearray(b[offset:offset + length])
                    for i in range(len(payload)):
                        payload[i] ^= masking_key[i % 4]
                else:
                    payload = b[offset:offset + length]

                msg = payload.decode("utf-8", errors="replace")
                try:
                    comando = json.loads(msg)
                    tipo = comando.get("tipo", "")
                    if tipo == "dac":
                        print(f"  DAC comando: canal={comando.get('canal')}, "
                              f"valor={comando.get('valor')} mV")
                    elif tipo == "pwm":
                        print(f"  PWM comando: canal={comando.get('canal')}, "
                              f"duty={comando.get('valor')}%")
                    else:
                        print(f"  Comando WS recibido: {msg}")
                except json.JSONDecodeError:
                    print(f"  Mensaje WS no-JSON: {msg}")

            except Exception as e:
                print(f"  Error WS escucha: {e}")
                break

        self.abierto = False
        print("  Cliente WS desconectado")


async_tasks = []


async def manejar_ws(reader, writer):
    """Maneja el handshake WebSocket inicial"""
    try:
        request = b""
        while b"\r\n\r\n" not in request:
            chunk = await reader.read(4096)
            if not chunk:
                break
            request += chunk
            if len(request) > 16384:
                break

        if not request:
            writer.close()
            return

        texto = request.decode("utf-8", errors="replace")

        # Extraer WebSocket-Key
        key = None
        for linea in texto.split("\r\n"):
            if linea.lower().startswith("sec-websocket-key:"):
                key = linea.split(":", 1)[1].strip()
                break

        if not key:
            writer.close()
            return

        # Calcular accept
        GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
        accept = base64.b64encode(
            hashlib.sha1((key + GUID).encode()).digest()
        ).decode()

        resp = (
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            f"Sec-WebSocket-Accept: {accept}\r\n\r\n"
        )
        writer.write(resp.encode())
        await writer.drain()

        print("  Cliente WS conectado")
        cliente = WebSocketCliente(reader, writer)
        await cliente.escuchar()

    except Exception as e:
        print(f"  Error handshake WS: {e}")
    finally:
        try:
            writer.close()
        except:
            pass


# ============================================================
# SERVIDOR PRINCIPAL
# ============================================================
async def main():
    # Determinar si el WS va en el mismo puerto o separado
    mismo_puerto = WS_PORT == HTTP_PORT

    if mismo_puerto:
        # WS en el mismo puerto que HTTP -
        # Necesitamos detectar si es WS o HTTP en la conexión
        async def manejar_conexion(reader, writer):
            """Detecta si la conexión es WS o HTTP y la enruta"""
            try:
                datos = await asyncio.wait_for(reader.read(4096), timeout=0.5)
            except asyncio.TimeoutError:
                datos = b""

            if not datos:
                try:
                    writer.close()
                except:
                    pass
                return

            if b"Upgrade: websocket" in datos or datos[:1] == b"\x81":
                print("  [auto-detect] WebSocket detectado")
                try:
                    resto = await asyncio.wait_for(reader.read(4096), timeout=2.0)
                except:
                    resto = b""
                texto = (datos + resto).decode("utf-8", errors="replace")

                key = None
                for linea in texto.split("\r\n"):
                    if linea.lower().startswith("sec-websocket-key:"):
                        key = linea.split(":", 1)[1].strip()
                        break

                if key:
                    GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
                    accept = base64.b64encode(
                        hashlib.sha1((key + GUID).encode()).digest()
                    ).decode()
                    resp = (
                        "HTTP/1.1 101 Switching Protocols\r\n"
                        "Upgrade: websocket\r\n"
                        "Connection: Upgrade\r\n"
                        f"Sec-WebSocket-Accept: {accept}\r\n\r\n"
                    )
                    writer.write(resp.encode())
                    await writer.drain()
                    print("  Cliente WS conectado (mismo puerto)")
                    cliente = WebSocketCliente(reader, writer)
                    await cliente.escuchar()
                else:
                    writer.close()
            else:
                if b"\r\n\r\n" in datos:
                    partes = datos.split(b"\r\n\r\n", 1)
                    request_line = partes[0]
                    body = partes[1] if len(partes) > 1 else b""
                    await manejar_http_completo(reader, writer, request_line + b"\r\n\r\n" + body)

            try:
                writer.close()
            except:
                pass

        server = await asyncio.start_server(
            manejar_conexion, HOST, HTTP_PORT
        )
    else:
        http_server = await asyncio.start_server(
            manejar_http, HOST, HTTP_PORT
        )
        server = http_server
        ws_server = await asyncio.start_server(
            manejar_ws, HOST, WS_PORT
        )

    addr = server.sockets[0].getsockname()
    print(f"  HTTP  → http://{addr[0]}:{addr[1]}")
    print(f"  WS    → ws://{addr[0]}:{addr[1]}/ws")
    print(f"  REST  → http://{addr[0]}:{addr[1]}/api/...")
    print(f"\n  Abre http://localhost:{addr[1]} en tu navegador")
    print(f"  Presiona Ctrl+C para detener")

    # Tarea de broadcasting de datos simulados
    clientes = []

    async def broadcast():
        while True:
            await asyncio.sleep(0.1)  # ~100ms = 10 Hz
            m = sim.medicion()
            for c in clientes:
                await c.enviar(m)

    async def broadcast_estado():
        while True:
            await asyncio.sleep(2.0)
            e = sim.estado()
            for c in clientes:
                await c.enviar(e)

    async def broadcast_archivos():
        while True:
            await asyncio.sleep(5.0)
            sim.agregar_archivo()
            a = sim.lista_archivos()
            for c in clientes:
                await c.enviar(a)

    # Modificar WebSocketCliente para auto-registrarse
    original_escuchar = WebSocketCliente.escuchar

    async def escuchar_con_registro(self):
        clientes.append(self)
        print(f"  Clientes activos: {len(clientes)}")
        try:
            await original_escuchar(self)
        finally:
            if self in clientes:
                clientes.remove(self)
            print(f"  Clientes activos: {len(clientes)}")

    WebSocketCliente.escuchar = escuchar_con_registro

    if mismo_puerto:
        # Reemplazar manejar_conexion para que registre clientes
        pass  # Ya se registran en escuchar

    asyncio.create_task(broadcast())
    asyncio.create_task(broadcast_estado())
    asyncio.create_task(broadcast_archivos())

    # Mantener vivo
    try:
        await asyncio.Event().wait()
    except asyncio.CancelledError:
        pass


async def manejar_http_completo(reader, writer, datos):
    """Maneja una petición HTTP cuando ya tenemos los datos completos"""
    try:
        # Reconstruir la request completa
        request = datos
        if b"\r\n\r\n" not in request:
            resto = await reader.read(65536)
            request += resto

        request_str = request.decode("utf-8", errors="replace")
        linea = request_str.split("\r\n")[0]
        parts = linea.split(" ")
        if len(parts) < 2:
            writer.close()
            return

        metodo = parts[0]
        ruta_partes = parts[1].split("?")
        ruta = ruta_partes[0]

        params = {}
        if len(ruta_partes) > 1:
            for p in ruta_partes[1].split("&"):
                if "=" in p:
                    k, v = p.split("=", 1)
                    params[k] = v

        if metodo == "GET":
            if ruta == "/":
                await responder_html(writer, HTML_PAGE)
            elif ruta == "/api/estado":
                await responder_json(writer, sim.estado())
            elif ruta == "/api/archivos":
                await responder_json(writer, sim.lista_archivos())
            elif ruta == "/api/descargar":
                archivo = params.get("archivo", "")
                if not archivo:
                    await responder_error(writer, 400, "Falta archivo")
                else:
                    contenido = sim.csv_content(archivo)
                    await responder_csv(writer, archivo, contenido)
            else:
                await responder_error(writer, 404, "No encontrado")
        else:
            await responder_error(writer, 405, "Método no permitido")
    except:
        pass
    finally:
        try:
            writer.close()
        except:
            pass


if __name__ == "__main__":
    print("=" * 56)
    print("  Argos - Simulador del Backend")
    print("=" * 56)
    print()

    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n  Simulador detenido.")
