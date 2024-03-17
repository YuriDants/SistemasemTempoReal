#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define ALTA 2
#define MEDIA 1
#define BAIXA 0
#define NUM_TRENS 15

#define A1 65
#define B1 66

// Definição da estrutura do Trem
typedef struct Trem {
    int id;
    int prioridade;
    char direcao_entrada;
    char direcao_saida;
} Trem;

// Definição do nó da fila
typedef struct Node {
    Trem *trem;
    struct Node* next;
} Node;

// Definição da fila
typedef struct {
    Node* frente; // Ponteiro para o primeiro elemento da fila
    Node* fundo; // Ponteiro para o último elemento da fila
    sem_t mutex; // Semáforo para garantir acesso seguro à fila
} Fila;

// Definição de uma estrutura para os argumentos da thread
typedef struct {
    Trem *trem;
    Fila *fila1;
    Fila *fila2;
} ThreadArgs; // strutura criada para poder passar mais de um argumento na criação das threads

sem_t sem_cruzamento, sem_desinfileirar, sem_finalizador;
int aux = 0; // variavel auxiliar para sabermos quando todos os trens cruzaram

// Função para criar uma nova fila vazia
Fila* criarFila() {
    Fila* fila = (Fila*)malloc(sizeof(Fila));
    fila->frente = fila->fundo = NULL;
    sem_init(&fila->mutex, 0, 1); // Inicializar o semáforo com 1 para exclusão mútua
    return fila;
}


// Função para enfileirar um trem na fila
void enfileirar(Fila* fila, Trem* trem) {
    sem_wait(&fila->mutex); // Aguardar permissão para acessar a fila

    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->trem = trem;
    newNode->next = NULL;
    if (fila->fundo == NULL) {
        fila->frente = fila->fundo = newNode;
    } else {
        fila->fundo->next = newNode;
        fila->fundo = newNode;
    }

    sem_post(&fila->mutex); // Liberar permissão após acessar a fila
}


// Função para desenfileirar um trem da fila
Trem* desinfileirar(Fila* fila) {
    sem_wait(&fila->mutex); // Aguardar permissão para acessar a fila
     sem_wait(&sem_desinfileirar);
    Node* temp = fila->frente;
    Trem* trem = temp->trem;
    fila->frente = fila->frente->next;
    if (fila->frente == NULL) {
        fila->fundo = NULL;
    }
    free(temp);

    sem_post(&fila->mutex); // Liberar permissão após acessar a fila
     //printf("desinfileirei o trem%d\n",trem->id); // mensagem de checagem
    return trem;
}

// Função para liberar a memória ocupada pela fila
void freeFila(Fila* fila) {
    sem_destroy(&fila->mutex); // Destruir o semáforo
    Node* current = fila->frente;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp->trem);
        free(temp);
    }
    free(fila);
}

// Função para comparar prioridades dos trens
int compararprioridade(Trem* trem1, Trem* trem2) {
    if (trem1->prioridade == trem2->prioridade){
        return rand()%2;
    }else   return (trem1->prioridade > trem2->prioridade) ? 1 : 0;
}


// Função para encontrar a fila com o trem de maior prioridade na frente
Fila* encontrarfila(Fila* fila1, Fila* fila2) {
    sem_wait(&fila1->mutex); // Aguardar permissão para acessar a fila
    sem_wait(&fila2->mutex); // Aguardar permissão para acessar a fila

    int prioridade_frente_fila1 = fila1->frente != NULL ? fila1->frente->trem->prioridade : -1; // se a fila for nula, adiciona -1 só para não pegar qualquer "lixo" de memória
    int prioridade_frente_fila2 = fila2->frente != NULL ? fila2->frente->trem->prioridade : -1; // se a fila for nula, adiciona -1 só para não pegar qualquer "lixo" de memória

    sem_post(&fila1->mutex); // Liberar permissão após acessar a fila
    sem_post(&fila2->mutex); // Liberar permissão após acessar a fila

    // logica comparativa das prioridades entre as filas
    if (prioridade_frente_fila1 > prioridade_frente_fila2) {
        // printf("fila 1 encontrada\n"); // mensagem de checagem
        return fila1;
    } else if(prioridade_frente_fila1 < prioridade_frente_fila2){
         //printf("fila 2 encontrada\n"); // mensagem de checagem
        return fila2;
    } else{
        if (rand()%2){
        //printf("fila 1 encontrada\n"); // mensagem de checagem
        return fila1;
        }else{
        //printf("fila 2 encontrada\n"); // mensagem de checagem
        return fila2;
        }
    }
}

