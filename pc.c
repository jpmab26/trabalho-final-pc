/**
 * Trabalho final de Programação Concorrente (ICP-361) 2025/2
 * Implementação de um Algoritmo Genético Concorrente
 * ------------------------------------------------------------
 * Nomes: 
 *   João Pedro Bianco - 120064499
 *   Lucas Pinheiro - 121123995
 * -----------------------------------------------------------
 * Modelo do arquivo de entrada e uso:
 * arquivo.txt:
 * 
 * ```
 * PESO_MAX_MOCHILA
 * N_ITENS
 * PESO_0 VALOR_0
 * PESO_0 VALOR_1
 * ...
 * PESO_N VALOR_N
 * NTHREADS
 * ```
 * 
 * Uso: ./pc < arquivo.txt
 **/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
double get_time() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}
#else
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
#endif

#define MAX_ITENS 10000
#define PC 0.95
#define PM 0.01

typedef struct {
    int peso;
    int valor;
} Item;

typedef struct {
    char cromossomo[MAX_ITENS];
    int peso_total;
    int valor_total;
    int fitness;
} Individuo;

typedef struct {
    int id;
    int n; // tamanho total (TAM_POP)
    int nthreads;
} ThreadArgs;

Item itens[MAX_ITENS];
int n_itens = 0;
int peso_max_mochila = 0;
int nthread = 0;

// Parâmetros dinâmicos do algoritmo genético
int TAM_POP = 0;
int MAX_HISTORICO = 0;

Individuo* populacao = NULL;
Individuo* nova_populacao = NULL;

// Arrays globais para threads (alocados uma vez)
pthread_t* threads_globais = NULL;
int threads_alocadas = 0;

// Operador de reparo: Remove itens aleatoriamente (mais rápido que guloso)
void reparar_individuo(Individuo* ind) {
    while (ind->peso_total > peso_max_mochila) {
        int item = rand() % n_itens;
        while (ind->cromossomo[item] == 0) {
            item = rand() % n_itens;
        }
        
        ind->cromossomo[item] = 0;
        ind->peso_total -= itens[item].peso;
        ind->valor_total -= itens[item].valor;
    }
}

// Heurística gulosa para inicialização: seleciona itens por razão valor/peso
void inicializar_individuo_guloso(Individuo* ind) {
    // Calcular razão valor/peso para cada item
    typedef struct {
        int indice;
        double razao;
    } ItemRazao;
    
    ItemRazao* razoes = (ItemRazao*) malloc(n_itens * sizeof(ItemRazao));
    for (int i = 0; i < n_itens; i++) {
        razoes[i].indice = i;
        razoes[i].razao = (double)itens[i].valor / (double)itens[i].peso;
    }
    
    // Ordenar por razão decrescente (bubble sort - suficiente para este tamanho)
    for (int i = 0; i < n_itens - 1; i++) {
        for (int j = 0; j < n_itens - i - 1; j++) {
            if (razoes[j].razao < razoes[j + 1].razao) {
                ItemRazao temp = razoes[j];
                razoes[j] = razoes[j + 1];
                razoes[j + 1] = temp;
            }
        }
    }
    
    // Inicializar cromossomo vazio
    for (int i = 0; i < n_itens; i++) {
        ind->cromossomo[i] = 0;
    }
    
    // Adicionar itens gulosos com probabilidade decrescente
    int peso_atual = 0;
    for (int i = 0; i < n_itens; i++) {
        int item_idx = razoes[i].indice;
        
        // Probabilidade decrescente: 90% para melhor item, diminui gradualmente
        double prob = 0.9 - (0.6 * i / n_itens);
        
        if ((rand() % 100) < (prob * 100)) {
            if (peso_atual + itens[item_idx].peso <= peso_max_mochila) {
                ind->cromossomo[item_idx] = 1;
                peso_atual += itens[item_idx].peso;
            }
        }
    }
    
    free(razoes);
}

