#!/bin/bash

EXECUTABLE="./raytracer"
ITERACIONES=200
ARCHIVO_SALIDA="resultados_benchmark.csv"

echo "======================================================="
echo "Iniciando Benchmark de Modelos de Ejecucion Multihilo"
echo "Total de iteraciones: $ITERACIONES"
echo "Archivo de salida: $ARCHIVO_SALIDA"
echo "======================================================="

echo "Secuencial_s,GranoGrueso_s,GranoFino_s,SMT_s" > "$ARCHIVO_SALIDA"

for ((i=1; i<=ITERACIONES; i++)); do
    echo "Ejecutando iteracion $i de $ITERACIONES..."
    $EXECUTABLE >> "$ARCHIVO_SALIDA"
done

echo "======================================================="
echo "Benchmark completado exitosamente."
echo "Los datos han sido guardados en $ARCHIVO_SALIDA"
echo "======================================================="