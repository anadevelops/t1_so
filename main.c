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

//------------------------------------------
// BATERIA: Definições, inicialização e threads
//------------------------------------------

// Constantes do jogo
#define LARGURA 800
#define ALTURA 600
#define NUM_SOLDADOS 10
#define NUM_BATERIAS 2
#define SOLDADOS_PARA_VITORIA 10
#define MAX_FOGUETES 20  // Máximo de foguetes por bateria

// Constantes dos soldados
#define SOLDADO_W 20
#define SOLDADO_H 30
#define SOLDADO_ESPACAMENTO -8  // Espaçamento negativo para sobreposição

// Constantes da ponte
#define PONTE_LARGURA 150  // Aumentada de 100 para 150 (50% mais larga)
#define PONTE_X (150 + REC_W + 20)  // Posicionada logo à direita do recarregador (movida 50px para direita)
#define PONTE_ALTURA 36  // Altura proporcional à imagem ponte.png

// Variáveis globais do jogo
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* fonte_padrao = NULL;
SDL_Texture* foguete_texture = NULL;
SDL_Texture* fundo_texture = NULL;
SDL_Texture* grama_texture = NULL;
SDL_Texture* grama_esquerda_texture = NULL;
SDL_Texture* ponte_texture = NULL;
SDL_Texture* soldado_texture = NULL;
SDL_Texture* base_texture = NULL;
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

// Estrutura para representar um soldado
typedef struct {
    Posicao pos;
    bool ativo;           // Se o soldado ainda está no lado esquerdo
    bool sendo_carregado; // Se está sendo carregado pelo helicóptero
    bool resgatado;       // Se já foi resgatado (no lado direito)
} Soldado;

// Array de soldados
Soldado soldados[NUM_SOLDADOS];

// Variáveis de controle do resgate
bool helicoptero_carregando_soldado = false;
int soldado_em_transporte = -1;  // ID do soldado sendo transportado (-1 = nenhum)

// Definição dos mutexes e variáveis de condição
pthread_mutex_t mutex_recarregador = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ponte = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_render = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_recarga = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_foguetes = PTHREAD_MUTEX_INITIALIZER;

// Flags e estruturas de controle
bool jogo_ativo = true;
TeclasMovimento teclas_mov = {0};
char motivo_derrota[128] = "";
int soldados_resgatados = 0;
NivelDificuldade nivel_dificuldade_global = FACIL;  // Nível de dificuldade global do jogo

// Adicionar enum para estados do jogo
typedef enum { MENU, JOGO } GameState;
GameState estado_jogo = MENU;

// Variáveis para menu
int menu_opcao = 0; // 0: Start, 1: Dificuldade, 2: Quit
int menu_dificuldade = 0; // 0: FACIL, 1: MEDIO, 2: DIFICIL
int ultimo_score = -1;

// Declarações de funções
void* thread_bateria(void* arg);
void desenhar_foguetes(SDL_Renderer* renderer, Bateria* bat);
bool verificar_colisao_foguete_helicoptero(Foguete* foguete, Posicao heli_pos, int heli_w, int heli_h);
void alterar_nivel_dificuldade(NivelDificuldade novo_nivel);
void desenhar_soldados(SDL_Renderer* renderer);
void inicializar_soldados(void);
void verificar_colisao_helicoptero_soldados(void);
void soltar_soldado(void);
void desenhar_menu(SDL_Renderer* renderer);

//------------------------------------------
// FOGUETE: Desenho e colisão
//------------------------------------------

//------------------------------------------
// PONTE: Definições, desenho e mutex
//------------------------------------------

// Função para desenhar texto na tela
void desenhar_texto(SDL_Renderer* rend, TTF_Font* font, const char* texto, SDL_Color color, int x, int y) {
    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, texto, color);
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

