# Argos

**Argos** es un framework de instrumentación modular basado en ESP-IDF para el ESP32. El objetivo es el desarrollo de un código base modular, escalable y reutilizable para el microcontrolador ESP32. Este proyecto sirve como infraestructura para diversas prácticas de laboratorio de física, incluyendo instrumentación, medición, calibración y sistemas de lazo cerrado.

## Características principales

- **Conectividad:** SoftAP con SSID y contraseña configurables.
- **Servidor Web:** Interfaz gráfica ligera para visualización en tiempo real de datos.
- **Almacenamiento:** Sistema de archivos LittleFS con gestión de memoria crítica.
- **Adquisición de Señales:** Lectura de pines ADC y control de salidas DAC/PWM.
- **Logging Multicanal:** Datos enrutados a monitor serial, almacenamiento interno y servidor web.

## Arquitectura

El proyecto sigue una arquitectura modular basada en componentes de ESP-IDF:

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

## Requerimientos

- **Hardware:** ESP32 (Arquitectura Xtensa).
- **Framework:** ESP-IDF nativo.
- **Lenguaje:** C / C++.
- **Sistema Operativo en Tiempo Real:** FreeRTOS.
- **Sistema de Archivos:** LittleFS.

## Instalación

1. Clona el repositorio:
   ```bash
   git clone <repository-url>
   cd Argos
   ```

2. Configura el entorno ESP-IDF según la documentación oficial.

3. Configura el proyecto:
   ```bash
   idf.py menuconfig
   ```

4. Compila y flashea el proyecto:
   ```bash
   idf.py build
   idf.py flash
   ```

## Uso

1. Conecta tu dispositivo al SoftAP del ESP32.
2. Abre un navegador y navega a la dirección IP del ESP32.
3. Visualiza los datos en tiempo real y descarga los registros en formato CSV o JSON.

## Licencia

Este proyecto está bajo la Licencia MIT. Consulta el archivo [LICENSE](LICENSE) para más detalles.
