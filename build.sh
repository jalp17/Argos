#!/usr/bin/env bash
set -e

echo "=== Argos - Script de compilación ==="

IDF_PATH="${IDF_PATH:-/tmp/esp-idf}"
IDF_TOOLS_PATH="${IDF_TOOLS_PATH:-/config/.espressif}"
PATH="/config/.local/bin:$PATH"

if [ ! -d "$IDF_PATH" ]; then
    echo "Error: ESP-IDF no encontrado en $IDF_PATH"
    echo "Clonar con: git clone --recursive -b v5.3 https://github.com/espressif/esp-idf.git $IDF_PATH"
    exit 1
fi

echo "[1/5] Inicializando submodulos de ESP-IDF..."
cd "$IDF_PATH"
git submodule update --init --recursive 2>/dev/null &
SUBMODULE_PID=$!
for i in $(seq 1 180); do
    if ! kill -0 $SUBMODULE_PID 2>/dev/null; then break; fi
    sleep 1
done
if kill -0 $SUBMODULE_PID 2>/dev/null; then
    echo "Advertencia: submodulos no inicializados (continuando de todas formas)"
    kill $SUBMODULE_PID 2>/dev/null || true
fi

echo "[2/5] Instalando herramientas ESP-IDF..."
if [ ! -f "$IDF_PATH/tools/idf.py" ]; then
    echo "Error: ESP-IDF no instalado correctamente"
    exit 1
fi
bash "$IDF_PATH/install.sh" esp32 2>&1 | tail -5

echo "[3/5] Configurando entorno..."
source "$IDF_PATH/python_env/idf5.3_py3.12_env/bin/activate"
export IDF_PYTHON_ENV_PATH="$IDF_PATH/python_env/idf5.3_py3.12_env"
export ESP_IDF_VERSION=5.3

echo "[4/5] Configurando proyecto..."
cd "$(dirname "$0")"
idf.py set-target esp32
idf.py reconfigure

echo "[5/5] Compilando..."
idf.py build

echo ""
echo "=== Compilacion completada ==="
echo "Archivos generados en build/"
echo "Flashear con: idf.py flash"
echo "Monitor con: idf.py monitor"
