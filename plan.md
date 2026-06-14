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

### ⏳ Fase 5: Desarrollo del Componente argos_store (Almacenamiento)
- [ ] Estructura de directorios para argos_store
- [ ] Inicialización de LittleFS
- [ ] Implementación de buffer circular
- [ ] Implementación de log rotation
- [ ] Gestión de memoria crítica (80-90% threshold)
- [ ] Pruebas de integridad de datos

### ⏳ Fase 6: Desarrollo del Componente argos_net (Conectividad y Servidor Web)
- [ ] Estructura de directorios para argos_net
- [ ] Configuración de SoftAP con IP estática
- [ ] Servidor web embebido
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
- [ ] Implementación de main.c (orquestador)
- [ ] Inicialización de todos los componentes
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

1. **Inmediato:** Desarrollo de argos_store (LittleFS + log rotation + buffer circular)
2. **Corto plazo:** argos_net (SoftAP + WebServer + WebSockets + REST API)
3. **Mediano plazo:** argos_router (FreeRTOS queues multicanal)
4. **Largo plazo:** Integración completa + Pruebas + Documentación final

## Rumbo del Proyecto

El proyecto sigue una arquitectura modular por componentes ESP-IDF. Cada componente se desarrolla en su propia rama y se fusiona a main tras validación. La prioridad es la integridad de datos y estabilidad del sistema para aplicaciones de lazo cerrado en laboratorios de física.

## Métricas de Calidad

- [x] Tests unitarios HAL: ADC, DAC, PWM, Diagnostics (10 tests)
- [ ] Cobertura de pruebas > 80%
- [ ] Sin fugas de memoria (valgrind/heap tracing)
- [ ] Tiempo de respuesta web < 100ms
- [ ] Precisión ADC según especificación ESP32
- [ ] Uso de heap < 70% en operación continua
