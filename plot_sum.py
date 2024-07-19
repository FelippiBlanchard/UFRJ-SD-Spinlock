import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

# Carregar os dados do arquivo CSV
data = pd.read_csv('performance_data.csv')

# Configurar o gráfico
plt.figure(figsize=(10, 6))

# Função para formatar os ticks do eixo x
def format_func(value, tick_number):
    if value == 0:
        return "0"
    return f'$10^{{{int(np.log10(value))}}}$'

# Função para formatar o texto da legenda
def format_vecsize(size):
    return f'$10^{{{int(np.log10(size))}}}$'

# Plotar os tempos médios de execução
for vec_size in data['VecSize'].unique():
    subset = data[data['VecSize'] == vec_size]
    plt.plot(subset['ThreadCount'], subset['AvgMultithreadTime'], 
             label=f'VecSize={format_vecsize(vec_size)} (Multithread)', marker='o')
    plt.plot(subset['ThreadCount'], subset['AvgSingleThreadTime'], 
             label=f'VecSize={format_vecsize(vec_size)} (Single Thread)', linestyle='--', marker='x')

# Configurar os eixos e título
plt.xlabel('Número de Threads')
plt.ylabel('Tempo Médio de Execução (s)')
plt.title('Tempo Médio de Execução por Número de Threads e Tamanho do Vetor')

# Usar escala logarítmica no eixo x
plt.xscale('log')

# Adicionar uma grade e legendas
plt.grid(True, which="both", ls="--", linewidth=0.5)
plt.legend()
plt.xticks(subset['ThreadCount'], [str(x) for x in subset['ThreadCount']])  # Adiciona rótulos no eixo x
plt.tight_layout()

# Salvar e exibir o gráfico
plt.savefig('performance_plot.png')
plt.show()