void ler_entrada() {
    if (scanf("%d", &peso_max_mochila) != 1) {
        fprintf(stderr, "Erro ao ler peso_max_mochila\n");
        exit(1);
    }
    
    if (scanf("%d", &n_itens) != 1) {
        fprintf(stderr, "Erro ao ler n_itens\n");
        exit(1);
    }
    
    if (n_itens <= 0 || n_itens > MAX_ITENS) {
        fprintf(stderr, "n_itens inválido: %d (deve estar entre 1 e %d)\n", n_itens, MAX_ITENS);
        exit(1);
    }
    
    for (int i = 0; i < n_itens; i++) {
        if (scanf("%d %d", &itens[i].peso, &itens[i].valor) != 2) {
            fprintf(stderr, "Erro ao ler item %d\n", i);
            exit(1);
        }
    }
    
        if (scanf("%d", &nthread) != 1) {
            fprintf(stderr, "Erro ao ler nthread\n");
            exit(1);
    }
    
    if (nthread <= 0) {
        fprintf(stderr, "nthread inválido: %d (deve ser > 0)\n", nthread);
        exit(1);
    }
}

void calcular_parametros_ag() {
    // Fórmula: TAM_POP = 10 × n_itens (escala linear com complexidade)
    // Limites: mín=50 (diversidade mínima), máx=5000 (viável computacionalmente)
    TAM_POP = 10 * n_itens;
    if (TAM_POP < 50) TAM_POP = 50;
    if (TAM_POP > 5000) TAM_POP = 5000;
    
    // Validação: nthread não pode ser maior que TAM_POP
    if (nthread > TAM_POP) {
        fprintf(stderr, "AVISO: nthread (%d) > TAM_POP (%d), ajustando nthread = TAM_POP\n", 
                nthread, TAM_POP);
        nthread = TAM_POP;
    }
    
    // Arredondar para múltiplo de nthread (otimização de balanceamento)
    TAM_POP = ((TAM_POP + nthread - 1) / nthread) * nthread;
    
    // Fórmula: MAX_HISTORICO = 20% da população
    // Limites: mín=50 (evitar parada prematura), máx=300 (evitar desperdício)
    MAX_HISTORICO = TAM_POP / 5;
    if (MAX_HISTORICO < 50) MAX_HISTORICO = 50;
    if (MAX_HISTORICO > 300) MAX_HISTORICO = 300;
    
    printf("Parâmetros calculados: n_itens=%d → TAM_POP=%d, MAX_HISTORICO=%d, nthread=%d\n", 
           n_itens, TAM_POP, MAX_HISTORICO, nthread);
}

void alocar_populacoes() {
    populacao = (Individuo*) malloc(TAM_POP * sizeof(Individuo));
    if (populacao == NULL) {
        fprintf(stderr, "ERRO: malloc() populacao\n");
        exit(-1);
    }
    
    nova_populacao = (Individuo*) malloc(TAM_POP * sizeof(Individuo));
    if (nova_populacao == NULL) {
        fprintf(stderr, "ERRO: malloc() nova_populacao\n");
        free(populacao);
        exit(-1);
    }
    
    // Alocar array de threads uma vez (será reutilizado)
    threads_globais = (pthread_t*) malloc(sizeof(pthread_t) * nthread);
    if (threads_globais == NULL) {
        fprintf(stderr, "ERRO: malloc() threads_globais\n");
        free(populacao);
        free(nova_populacao);
        exit(-1);
    }
    threads_alocadas = nthread;
}

