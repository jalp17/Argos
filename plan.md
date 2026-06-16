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

### 🔄 Fase 6: Desarrollo del Componente argos_net (Conectividad y Servidor Web)
- [ ] Estructura de directorios para argos_net
- [ ] Configuración de SoftAP con IP estática
- [ ] Servidor web embebido (HTTP server)
- [ ] Interfaz web (HTML/CSS/JS minificados)
- [ ] WebSockets para datos en tiempo real
- [ ] REST API para descarga de datos (CSV/JSON)
- [ ] Pruebas de conectividad

### ⏳ Fase 7: Desarrollo del Componente argos_router (Enrutamiento de Datos)
- [ ] Estructura de directorios para argos_router
- [ ] Colas FreeRTOS para ruteo entre componentes
- [ ] Logging multicanal (Serial, Almacenamiento, Web)
- [ ] Sincronización con mutex
- [ ] Pruebas de concurrencia

### ⏳ Fase 8: Integración y Main
- [x] Integración de HAL + Store en main.c (orquestador)
- [ ] Inicialización de todos los componentes (argos_net + argos_router)
- [ ] Configuración de Watchdogs
- [ ] Pruebas de integración completa
- [ ] Optimización de memoria

### ⏳ Fase 9: Pruebas y Validación
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

1. **Inmediato:** Desarrollo de argos_net (SoftAP + WebServer + WebSockets + REST API)
2. **Corto plazo:** argos_router (FreeRTOS queues multicanal)
3. **Mediano plazo:** Integración completa + Pruebas + Optimización
4. **Largo plazo:** Documentación final + Ejemplos de uso

## Rumbo del Proyecto

El proyecto sigue una arquitectura modular por componentes ESP-IDF. Cada componente se desarrolla en su propia rama y se fusiona a main tras validación. La prioridad es la integridad de datos y estabilidad del sistema para aplicaciones de lazo cerrado en laboratorios de física.

## Métricas de Calidad

- [x] Tests unitarios HAL: ADC, DAC, PWM, Diagnostics (10 tests)
- [x] Tests unitarios STORE: init, write, flush, stats, list, rotation, batch, self-test (11 tests)
- [ ] Cobertura de pruebas > 80%
- [ ] Sin fugas de memoria (valgrind/heap tracing)
- [ ] Tiempo de respuesta web < 100ms
- [ ] Precisión ADC según especificación ESP32
- [ ] Uso de heap < 70% en operación continua