// Função para desenhar os soldados do lado esquerdo do carregador
void desenhar_soldados(SDL_Renderer* renderer) {
    // Desenhar soldados ativos (lado esquerdo)
    for (int i = 0; i < NUM_SOLDADOS; i++) {
        if (soldados[i].ativo) {
            SDL_Rect soldado_rect = {
                soldados[i].pos.x,
                soldados[i].pos.y,
                SOLDADO_W,
                SOLDADO_H
            };
            
            if (soldado_texture) {
                SDL_RenderCopy(renderer, soldado_texture, NULL, &soldado_rect);
            } else {
                // Fallback: desenhar um retângulo verde se a textura não carregar
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderFillRect(renderer, &soldado_rect);
            }
        }
    }
    
    // Desenhar soldados resgatados (lado direito)
    for (int i = 0; i < NUM_SOLDADOS; i++) {
        if (soldados[i].resgatado) {
            SDL_Rect soldado_rect = {
                soldados[i].pos.x,
                soldados[i].pos.y,
                SOLDADO_W,
                SOLDADO_H
            };
            
            if (soldado_texture) {
                SDL_RenderCopy(renderer, soldado_texture, NULL, &soldado_rect);
            } else {
                // Fallback: desenhar um retângulo azul para soldados resgatados
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                SDL_RenderFillRect(renderer, &soldado_rect);
            }
        }
    }
}

// Função para inicializar os soldados
void inicializar_soldados(void) {
    int soldado_x_inicial = 10;
    int limite_direita = recarregador.pos.x - SOLDADO_W;
    int espaco_total = limite_direita - soldado_x_inicial;
    int espaco_por_soldado = espaco_total / NUM_SOLDADOS;
    int soldado_y = ALTURA - 40 - SOLDADO_H;
    for (int i = 0; i < NUM_SOLDADOS; i++) {
        soldados[i].pos.x = soldado_x_inicial + i * espaco_por_soldado;
        soldados[i].pos.y = soldado_y;
        soldados[i].ativo = true;
        soldados[i].sendo_carregado = false;
        soldados[i].resgatado = false;
    }
}

// Função para verificar colisão entre helicóptero e soldados
void verificar_colisao_helicoptero_soldados(void) {
    // Só verifica se o helicóptero não está carregando outro soldado
    if (!helicoptero_carregando_soldado) {
        for (int i = 0; i < NUM_SOLDADOS; i++) {
            if (soldados[i].ativo && !soldados[i].sendo_carregado && !soldados[i].resgatado) {
                // Verificar colisão AABB
                if (helicoptero.pos.x < soldados[i].pos.x + SOLDADO_W &&
                    helicoptero.pos.x + HELI_W > soldados[i].pos.x &&
                    helicoptero.pos.y < soldados[i].pos.y + SOLDADO_H &&
                    helicoptero.pos.y + HELI_H > soldados[i].pos.y) {
                    
                    // Helicóptero pegou o soldado
                    helicoptero_carregando_soldado = true;
                    soldado_em_transporte = i;
                    soldados[i].sendo_carregado = true;
                    soldados[i].ativo = false;  // Remove do lado esquerdo
                    
                    printf("Helicóptero pegou o soldado %d!\n", i);
                    break;
                }
            }
        }
    }
}

