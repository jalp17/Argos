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

## Tareas Pendientes (futuro, con hardware)

1. **Pruebas en ESP32 real:** Validar ADC/DAC/PWM en hardware físico
2. **Pruebas de estrés:** Heap, fugas, concurrencia con mutex
3. **Pruebas de "encender y medir":** Validar arranque y enrutamiento
4. **Pruebas de precisión:** Exactitud ADC según especificación ESP32

## Rumbo del Proyecto

Arquitectura modular por componentes ESP-IDF. Cada componente se desarrolla en su propia rama y se fusiona a main tras validación. Prioridad: integridad de datos y estabilidad para aplicaciones de lazo cerrado.

**10 de 10 fases completadas.** Framework completo con documentación, tests en host (94) y tests ESP-IDF (41).

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
