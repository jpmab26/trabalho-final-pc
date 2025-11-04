/**
 * Trabalho final de Programação Concorrente (ICP-361) 2025/2
 * Implementação de um Algoritmo Genético Concorrente
 * ------------------------------------------------------------
 * Nomes: 
 *   João Pedro Bianco - 120064499
 *   Lucas Pinheiro - 
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
}

void inicializar_populacao() {
    for (int i = 0; i < TAM_POP; i++) {
        for (int j = 0; j < n_itens; j++) {
            populacao[i].cromossomo[j] = rand() & 1;  // Mais eficiente que % 2
        }
    }
}

void* thread_calcular_fitness(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int ini, fim, fatia;
    
    fatia = args->n / args->nthreads;
    ini = args->id * fatia;
    fim = ini + fatia;
    if (args->id == (args->nthreads - 1)) {
        fim = args->n;
    }
    
    for (int i = ini; i < fim; i++) {
        // Calcular peso_total e valor_total
        int peso = 0;
        int valor = 0;
        for (int j = 0; j < n_itens; j++) {
            if (populacao[i].cromossomo[j] == 1) {
                peso += itens[j].peso;
                valor += itens[j].valor;
            }
        }
        populacao[i].peso_total = peso;
        populacao[i].valor_total = valor;
        if (peso > peso_max_mochila) {
            populacao[i].fitness = 0;
        } else {
            populacao[i].fitness = valor;
        }
    }
    
    free(args);
    pthread_exit(NULL);
}

void calcular_fitness_populacao() {
    pthread_t* tid_sistema;
    tid_sistema = (pthread_t*) malloc(sizeof(pthread_t) * nthread);
    if (tid_sistema == NULL) {
        fprintf(stderr, "ERRO: malloc() tid_sistema\n");
        exit(-1);
    }
    for (int i = 0; i < nthread; i++) {
        ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
        if (args == NULL) {
            fprintf(stderr, "ERRO: malloc() argumentos thread\n");
            exit(-1);
        }
        args->n = TAM_POP;
        args->nthreads = nthread;
        args->id = i;   
        if (pthread_create(&tid_sistema[i], NULL, thread_calcular_fitness, (void*) args)) {
            fprintf(stderr, "ERRO: pthread_create()\n");
            exit(-1);
        }
    }
    
    // Espera todas as threads terminarem
    for (int i = 0; i < nthread; i++) {
        if (pthread_join(tid_sistema[i], NULL)) {
            fprintf(stderr, "ERRO: pthread_join()\n");
            exit(-1);
        }
    }
    
    free(tid_sistema);
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

void mutacao(Individuo* individuo) {
    for (int i = 0; i < n_itens; i++) {
        float prob = (float)rand() / RAND_MAX;
        if (prob < PM) {
            individuo->cromossomo[i] = 1 - individuo->cromossomo[i];
        }
    }
}

void criar_nova_geracao() {
    // Elitismo: copiar melhor indivíduo
    int melhor_idx = encontrar_melhor_individuo();
    nova_populacao[0] = populacao[melhor_idx];
    
    // Gerar restante da população
    for (int i = 1; i < TAM_POP; i += 2) {
        int pai1_idx = selecionar_por_roleta();
        int pai2_idx = selecionar_por_roleta();
        
        if (i + 1 < TAM_POP) {
            cruzamento(&populacao[pai1_idx], &populacao[pai2_idx], 
                      &nova_populacao[i], &nova_populacao[i + 1]);
            mutacao(&nova_populacao[i]);
            mutacao(&nova_populacao[i + 1]);
        } else {
            // População ímpar: último indivíduo
            cruzamento(&populacao[pai1_idx], &populacao[pai2_idx], 
                      &nova_populacao[i], &nova_populacao[i]);
            mutacao(&nova_populacao[i]);
        }
    }
    
    memcpy(populacao, nova_populacao, TAM_POP * sizeof(Individuo));
}

// Verifica se os últimos MAX_HISTORICO valores são iguais
int verificar_criterio_parada(int* historico, int geracao, int idx_atual) {
    if (geracao < MAX_HISTORICO) {
        return 0;
    }
    
    // Pegar o valor mais recente (índice atual - 1, com wrap-around)
    int idx_ref = (idx_atual - 1 + MAX_HISTORICO) % MAX_HISTORICO;
    int valor_ref = historico[idx_ref];
    
    // Verificar se todos os últimos MAX_HISTORICO valores são iguais
    for (int i = 0; i < MAX_HISTORICO; i++) {
        if (historico[i] != valor_ref) {
            return 0;
        }
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
    int idx_historico = 0;
    
    int geracao = 0;
    
    while (1) {
        calcular_fitness_populacao();
        int melhor_idx = encontrar_melhor_individuo();
        
        // Atualizar histórico
        historico[idx_historico] = populacao[melhor_idx].valor_total;
        idx_historico = (idx_historico + 1) % MAX_HISTORICO;
        
        if (verificar_criterio_parada(historico, geracao, idx_historico)) {
            printf("Geração %d: Critério de parada atingido\n", geracao);
            break;
        }
        
        // Imprimir informações da geração
        if (geracao % 100 == 0) {
            printf("Geração %d: Melhor fitness = %d, Valor = %d, Peso = %d\n",
                   geracao, populacao[melhor_idx].fitness,
                   populacao[melhor_idx].valor_total,
                   populacao[melhor_idx].peso_total);
        }
        
        criar_nova_geracao();
        geracao++;
    }
    
    calcular_fitness_populacao();
    int melhor_idx = encontrar_melhor_individuo();
    
    // Imprimir resultado final
    printf("\n=== RESULTADO FINAL ===\n");
    printf("Número de gerações: %d\n", geracao);
    printf("Melhor fitness: %d\n", populacao[melhor_idx].fitness);
    printf("Valor total: %d\n", populacao[melhor_idx].valor_total);
    printf("Peso total: %d\n", populacao[melhor_idx].peso_total);
    printf("Itens selecionados:\n");
    for (int i = 0; i < n_itens; i++) {
        if (populacao[melhor_idx].cromossomo[i] == 1) {
            printf("  Item %d: Peso=%d, Valor=%d\n", i, itens[i].peso, itens[i].valor);
        }
    }
    
    // Liberar memória
    free(historico);
    free(populacao);
    free(nova_populacao);
    
    return 0;
}