// Função para soltar o soldado no lado direito
void soltar_soldado(void) {
    if (helicoptero_carregando_soldado && soldado_em_transporte >= 0) {
        // Verificar se o helicóptero está no lado direito da tela
        if (helicoptero.pos.x > LARGURA - 100) {  // 100 pixels do lado direito
            // Verificar se está próximo ao chão
            if (helicoptero.pos.y > ALTURA - 100) {  // 100 pixels do chão
                // Soltar o soldado
                soldados[soldado_em_transporte].sendo_carregado = false;
                soldados[soldado_em_transporte].resgatado = true;
                soldados[soldado_em_transporte].pos.x = helicoptero.pos.x;
                soldados[soldado_em_transporte].pos.y = ALTURA - 40 - SOLDADO_H;
                
                // Incrementar contador
                soldados_resgatados++;
                
                printf("Soldado %d foi resgatado! Total: %d/%d\n", 
                       soldado_em_transporte, soldados_resgatados, SOLDADOS_PARA_VITORIA);
                
                // Resetar variáveis de controle
                helicoptero_carregando_soldado = false;
                soldado_em_transporte = -1;
            }
        }
    }
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

    fundo_texture = IMG_LoadTexture(renderer, "fundo.png");
    if (!fundo_texture) {
        printf("Erro ao carregar fundo.png: %s\n", IMG_GetError());
    }

    foguete_texture = IMG_LoadTexture(renderer, "foguete.png");
    if (!foguete_texture) {
        printf("Erro ao carregar foguete.png: %s\n", IMG_GetError());
    }

    grama_texture = IMG_LoadTexture(renderer, "grama_direita.png");
    if (!grama_texture) {
        printf("Erro ao carregar grama_direita.png: %s\n", IMG_GetError());
    }
    grama_esquerda_texture = IMG_LoadTexture(renderer, "grama_esquerda.png");
    if (!grama_esquerda_texture) {
        printf("Erro ao carregar grama_esquerda.png: %s\n", IMG_GetError());
    }

    ponte_texture = IMG_LoadTexture(renderer, "ponte.png");
    if (!ponte_texture) {
        printf("Erro ao carregar ponte.png: %s\n", IMG_GetError());
    }

    soldado_texture = IMG_LoadTexture(renderer, "soldado.png");
    if (!soldado_texture) {
        printf("Erro ao carregar soldado.png: %s\n", IMG_GetError());
    }

    base_texture = IMG_LoadTexture(renderer, "base.png");
    if (!base_texture) {
        printf("Erro ao carregar base.png: %s\n", IMG_GetError());
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
            baterias[i].pos.x = 370;  // Exatamente no fim da ponte (lado direito) - atualizado
            baterias[i].direcao = -1;  // Andando para esquerda
        }
        baterias[i].pos.y = ALTURA - 40 - BAT_H;  // Posicionadas em cima do chão
        baterias[i].id = i;  // Definir ID da bateria
        carregar_bateria(renderer, &baterias[i], "bateria.png");
        inicializar_bateria(&baterias[i], nivel_dificuldade_global);  // Usar nível global
    }
    
    // Inicializar soldados
    inicializar_soldados();
    
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
    if (foguete_texture) SDL_DestroyTexture(foguete_texture);
    if (fundo_texture) SDL_DestroyTexture(fundo_texture);
    if (grama_texture) SDL_DestroyTexture(grama_texture);
    if (grama_esquerda_texture) SDL_DestroyTexture(grama_esquerda_texture);
    if (ponte_texture) SDL_DestroyTexture(ponte_texture);
    if (soldado_texture) SDL_DestroyTexture(soldado_texture);
    if (base_texture) SDL_DestroyTexture(base_texture);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

// Função da Thread do helicóptero
void* thread_helicoptero(void* arg) {
    while (jogo_ativo) {
        mover_helicoptero(&helicoptero, &teclas_mov);
        verificar_colisao_helicoptero_soldados();
        soltar_soldado();
        if (helicopero_fora_da_tela(&helicoptero, LARGURA, ALTURA)) {
            strcpy(motivo_derrota, "O helicóptero colidiu com a parede!");
            printf("%s\n", motivo_derrota);
            jogo_ativo = false;
        }
        usleep(16000); // 16ms para 60fps
    }
    return NULL;
}