void inicializar_populacao() {
    for (int i = 0; i < TAM_POP; i++) {
        // Estratégia mista de inicialização:
        // - 30% da população: heurística gulosa (ótimos locais)
        // - 40% da população: esparsa (diversidade com viabilidade)
        // - 30% da população: aleatória reparada (diversidade máxima)
        
        if (i < TAM_POP * 0.3) {
            inicializar_individuo_guloso(&populacao[i]);
        } else if (i < TAM_POP * 0.7) {
            for (int j = 0; j < n_itens; j++) {
                populacao[i].cromossomo[j] = (rand() % 100 < 15) ? 1 : 0;
            }
        } else {
            for (int j = 0; j < n_itens; j++) {
                populacao[i].cromossomo[j] = rand() & 1;
            }
        }
        
        populacao[i].peso_total = 0;
        populacao[i].valor_total = 0;
        for (int j = 0; j < n_itens; j++) {
            if (populacao[i].cromossomo[j]) {
                populacao[i].peso_total += itens[j].peso;
                populacao[i].valor_total += itens[j].valor;
            }
        }
        
        // GARANTIR que todos os indivíduos iniciais sejam válidos
        reparar_individuo(&populacao[i]);
    }
}

void* thread_calcular_fitness(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int ini = args->id * (args->n / args->nthreads);
    int fim = (args->id == args->nthreads - 1) ? args->n : ini + (args->n / args->nthreads);
    
    for (int i = ini; i < fim; i++) {
        int peso = 0, valor = 0;
        for (int j = 0; j < n_itens; j++) {
            if (populacao[i].cromossomo[j]) {
                peso += itens[j].peso;
                valor += itens[j].valor;
            }
        }
        populacao[i].peso_total = peso;
        populacao[i].valor_total = valor;
        
        // Fitness simples: apenas o valor (todas soluções já são válidas devido ao reparo)
        // Se por algum motivo houver inválida, penalizar fortemente
        populacao[i].fitness = (peso > peso_max_mochila) ? 1 : valor;
    }
    
    free(args);
    pthread_exit(NULL);
}

void calcular_fitness_populacao() {
    // Usar array de threads pré-alocado
    for (int i = 0; i < nthread; i++) {
        ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
        if (args == NULL) {
            fprintf(stderr, "ERRO: malloc() argumentos thread\n");
            exit(-1);
        }
        args->n = TAM_POP;
        args->nthreads = nthread;
        args->id = i;   
        if (pthread_create(&threads_globais[i], NULL, thread_calcular_fitness, (void*) args)) {
            fprintf(stderr, "ERRO: pthread_create()\n");
            exit(-1);
        }
    }
    
    // Espera todas as threads terminarem
    for (int i = 0; i < nthread; i++) {
        if (pthread_join(threads_globais[i], NULL)) {
            fprintf(stderr, "ERRO: pthread_join()\n");
            exit(-1);
        }
    }
}

int encontrar_melhor_individuo() {
    int melhor_idx = 0;
    for (int i = 1; i < TAM_POP; i++) {
        if (populacao[i].fitness > populacao[melhor_idx].fitness) {
            melhor_idx = i;
        }
    }
    return melhor_idx;
}

int selecionar_por_roleta() {
    // Calcular fitness total
    long long fitness_total = 0;
    for (int i = 0; i < TAM_POP; i++) {
        fitness_total += populacao[i].fitness;
    }
    
    if (fitness_total == 0) {
        return rand() % TAM_POP;
    }
    
    // Gerar número aleatório
    long long valor = rand() % fitness_total;
    
    // Selecionar indivíduo
    long long soma = 0;
    for (int i = 0; i < TAM_POP; i++) {
        soma += populacao[i].fitness;
        if (soma > valor) {
            return i;
        }
    }
    
    return TAM_POP - 1;
}

// Cruzamento uniforme
void cruzamento(Individuo* pai1, Individuo* pai2, Individuo* filho1, Individuo* filho2) {
    if ((float)rand() / RAND_MAX < PC) {
        for (int i = 0; i < n_itens; i++) {
            int mascara = rand() & 1;  // máscara aleatória para cada gene
            if (mascara == 1) {
                // Máscara = 1: filho1 recebe gene do pai1, filho2 recebe gene do pai2
                filho1->cromossomo[i] = pai1->cromossomo[i];
                filho2->cromossomo[i] = pai2->cromossomo[i];
            } else {
                // Máscara = 0: filho1 recebe gene do pai2, filho2 recebe gene do pai1
                filho1->cromossomo[i] = pai2->cromossomo[i];
                filho2->cromossomo[i] = pai1->cromossomo[i];
            }
        }
    } else {
        // Copiar pais diretamente
        memcpy(filho1->cromossomo, pai1->cromossomo, n_itens);
        memcpy(filho2->cromossomo, pai2->cromossomo, n_itens);
    }
}

