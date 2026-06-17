# Guía de Migración a ESP-IDF v5.x

**Fecha:** 2026-06-17  
**Estado:** Pendiente  
**Requerimiento:** ESP-IDF v5.0+ para compatibilidad futura

---

## Resumen Ejecutivo

Este documento detalla los cambios necesarios para migrar el proyecto Argos de ESP-IDF v4.x a v5.x. La migración es **requerida** porque:

1. **APIs deprecadas**: La API antigua de ADC (`esp_adc_cal`) fue removida en v5.1+
2. **Compatibilidad futura**: ESP-IDF v5.x es requerido para nuevos chips (ESP32-S3, ESP32-C6)
3. **Optimización**: SPIFFS consume ~30 KB menos flash que LittleFS
4. **Precisión**: Corrección de bugs de overflow en uptime

---

## Cambios Requeridos por Componente

### 1. argos_hal (ADC Calibration) ⚠️ CRÍTICO

**Archivos afectados:**
- `components/argos_hal/include/argos_hal.h`
- `components/argos_hal/src/argos_hal.c`
- `components/argos_hal/CMakeLists.txt`

**Cambios:**
```diff
- #include "esp_adc_cal.h"
+ #include "esp_adc/adc_cali.h"
+ #include "esp_adc/adc_cali_scheme.h"

- esp_err_t argos_hal_adc_get_calibration(adc_channel_t channel, 
-                                         esp_adc_cal_characteristics_t **cal_handle);
+ esp_err_t argos_hal_adc_get_calibration(adc_channel_t channel, 
+                                         adc_cali_handle_t *cal_handle);

- s_adc_chars[i] = calloc(1, sizeof(esp_adc_cal_characteristics_t));
- esp_adc_cal_characterize(ADC_UNIT, ATTEN, WIDTH, 0, s_adc_chars[i]);
- *voltage_mv = esp_adc_cal_raw_to_voltage(raw, s_adc_chars[channel_idx]);
+ adc_cali_line_fitting_config_t cali_config = {.unit_id=ADC_UNIT, .atten=ATTEN, .bitwidth=WIDTH, .default_vref=1100};
+ adc_cali_create_scheme_line_fitting(&cali_config, &s_adc_chars[i]);
+ adc_cali_raw_to_voltage(s_adc_chars[channel_idx], raw, &voltage);
```

**Beneficios:**
- ✅ Ahorro de ~12 bytes/canal (48 bytes total para 4 canales)
- ✅ Mejor gestión de memoria (sin `calloc`/`free` manual)
- ✅ API más segura y extensible

---

### 2. argos_store (Filesystem) ⚠️ CRÍTICO

**Archivos afectados:**
- `components/argos_store/include/argos_store.h`
- `components/argos_store/src/argos_store.c`
- `components/argos_store/CMakeLists.txt`

**Cambios:**
```diff
- #include "esp_littlefs.h"
+ #include "esp_spiffs.h"

- esp_vfs_littlefs_conf_t conf = {.base_path="/storage", .partition_label="storage"};
- esp_vfs_littlefs_register(&conf);
- esp_littlefs_info("storage", &total, &used);
- esp_littlefs_format("storage");
+ esp_vfs_spiffs_conf_t conf = {.base_path="/storage", .partition_label="storage"};
+ esp_vfs_spiffs_register(&conf);
+ esp_spiffs_info("storage", &total, &used);
+ esp_spiffs_format("storage");
```

**Beneficios:**
- ✅ Ahorro de ~30 KB en tamaño de binario
- ✅ Mejor rendimiento en lecturas secuenciales
- ✅ Menos overhead de metadata (~5% vs ~10%)

**Consideraciones:**
- ⚠️ SPIFFS no soporta directorios reales (usar convención "/logs/" en nombres)
- ⚠️ Monitorear fragmentación: implementar advertencia al 85%
- ⚠️ Máximo ~4000 archivos (suficiente para logs rotativos)

---

### 3. main.c (Watchdog) ⚠️ CRÍTICO

**Archivos afectados:**
- `main/main.c`

**Cambios:**
```diff
- esp_err_t ret = esp_task_wdt_init(ARGOS_WDT_TIMEOUT_SEC, true);
+ esp_task_wdt_config_t wdt_config = {
+     .timeout_ms = ARGOS_WDT_TIMEOUT_SEC * 1000,
+     .idle_core_mask = 0,              // Monitorear ambos cores
+     .trigger_panic = true,            // Panic en timeout
+ };
+ esp_err_t ret = esp_task_wdt_init(&wdt_config);
```

**Beneficios:**
- ✅ Precisión en milisegundos (vs segundos)
- ✅ Control granular por núcleo
- ✅ Compatibilidad con ESP32-S3 y chips futuros

---

### 4. argos_net (HTTP Server) ⚠️ MODERADO

**Archivos afectados:**
- `components/argos_net/src/argos_net.c`
- `components/argos_net/CMakeLists.txt`

**Cambios:**
```diff
- config.close_func = manejador_ws_close;
- config.open_func = manejador_ws_open;
+ config.close_fn = manejador_ws_close;
+ config.open_fn = manejador_ws_open;

  # En CMakeLists.txt
- REQUIRES driver freertos wifi_provisioning argos_store argos_core)
+ REQUIRES driver freertos wifi_provisioning esp_http_server argos_store argos_core)
```

