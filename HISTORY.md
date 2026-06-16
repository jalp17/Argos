# Historial de Desarrollo

## 14/06/2026

### 10:00 - Inicio del Proyecto
- **Commit:** `fb26d5e`
- **Descripción:** Se agregó el archivo AGENT.md con la definición del proyecto.
- **Detalles:**
  - Se definió la estructura del proyecto.
  - Se establecieron los requerimientos funcionales y técnicos.
  - Se configuró la identidad de Git.

### 10:05 - Documentación Inicial
- **Commit:** `87c8820`
- **Descripción:** Se creó el archivo README.md con la documentación inicial del proyecto.
- **Detalles:**
  - Se describió el propósito del proyecto.
  - Se detallaron las características principales.
  - Se explicó la arquitectura del proyecto.
  - Se agregaron instrucciones de instalación y uso.

### 10:10 - Licencia
- **Commit:** `87c8820`
- **Descripción:** Se agregó el archivo LICENSE con la licencia MIT.
- **Detalles:**
  - Se estableció la licencia MIT para el proyecto.

### 10:15 - Historial de Desarrollo
- **Commit:** `87c8820`
- **Descripción:** Se creó el archivo HISTORY.md para registrar el progreso del desarrollo.
- **Detalles:**
  - Se registraron las acciones iniciales del proyecto.

### 10:30 - Desarrollo del Componente argos_core
- **Commit:** `a78c3cf`
- **Descripción:** Se desarrolló el componente argos_core con estructuras y funciones básicas.
- **Detalles:**
  - Se creó la estructura de directorios para el componente.
  - Se definieron tipos de datos y estructuras para medición y configuración.
  - Se implementaron funciones para inicialización, creación de colas y mutex.
  - Se integró el componente a la rama principal.

### 10:45 - Configuración de Gitignore y Plan de Desarrollo
- **Commit:** `00de835`
- **Descripción:** Se agregó .gitignore y se creó plan.md con fases de desarrollo.
- **Detalles:**
  - Se excluyeron archivos de documentación (AGENT.md, HISTORY.md, plan.md) del control de versiones.
  - Se definieron 10 fases de desarrollo con tareas específicas.
  - Se marcaron 3 fases como completadas.
  - Se establecieron tareas pendientes y rumbo del proyecto.

### 11:00 - Desarrollo del Componente argos_hal (Hardware Abstraction Layer)
- **Commit:** `3ba5ea3`
- **Descripción:** Se desarrolló el componente argos_hal con ADC, DAC, PWM y diagnósticos.
- **Detalles:**
  - Se creó hw_config.h con configuración centralizada de hardware (pines, frecuencias, resoluciones).
  - Se implementó ADC con calibración multiescala (esp_adc_cal) para 4 canales.
  - Se implementó DAC de 8-bit para 2 canales con escritura por voltaje (mV).
  - Se implementó PWM (LEDC) de 13-bit para 4 canales a 10 kHz.
  - Se añadieron métodos de depuración: self-test, print_diagnostics, verbose logging.
  - Se crearon tests unitarios en test/test_hal.c para ADC, DAC, PWM y diagnósticos.
  - Se integró main.c de prueba con monitoreo continuo.
  - Se fusionó a rama principal tras verificación.

### 11:30 - Desarrollo del Componente argos_store (Almacenamiento)
- **Commit:** `83a350e`
- **Descripción:** Se desarrolló el componente argos_store con LittleFS, buffer circular y log rotation.
- **Detalles:**
  - Se implementó buffer circular en RAM de 64 KB con mutex para escritura segura.
  - Se implementó LittleFS como sistema de archivos con montaje automático.
  - Se implementó tarea de flush periódico (1s) a flash.
  - Se implementó log rotation automática al 85% de uso de partición.
  - Se implementó estado crítico (95%) que detiene escrituras para evitar corrupción.
  - Se implementó exportación de archivos CSV con cabecera estándar.
  - Se implementaron callbacks para eventos de almacenamiento.
  - Se implementaron métodos de depuración: self-test, diagnostics, format.
  - Se crearon tests unitarios (11 tests) en test/test_store.c.
  - Se integró con main.c para logging continuo de mediciones ADC.
  - Se fusionó a rama principal tras verificación.

