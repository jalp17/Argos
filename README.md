# Argos

Framework de instrumentación modular basado en ESP-IDF para ESP32. Diseñado para prácticas de laboratorio de física que requieren adquisición de señales, control de lazo cerrado, almacenamiento seguro y visualización web en tiempo real —sin depender de Arduino.

**Licencia:** MIT  
**Repositorio:** `git@github.com:jalp17/Argos.git`

---

## Características

| Característica | Detalle |
|---|---|
| **ADC** | 4 canales, 12 bits, calibración multiescala (9/10/11/12 bits, 0/2.5/6/11 dB) |
| **DAC** | 2 canales, 8 bits, escritura por valor o voltaje directo |
| **PWM** | 4 canales, 13 bits via LEDC, frecuencia y duty configurables |
| **Almacenamiento** | SPIFFS con buffer circular de 64 KB, flush periódico a flash |
| **Rotación de logs** | Automática al 85%, crítica al 95% (modo solo lectura) |
| **Conectividad** | SoftAP (SSID `Argos-AP`, IP `192.168.4.1`) |
| **Servidor web** | HTTP en puerto 80, API REST, WebSocket en `/ws` |
| **Panel web** | Tiempo real con Chart.js, controles DAC/PWM, descarga CSV |
| **Experimentación** | 6 plantillas predefinidas, 6 algoritmos, columnas CSV configurables |
| **Watchdogs** | TWDT 10s + IWDT 300ms para lazo cerrado seguro |
| **Inicialización** | Tolerante a fallos: cada componente arranca independientemente |

---

## Arquitectura

```text
Argos/
├── main/
│   ├── main.c                ← Orquestador (app_main)
│   └── CMakeLists.txt
├── components/
│   ├── argos_core/           ← Tipos base, colas FreeRTOS, mutex
│   ├── argos_hal/            ← ADC, DAC, PWM, self-test, diagnósticos
│   ├── argos_store/          ← SPIFFS, buffer circular, rotación
│   ├── argos_net/            ← SoftAP, HTTP, WebSocket, REST API
│   └── argos_router/         ← Enrutamiento, algoritmos, plantillas
├── test/                     ← Tests unitarios ESP-IDF (42 tests)
├── tests_host/               ← Tests en host sin hardware (94 tests)
├── sdkconfig.defaults        ← Configuración optimizada del SDK
├── build.sh                  ← Script de compilación automatizada
└── CMakeLists.txt            ← Proyecto raíz IDF
```

**Flujo de datos:**

```
ADC → argos_hal → argos_router → argos_store (SPIFFS)
                                → UART Serial
                                → argos_net (WebSocket → panel web)
```

---

## Requerimientos mínimos

| Requisito | Versión |
|---|---|
| ESP-IDF | ≥ v5.3 |
| ESP32 | Xtensa (cualquier variante con ADC/DAC) |
| CMake | ≥ 3.16 |
| Ninja | ≥ 1.10 |
| Python | ≥ 3.8 |

✅ **Compatibilidad:** Migrado a ESP-IDF v5.x (ADC calibration nueva API, SPIFFS, watchdog configurable, HTTP server actualizado).

---

## Instalación y compilación

```bash
# 1. Clonar
git clone git@github.com:jalp17/Argos.git && cd Argos

# 2. Compilar (automatiza init de submodules e instalación)
./build.sh

# 3. O manualmente:
source /path/to/esp-idf/export.sh
idf.py set-target esp32
idf.py build

# 4. Flashear
idf.py flash monitor
```

---

## Uso básico

1. Conecte su dispositivo WiFi al SoftAP:
   - **SSID:** `Argos-AP`
   - **Contraseña:** `argos1234`
   - **IP del servidor:** `http://192.168.4.1`

2. Abra un navegador en `http://192.168.4.1`.

3. El panel web muestra:
   - 4 gráficos de ADC en tiempo real (Chart.js)
   - Controles de DAC (0-3300 mV) y PWM (0-100%)
   - Botones de inicio/parada/pausa de experimento
   - Selector de plantilla de experimento
   - Listado y descarga de archivos CSV

4. **API REST:**
   - `GET /api/estado` — Estado del sistema (JSON)
   - `GET /api/archivos` — Lista de archivos almacenados
   - `GET /api/descargar?archivo=...` — Descarga CSV
   - `ws://192.168.4.1/ws` — WebSocket para datos en vivo

---

## Red

| Parámetro | Valor |
|---|---|
| **SSID** | `Argos-AP` |
| **Contraseña** | `argos1234` |
| **IP del servidor** | `192.168.4.1` |
| **Puerto HTTP** | 80 |
| **Puerto WebSocket** | 80 |

---

## Watchdogs

| Watchdog | Timeout | Propósito |
|---|---|---|
| **TWDT** | 10 s | Monitorea tareas FreeRTOS, pánico en fallo |
| **IWDT** | 300 ms | Monitorea ISRs y bucles críticos |

