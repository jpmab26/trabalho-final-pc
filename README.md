# Algoritmo Genético para o Problema da Mochila 0/1

Este projeto implementa um algoritmo genético concorrente para resolver o problema clássico da mochila 0/1. O programa utiliza a biblioteca pthread para paralelizar o cálculo de fitness da população, proporcionando ganho significativo de desempenho.

## Sobre o Problema

Dado um conjunto de n itens únicos, cada um com um valor v_i e peso w_i, e uma mochila com capacidade máxima W_max, o objetivo é determinar qual subconjunto de itens maximiza o valor total sem exceder a capacidade da mochila.

## Sobre a Implementação

O algoritmo genético implementado utiliza:

- **Representação binária**: Cada indivíduo é um array binário onde x_i = 1 indica que o item i foi selecionado;
- **Seleção por roleta**: Método probabilístico que favorece indivíduos com maior fitness;
- **Cruzamento uniforme**: Probabilidade de 95%, gerando máscara aleatória bit a bit;
- **Mutação uniforme**: Probabilidade de 1% por gene;
- **Elitismo**: O melhor indivíduo sempre passa para a próxima geração;
- **Critério de parada por convergência**: O algoritmo para quando o melhor fitness se mantém estável por várias gerações;

A paralelização é aplicada apenas no cálculo de fitness, que representa aproximadamente 60% do tempo total de execução. As threads dividem a população em fatias iguais, processando-as simultaneamente sem necessidade de sincronização complexa.

## Como Compilar

Para compilar o programa, utilize o gcc com suporte a threads:

```bash
gcc -o pc pc.c -pthread -O2 -Wall -Wextra
```

Onde:
- `-pthread`: habilita suporte para threads POSIX
- `-O2`: ativa otimizações do compilador
- `-Wall -Wextra`: exibe avisos durante a compilação

## Como Executar

O programa lê a entrada via redirecionamento de entrada padrão. O arquivo de entrada deve seguir o formato:

```
PESO_MAX_MOCHILA
N_ITENS
PESO_0 VALOR_0
PESO_1 VALOR_1
...
PESO_N-1 VALOR_N-1
NTHREADS
```

Exemplo de execução:

```bash
./pc < entrada.txt
```

O programa exibirá o progresso a cada 100 gerações e, ao final, apresentará:
- Número total de gerações executadas
- Fitness final (valor total se válido, 0 se inválido)
- Valor e peso totais da solução
- Lista detalhada dos itens selecionados

## Script de Testes Automatizados

O script `run_tests.sh` facilita a execução de múltiplos casos de teste. Ele:

1. Compila o programa automaticamente
2. Executa todos os arquivos de teste encontrados no diretório `testes/`
3. Salva os resultados no diretório `outputs/` com nomenclatura padronizada
4. Registra o tempo de execução de cada teste

Para executar todos os testes:

```bash
./run_tests.sh
```

Os arquivos de saída terão o mesmo nome dos arquivos de entrada, mas serão salvos em `outputs/output_NomeDoTeste.txt`.

## Estrutura de Diretórios

```
.
├── pc.c                  # Código-fonte do programa
├── pc                    # Executável (gerado após compilação)
├── run_tests.sh          # Script para execução automática de testes
├── entrada.txt           # Exemplo de arquivo de entrada
├── testes/               # Diretório com casos de teste
│   ├── Teste1seq.txt
│   ├── Teste1conc.txt
│   └── ...
└── outputs/              # Diretório com resultados dos testes
    ├── output_Teste1seq.txt
    ├── output_Teste1conc.txt
    └── ...

## Parâmetros Adaptativos

O programa calcula automaticamente os parâmetros do algoritmo genético com base no número de itens:

- **Tamanho da população**: 10 × n_itens (mínimo 50, máximo 5000)
- **Histórico de convergência**: 20% do tamanho da população (mínimo 50, máximo 300)

Esses valores são ajustados para garantir diversidade adequada sem desperdiçar recursos computacionais.

## Autores

- João Pedro Bianco
- Lucas Pinheiro

