#!/usr/bin/env bash
set -euo pipefail

# ==========================================================
# argos.sh — Asistente de compilación, flasheo y monitor
#             para el framework de instrumentación Argos
# ==========================================================

# ---- Configuración ----
IDF_PATH="${IDF_PATH:-/tmp/esp-idf}"
IDF_TOOLS_PATH="${IDF_TOOLS_PATH:-/config/.espressif}"
IDF_VENV="$IDF_PATH/python_env/idf5.3_py3.12_env"
TOOLCHAIN_DIR="$IDF_TOOLS_PATH/tools/xtensa-esp-elf/esp-13.2.0_20240530/xtensa-esp-elf/bin"
PROYECTO="$(cd "$(dirname "$0")" && pwd)"
SDKCONFIG="$PROYECTO/sdkconfig"

# ---- Colores ----
VERDE='\033[0;32m'; AMARILLO='\033[1;33m'; ROJO='\033[0;31m'; CYAN='\033[0;36m'; NORMAL='\033[0m'
ok()   { echo -e "${VERDE}[OK]${NORMAL} $1"; }
warn() { echo -e "${AMARILLO}[!]${NORMAL} $1"; }
err()  { echo -e "${ROJO}[ERROR]${NORMAL} $1"; }
info() { echo -e "${CYAN}[i]${NORMAL} $1"; }

# ==========================================================
# 1. Verificar entorno
# ==========================================================
verificar_entorno() {
    echo ""
    echo "============================================"
    echo "  Argos - Asistente de compilación"
    echo "============================================"
    echo ""

    if [ ! -d "$IDF_PATH" ]; then
        err "ESP-IDF no encontrado en $IDF_PATH"
        echo "  Clonar con:"
        echo "    git clone --recursive -b v5.3 https://github.com/espressif/esp-idf.git $IDF_PATH"
        exit 1
    fi
    ok "ESP-IDF: $IDF_PATH"

    if [ ! -f "$IDF_VENV/bin/activate" ]; then
        err "Python venv no encontrado en $IDF_VENV"
        echo "  Instalar con: bash $IDF_PATH/install.sh esp32 esp32s3 esp32s2"
        exit 1
    fi
    ok "Python venv: $IDF_VENV"

    if [ ! -f "$TOOLCHAIN_DIR/xtensa-esp32-elf-gcc" ]; then
        warn "Toolchain Xtensa no encontrado en $TOOLCHAIN_DIR"
        echo "  Puede que esté en otra ruta. Verificar con:"
        echo "    find $IDF_TOOLS_PATH/tools -name 'xtensa-esp32-elf-gcc' -type f"
    else
        ok "Toolchain: $TOOLCHAIN_DIR"
    fi
}

# ==========================================================
# 2. Activar entorno (venv + PATH)
# ==========================================================
activar_entorno() {
    source "$IDF_VENV/bin/activate"
    export IDF_PYTHON_ENV_PATH="$IDF_VENV"
    export IDF_SKIP_CHECK_SUBMODULES=1
    export ESP_IDF_VERSION=5.3
    export PATH="/config/.local/bin:$TOOLCHAIN_DIR:$PATH"
    ok "Entorno activado (Python: $(which python), CMake: $(cmake --version 2>/dev/null | head -1))"
}

# ==========================================================
# 3. Detectar target actual
# ==========================================================
target_actual() {
    if [ -f "$SDKCONFIG" ]; then
        grep -E "^CONFIG_IDF_TARGET=" "$SDKCONFIG" 2>/dev/null | cut -d= -f2 | tr -d '"' || echo "esp32"
    else
        echo "esp32"
    fi
}

# ==========================================================
# 4. Elegir target
# ==========================================================
elegir_target() {
    local actual
    actual=$(target_actual)
    echo ""
    echo "Target actual: ${CYAN}$actual${NORMAL}"
    echo "Opciones:"
    echo "  1) esp32    (ESP32 clásico, con DAC)"
    echo "  2) esp32s3  (ESP32-S3, sin DAC)"
    echo "  3) esp32s2  (ESP32-S2, con DAC)"
    echo "  4) Mantener actual ($actual)"
    echo ""
    read -r -p "Selecciona [1-4]: " opcion
    case "$opcion" in
        1) TARGET="esp32" ;;
        2) TARGET="esp32s3" ;;
        3) TARGET="esp32s2" ;;
        *) TARGET="$actual" ;;
    esac

    if [ "$TARGET" != "$actual" ]; then
        warn "Cambiando target: $actual → $TARGET"
        idf.py set-target "$TARGET" 2>&1 | tail -5
        ok "Target cambiado a $TARGET"
    else
        ok "Target: $TARGET"
    fi
}