---

## Rotación de Logs

| Umbral | Acción |
|---|---|
| **85%** | Advertencia + eliminar archivo más antiguo |
| **95%** | Modo crítico: solo lectura, evitar corrupción |

---

## Algoritmos

| Algoritmo | Parámetros | Descripción |
|---|---|---|
| `NINGUNO` | — | Captura directa sin procesamiento |
| `BARRIDO_DAC` | param1=Vmax, param2=pasos | Barre DAC de 0 a Vmax en N pasos |
| `LAZO_CERRADO_PID` | param1=Kp, param2=Ki, param3=Kd, param4=SP | Control PID con realimentación ADC |
| `RAMPA` | param1=Vmax, param2=duración | Rampa lineal en DAC |
| `SENO` | param1=amplitud, param2=offset, param3=freq | Onda senoidal en DAC |
| `CUADRADA` | param1=alto, param2=bajo, param3=periodo, param4=duty | Onda cuadrada en DAC |

---

## Plantillas de experimento

| Plantilla | Algoritmo | ADC | DAC | Muestras | Intervalo | Columnas |
|---|---|---|---|---|---|---|
| `default` | Ninguno | 4 | 2 (off) | ∞ | 100 ms | 4 |
| `barrido_dac` | Barrido DAC | 1 | 1 | 256 | 100 ms | 5 |
| `lazo_cerrado_pid` | PID | 1 | 1 | ∞ | 50 ms | 6 |
| `rampa` | Rampa | 0+ | 1+ | ∞ | 10 ms | 4 |
| `seno` | Senoidal | 0+ | 1+ | 500 | 20 ms | 4 |
| `cuadrada` | Cuadrada | 0+ | 1+ | 1000 | 10 ms | 4 |

---

## Referencia de API

### argos_core

```c
typedef struct { uint32_t timestamp; float value; uint8_t channel; } argos_measurement_t;
void argos_core_init(argos_config_t *config);
QueueHandle_t argos_core_create_queue(size, item_size);
SemaphoreHandle_t argos_core_create_mutex(void);
```

### argos_hal

| Función | Descripción |
|---|---|
| `argos_hal_adc_init(config)` | Inicializa ADC |
| `argos_hal_adc_read_raw(ch, *raw)` | Lee valor crudo (0-4095) |
| `argos_hal_adc_read_voltage(ch, *mV)` | Lee voltaje calibrado |
| `argos_hal_adc_read_multi(ch[], n, v[])` | Lectura multicanal |
| `argos_hal_adc_set_atten(ch, atten)` | Configura atenuación |
| `argos_hal_dac_init()` | Inicializa DAC |
| `argos_hal_dac_write(ch, value)` | Escribe 8-bit (0-255) |
| `argos_hal_dac_write_voltage(ch, mV)` | Escribe voltaje (0-3300 mV) |
| `argos_hal_dac_enable(ch, enable)` | Habilita salida |
| `argos_hal_pwm_init()` | Inicializa PWM |
| `argos_hal_pwm_config_channel(ch, gpio, freq, duty)` | Configura canal |
| `argos_hal_pwm_set_duty(ch, duty)` | Duty 0-8191 (13 bits) |
| `argos_hal_pwm_start(ch)` / `argos_hal_pwm_stop(ch)` | Control PWM |
| `argos_hal_self_test()` | Auto-test de hardware |
| `argos_hal_print_diagnostics()` | Diagnóstico HAL |

### argos_store

| Función | Descripción |
|---|---|
| `argos_store_init()` / `deinit()` / `is_ready()` | Ciclo de vida |
| `argos_store_write_measurement(*m)` | Escribe al buffer circular |
| `argos_store_write_measurements(*m, count)` | Escritura por lote |
| `argos_store_flush()` | Fuerza flush a flash |
| `argos_store_create_log_file(format)` | Crea archivo de log |
| `argos_store_list_files(files[], max, *count)` | Lista archivos |
| `argos_store_export_file(name, buf, size, *read)` | Exporta a buffer |
| `argos_store_get_stats(*stats)` | Estadísticas de uso |
| `argos_store_check_rotation()` | Verifica y rota logs |
| `argos_store_set_thresholds(warn%, crit%)` | Umbrales (defecto 85/95) |
| `argos_store_self_test()` | Auto-test |
| `argos_store_format()` | Formatea partición (peligroso) |

### argos_net

| Función | Descripción |
|---|---|
| `argos_net_init()` / `deinit()` / `is_ready()` | Ciclo de vida |
| `argos_net_ws_broadcast(data, len)` | Broadcast a todos los WS |
| `argos_net_ws_send_measurement(*m)` | Envía medición como JSON |
| `argos_net_ws_get_client_count()` | Clientes conectados |
| `argos_net_get_ap_ip(ip)` / `get_ap_ssid(ssid)` | Info de red |
| `argos_net_self_test()` | Auto-test |