// Função da Thread de renderização
void* thread_render(void* arg) {
    char texto_soldados[50];
    SDL_Color cor_branca = {255, 255, 255, 255};
    while (1) {
        pthread_mutex_lock(&mutex_render);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (estado_jogo == MENU) {
            desenhar_menu(renderer);
        } else {
            if (fundo_texture) {
                SDL_Rect dst = {0, 0, LARGURA, ALTURA};
                SDL_RenderCopy(renderer, fundo_texture, NULL, &dst);
            }
            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
            SDL_Rect chao_esquerdo = {0, ALTURA - 40, PONTE_X, 40};
            SDL_RenderCopy(renderer, grama_esquerda_texture, NULL, &chao_esquerdo);
            if (grama_texture) {
                SDL_Rect chao_direito = {PONTE_X + PONTE_LARGURA, ALTURA - 40, LARGURA - (PONTE_X + PONTE_LARGURA), 40};
                SDL_RenderCopy(renderer, grama_texture, NULL, &chao_direito);
            } else {
                SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
                SDL_Rect chao_direito = {PONTE_X + PONTE_LARGURA, ALTURA - 40, LARGURA - (PONTE_X + PONTE_LARGURA), 40};
                SDL_RenderFillRect(renderer, &chao_direito);
            }
            // Desenhar base sobre a grama direita
            if (base_texture) {
                SDL_Rect base_rect = {LARGURA - 100, ALTURA - 40, 100, 40};
                SDL_RenderCopy(renderer, base_texture, NULL, &base_rect);
            }
            SDL_Rect ponte = {PONTE_X, ALTURA - 40, PONTE_LARGURA, PONTE_ALTURA};
            if (ponte_texture) {
                SDL_RenderCopy(renderer, ponte_texture, NULL, &ponte);
            } else {
                SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
                SDL_RenderFillRect(renderer, &ponte);
            }
            desenhar_helicoptero(renderer, &helicoptero);
            if (helicoptero_carregando_soldado) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                int center_x = helicoptero.pos.x + HELI_W / 2;
                int center_y = helicoptero.pos.y - 10;
                int radius = 8;
                SDL_Rect indicador = {center_x - radius, center_y - radius, radius * 2, radius * 2};
                SDL_RenderFillRect(renderer, &indicador);
            }
            desenhar_recarregador(renderer, &recarregador);
            for (int i = 0; i < NUM_BATERIAS; i++) {
                desenhar_bateria(renderer, &baterias[i]);
                desenhar_foguetes(renderer, &baterias[i]);
            }
            sprintf(texto_soldados, "Soldados: %d/%d", soldados_resgatados, SOLDADOS_PARA_VITORIA);
            desenhar_texto(renderer, fonte_padrao, texto_soldados, cor_branca, 10, 10);
            char texto_nivel[50];
            const char* nomes_nivel[] = {"FACIL", "MEDIO", "DIFICIL"};
            sprintf(texto_nivel, "Nivel: %s", nomes_nivel[nivel_dificuldade_global]);
            desenhar_texto(renderer, fonte_padrao, texto_nivel, cor_branca, 10, 40);
            desenhar_soldados(renderer);
        }
        SDL_RenderPresent(renderer);
        pthread_mutex_unlock(&mutex_render);
        usleep(16000);
    }
    return NULL;
}

