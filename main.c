#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "tipos.h"
#include "helicoptero.h"
#include "recarregador.h"
#include "bateria.h"

// Constantes do jogo
#define LARGURA 800
#define ALTURA 600
#define SOLDIERS 10
#define NUM_BATERIAS 2

// Variáveis globais do jogo
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
Helicoptero helicoptero = {
    .pos = {.x = 50, .y = ALTURA/2},
    .ativo = true,
    .texture = NULL
};

Recarregador recarregador = {
    .pos = {.x = 100, .y = 500},  // Posição fixa
    .ativo = true,
    .ocupado = false,
    .nivel = MEDIO,  // Nível médio por padrão
    .tempo_recarga = 3000,
    .tempo_atual = 0,
    .bateria_conectada = NULL,
    .texture = NULL
};

// Array de baterias
Bateria baterias[NUM_BATERIAS];

// Definição dos mutexes e variáveis de condição
pthread_mutex_t mutex_deposito = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ponte = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_render = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_recarga = PTHREAD_COND_INITIALIZER;

// Flags e estruturas de controle
bool jogo_ativo = true;
char tecla_pressionada = 0;
char motivo_derrota[128];

// Função para inicializar SDL e SDL_image
bool init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL não pôde ser inicializado! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("Erro ao inicializar SDL_image: %s\n", IMG_GetError());
        return false;
    }

    window = SDL_CreateWindow("Resgate de Soldados", 
                            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            LARGURA, ALTURA, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Janela não pôde ser criada! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer não pôde ser criado! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    // Carregar textura do helicóptero usando o módulo
    carregar_helicoptero(renderer, &helicoptero, "helicoptero.png");
    
    // Carregar recarregador (sem imagem, será um retângulo vermelho)
    carregar_recarregador(renderer, &recarregador, NULL);
    inicializar_recarregador(&recarregador, MEDIO);  // Nível médio
    
    // Inicializar baterias
    for (int i = 0; i < NUM_BATERIAS; i++) {
        baterias[i].pos.x = 200 + i * 150;  // Posições espalhadas
        baterias[i].pos.y = 520;
        carregar_bateria(renderer, &baterias[i], NULL);
        inicializar_bateria(&baterias[i], (NivelDificuldade)(i % 3));  // Cada bateria com nível diferente
    }
    
    return true;
}

// Função para limpar recursos SDL
void cleanup_sdl() {
    liberar_helicoptero(&helicoptero);
    liberar_recarregador(&recarregador);
    
    // Liberar baterias
    for (int i = 0; i < NUM_BATERIAS; i++) {
        liberar_bateria(&baterias[i]);
    }
    
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

// Função da Thread do helicóptero
void* thread_helicoptero(void* arg) {
    while (jogo_ativo) {
        pthread_mutex_lock(&mutex_render);
        mover_helicoptero(&helicoptero, tecla_pressionada);
        tecla_pressionada = 0;
        // Verificar colisão com as bordas da tela
        if (helicopero_fora_da_tela(&helicoptero, LARGURA, ALTURA)) {
            printf("O helicóptero saiu da tela! DERROTA!\n");
            jogo_ativo = false;
        }
        pthread_mutex_unlock(&mutex_render);
        usleep(50000);
    }
    return NULL;
}

// Função da Thread de renderização
void* thread_render(void* arg) {
    while (jogo_ativo) {
        pthread_mutex_lock(&mutex_render);
        // Limpar tela
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // Desenhar helicóptero
        desenhar_helicoptero(renderer, &helicoptero);
        // Desenhar recarregador fixo
        desenhar_recarregador(renderer, &recarregador);
        // Desenhar baterias
        for (int i = 0; i < NUM_BATERIAS; i++) {
            desenhar_bateria(renderer, &baterias[i]);
        }
        // Atualizar tela
        SDL_RenderPresent(renderer);
        pthread_mutex_unlock(&mutex_render);
        usleep(16666); // ~60 FPS
    }
    return NULL;
}

// Função da Thread do recarregador
void* thread_recarregador(void* arg) {
    while (jogo_ativo) {
        pthread_mutex_lock(&mutex_render);
        // Atualizar recarregador
        atualizar_recarregador(&recarregador);
        // Detectar colisão com helicóptero
        if (helicoptero.pos.x < recarregador.pos.x + REC_W &&
            helicoptero.pos.x + HELI_W > recarregador.pos.x &&
            helicoptero.pos.y < recarregador.pos.y + REC_H &&
            helicoptero.pos.y + HELI_H > recarregador.pos.y) {
            strcpy(motivo_derrota, "O helicóptero colidiu com o recarregador!");
            printf("%s\n", motivo_derrota);
            jogo_ativo = false;
        }
        // Verificar colisões com baterias
        for (int i = 0; i < NUM_BATERIAS; i++) {
            if (baterias[i].ativa && !baterias[i].conectada) {
                if (detectar_colisao_bateria_recarregador(&baterias[i], recarregador.pos, REC_W, REC_H)) {
                    if (!recarregador.ocupado) {
                        conectar_bateria(&recarregador, &baterias[i]);
                    }
                }
                // Colisão helicóptero-bateria
                if (helicoptero.pos.x < baterias[i].pos.x + BAT_W &&
                    helicoptero.pos.x + HELI_W > baterias[i].pos.x &&
                    helicoptero.pos.y < baterias[i].pos.y + BAT_H &&
                    helicoptero.pos.y + HELI_H > baterias[i].pos.y) {
                    strcpy(motivo_derrota, "O helicóptero colidiu com uma bateria!");
                    printf("%s\n", motivo_derrota);
                    jogo_ativo = false;
                }
            }
        }
        pthread_mutex_unlock(&mutex_render);
        usleep(100000); // Verificar colisão a cada 100ms
    }
    return NULL;
}

int main() {
    printf("Iniciando o jogo...\n");
    
    if (!init_sdl()) {
        printf("Falha na inicialização do SDL!\n");
        return 1;
    }
    
    printf("SDL inicializado com sucesso!\n");

    pthread_t t_heli, t_render, t_rec;

    // Criar threads
    printf("Criando threads...\n");
    pthread_create(&t_heli, NULL, thread_helicoptero, NULL);
    pthread_create(&t_render, NULL, thread_render, NULL);
    pthread_create(&t_rec, NULL, thread_recarregador, NULL);
    printf("Threads criadas!\n");

    // Loop principal (na thread principal)
    printf("Iniciando loop principal...\n");
    SDL_Event event;
    while (jogo_ativo) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                jogo_ativo = false;
            }
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_w: tecla_pressionada = 'w'; break;
                    case SDLK_s: tecla_pressionada = 's'; break;
                    case SDLK_a: tecla_pressionada = 'a'; break;
                    case SDLK_d: tecla_pressionada = 'd'; break;
                    case SDLK_q: jogo_ativo = false; break;
                }
            }
        }
        usleep(10000);
    }

    printf("Finalizando threads...\n");
    // Aguardar threads terminarem
    pthread_join(t_heli, NULL);
    pthread_join(t_render, NULL);
    pthread_join(t_rec, NULL);

    printf("Limpando recursos...\n");
    cleanup_sdl();

    pthread_mutex_destroy(&mutex_deposito);
    pthread_mutex_destroy(&mutex_ponte);
    pthread_mutex_destroy(&mutex_render);
    pthread_cond_destroy(&cond_recarga);

    printf("Jogo finalizado!\n");
    return 0;
}