CXX = g++
CXXFLAGS = -std=c++20 -O3 -pthread -Wall

.DEFAULT_GOAL := help

TARGET = raytracer
SRC = src/main.cpp
DUMMY_TARGET = dummy_threads
DUMMY_SRC = src/dummy_threads.cpp
BENCH_SCRIPT ?= benchmark.sh
PYTHON ?= python3

.PHONY: help all run-benchmark run-benchmark-raytracer run-benchmark-dummy clean

help:
	@echo "Opciones disponibles:"
	@echo "  make help                     - Muestra esta ayuda"
	@echo "  make all                      - Compila raytracer y dummy_threads"
	@echo "  make run-benchmark            - Ejecuta benchmark seleccionado y luego analisis"
	@echo "  make run-benchmark-raytracer  - Usa benchmark.sh (raytracer) + analisis"
	@echo "  make run-benchmark-dummy      - Usa benchmark_dummy.sh (dummy_threads) + analisis"
	@echo "  make clean                    - Limpia binarios y archivos .ppm"
	@echo ""
	@echo "Seleccion manual de script:"
	@echo "  make run-benchmark BENCH_SCRIPT=benchmark.sh"
	@echo "  make run-benchmark BENCH_SCRIPT=benchmark_dummy.sh"

all: $(TARGET) $(DUMMY_TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

$(DUMMY_TARGET): $(DUMMY_SRC)
	$(CXX) $(CXXFLAGS) -o $(DUMMY_TARGET) $(DUMMY_SRC)

run-benchmark: all
	bash $(BENCH_SCRIPT)
	$(PYTHON) src/analisis_resultados.py

run-benchmark-raytracer: BENCH_SCRIPT=benchmark.sh
run-benchmark-raytracer: run-benchmark

run-benchmark-dummy: BENCH_SCRIPT=benchmark_dummy.sh
run-benchmark-dummy: run-benchmark

clean:
	rm -f $(TARGET) $(DUMMY_TARGET) *.ppm