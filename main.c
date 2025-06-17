#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "tipos.h"
#include "helicoptero.h"
#include "recarregador.h"
#include "bateria.h"

// Constantes do jogo
#define LARGURA 800
#define ALTURA 600
#define SOLDIERS 10
#define NUM_BATERIAS 2
#define SOLDADOS_PARA_VITORIA 10
#define MAX_FOGUETES 20  // Máximo de foguetes por bateria

// Constantes da ponte
#define PONTE_LARGURA 150  // Aumentada de 100 para 150 (50% mais larga)
#define PONTE_X (100 + REC_W + 20)  // Posicionada logo à direita do recarregador

// Variáveis globais do jogo
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* fonte_padrao = NULL;
Helicoptero helicoptero = {
    .pos = {.x = 50, .y = ALTURA/2},
    .ativo = true,
    .texture = NULL
};

Recarregador recarregador = {
    .pos = {.x = 100, .y = ALTURA - 40 - REC_H},  // Posição fixa em cima do chão
    .ativo = true,
    .ocupado = false,
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
char motivo_derrota[128] = "";
int soldados_resgatados = 0;
NivelDificuldade nivel_dificuldade_global = FACIL;  // Nível de dificuldade global do jogo
int bateria_atravessando_ponte = -1;  // ID da bateria atravessando a ponte (-1 = nenhuma)

// Declarações de funções
void* thread_bateria(void* arg);
void desenhar_foguetes(SDL_Renderer* renderer, Bateria* bat);
bool verificar_colisao_foguete_helicoptero(Foguete* foguete, Posicao heli_pos, int heli_w, int heli_h);
void alterar_nivel_dificuldade(NivelDificuldade novo_nivel);

// Função para desenhar texto na tela
void desenhar_texto(SDL_Renderer* rend, TTF_Font* font, const char* texto, SDL_Color color, int x, int y) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, texto, color);
    if (!surface) {
        printf("Erro ao renderizar texto: %s\n", TTF_GetError());
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(rend, surface);
    if (!texture) {
        printf("Erro ao criar textura de texto: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect dst = {x, y, surface->w, surface->h};
    SDL_RenderCopy(rend, texture, NULL, &dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

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
    if (TTF_Init() == -1) {
        printf("Erro ao inicializar SDL_ttf: %s\n", TTF_GetError());
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

    // Carregar fonte
    fonte_padrao = TTF_OpenFont("Arial.ttf", 24); // Tente usar uma fonte comum
    if (!fonte_padrao) {
        printf("Erro ao carregar fonte Arial.ttf: %s\nTentando DejaVuSans.ttf...\n", TTF_GetError());
        fonte_padrao = TTF_OpenFont("DejaVuSans.ttf", 24);
        if (!fonte_padrao) {
            printf("Erro ao carregar fonte DejaVuSans.ttf: %s\nTextos não serão exibidos!\n", TTF_GetError());
        }
    }

    // Carregar textura do helicóptero usando o módulo
    carregar_helicoptero(renderer, &helicoptero, "helicoptero.png");
    
    // Carregar recarregador (sem imagem, será um retângulo vermelho)
    carregar_recarregador(renderer, &recarregador, "recarregador.png");
    inicializar_recarregador(&recarregador, nivel_dificuldade_global);  // Usar nível global
    
    // Inicializar baterias
    for (int i = 0; i < NUM_BATERIAS; i++) {
        if (i == 0) {
            // Primeira bateria: exatamente à esquerda do limite direito de movimento
            baterias[i].pos.x = 600 - BAT_W - 20;  // 20 pixels antes do limite direito
            baterias[i].direcao = 1;  // Andando para direita
        } else {
            // Segunda bateria: exatamente à direita da ponte
            baterias[i].pos.x = 320;  // Exatamente no fim da ponte (lado direito)
            baterias[i].direcao = -1;  // Andando para esquerda
        }
        baterias[i].pos.y = ALTURA - 40 - BAT_H;  // Posicionadas em cima do chão
        baterias[i].id = i;  // Definir ID da bateria
        carregar_bateria(renderer, &baterias[i], "bateria.png");
        inicializar_bateria(&baterias[i], nivel_dificuldade_global);  // Usar nível global
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
    
    if (fonte_padrao) TTF_CloseFont(fonte_padrao);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
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
            strcpy(motivo_derrota, "O helicóptero colidiu com a parede!");
            printf("%s\n", motivo_derrota);
            jogo_ativo = false;
        }
        pthread_mutex_unlock(&mutex_render);
        usleep(50000);
    }
    return NULL;
}

// Função da Thread de renderização
void* thread_render(void* arg) {
    char texto_soldados[50];
    SDL_Color cor_branca = {255, 255, 255, 255};

    while (jogo_ativo) {
        pthread_mutex_lock(&mutex_render);
        // Limpar tela
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Desenhar chão (duas seções cinzas separadas pela ponte)
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // Cor cinza
        
        // Seção esquerda do chão
        SDL_Rect chao_esquerdo = {0, ALTURA - 40, PONTE_X, 40};
        SDL_RenderFillRect(renderer, &chao_esquerdo);
        
        // Seção direita do chão
        SDL_Rect chao_direito = {PONTE_X + PONTE_LARGURA, ALTURA - 40, LARGURA - (PONTE_X + PONTE_LARGURA), 40};
        SDL_RenderFillRect(renderer, &chao_direito);
        
        // Desenhar ponte marrom como divisão vertical no chão
        SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // Cor marrom
        SDL_Rect ponte = {PONTE_X, ALTURA - 40, PONTE_LARGURA, 15}; // Altura reduzida, topo mantido
        SDL_RenderFillRect(renderer, &ponte);
        
        // Desenhar helicóptero
        desenhar_helicoptero(renderer, &helicoptero);
        // Desenhar recarregador fixo
        desenhar_recarregador(renderer, &recarregador);
        // Desenhar baterias
        for (int i = 0; i < NUM_BATERIAS; i++) {
            desenhar_bateria(renderer, &baterias[i]);
            // Desenhar foguetes da bateria
            desenhar_foguetes(renderer, &baterias[i]);
        }

        // (HUD) Exibir contador de soldados resgatados na tela
        sprintf(texto_soldados, "Soldados: %d/%d", soldados_resgatados, SOLDADOS_PARA_VITORIA);
        desenhar_texto(renderer, fonte_padrao, texto_soldados, cor_branca, 10, 10);
        
        // Exibir nível de dificuldade atual
        char texto_nivel[50];
        const char* nomes_nivel[] = {"FACIL", "MEDIO", "DIFICIL"};
        sprintf(texto_nivel, "Nivel: %s", nomes_nivel[nivel_dificuldade_global]);
        desenhar_texto(renderer, fonte_padrao, texto_nivel, cor_branca, 10, 40);

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
                
                // Verificar colisões entre foguetes e helicóptero
                for (int j = 0; j < MAX_FOGUETES; j++) {
                    if (baterias[i].foguetes[j].ativo) {
                        if (verificar_colisao_foguete_helicoptero(&baterias[i].foguetes[j], helicoptero.pos, HELI_W, HELI_H)) {
                            strcpy(motivo_derrota, "O helicóptero foi atingido por um foguete!");
                            printf("%s\n", motivo_derrota);
                            jogo_ativo = false;
                            break;
                        }
                    }
                }
            }
        }
        pthread_mutex_unlock(&mutex_render);
        usleep(100000); // Verificar colisão a cada 100ms
    }
    return NULL;
}

// Simulação: aumentar soldados resgatados a cada 3 segundos (placeholder)
void* simula_soldados(void* arg) {
    while (jogo_ativo && soldados_resgatados < SOLDADOS_PARA_VITORIA) {
        sleep(3);
        soldados_resgatados++;
    }
    return NULL;
}

// Função para alterar o nível de dificuldade global
void alterar_nivel_dificuldade(NivelDificuldade novo_nivel) {
    nivel_dificuldade_global = novo_nivel;
    
    // Atualizar recarregador
    inicializar_recarregador(&recarregador, novo_nivel);
    
    // Atualizar todas as baterias
    for (int i = 0; i < NUM_BATERIAS; i++) {
        inicializar_bateria(&baterias[i], novo_nivel);
    }
    
    printf("Nível de dificuldade alterado para: %d\n", novo_nivel);
}

int main() {
    printf("Iniciando o jogo...\n");
    
    // Inicializar gerador de números aleatórios
    srand(time(NULL));
    
    if (!init_sdl()) {
        printf("Falha na inicialização do SDL!\n");
        return 1;
    }
    
    printf("SDL inicializado com sucesso!\n");

    pthread_t t_heli, t_render, t_rec, t_simula;
    pthread_t t_baterias[NUM_BATERIAS]; // Threads para as baterias

    // Criar threads
    printf("Criando threads...\n");
    pthread_create(&t_heli, NULL, thread_helicoptero, NULL);
    pthread_create(&t_render, NULL, thread_render, NULL);
    pthread_create(&t_rec, NULL, thread_recarregador, NULL);
    pthread_create(&t_simula, NULL, simula_soldados, NULL);
    
    // Criar threads para as baterias
    for (int i = 0; i < NUM_BATERIAS; i++) {
        pthread_create(&t_baterias[i], NULL, thread_bateria, &baterias[i]);
    }
    
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
                    case SDLK_1: alterar_nivel_dificuldade(FACIL); break;   // Tecla 1 para nível fácil
                    case SDLK_2: alterar_nivel_dificuldade(MEDIO); break;   // Tecla 2 para nível médio
                    case SDLK_3: alterar_nivel_dificuldade(DIFICIL); break; // Tecla 3 para nível difícil
                }
            }
        }
        usleep(10000);
        // Lógica de vitória
        if (soldados_resgatados >= SOLDADOS_PARA_VITORIA) {
            strcpy(motivo_derrota, "Vitória! Todos os soldados foram resgatados!");
            printf("\n%s\n", motivo_derrota);
            jogo_ativo = false;
        }
    }

    printf("Finalizando threads...\n");
    // Aguardar threads terminarem
    pthread_join(t_heli, NULL);
    pthread_join(t_render, NULL);
    pthread_join(t_rec, NULL);
    pthread_join(t_simula, NULL);
    
    // Aguardar threads das baterias terminarem
    for (int i = 0; i < NUM_BATERIAS; i++) {
        pthread_join(t_baterias[i], NULL);
    }

    printf("Limpando recursos...\n");
    cleanup_sdl();

    pthread_mutex_destroy(&mutex_deposito);
    pthread_mutex_destroy(&mutex_ponte);
    pthread_mutex_destroy(&mutex_render);
    pthread_cond_destroy(&cond_recarga);

    printf("Jogo finalizado!\n");
    return 0;
}