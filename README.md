# Algoritmo Genético para o Problema da Mochila 0/1

Este projeto implementa um algoritmo genético concorrente para resolver o problema clássico da mochila 0/1. O programa utiliza a biblioteca pthread para paralelizar o cálculo de fitness da população, proporcionando ganho significativo de desempenho.

## Sobre o Problema

Dado um conjunto de n itens únicos, cada um com um valor v_i e peso w_i, e uma mochila com capacidade máxima W_max, o objetivo é determinar qual subconjunto de itens maximiza o valor total sem exceder a capacidade da mochila.

## Sobre a Implementação

O algoritmo genético implementado utiliza:

- **Representação binária compacta**: Cada indivíduo é um array de chars (1 byte) onde x_i = 1 indica que o item i foi selecionado;
- **Operador de reparo**: Garante que 100% das soluções sejam viáveis, removendo itens aleatoriamente até respeitar a capacidade;
- **Inicialização mista inteligente**: 30% heurística gulosa (razão valor/peso), 40% esparsa (baixa densidade), 30% aleatória;
- **Seleção por roleta**: Método probabilístico que favorece indivíduos com maior fitness;
- **Cruzamento uniforme**: Probabilidade de 95%, gerando máscara aleatória bit a bit;
- **Mutação uniforme**: Probabilidade de 1% por gene, seguida de reparo automático;
- **Elitismo**: O melhor indivíduo sempre passa para a próxima geração sem alterações;
- **Critério de parada por convergência**: O algoritmo para quando o melhor fitness se mantém estável por várias gerações;
- **Otimização de threads**: Array global de threads reutilizado em todas as gerações, evitando overhead de criação/destruição;

A paralelização é aplicada no cálculo de fitness através do modelo primário-secundário (Master-Worker). As threads dividem a população em fatias iguais, processando-as simultaneamente sem necessidade de sincronização complexa (apenas pthread_join). Esta estratégia foi escolhida por sua simplicidade, corretude e eficiência.

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

O programa exibirá informações sobre os parâmetros calculados e, ao final, apresentará duas execuções completas:

**Execução Sequencial (1 thread)**:
- Tempo total de execução
- Número de gerações executadas
- Fitness, valor e peso da melhor solução
- Lista detalhada dos itens selecionados

**Execução Paralela (N threads)**:
- Tempo total de execução
- Número de gerações executadas
- Fitness, valor e peso da melhor solução
- Lista detalhada dos itens selecionados

**Resumo Final**:
- Aceleração (speedup) obtida
- Eficiência da paralelização em porcentagem

Todas as soluções apresentadas são garantidamente viáveis (peso ≤ capacidade) devido ao operador de reparo.

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

- **Tamanho da população**: 10 × n_itens (mínimo 50, máximo 5000), arredondado para múltiplo de nthreads
- **Histórico de convergência**: 20% do tamanho da população (mínimo 50, máximo 300)

Esses valores são ajustados para garantir diversidade adequada sem desperdiçar recursos computacionais. O arredondamento para múltiplo de threads garante balanceamento perfeito de carga na execução paralela.

## Principais Características Técnicas

### Garantia de Viabilidade

O operador de reparo é aplicado sistematicamente após:
1. Inicialização da população
2. Operação de cruzamento
3. Operação de mutação

Isso garante que **100% das soluções em todas as gerações sejam viáveis**, eliminando completamente a possibilidade de soluções que excedam a capacidade da mochila.

### Estratégia de Inicialização

A população inicial combina três abordagens:
- **30% Gulosa**: Itens ordenados por razão valor/peso e selecionados com probabilidade decrescente
- **40% Esparsa**: Apenas 15% de chance de cada item ser incluído (favorece viabilidade natural)
- **30% Aleatória**: Seleção completamente aleatória (máxima diversidade)

### Otimização de Desempenho

- **Memória compacta**: Cromossomos usam tipo `char` (1 byte) ao invés de `int`, economizando até 75% de memória
- **Reutilização de threads**: Array global alocado uma vez e reutilizado em todas as gerações
- **Sem race conditions**: Cada thread processa índices exclusivos, eliminando necessidade de mutexes/semáforos
- **Balanceamento de carga**: População dimensionada como múltiplo exato do número de threads

## Validação e Corretude

O programa foi testado com valgrind e não apresenta memory leaks:
- Todas as alocações são devidamente liberadas
- Número de allocs = número de frees
- 0 bytes em uso ao final da execução

## Autores

- João Pedro Bianco
- Lucas Pinheiro

