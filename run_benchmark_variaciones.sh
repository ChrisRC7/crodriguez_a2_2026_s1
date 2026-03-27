#!/bin/bash

set -euo pipefail

CORES_LIST=(2 4)
QUANTUM_LIST=(2 20 200)
RESULTS_DIR="resultados_variaciones"

mkdir -p "$RESULTS_DIR"

echo "======================================================="
echo "Automatizacion de variaciones: make run-benchmark-raytracer"
echo "Casos: (2,2) (2,20) (2,200) (4,2) (4,20) (4,200)"
echo "======================================================="

for cores in "${CORES_LIST[@]}"; do
    for quantum in "${QUANTUM_LIST[@]}"; do
        case_tag="${cores}_nucleos_${quantum}_quantum"
        case_label="${cores} nucleos ${quantum} quantum"
        tabla_case_file="tabla_resultados ${case_label}.png"
        grafico_case_file="grafico_tiempos ${case_label}.png"

        echo ""
        echo "-------------------------------------------------------"
        echo "Ejecutando caso: $case_tag"
        echo "-------------------------------------------------------"

        BENCH_CORES="$cores" BENCH_QUANTUM="$quantum" BENCH_LABEL="$case_label" make run-benchmark-raytracer

        if [ -f "$tabla_case_file" ]; then
            cp "$tabla_case_file" "$RESULTS_DIR/tabla_resultados_${case_tag}.png"
        fi

        if [ -f "$grafico_case_file" ]; then
            cp "$grafico_case_file" "$RESULTS_DIR/grafico_tiempos_${case_tag}.png"
        fi

        for f in metricas_*.csv tiempos_*.csv; do
            if [ -f "$f" ]; then
                base_name="${f%.csv}"
                cp "$f" "$RESULTS_DIR/${base_name}_${case_tag}.csv"
            fi
        done

        echo "Archivos guardados para: $case_tag"
    done
done

echo ""
echo "======================================================="
echo "Variaciones finalizadas. Resultados en: $RESULTS_DIR"
echo "======================================================="