// Função da Thread do recarregador
void* thread_recarregador(void* arg) {
    int contador_verificacao = 0;
    while (jogo_ativo) {
        atualizar_recarregador(&recarregador);
        
        // Verificação periódica para detectar baterias travadas (a cada 100 frames ~ 1.6 segundos)
        contador_verificacao++;
        if (contador_verificacao >= 100) {
            contador_verificacao = 0;
            
            // Verificar se há baterias que estão recarregando mas não estão conectadas ao recarregador
            for (int i = 0; i < NUM_BATERIAS; i++) {
                if (baterias[i].ativa && baterias[i].recarregando && !baterias[i].conectada) {
                    printf("Bateria %d detectada como travada (recarregando mas não conectada), forçando liberação\n", i);
                    baterias[i].conectada = false;
                    baterias[i].recarregando = false;
                    baterias[i].voltando_para_area_original = true;
                    baterias[i].foguetes_atual = baterias[i].foguetes_max;
                }
            }
            
            // Verificar se o recarregador está ocupado mas a bateria não está mais recarregando
            if (recarregador.ocupado && recarregador.bateria_conectada) {
                Bateria* bat = (Bateria*)recarregador.bateria_conectada;
                if (!bat->recarregando || !bat->ativa) {
                    printf("Recarregador detectado como travado, forçando liberação\n");
                    desconectar_bateria(&recarregador);
                }
            }
        }
        
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
            if (baterias[i].ativa && !baterias[i].conectada && !baterias[i].recarregando) {
                if (detectar_colisao_bateria_recarregador(&baterias[i], recarregador.pos, REC_W, REC_H)) {
                    if (!recarregador.ocupado) {
                        conectar_bateria(&recarregador, &baterias[i]);
                        printf("Bateria %d conectada ao recarregador\n", i);
                    }
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
        usleep(16000); // 16ms para 60fps
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

// Adicionar função para desenhar o menu
void desenhar_menu(SDL_Renderer* renderer) {
    SDL_Color cor_branca = {255, 255, 255, 255};
    SDL_Color cor_amarela = {255, 255, 0, 255};
    int y = 120;
    // Centralizar o título
    const char* titulo = "RESGATE DE SOLDADOS";
    int tw = 0, th = 0;
    if (fonte_padrao) {
        TTF_SizeUTF8(fonte_padrao, titulo, &tw, &th);
    }
    int x_titulo = (LARGURA - tw) / 2;
    desenhar_texto(renderer, fonte_padrao, titulo, cor_branca, x_titulo, y);
    y += 80;
    // Centralizar os botões do menu (sem setas)
    const char* start_str = "Iniciar Jogo";
    int sw = 0, sh = 0;
    TTF_SizeUTF8(fonte_padrao, start_str, &sw, &sh);
    int x_start = (LARGURA - sw) / 2;
    desenhar_texto(renderer, fonte_padrao, start_str, menu_opcao == 0 ? cor_amarela : cor_branca, x_start, y);
    y += 50;
    // Dificuldade
    const char* nomes_nivel[] = {"FACIL", "MEDIO", "DIFICIL"};
    char buf[64];
    snprintf(buf, sizeof(buf), "Dificuldade: %s", nomes_nivel[menu_dificuldade]);
    int dw = 0, dh = 0;
    TTF_SizeUTF8(fonte_padrao, buf, &dw, &dh);
    int x_dif = (LARGURA - dw) / 2;
    desenhar_texto(renderer, fonte_padrao, buf, menu_opcao == 1 ? cor_amarela : cor_branca, x_dif, y);
    y += 50;
    // Quit
    const char* quit_str = "Sair";
    int qw = 0, qh = 0;
    TTF_SizeUTF8(fonte_padrao, quit_str, &qw, &qh);
    int x_quit = (LARGURA - qw) / 2;
    desenhar_texto(renderer, fonte_padrao, quit_str, menu_opcao == 2 ? cor_amarela : cor_branca, x_quit, y);
    // Último score no canto inferior esquerdo
    if (ultimo_score >= 0) {
        char scorebuf[64];
        snprintf(scorebuf, sizeof(scorebuf), "Última pontuação: %d", ultimo_score);
        int sw = 0, sh = 0;
        if (fonte_padrao) {
            TTF_SizeUTF8(fonte_padrao, scorebuf, &sw, &sh);
        }
        desenhar_texto(renderer, fonte_padrao, scorebuf, cor_branca, 10, ALTURA - sh - 10);
    }
}

// Atualizar main loop para lidar com o menu
int main() {
    printf("Iniciando o jogo...\n");
    srand(time(NULL));
    if (!init_sdl()) {
        printf("Falha na inicialização do SDL!\n");
        return 1;
    }
    printf("SDL inicializado com sucesso!\n");

    pthread_t t_heli, t_render, t_rec;
    pthread_t t_baterias[NUM_BATERIAS];

    // Criar thread de renderização (sempre ativa)
    pthread_create(&t_render, NULL, thread_render, NULL);

    SDL_Event event;
    while (1) {
        if (estado_jogo == MENU) {
            // Menu principal
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) return 0;
                if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                        case SDLK_UP:
                        case SDLK_w:
                            menu_opcao = (menu_opcao + 2) % 3;
                            break;
                        case SDLK_DOWN:
                        case SDLK_s:
                            menu_opcao = (menu_opcao + 1) % 3;
                            break;
                        case SDLK_LEFT:
                        case SDLK_a:
                        case SDLK_RIGHT:
                        case SDLK_d:
                            if (menu_opcao == 1) {
                                if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a)
                                    menu_dificuldade = (menu_dificuldade + 2) % 3;
                                else
                                    menu_dificuldade = (menu_dificuldade + 1) % 3;
                            }
                            break;
                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                        case SDLK_SPACE:
                            if (menu_opcao == 0) { // Start
                                // Setar dificuldade
                                nivel_dificuldade_global = (NivelDificuldade)menu_dificuldade;
                                // Resetar variáveis do jogo
                                soldados_resgatados = 0;
                                helicoptero.pos.x = 50;
                                helicoptero.pos.y = ALTURA/2;
                                helicoptero.ativo = true;
                                helicoptero_carregando_soldado = false;
                                soldado_em_transporte = -1;
                                motivo_derrota[0] = '\0';
                                memset(&teclas_mov, 0, sizeof(teclas_mov));
                                inicializar_soldados();
                                for (int i = 0; i < NUM_BATERIAS; i++) {
                                    baterias[i].id = i;
                                    baterias[i].pos.y = ALTURA - 40 - BAT_H;
                                    baterias[i].conectada = false;
                                    baterias[i].na_ponte = false;
                                    baterias[i].recarregando = false;
                                    baterias[i].voltando_para_area_original = false;
                                    baterias[i].nivel = nivel_dificuldade_global;
                                    baterias[i].ativa = true;
                                    baterias[i].tempo_ultimo_disparo = 0;
                                    baterias[i].foguetes_atual = 0;
                                    baterias[i].foguetes_max = 0;
                                    baterias[i].tempo_disparo_personalizado = 0;
                                    baterias[i].velocidade = 0;
                                    baterias[i].direcao = (i == 0) ? 1 : -1;
                                    if (i == 0)
                                        baterias[i].pos.x = 600 - BAT_W - 20;
                                    else
                                        baterias[i].pos.x = 370;  // Atualizado para a nova posição da ponte
                                    inicializar_bateria(&baterias[i], nivel_dificuldade_global);
                                }
                                inicializar_recarregador(&recarregador, nivel_dificuldade_global);
                                // Resetar estado do recarregador para garantir início limpo
                                recarregador.ocupado = false;
                                recarregador.bateria_conectada = NULL;
                                recarregador.tempo_atual = 0;
                                jogo_ativo = true;
                                pthread_create(&t_heli, NULL, thread_helicoptero, NULL);
                                pthread_create(&t_rec, NULL, thread_recarregador, NULL);
                                for (int i = 0; i < NUM_BATERIAS; i++)
                                    pthread_create(&t_baterias[i], NULL, thread_bateria, &baterias[i]);
                                estado_jogo = JOGO;
                            } else if (menu_opcao == 1) {
                                // Nada, só muda dificuldade
                            } else if (menu_opcao == 2) {
                                // Quit
                                goto sair;
                            }
                            break;
                    }
                }
            }
            usleep(10000);
        } else if (estado_jogo == JOGO) {
            // Loop principal do jogo
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    jogo_ativo = false;
                    goto sair;
                }
                else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                    bool pressed = (event.type == SDL_KEYDOWN);
                    switch (event.key.keysym.sym) {
                        case SDLK_w:
                        case SDLK_UP: teclas_mov.w = pressed; break;
                        case SDLK_s:
                        case SDLK_DOWN: teclas_mov.s = pressed; break;
                        case SDLK_a:
                        case SDLK_LEFT: teclas_mov.a = pressed; break;
                        case SDLK_d:
                        case SDLK_RIGHT: teclas_mov.d = pressed; break;
                        case SDLK_e:
                            if (pressed && helicoptero_carregando_soldado) {
                                soltar_soldado();
                            }
                            break;
                        case SDLK_q: if (pressed) { jogo_ativo = false; goto sair; } break;
                    }
                }
            }
            usleep(10000);
            // Lógica de vitória/derrota
            if (!jogo_ativo || soldados_resgatados >= SOLDADOS_PARA_VITORIA || motivo_derrota[0] != '\0') {
                // Salvar score
                ultimo_score = soldados_resgatados;
                // Encerrar threads
                pthread_cancel(t_heli);
                pthread_cancel(t_rec);
                for (int i = 0; i < NUM_BATERIAS; i++) pthread_cancel(t_baterias[i]);
                estado_jogo = MENU;
            }
        }
    }
sair:
    printf("Limpando recursos...\n");
    cleanup_sdl();
    pthread_mutex_destroy(&mutex_recarregador);
    pthread_mutex_destroy(&mutex_ponte);
    pthread_mutex_destroy(&mutex_render);
    pthread_cond_destroy(&cond_recarga);
    pthread_mutex_destroy(&mutex_foguetes);
    printf("Jogo finalizado!\n");
    return 0;
}