### 12:00 - Desarrollo del Componente argos_net (Conectividad y Servidor Web)
- **Commit:** `80c5a80`
- **Descripción:** Se desarrolló el componente argos_net con SoftAP, servidor web, WebSockets y API REST.
- **Detalles:**
  - Se implementó SoftAP con SSID "Argos-AP", contraseña "argos1234" e IP estática 192.168.4.1.
  - Se implementó servidor web HTTP con página de control en español (HTML/CSS/JS).
  - Se implementó WebSocket en /ws para transmisión de datos en tiempo real.
  - Se implementó API REST: /api/estado, /api/archivos, /api/descargar.
  - Se implementó panel de control con visualización de 4 canales ADC en tiempo real.
  - Se implementó gráfico en tiempo real usando Chart.js.
  - Se implementó control de DAC y PWM desde la interfaz web.
  - Se implementó lista de archivos de registro con descarga directa.
  - Se implementaron métodos de depuración: self-test, diagnostics.
  - Se crearon tests unitarios (7 tests) en test/test_net.c.
  - Se integró con main.c para envío de mediciones por WebSocket.
  - Se fusionó a rama principal tras verificación.

### 12:30 - Desarrollo del Componente argos_router (Enrutamiento y Control de Experimentos)
- **Commit:** `c464870`
- **Descripción:** Se desarrolló el componente argos_router con configuración de experimentos, plantillas, algoritmos y enrutamiento multicanal.
- **Detalles:**
  - Se implementó `experiment_config.h` con configuración completa de experimentos: pines ADC/DAC/PWM, sensibilidad, intervalo de muestreo, número de muestras, modo de disparo, orden de columnas CSV.
  - Se implementaron 6 plantillas predefinidas: default, barrido_dac, lazo_cerrado_pid, rampa, seno, cuadrada.
  - Se implementó sistema de guardado/carga de plantillas en LittleFS (JSON).
  - Se implementaron 5 algoritmos de práctica: ninguno (captura directa), barrido DAC, PID, rampa, senoidal, cuadrada.
  - Se implementó enrutamiento multicanal de mediciones a tres destinos simultáneos: Serial, Store y WebSocket.
  - Se implementó tarea de adquisición con control de inicio/parada/pausa.
  - Se implementó generación de CSV configurable según orden de columnas.
  - Se implementó logging multicanal con formato personalizable.
  - Se implementaron métodos de depuración: self-test, diagnostics, stats.
  - Se crearon tests unitarios (13 tests) en test/test_router.c.
  - Se integró con main.c como orquestador central del sistema.
  - Se fusionó a rama principal tras verificación.

### 13:00 - Fase 8: Integración Completa y Configuración
- **Commit:** `8a6c3bb`
- **Descripción:** Integración completa de todos los componentes con watchdogs, optimización de memoria y script de compilación.
- **Detalles:**
  - Se implementó Task Watchdog Timer (TWDT) con timeout de 10 segundos y pánico en fallo.
  - Se implementó Interrupt Watchdog Timer (IWDT) con timeout de 300ms.
  - Se agregó monitoreo de memoria heap con métricas de uso y alertas de memoria baja.
  - Se implementó inicialización tolerante a fallos: cada componente puede fallar sin detener el sistema.
  - Se agregó reset periódico del TWDT en el bucle principal.
  - Se implementó apagado ordenado con desinicialización secuencial de componentes.
  - Se creó sdkconfig.defaults con configuración optimizada para watchdogs, WiFi, HTTP, FreeRTOS y LittleFS.
  - Se creó build.sh para automatizar la compilación del proyecto.
  - Se actualizó main/CMakeLists.txt con dependencias de esp_timer y heap.
  - Se fusionó a rama principal tras verificación.
