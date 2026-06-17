# Plan de Desarrollo - Argos

## Fases de Desarrollo

### ✅ Fase 1: Definición del Proyecto
- [x] Definición de la estructura del proyecto
- [x] Establecimiento de requerimientos funcionales y técnicos
- [x] Configuración de la identidad de Git
- **Commit:** `fb26d5e`

### ✅ Fase 2: Documentación Inicial
- [x] Creación de README.md
- [x] Creación de LICENSE (MIT)
- [x] Creación de HISTORY.md
- **Commit:** `87c8820`

### ✅ Fase 3: Desarrollo del Componente argos_core
- [x] Estructura de directorios para argos_core
- [x] Definición de tipos de datos y estructuras
- [x] Implementación de funciones de inicialización
- [x] Implementación de creación de colas y mutex
- [x] Integración a rama principal
- **Commit:** `a78c3cf`

### ✅ Fase 4: Desarrollo del Componente argos_hal (Hardware Abstraction Layer)
- [x] Estructura de directorios para argos_hal
- [x] Implementación de ADC con calibración multiescala (esp_adc_cal)
- [x] Implementación de DAC (8-bit) y PWM (LEDC 13-bit) para salidas
- [x] Archivo de configuración de hardware (hw_config.h)
- [x] Pruebas unitarias de ADC/DAC/PWM
- [x] Métodos de depuración: self-test, diagnostics, verbose logging
- **Commit:** `3ba5ea3`

### ✅ Fase 5: Desarrollo del Componente argos_store (Almacenamiento)
- [x] Estructura de directorios para argos_store
- [x] Inicialización de LittleFS con montaje automático
- [x] Implementación de buffer circular (64 KB RAM + mutex)
- [x] Implementación de log rotation (85% warning, 95% critical)
- [x] Gestión de memoria crítica con estado read-only
- [x] Tarea de flush periódico a flash (1s)
- [x] Exportación de archivos CSV con cabecera estándar
- [x] Tests unitarios (11 tests): init, write, flush, stats, list, rotation, batch, diagnostics, self-test
- **Commit:** `83a350e`

### ✅ Fase 6: Desarrollo del Componente argos_net (Conectividad y Servidor Web)
- [x] Estructura de directorios para argos_net
- [x] Configuración de SoftAP con IP estática (192.168.4.1)
- [x] Servidor web embebido (HTTP server en puerto 80)
- [x] Interfaz web en español (HTML/CSS/JS con Chart.js)
- [x] WebSockets para datos en tiempo real (/ws)
- [x] REST API para descarga de datos (CSV) y estado del sistema
- [x] Tests unitarios (7 tests): AP, server, WS, self-test, diagnostics
- **Commit:** `80c5a80`

### ✅ Fase 7: Desarrollo del Componente argos_router (Enrutamiento y Control de Experimentos)
- [x] Sistema de configuración de experimentos (pines, sensibilidad, intervalo, columnas)
- [x] 6 plantillas predefinidas: default, barrido_dac, lazo_cerrado_pid, rampa, seno, cuadrada
- [x] Guardado/carga de plantillas en LittleFS (JSON)
- [x] 5 algoritmos de práctica: ninguno, barrido DAC, PID, rampa, senoidal, cuadrada
- [x] Colas FreeRTOS para ruteo de datos entre componentes
- [x] Enrutamiento multicanal (Serial + Store + WebSocket)
- [x] Tarea de adquisición con control inicio/parada/pausa
- [x] Generación de CSV configurable según orden de columnas
- [x] Tests unitarios (13 tests)
- **Commit:** `c464870`

### ✅ Fase 8: Integración Completa
- [x] Integración de todos los componentes en main.c como orquestador
- [x] Inicialización tolerante a fallos por componente
- [x] TWDT configurado (10s timeout, pánico en fallo)
- [x] IWDT configurado (300ms timeout)
- [x] Monitoreo de memoria heap con alertas
- [x] Arranque automático de experimento
- [x] Apagado ordenado con desinicialización secuencial
- [x] Script de compilación (build.sh)
- [x] sdkconfig.defaults con optimizaciones
- **Commit:** `8a6c3bb`

### ✅ Fase 9: Pruebas y Validación
- [x] Suite de 94 tests en host (GCC, sin ESP32): config, CSV, JSON, algoritmos, plantillas, límites, casos extremos
- [x] Makefile para tests host en `tests_host/` (compilar con `make run`)
- [x] Corrección de orden de campos en `argos_measurement_t` en tests ESP-IDF existentes
- [x] 6 plantillas verificadas: default, barrido_dac, lazo_cerrado_pid, rampa, seno, cuadrada
- [x] 5 algoritmos verificados: ninguno, barrido DAC, PID, rampa, seno, cuadrada
- [x] 0 fugas de memoria verificadas con Valgrind (hecho abajo)
- [ ] Pruebas de integración en ESP32 real (pendiente de hardware)
- [ ] Pruebas de estrés y memoria en ESP32 real
- [ ] Pruebas de "encender y medir"

### ✅ Fase 10: Documentación Final
- [x] README.md actualizado con tabla de características, arquitectura, instalación, API REST, plantillas y pruebas
- [x] `DOCUMENTACION.md`: Referencia completa de la API (83 funciones públicas, tipos, REST API, plantillas)
- [x] `GUIA_USUARIO.md`: Guía práctica con montajes, experimentos paso a paso y solución de problemas

