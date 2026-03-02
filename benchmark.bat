@echo off
setlocal enabledelayedexpansion

:: Configuracion
set EXECUTABLE=raytracer.exe
set ITERACIONES=200
set ARCHIVO_SALIDA=resultados_benchmark.csv

echo =======================================================
echo Iniciando Benchmark de Modelos de Ejecucion Multihilo
echo Total de iteraciones: %ITERACIONES%
echo Archivo de salida: %ARCHIVO_SALIDA%
echo =======================================================

:: Crear el encabezado del archivo CSV
echo Secuencial_s;GranoGrueso_s;GranoFino_s> %ARCHIVO_SALIDA%

:: Bucle for para ejecutar el programa 200 veces
for /l %%i in (1, 1, %ITERACIONES%) do (
    echo Ejecutando iteracion %%i de %ITERACIONES%...
    
    :: Ejecuta el programa y anexa la salida (>>) al archivo CSV
    %EXECUTABLE% >> %ARCHIVO_SALIDA%
)

echo =======================================================
echo Benchmark completado exitosamente.
echo Los datos han sido guardados en %ARCHIVO_SALIDA%
echo =======================================================
pause