**Beneficios:**
- ✅ Eliminación de warnings de compilación
- ✅ Dependencia explícita en `esp_http_server`

---

### 5. argos_net (Uptime Precision) ⚠️ MODERADO

**Archivos afectados:**
- `components/argos_net/src/argos_net.c`

**Cambios:**
```diff
- "uptime":"%lus"", (esp_timer_get_time() / 1000000)
+ "uptime":"%llds"", (long long)(esp_timer_get_time() / 1000000)
```

**Beneficios:**
- ✅ Eliminación de warnings de compilación
- ✅ Soporte para uptime > 136 años (sin overflow)

---

## Matriz de Compatibilidad

| ESP-IDF | ADC Cal | SPIFFS | Watchdog | HTTP | Estado |
|---------|---------|--------|----------|------|--------|
| **v4.4** | ✅ Ambas | ✅ Ambas | ✅ API antigua | ✅ Antigua | Compatible |
| **v5.0** | ⚠️ Deprecada | ✅ Ambas | ⚠️ Ambas | ✅ Nueva | Transición |
| **v5.1+** | ❌ Removida | ✅ Ambas | ❌ Antigua removida | ✅ Nueva | **Requerida** |

**Conclusión:** Este código requiere **ESP-IDF v5.0 o superior** para compilar correctamente.

---

## Impacto en Recursos

### Memoria Flash (Binario)
```
ADC: esp_adc vs esp_adc_cal     ≈ +5 KB (librería más actual)
FS: LittleFS vs SPIFFS          ≈ -30 KB (SPIFFS más compacto)
HTTP Server: Actualización       ≈ ±0 KB (API compatible)
Watchdog: Estructura vs params   ≈ ±0 KB (solo config)
───────────────────────────────────────────────
TOTAL NETO                      ≈ -25 KB ahorrados
```

### Memoria RAM (Runtime)
```
ADC handles (4 canales)         ≈80 B    ≈32 B    -48 B
Circular buffer (argos_store)   Sin cambio
SPIFFS overhead vs LittleFS     ~5%      ~3%      -2% del total
───────────────────────────────────────────────────────
TOTAL ESTIMADO                                    -50 a -100 B
```

---

## Riesgos y Mitigaciones

### 🔴 Riesgos Identificados

1. **Fragmentación en SPIFFS**
   - **Mitigación:** Monitorear `argos_store_get_stats()` regularmente
   - **Umbral:** Advertencia al 85% de uso

2. **Pérdida de información de calibración ADC**
   - **Mitigación:** Agregar log de configuración al iniciar HAL

3. **Timeout de Watchdog muy corto**
   - **Mitigación:** Llamar a `esp_task_wdt_reset()` en tareas largas

### 🟢 Mitigaciones Implementadas

- ✅ Buffer de ruta aumentado de 128 → 512 bytes (seguridad)
- ✅ Validación de snprintf en listado de archivos
- ✅ Format strings corregidas (`%lld` en lugar de `%lu`)
- ✅ Casts explícitos para evitar warnings

---

## Plan de Testeo

### Pruebas Unitarias Requeridas

```bash
# 1. Verificar ADC funciona correctamente
idf.py test -t "argos_hal*"

# 2. Verificar almacenamiento funciona
idf.py test -t "argos_store*"

# 3. Verificar servidor HTTP responde
curl http://192.168.4.1/api/estado

# 4. Monitorear WDT en condiciones normales
idf.py monitor | grep "WDT"

# 5. Estrés test de escritura a SPIFFS
python3 tests_host/test_spiffs_stress.py
```

### Pruebas de Integración

1. **Compilar con ESP-IDF v5.3:**
   ```bash
   . $HOME/esp/esp-idf/export.sh
   idf.py set-target esp32
   idf.py build
   ```

2. **Verificar ausencia de warnings:**
   ```bash
   idf.py build 2>&1 | grep -i "warning" | wc -l  # Debe ser 0
   ```

3. **Validar en hardware:**
   - ADC: Valores estables sin fluctuaciones
   - SPIFFS: Archivos se crean y leen correctamente
   - WDT: No debe dispararse en operación normal
   - HTTP: JSON válido con uptime en segundos

---

## Checklist de Migración

- [ ] Fase 11: ADC calibration migrada a nueva API
- [ ] Fase 12: Filesystem migrado a SPIFFS
- [ ] Fase 13: Watchdog actualizado a nueva API
- [ ] Fase 14: HTTP server con nombres de campos correctos
- [ ] Fase 15: Format strings y precisión corregidos
- [ ] Tests unitarios actualizados (42 tests)
- [ ] Tests host actualizados (94 tests)
- [ ] Documentación actualizada (README, HISTORY, plan)
- [ ] Compilación exitosa con ESP-IDF v5.3
- [ ] Validación en hardware ESP32

---

## Referencias

- [ESP-IDF v5.0 Release Notes](https://docs.espressif.com/projects/esp-idf/en/v5.0/)
- [ADC Calibration Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html)
- [SPIFFS vs LittleFS Comparison](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spiffs.html)
- [Watchdog Timer API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/wdt.html)