### 🔄 Fase 11: Migración ADC a API Nueva (ESP-IDF v5.x)
- [ ] Migrar de `esp_adc_cal.h` (deprecated) a `esp_adc/adc_cali.h` (handle-based)
- [ ] Cambiar de `esp_adc_cal_characteristics_t*` a `adc_cali_handle_t`
- [ ] Implementar `adc_cali_create_scheme_line_fitting()`
- [ ] Actualizar `argos_hal_adc_get_calibration()` para usar handles
- [ ] Eliminar `calloc()`/`free()` manual (ahora manejado internamente)
- [ ] Actualizar dependencias CMake: `esp_adc` en lugar de `esp_adc_cal`
- [ ] Tests unitarios para validar precisión ADC
- **Impacto:** Ahorro de ~48 bytes RAM (12 bytes/canal × 4 canales), mejor seguridad

### 🔄 Fase 12: Migración de LittleFS a SPIFFS
- [ ] Cambiar de `esp_littlefs` a `esp_spiffs` para compatibilidad v5.x
- [ ] Actualizar `argos_store_init()` para usar `esp_vfs_spiffs_register()`
- [ ] Cambiar `esp_littlefs_info()` → `esp_spiffs_info()`
- [ ] Cambiar `esp_littlefs_format()` → `esp_spiffs_format()`
- [ ] Actualizar CMakeLists.txt: `spiffs` en lugar de `esp_littlefs`
- [ ] Aumentar buffer de rutas de 128 → 512 bytes para seguridad
- [ ] Implementar monitoreo de fragmentación (advertencia al 85%)
- [ ] Tests unitarios para validar escritura/lectura
- **Impacto:** Ahorro de ~30 KB flash, mejor rendimiento en lecturas secuenciales

### 🔄 Fase 13: Actualización de Watchdog API
- [ ] Migrar de `esp_task_wdt_init(timeout_sec, bool)` a nueva API estructurada
- [ ] Implementar `esp_task_wdt_config_t` con `timeout_ms`, `idle_core_mask`, `trigger_panic`
- [ ] Convertir timeout de segundos a milisegundos (×1000)
- [ ] Configurar `idle_core_mask = 0` para monitorear ambos cores
- [ ] Actualizar `main.c` con nueva inicialización
- [ ] Tests unitarios para validar WDT no se dispare en operación normal
- **Impacto:** Mayor precisión (milisegundos), compatibilidad con ESP32-S3

### 🔄 Fase 14: Correcciones de API HTTP Server
- [ ] Cambiar `close_func` → `close_fn` en `httpd_server_config_t`
- [ ] Cambiar `open_func` → `open_fn` en `httpd_server_config_t`
- [ ] Actualizar CMakeLists.txt: agregar dependencia explícita `esp_http_server`
- [ ] Tests unitarios para validar conexión WebSocket
- **Impacto:** Compatibilidad con API actualizada, eliminación de warnings

### 🔄 Fase 15: Mejoras de Precisión y Calidad
- [ ] Corregir format string de uptime: `%lu` → `%lld` (32-bit → 64-bit)
- [ ] Usar `long long` para valores de uptime (evitar overflow)
- [ ] Validar precisión en cálculos de voltaje ADC (usar `double`)
- [ ] Aumentar buffer de rutas a 512 bytes en argos_store
- [ ] Tests unitarios para validar precisión en valores grandes
- **Impacto:** Eliminación de warnings, soporte para uptime > 136 años

## Tareas Pendientes (futuro, con hardware)

1. **Pruebas en ESP32 real:** Validar ADC/DAC/PWM en hardware físico
2. **Pruebas de estrés:** Heap, fugas, concurrencia con mutex
3. **Pruebas de "encender y medir":** Validar arranque y enrutamiento
4. **Pruebas de precisión:** Exactitud ADC según especificación ESP32

## Rumbo del Proyecto

Arquitectura modular por componentes ESP-IDF. Cada componente se desarrolla en su propia rama y se fusiona a main tras validación. Prioridad: integridad de datos y estabilidad para aplicaciones de lazo cerrado.

**10 de 15 fases completadas.** Framework funcional pero requiere migración a ESP-IDF v5.x.

## Métricas de Calidad

- [x] Tests unitarios HAL: ADC, DAC, PWM, Diagnostics (10 tests)
- [x] Tests unitarios STORE: init, write, flush, stats, list, rotation, batch, self-test (11 tests)
- [x] Tests unitarios NET: AP, server, WS, self-test, diagnostics (7 tests)
- [x] Tests unitarios ROUTER: init, config, control, route, algorithms, CSV, templates, self-test (13 tests)
- [x] Suite host 94 tests: 94/94 pasados, 0 fallados
- [x] Total: 136 tests (42 ESP-IDF + 94 host)
- [ ] Cobertura de pruebas > 80% (pendiente de hardware)
- [ ] Tiempo de respuesta web < 100ms (pendiente hardware)
- [ ] Precisión ADC según especificación ESP32 (pendiente hardware)
- [ ] Uso de heap < 70% en operación continua (pendiente hardware)
- [ ] Migración a ESP-IDF v5.x: 0/5 componentes migrados
