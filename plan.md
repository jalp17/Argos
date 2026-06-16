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

### 🔄 Fase 9: Pruebas y Validación
- [ ] Pruebas unitarias completas
- [ ] Pruebas de integración
- [ ] Pruebas de estrés y memoria
- [ ] Validación de integridad de datos
- [ ] Verificación de uso de Mutex
- [ ] Pruebas de "encender y medir"

### ⏳ Fase 10: Documentación Final
- [ ] Actualización de README.md
- [ ] Documentación de API
- [ ] Guía de usuario
- [ ] Ejemplos de uso

## Tareas Pendientes

1. **Inmediato:** Pruebas de integración y validación
2. **Corto plazo:** Pruebas de estrés y memoria
3. **Mediano plazo:** Documentación de API
4. **Largo plazo:** Documentación final, guía de usuario y ejemplos

## Rumbo del Proyecto

Arquitectura modular por componentes ESP-IDF. Cada componente se desarrolla en su propia rama y se fusiona a main tras validación. Prioridad: integridad de datos y estabilidad para aplicaciones de lazo cerrado.

**8 de 10 fases completadas.** Núcleo funcional completo. Pendiente: pruebas de validación y documentación final.

## Métricas de Calidad

- [x] Tests unitarios HAL: ADC, DAC, PWM, Diagnostics (10 tests)
- [x] Tests unitarios STORE: init, write, flush, stats, list, rotation, batch, self-test (11 tests)
- [x] Tests unitarios NET: AP, server, WS, self-test, diagnostics (7 tests)
- [x] Tests unitarios ROUTER: init, config, control, route, algorithms, CSV, templates, self-test (13 tests)
- [ ] Cobertura de pruebas > 80%
- [ ] Sin fugas de memoria (valgrind/heap tracing)
- [ ] Tiempo de respuesta web < 100ms
- [ ] Precisión ADC según especificación ESP32
- [ ] Uso de heap < 70% en operación continua