// simulando o comportamento do cruzamento do trem
void cruzar(Trem *trem) {
    printf("Trem %d (Prioridade %s) está cruzando o cruzamento de %c1 para %c2.\n",
           trem->id,
           trem->prioridade == ALTA ? "ALTA" : (trem->prioridade == MEDIA ? "MEDIA" : "BAIXA"),
           trem->direcao_entrada,
           trem->direcao_saida);
    sleep(1); //Todos os trens com tamanho 1
    printf("Trem %d cruzou o cruzamento e saiu pela direção %c2.\n", trem->id, trem->direcao_saida);
     sem_post(&sem_cruzamento); // liberando o cruzamento
     sem_post(&sem_desinfileirar); // permitindo que o proximo trem possa ser desinfileirado
     aux++;
     if(aux == NUM_TRENS ){ // logica para a liberação da finalização do programa quando todos os trens tiverem cruzado
         sem_post(&sem_finalizador);
     }
}


void *simular_trem(void *arg) {
    // atribuindo todos os objetos a partir da unica estrutura passada como argumento.
    ThreadArgs *args = (ThreadArgs *)arg;
    Trem *trem = args->trem;
    Fila *fila1 = args->fila1;
    Fila *fila2 = args->fila2;

    // Etapa 1: Aproximar-se do cruzamento e ser adicionado à fila correta
    printf("Trem %d (Prioridade %s) está se aproximando do cruzamento pela entrada %c1.\n",
           trem->id,
           trem->prioridade == ALTA ? "ALTA" : (trem->prioridade == MEDIA ? "MEDIA" : "BAIXA"),
           trem->direcao_entrada);

    // Adiciona o trem à fila correspondente
    switch (trem->direcao_entrada) {
        case A1:
            enfileirar(fila1, trem);
            //printf("trem adicionado a fila 1\n");// mensagem de checagem
            break;
        case B1:
            enfileirar(fila2, trem);
            //printf("trem adicionado a fila 2\n"); // mensagem de checagem
            break;
    }

    // Etapa 2: Aguardar na fila até que seja a vez de cruzar o cruzamento
    sem_wait(&sem_cruzamento);
    // Obtem a fila do próximo trem com base na prioridade
    Fila* filaencontrada = encontrarfila(fila1, fila2);

    //printf("checagem\n"); // mensagem de checagem
    //sem_post(&sem_cruzamento); // Libera mutex
    // Etapa 3: Cruzar o cruzamento
    Trem *proximo_trem = desinfileirar(filaencontrada); // desinfileira o trem de maior prioridade de sua respectiva fila
    cruzar(proximo_trem); //Realizar o cruzamento
    free(proximo_trem); // Libera a memória alocada para o trem que cruzou o cruzamento

    return NULL;
}


int main() {
    sem_init(&sem_cruzamento, 0, 1);
    sem_init(&sem_desinfileirar, 0, 1);
    sem_init(&sem_finalizador, 0, 0);
    Fila* fila1 = criarFila();
    Fila* fila2 = criarFila();

    pthread_t threads[NUM_TRENS];

    // Criação das threads
    for (int i = 0; i < NUM_TRENS; i++) {
        Trem *trem = (Trem *)malloc(sizeof(*trem));
        trem->id = i;
        trem->prioridade = rand() % 3;
        trem->direcao_entrada = rand() % 2 == 0 ? 'A' : 'B';
        trem->direcao_saida = rand() % 2 == 0 ? 'A' : 'B';
        ThreadArgs *args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
        args->trem = trem;
        args->fila1 = fila1;
        args->fila2 = fila2;
        sleep(rand() % 3);
        pthread_create(&threads[i], NULL, simular_trem, args);
    }

    // segurar esse programa até todos os trens efetuarem o seu cruzamento
    sem_wait(&sem_finalizador);

    // Liberar memória ocupada pelas filas e pelos trens
    freeFila(fila1);
    freeFila(fila2);
    // Aguardar todas as threads terminarem
    for (int i = 0; i < NUM_TRENS; i++) {
        pthread_join(threads[i], NULL);
    }
    // Destruição de todos os semaforos
    sem_destroy(&sem_cruzamento);
    sem_destroy(&sem_desinfileirar);
    sem_destroy(&sem_finalizador);
    pthread_exit(NULL); // Encerrar a thread principal


return 0;

}
