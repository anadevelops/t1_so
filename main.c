#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// Constantes do jogo
#define LARGURA 800
#define ALTURA 600
#define SOLDIERS 10
#define HELI_W 60
#define HELI_H 60

// Estrutura para posição
typedef struct {
    int x;
    int y;
} Posicao;

// Estrutura do helicóptero
typedef struct {
    Posicao pos;
    bool ativo;
    SDL_Texture* texture;
} Helicoptero;

// Variáveis globais do jogo
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
Helicoptero helicoptero = {
    .pos = {.x = 50, .y = ALTURA/2},
    .ativo = true,
    .texture = NULL
};

// Definição dos mutexes e variáveis de condição
pthread_mutex_t mutex_deposito = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ponte = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_render = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_recarga = PTHREAD_COND_INITIALIZER;

// Flags e estruturas de controle
bool jogo_ativo = true;
char tecla_pressionada = 0;

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

    // Carregar textura do helicóptero
    helicoptero.texture = IMG_LoadTexture(renderer, "helicoptero.png");
    if (!helicoptero.texture) {
        printf("Erro ao carregar imagem do helicóptero: %s\n", IMG_GetError());
        // Se não encontrar a imagem, cria um retângulo vermelho como fallback
        helicoptero.texture = SDL_CreateTexture(renderer, 
                                          SDL_PIXELFORMAT_RGBA8888,
                                          SDL_TEXTUREACCESS_TARGET,
                                          HELI_W, HELI_H);
        SDL_SetRenderTarget(renderer, helicoptero.texture);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, NULL);
        SDL_SetRenderTarget(renderer, NULL);
    }

    return true;
}

// Função para limpar recursos SDL
void cleanup_sdl() {
    if (helicoptero.texture) SDL_DestroyTexture(helicoptero.texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

// Função da Thread do helicóptero
void* thread_helicoptero(void* arg) {
    while (jogo_ativo) {
        pthread_mutex_lock(&mutex_render);
        switch (tecla_pressionada) {
            case 'w':
                if (helicoptero.pos.y > 0) helicoptero.pos.y -= 5;
                break;
            case 's':
                if (helicoptero.pos.y < ALTURA - HELI_H) helicoptero.pos.y += 5;
                break;
            case 'a':
                if (helicoptero.pos.x > 0) helicoptero.pos.x -= 5;
                break;
            case 'd':
                if (helicoptero.pos.x < LARGURA - HELI_W) helicoptero.pos.x += 5;
                break;
        }
        tecla_pressionada = 0;
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
        SDL_Rect heli_rect = {
            helicoptero.pos.x,
            helicoptero.pos.y,
            HELI_W, HELI_H
        };
        SDL_RenderCopy(renderer, helicoptero.texture, NULL, &heli_rect);

        // Atualizar tela
        SDL_RenderPresent(renderer);
        
        pthread_mutex_unlock(&mutex_render);
        usleep(16666); // ~60 FPS
    }
    return NULL;
}

int main() {
    if (!init_sdl()) {
        return 1;
    }

    pthread_t t_heli, t_render;

    // Criar threads
    pthread_create(&t_heli, NULL, thread_helicoptero, NULL);
    pthread_create(&t_render, NULL, thread_render, NULL);

    // Loop principal (na thread principal)
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

    // Aguardar threads terminarem
    pthread_join(t_heli, NULL);
    pthread_join(t_render, NULL);

    cleanup_sdl();

    pthread_mutex_destroy(&mutex_deposito);
    pthread_mutex_destroy(&mutex_ponte);
    pthread_mutex_destroy(&mutex_render);
    pthread_cond_destroy(&cond_recarga);

    return 0;
}