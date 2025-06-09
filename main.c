#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h>
#include <fcntl.h>

// Definição dos mutexes e variáveis de condição
pthread_mutex_t mutex_deposito = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ponte = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_render = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_recarga = PTHREAD_COND_INITIALIZER;

// Flags e estruturas de controle
bool jogo_ativo = true;
char tecla_pressionada = 0;

// Função para configurar terminal em modo não canônico (Unix)
void configurar_terminal() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~(ICANON | ECHO); // Desabilita buffer de linha e echo
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

// Função para restaurar configuração original do terminal
void restaurar_terminal() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

// Função para verificar se há tecla pressionada (sem bloqueio)
int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    
    return 0;
}


// Função da Thread principal
void* thread_principal(void* arg) {
    while (jogo_ativo) {
        if (kbhit()) {
            tecla_pressionada = getchar();
            if (tecla_pressionada == 'q') {
                jogo_ativo = false; // Sair do jogo
            }
            // Pode incluir outras teclas: wasd para mover, espaço para atirar etc.
        }
        usleep(10000);
    }
    return NULL;
}

// Função da Thread do helicóptero
void* thread_helicoptero(void* arg) {
    while (jogo_ativo) {
        pthread_mutex_lock(&mutex_render);
        switch (tecla_pressionada) {
            case 'w':
                printf("Helicóptero sobe\n");
                break;
            case 's':
                printf("Helicóptero desce\n");
                break;
            case 'a':
                printf("Helicóptero para esquerda\n");
                break;
            case 'd':
                printf("Helicóptero para direita\n");
                break;
        }
        tecla_pressionada = 0; // Reseta tecla
        pthread_mutex_unlock(&mutex_render);
        usleep(50000);
    }
    return NULL;
}

// Função da Thread de bateria
void* thread_bateria(void* arg) {
    int id = *(int*)arg;
    while (jogo_ativo) {
        // Movimenta até o depósito

        pthread_mutex_lock(&mutex_ponte);
        // Usa ponte
        pthread_mutex_unlock(&mutex_ponte);

        pthread_mutex_lock(&mutex_deposito);
        // Solicita recarga
        pthread_cond_wait(&cond_recarga, &mutex_deposito);
        // Recebe recarga
        pthread_mutex_unlock(&mutex_deposito);

        pthread_mutex_lock(&mutex_ponte);
        // Volta pela ponte
        pthread_mutex_unlock(&mutex_ponte);

        // Movimenta de volta e atira foguetes
        usleep(100000);
    }
    return NULL;
}

// Função da Thread do recarregador
void* thread_recarregador(void* arg) {
    while (jogo_ativo) {
        pthread_mutex_lock(&mutex_deposito);
        // Aguarda uma bateria no depósito
        usleep(100000 + rand() % 400000); // Tempo aleatório de 0.1s a 0.5s
        pthread_cond_signal(&cond_recarga);
        pthread_mutex_unlock(&mutex_deposito);
    }
    return NULL;
}

// Função da Thread de foguete
void* thread_foguete(void* arg) {
    while (jogo_ativo) {
        // Move foguete na tela
        // Verifica colisões
        usleep(30000);
    }
    pthread_exit(NULL);
}

// Função opcional de renderização
void* thread_render(void* arg) {
    while (jogo_ativo) {
        pthread_mutex_lock(&mutex_render);
        // Desenha tela
        pthread_mutex_unlock(&mutex_render);
        usleep(16666); // Aproximadamente 60 FPS
    }
    return NULL;
}


int main() {
    pthread_t t_principal, t_heli;

    configurar_terminal(); // Configura terminal no início

    pthread_create(&t_principal, NULL, thread_principal, NULL);
    pthread_create(&t_heli, NULL, thread_helicoptero, NULL);

    pthread_join(t_principal, NULL);
    jogo_ativo = false;

    pthread_join(t_heli, NULL);

    restaurar_terminal(); // Restaura terminal ao final

    pthread_mutex_destroy(&mutex_deposito);
    pthread_mutex_destroy(&mutex_ponte);
    pthread_mutex_destroy(&mutex_render);
    pthread_cond_destroy(&cond_recarga);

    return 0;
}