### argos_router

| Función | Descripción |
|---|---|
| `exp_config_init_default(*cfg)` | Config por defecto |
| `exp_config_apply_template(*cfg, "nombre")` | Aplica plantilla |
| `exp_config_save_template(*cfg, "nombre")` | Guarda plantilla |
| `exp_config_load_template(*cfg, "nombre")` | Carga plantilla |
| `exp_config_generate_csv_header(*cfg, buf, size)` | Encabezado CSV |
| `exp_config_measurement_to_csv(*cfg, *m, buf, size)` | Medición a CSV |
| `exp_config_to_json(*cfg, buf, size)` | Config a JSON |
| `argos_router_init(*cfg)` / `deinit()` / `is_ready()` | Ciclo de vida |
| `argos_router_start_experiment()` / `stop()` / `pause()` | Control |
| `argos_router_get_state()` | Estado actual |
| `argos_router_get_config()` / `set_config(*cfg)` | Config |
| `argos_router_route_measurement(*m)` | Enruta medición |
| `argos_router_run_algorithm_step(entrada, *salida)` | Paso de algoritmo |
| `argos_router_get_stats(*stats)` | Estadísticas |
| `argos_router_self_test()` | Auto-test |

### Secuencia de inicialización (app_main)

1. Watchdogs: TWDT (10s) + IWDT (300ms)
2. argos_core → argos_hal (ADC→DAC→PWM→self-test) → argos_store (init→self-test) → argos_net (AP→HTTP)
3. argos_router: config por defecto → tarea de adquisición
4. Bucle: monitoreo heap, estadísticas cada 30s, reset WDT cada 1s

---

## Guía de laboratorio

### Experimentos prácticos

**Captura directa (`default`):**
1. Conecte señales 0-3.3V a GPIO36/39/32/33
2. Seleccione plantilla "default" en el panel web
3. Presione **Iniciar** y observe las 4 gráficas
4. Presione **Detener** y descargue el CSV

**Barrido DAC (`barrido_dac`):**
1. Conecte DAC0 (GPIO25) a ADC0 (GPIO36) con un cable
2. Seleccione plantilla "barrido_dac" e **Inicie**
3. DAC barre 0→3300 mV en 256 pasos, ADC lee simultáneamente
4. Al completar se detiene automáticamente. Descargue y grafique.

**Control PID (`lazo_cerrado_pid`):**
```
DAC0(GPIO25) ──[R_limit]──┐
                           ├──┬── Termistor ──┐
ADC0(GPIO36) ──[R_fija]───┘  │               │
                              └── GND ────────┘
```
- Kp=1.0, Ki=0.1, Kd=0.05, Setpoint=1500 mV
- DAC se ajusta para mantener ADC en 1500 mV. Perturbe y observe.

**Rampa:** DAC genera rampa lineal (param1=Vmax, param2=duración ms)
**Seno:** DAC genera senoidal (param1=amplitud, param2=offset, param3=freq Hz)
**Cuadrada:** DAC genera cuadrada (param1=alto, param2=bajo, param3=periodo ms, param4=duty %)

### Solución de problemas

| Problema | Causa | Solución |
|---|---|---|
| No aparece red Argos-AP | SoftAP falló | `idf.py monitor` para ver errores |
| ADC lee 0 | Señal fuera de rango | Verificar atenuación y conexiones |
| DAC no produce voltaje | Canal deshabilitado | `canales_dac[i].habilitado = true` |
| WebSocket no conecta | Puerto incorrecto | Usar `ws://192.168.4.1/ws` |
| Watchdog reinicia | Bucle bloqueado | Aumentar TWDT a 15s en sdkconfig |

### Comandos útiles

```bash
./build.sh                    # Compilar todo
idf.py -p /dev/ttyUSB0 flash  # Flashear
idf.py -p /dev/ttyUSB0 monitor  # Monitor serie
idf.py test                   # Tests ESP-IDF
cd tests_host && make run     # Tests host (94 tests)
idf.py size-components        # Tamaño del firmware
idf.py menuconfig             # Configuración
```

---

## Pruebas

### En host (sin ESP32)

```bash
cd tests_host
make run
# 94 tests: configuración, CSV, JSON, algoritmos, plantillas, límites
```

### En ESP32 (con IDF)

```bash
idf.py test
# 42 tests distribuidos en HAL (11), Store (11), Net (7) y Router (13)
```

---

## Componentes del proyecto

| Archivo | Propósito |
|---|---|
| `README.md` | Esta documentación |
| `LICENSE` | Licencia MIT |
| `sdkconfig.defaults` | Configuración SDK optimizada |
| `build.sh` | Script de compilación |