// Mutação inteligente que tenta manter viabilidade
void mutacao(Individuo* individuo) {
    for (int i = 0; i < n_itens; i++) {
        if ((float)rand() / RAND_MAX < PM) {
            individuo->cromossomo[i] = 1 - individuo->cromossomo[i];
        }
    }
    
    individuo->peso_total = 0;
    individuo->valor_total = 0;
    for (int i = 0; i < n_itens; i++) {
        if (individuo->cromossomo[i]) {
            individuo->peso_total += itens[i].peso;
            individuo->valor_total += itens[i].valor;
        }
    }
    
    reparar_individuo(individuo);
}

void criar_nova_geracao() {
    // Elitismo: copiar melhor indivíduo
    nova_populacao[0] = populacao[encontrar_melhor_individuo()];
    
    for (int i = 1; i < TAM_POP; i += 2) {
        int pai1_idx = selecionar_por_roleta();
        int pai2_idx = selecionar_por_roleta();
        
        if (i + 1 < TAM_POP) {
            cruzamento(&populacao[pai1_idx], &populacao[pai2_idx], 
                      &nova_populacao[i], &nova_populacao[i + 1]);
            
            // Recalcular peso e valor dos filhos após cruzamento
            for (int k = i; k <= i + 1; k++) {
                nova_populacao[k].peso_total = 0;
                nova_populacao[k].valor_total = 0;
                for (int j = 0; j < n_itens; j++) {
                    if (nova_populacao[k].cromossomo[j]) {
                        nova_populacao[k].peso_total += itens[j].peso;
                        nova_populacao[k].valor_total += itens[j].valor;
                    }
                }
                reparar_individuo(&nova_populacao[k]);
            }
            
            mutacao(&nova_populacao[i]);
            mutacao(&nova_populacao[i + 1]);
        } else {
            cruzamento(&populacao[pai1_idx], &populacao[pai2_idx], 
                      &nova_populacao[i], &nova_populacao[i]);
            
            nova_populacao[i].peso_total = 0;
            nova_populacao[i].valor_total = 0;
            for (int j = 0; j < n_itens; j++) {
                if (nova_populacao[i].cromossomo[j]) {
                    nova_populacao[i].peso_total += itens[j].peso;
                    nova_populacao[i].valor_total += itens[j].valor;
                }
            }
            reparar_individuo(&nova_populacao[i]);
            mutacao(&nova_populacao[i]);
        }
    }
    
    memcpy(populacao, nova_populacao, TAM_POP * sizeof(Individuo));
}

// Verifica se os últimos MAX_HISTORICO valores são iguais
int verificar_criterio_parada(int* historico, int geracao, int idx_atual) {
    if (geracao < MAX_HISTORICO) return 0;
    
    int idx_ref = (idx_atual - 1 + MAX_HISTORICO) % MAX_HISTORICO;
    int valor_ref = historico[idx_ref];
    
    for (int i = 0; i < MAX_HISTORICO; i++) {
        if (historico[i] != valor_ref) return 0;
    }
    
    return 1;
}

