#!/bin/bash

# Si el script termina por cualquier razón, volver a activar SMT
trap 'echo on | sudo tee /sys/devices/system/cpu/smt/control' EXIT

# Desactivar SMT antes de empezar
echo off | sudo tee /sys/devices/system/cpu/smt/control

EXECUTABLE="./dummy_threads"
MODELOS=("seq" "coarse" "fine" "cmp" "smt")

echo "======================================================="
echo " Demostración de ejecución de hilos (dummy_threads)"
echo "======================================================="

for modelo in "${MODELOS[@]}"; do
    if [ "$modelo" == "smt" ]; then
        echo "Activando SMT para modelo SMT..."
        echo on | sudo tee /sys/devices/system/cpu/smt/control
    fi
    
    echo ""
    echo "-------------------------------------------------------"
    echo " MODO: $modelo"
    echo "-------------------------------------------------------"
    $EXECUTABLE $modelo
done

echo ""
echo "======================================================="
echo " Demostración finalizada"
echo "======================================================="
