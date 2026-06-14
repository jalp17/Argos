## 1. Definición del Proyecto
**Argos** es un framework de instrumentación modular basado en ESP-IDF para el ESP32. El objetivo es el desarrollo de un código base modular, escalable y reutilizable para el microcontrolador ESP32. El objetivo principal es servir como infraestructura para diversas prácticas de laboratorio de física (instrumentación, medición, calibración y sistemas de lazo cerrado). El sistema debe manejar la adquisición de señales analógicas, control de salidas de voltaje, registro de datos (logging) seguro, y proporcionar una interfaz de usuario a través de un servidor web alojado en un Punto de Acceso (AP) propio.

## 2. Entorno y Herramientas
* **Hardware:** Microcontrolador ESP32 (Arquitectura Xtensa).
* **Framework:** ESP-IDF (Espressif IoT Development Framework) nativo. **No utilizar el core de Arduino.**
* **Lenguaje:** C / C++.
* **Sistema Operativo en Tiempo Real:** FreeRTOS (manejo de tareas, colas, semáforos/mutex).
* **Sistema de Archivos:** LittleFS o SPIFFS (preferiblemente LittleFS por su eficiencia y manejo de desgaste en memoria Flash).

## 3. Requerimientos Funcionales Core

### A. Conectividad (Punto de Acceso)
* Montar un SoftAP con un SSID y contraseña fijos y predefinidos (configurables vía `menuconfig` o macros en un archivo header).
* Asignación de IP estática predecible para facilitar la conexión desde un ordenador o móvil.

### B. Servidor Web e Interfaz
* Servidor web embebido que sirva una interfaz gráfica ligera (HTML/CSS/JS puros, preferiblemente minificados).
* Visualización en tiempo real de los datos medidos (usando WebSockets o peticiones AJAX periódicas vía REST API).
* Endpoints específicos para descargar los datos almacenados en formato `.csv` o `.json` al ordenador del usuario.

### C. Almacenamiento y Manejo de Memoria
* Guardar los datos recopilados en la memoria interna (Flash) del ESP32.
* **Gestión crítica de memoria:** Implementar un mecanismo de "Buffer Circular" o "Rotación de Logs" (Log Rotation) en el sistema de archivos. Si la memoria asignada al particionamiento de datos llega a su límite de capacidad (ej. 80-90%), el sistema debe sobreescribir los datos más antiguos o detener la grabación de forma segura, evitando un *Kernel Panic* o desbordamiento (Overflow).

### D. Adquisición y Emisión de Señales (HAL)
* **Entradas:** Lectura de pines ADC (Analógico-Digital) para medición de voltajes. Debe incluir soporte para atenuación configurable y calibración multiescala (`esp_adc_cal`).
* **Salidas:** Uso de pines DAC (Digital-Analógico) o PWM (vía LEDC) para el envío de voltajes controlados a la planta física.

### E. Sistema de Registro (Logging) Multicanal
* Todo dato de medición relevante debe enrutarse hacia tres destinos simultáneamente de forma eficiente:
    1. Monitor Serial (UART).
    2. Almacenamiento Interno (Archivo en LittleFS/SPIFFS).
    3. Servidor Web (Transmisión a clientes conectados).

## 4. Arquitectura de Software (Requisitos de Diseño)

El agente debe estructurar el proyecto utilizando la arquitectura de **Componentes** de ESP-IDF para garantizar que el código sea modular, depurable y fácilmente adaptable a diferentes prácticas de lazo cerrado.

```text
argos_root/
├── main/
│   ├── main.c           (Orquestador: inicia componentes y gestiona Watchdogs)
│   └── CMakeLists.txt
└── components/
    ├── argos_core/      (Definiciones globales, tipos de datos, queues)
    ├── argos_hal/       (ADC, DAC, PWM - Abstracción de pines)
    ├── argos_net/       (SoftAP, WebServer, REST API)
    ├── argos_store/     (LittleFS, log rotation, buffer management)
    └── argos_router/    (FreeRTOS queues para ruteo de datos entre componentes)
```


## 5. Requerimientos de Implementación (Reglas de Oro)
Naming Convention: Todos los componentes deben prefijarse con argos_ (ej: argos_adc_read()).

Memoria: Uso de colas (xQueue) para el paso de mensajes entre tareas. Prohibido el uso de malloc en loops de control para evitar heap fragmentation.


Seguridad de Hilos (Thread Safety): Las lecturas del ADC y el empaquetado de datos ocurrirán en tareas separadas a las peticiones del servidor HTTP. Es mandatorio utilizar colas de FreeRTOS (xQueue) y Mutex (xSemaphoreCreateMutex) para el intercambio de datos entre tareas.

No usar memoria dinámica en bucles: Evitar a toda costa malloc() o free() repetitivos dentro de los bucles de adquisición de datos para prevenir fragmentación de RAM. Usar buffers estáticos y enviar por referencia.

Control de Errores: Utilizar la macro ESP_ERROR_CHECK() para la inicialización de periféricos y verificar siempre que los punteros y buffers no sean NULL antes de operar sobre ellos.

Escalabilidad: Los pines y frecuencias de muestreo deben estar centralizados en un único archivo de configuración (ej. hw_config.h) para que el usuario pueda cambiarlos fácilmente al cambiar de práctica sin tocar la lógica interna del sistema.

## 6. Objetivo del Agente (OpenCode)
Al interactuar con el código, el agente debe:

Asegurar que la interfaz web sea responsiva y consuma los datos vía JSON.

Priorizar la integridad de los datos en el sistema de archivos (evitar corrupción durante desconexiones).

Permitir que el sistema funcione como un dispositivo de "encender y medir" con configuración mínima.

## Notas
Procurar en las fases de desarrollo y pruebas **"validar la integridad de la memoria"** y que **"verificar el uso de los Mutex"**. Como estudiante de instrumentación, sabes que un sistema que cuelga el servidor web durante una medición de lazo cerrado es un desastre; estos recordatorios mantendrán a **Argos** estable y confiable.