int main() {
    srand(time(NULL));

    ler_entrada();
    calcular_parametros_ag();
    alocar_populacoes();
    inicializar_populacao();

    // Alocar histórico dinamicamente
    int* historico = (int*) malloc(MAX_HISTORICO * sizeof(int));
    if (historico == NULL) {
        fprintf(stderr, "ERRO: malloc() historico\n");
        exit(-1);
    }

    printf("\n=== TESTE DE DESEMPENHO ===\n");

    int nthread_backup = nthread;
    double ini, fim, tempo_seq, tempo_par, aceleracao, eficiencia;

    nthread = 1;
    inicializar_populacao();
    int idx_historico = 0;
    int geracao = 0;
    ini = get_time();
    calcular_fitness_populacao();  // Calcular fitness inicial
    while (1) {
        int melhor_idx = encontrar_melhor_individuo();
        historico[idx_historico] = populacao[melhor_idx].valor_total;
        idx_historico = (idx_historico + 1) % MAX_HISTORICO;
        if (verificar_criterio_parada(historico, geracao, idx_historico)) break;
        criar_nova_geracao();
        geracao++;
        calcular_fitness_populacao();  // Calcular fitness da nova geração
    }
    fim = get_time();
    tempo_seq = fim - ini;

    int melhor_seq = encontrar_melhor_individuo();

    printf("\n--- EXECUÇÃO SEQUENCIAL ---\n");
    printf("Threads: 1\n");
    printf("Tempo total: %.6f s\n", tempo_seq);
    printf("Gerações: %d\n", geracao);
    printf("Melhor fitness: %d\n", populacao[melhor_seq].fitness);
    printf("Valor total: %d\n", populacao[melhor_seq].valor_total);
    printf("Peso total: %d\n", populacao[melhor_seq].peso_total);
    printf("Capacidade da mochila: %d\n", peso_max_mochila);
    printf("Itens selecionados:\n");
    for (int i = 0; i < n_itens; i++) {
        if (populacao[melhor_seq].cromossomo[i] == 1) {
            printf("  Item %d: Peso=%d, Valor=%d\n", i, itens[i].peso, itens[i].valor);
        }
    }

    // --- Execução paralela (nthread = original)
    nthread = nthread_backup;
    inicializar_populacao();
    idx_historico = 0;
    geracao = 0;

    ini = get_time();
    calcular_fitness_populacao();  // Calcular fitness inicial
    while (1) {
        int melhor_idx = encontrar_melhor_individuo();
        historico[idx_historico] = populacao[melhor_idx].valor_total;
        idx_historico = (idx_historico + 1) % MAX_HISTORICO;
        if (verificar_criterio_parada(historico, geracao, idx_historico)) break;
        criar_nova_geracao();
        geracao++;
        calcular_fitness_populacao();  // Calcular fitness da nova geração
    }
    fim = get_time();
    tempo_par = fim - ini;

    int melhor_par = encontrar_melhor_individuo();

    aceleracao = tempo_seq / tempo_par;
    eficiencia = (aceleracao / nthread) * 100.0;

    printf("\n--- EXECUÇÃO PARALELA ---\n");
    printf("Threads: %d\n", nthread);
    printf("Tempo total: %.6f s\n", tempo_par);
    printf("Gerações: %d\n", geracao);
    printf("Melhor fitness: %d\n", populacao[melhor_par].fitness);
    printf("Valor total: %d\n", populacao[melhor_par].valor_total);
    printf("Peso total: %d\n", populacao[melhor_par].peso_total);
    printf("Capacidade da mochila: %d\n", peso_max_mochila);
    printf("Itens selecionados:\n");
    for (int i = 0; i < n_itens; i++) {
        if (populacao[melhor_par].cromossomo[i] == 1) {
            printf("  Item %d: Peso=%d, Valor=%d\n", i, itens[i].peso, itens[i].valor);
        }
    }

    printf("\n=== RESUMO FINAL ===\n");
    printf("Tempo sequencial: %.6f s\n", tempo_seq);
    printf("Tempo paralelo: %.6f s\n", tempo_par);
    printf("Aceleracao: %.3f\n", aceleracao);
    printf("Eficiencia: %.2f%%\n", eficiencia);

    free(historico);
    free(populacao);
    free(nova_populacao);
    free(threads_globais);
    return 0;
}
