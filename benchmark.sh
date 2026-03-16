#!/bin/bash

# Si el script termina por cualquier razón, volver a activar SMT
trap 'echo on | sudo tee /sys/devices/system/cpu/smt/control' EXIT

# Desactivar SMT antes de empezar
echo off | sudo tee /sys/devices/system/cpu/smt/control

# Configuración
ITERACIONES=200
EXECUTABLE="./raytracer"
# Eventos de hardware a medir (sin espacios después de las comas)
EVENTOS="cycles,instructions,cache-misses,stalled-cycles-frontend,stalled-cycles-backend"

# Modelos a evaluar
MODELOS=("seq" "coarse" "fine" "cmp" "smt")

echo "======================================================="
echo "Iniciando Benchmark de $ITERACIONES iteraciones por modelo"
echo "======================================================="

for modelo in "${MODELOS[@]}"; do
    # Activar SMT solo cuando el modelo sea smt
    if [ "$modelo" == "smt" ]; then
        echo "Activando SMT para modelo SMT..."
        echo on | sudo tee /sys/devices/system/cpu/smt/control
    fi

    ARCHIVO_PERF="metricas_${modelo}.csv"
    ARCHIVO_TIEMPOS="tiempos_${modelo}.csv"
    
    # Limpiar archivos anteriores
    rm -f $ARCHIVO_PERF $ARCHIVO_TIEMPOS
    
    echo "-------------------------------------------------------"
    echo "Evaluando modelo: $modelo"
    echo "-------------------------------------------------------"
    
    for i in $(seq 1 $ITERACIONES); do
        # 1. Ejecutar el programa con perf. 
        # -x ',' formatea la salida como CSV
        # -o especifica el archivo de salida
        # --append añade los datos de la nueva iteración al final del archivo
        # 2>> redirige la salida del tiempo del C++ a otro archivo
        perf stat -x ',' -e $EVENTOS -o $ARCHIVO_PERF --append $EXECUTABLE $modelo >> $ARCHIVO_TIEMPOS
        
        # Imprimir una barra de progreso simple
        echo -n "#"
    done
    echo " ¡Completado!"
done


echo "======================================================="
echo "¡Todos los benchmarks finalizaron exitosamente!"
echo "Revisa los archivos metricas_*.csv y tiempos_*.csv"
echo "======================================================="