# ==========================================================
# 5. Compilar
# ==========================================================
compilar() {
    echo ""
    info "Compilando para $(target_actual)..."
    echo ""
    if idf.py build 2>&1; then
        ok "Compilación exitosa"
        local bin
        bin=$(ls -lh build/argos.bin 2>/dev/null | awk '{print $5}')
        echo "  argos.bin: $bin"
        return 0
    else
        err "Compilación fallida"
        return 1
    fi
}

# ==========================================================
# 6. Detectar puerto serie
# ==========================================================
detectar_puerto() {
    for p in /dev/ttyUSB* /dev/ttyACM*; do
        if [ -e "$p" ]; then
            echo "$p"
            return 0
        fi
    done
    return 1
}

elegir_puerto() {
    local auto
    auto=$(detectar_puerto || true)

    if [ -n "$auto" ]; then
        echo ""
        warn "Puerto detectado: $auto"
        read -r -p "Usar este puerto? [Y/n]: " conf
        if [[ "$conf" =~ ^[nN] ]]; then
            read -r -p "Ingresa el puerto manualmente (ej: /dev/ttyUSB0): " PUERTO
        else
            PUERTO="$auto"
        fi
    else
        echo ""
        read -r -p "No se detectó puerto automáticamente. Ingresa el puerto (ej: /dev/ttyUSB0): " PUERTO
    fi

    if [ ! -e "$PUERTO" ]; then
        warn "El puerto $PUERTO no existe. Conecta el ESP32 y verifica con: ls /dev/ttyUSB*"
        return 1
    fi
    ok "Puerto: $PUERTO"
}

# ==========================================================
# 7. Flashear
# ==========================================================
flashear() {
    local puerto=$1
    echo ""
    info "Flasheando a $puerto..."
    echo ""
    if idf.py -p "$puerto" flash 2>&1; then
        ok "Flasheo completado"
    else
        err "Flasheo fallido"
        return 1
    fi
}

# ==========================================================
# 8. Monitor serie
# ==========================================================
monitor() {
    local puerto=$1
    echo ""
    info "Iniciando monitor en $puerto (Ctrl+] para salir)..."
    echo ""
    idf.py -p "$puerto" monitor || true
}

# ==========================================================
# HELP
# ==========================================================
mostrar_ayuda() {
    echo "Uso: ./argos.sh [comando]"
    echo ""
    echo "Comandos (opcionales):"
    echo "  build       Solo compilar (usa target actual)"
    echo "  flash       Compilar + flashear (detecta puerto)"
    echo "  monitor     Abrir monitor serial"
    echo "  full        Paso a paso interactivo (por defecto)"
    echo ""
    echo "Sin argumentos ejecuta el modo interactivo completo."
}

# ==========================================================
# MAIN
# ==========================================================
main() {
    local CMD="${1:-full}"

    verificar_entorno
    activar_entorno

    cd "$PROYECTO"

    case "$CMD" in
        build)
            elegir_target
            compilar
            ;;
        flash)
            elegir_target
            compilar || exit 1
            elegir_puerto || exit 1
            flashear "$PUERTO"
            ;;
        monitor)
            elegir_puerto || exit 1
            monitor "$PUERTO"
            ;;
        full|"")
            elegir_target
            compilar || exit 1

            echo ""
            read -r -p "¿Flashear al ESP32? [y/N]: " flash_resp
            if [[ "$flash_resp" =~ ^[sSyY] ]]; then
                elegir_puerto || exit 1
                flashear "$PUERTO"

                echo ""
                read -r -p "¿Abrir monitor serial? [y/N]: " mon_resp
                if [[ "$mon_resp" =~ ^[sSyY] ]]; then
                    monitor "$PUERTO"
                fi
            fi
            ;;
        -h|--help|help)
            mostrar_ayuda
            ;;
        *)
            err "Comando desconocido: $CMD"
            mostrar_ayuda
            exit 1
            ;;
    esac

    echo ""
    echo "============================================"
    echo "  Hecho."
    echo "============================================"
}

main "$@"
