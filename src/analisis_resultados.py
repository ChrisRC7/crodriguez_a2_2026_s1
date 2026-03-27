import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import scipy.stats as st
import os

# Configuración
modelos = ['seq', 'coarse', 'fine', 'smt', 'cmp', 'smt2']
etiquetas = ['Secuencial', 'Grano Grueso', 'Grano Fino', 'SMT', 'CMP', 'SMT 2']
resultados = []
datos_tiempos = {}
bench_label = os.getenv('BENCH_LABEL', '').strip()

if bench_label:
    tabla_output = f"tabla_resultados {bench_label}.png"
    grafico_output = f"grafico_tiempos {bench_label}.png"
else:
    tabla_output = 'tabla_resultados.png'
    grafico_output = 'grafico_tiempos.png'

print("======================================================")
print(" Procesando Benchmark Multihilo y Hardware (perf)")
print("======================================================\n")

for mod, etiq in zip(modelos, etiquetas):
    # --- 1. PROCESAR TIEMPOS ---
    t_file = f'tiempos_{mod}.csv'
    if os.path.exists(t_file):
        df_t = pd.read_csv(t_file, header=None, names=['Tiempo'])
        tiempos = df_t['Tiempo'].values
        datos_tiempos[etiq] = tiempos
        media_t = np.mean(tiempos)
        std_t = np.std(tiempos, ddof=1)
    else:
        media_t, std_t = np.nan, np.nan

    # --- 2. PROCESAR MÉTRICAS DE HARDWARE ---
    m_file = f'metricas_{mod}.csv'
    ciclos, instrucciones, misses = [], [], []
    
    if os.path.exists(m_file):
        with open(m_file, 'r') as f:
            for line in f:
                # Dividir y limpiar cada campo de la línea CSV
                parts = [p.strip() for p in line.split(',')]
                # Validar que la línea tenga el formato correcto de perf stat CSV
                if len(parts) < 3:
                    continue
                if not parts[0].isdigit():
                    continue
                valor = int(parts[0])
                evento = parts[2].strip()

                # Validar que el evento sea uno de los esperados antes de usarlo
                if evento == 'cycles':
                    ciclos.append(valor)
                elif evento == 'instructions':
                    instrucciones.append(valor)
                    misses.append(valor)
    # Promedios de hardware
    media_ciclos = np.mean(ciclos) if ciclos else np.nan
    media_inst = np.mean(instrucciones) if instrucciones else np.nan
    media_misses = np.mean(misses) if misses else np.nan
    # IPC (Instrucciones por ciclo)
    ipc = media_inst / media_ciclos if (not np.isnan(media_ciclos) and media_ciclos > 0) else np.nan

    # Guardar en la tabla
    resultados.append({
        'Modelo': etiq.replace('\n', ' '),
        'Tiempo (s)': media_t,
        'Desv.Est (s)': std_t,
        'Ciclos (Millones)': media_ciclos / 1e6,
        'Instrucciones (Millones)': media_inst / 1e6,
        'IPC': ipc,
        'Cache Misses': media_misses
    })

# --- 3. GENERAR TABLA FINAL ---
df = pd.DataFrame(resultados)

# Calcular Speedup (Tomando el secuencial como base)
t_seq = df.loc[df['Modelo'] == 'Secuencial', 'Tiempo (s)'].values[0]
df.insert(2, 'Speedup', t_seq / df['Tiempo (s)'])

# Formatear para imprimir bonito en consola
pd.set_option('display.float_format', lambda x: '%.4f' % x)
pd.set_option('display.max_columns', None)
pd.set_option('display.width', 1000)

print("TABLA DE RESULTADOS:")
print("-" * 110)
print(df.to_string(index=False))
print("-" * 110)

# Guardar la tabla también como imagen
fig_tabla, ax_tabla = plt.subplots(figsize=(13, 3.5))
ax_tabla.axis('off')

# Formatear solo columnas numéricas para evitar errores con columnas de texto.
df_img = df.copy()
for col in df_img.columns:
    if pd.api.types.is_numeric_dtype(df_img[col]):
        df_img[col] = df_img[col].map(lambda x: f"{x:.4f}" if pd.notna(x) else "NaN")

tabla = ax_tabla.table(
    cellText=df_img.values,
    colLabels=df.columns,
    cellLoc='center',
    loc='center'
)

tabla.auto_set_font_size(False)
tabla.set_fontsize(9)
tabla.scale(1.0, 1.4)

plt.tight_layout()
fig_tabla.savefig(tabla_output, dpi=300, bbox_inches='tight')
plt.close(fig_tabla)
print(f"Tabla guardada como '{tabla_output}'.")

# --- 4. GRAFICAR ---
if datos_tiempos:
    plt.figure(figsize=(10, 6))
    
    # Boxplot de Tiempos
    bp = plt.boxplot(datos_tiempos.values(), labels=datos_tiempos.keys(), patch_artist=True)
    colores = ['#ff9999', '#66b3ff', '#99ff99', '#ffcc99', '#c2c2f0', "#f01717"]
    for patch, color in zip(bp['boxes'], colores):
        patch.set_facecolor(color)
        
    plt.title('Distribución de Tiempos de Ejecución - Ray Tracing', fontsize=14)
    plt.ylabel('Tiempo (Segundos)', fontsize=12)
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    
    plt.savefig(grafico_output, dpi=300, bbox_inches='tight')
    print(f"\nGráfico guardado como '{grafico_output}'.")

    # Mostrar el gráfico en pantalla
    #plt.show()