#include "bateria.h"
#include "tipos.h" // Para BAT_W e BAT_H
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL_image.h> // Para IMG_LoadTexture
#include <unistd.h> // Para usleep
#include <time.h> // Para rand()
#include <pthread.h> // Para pthread_mutex_t

bool carregar_bateria(SDL_Renderer* renderer, Bateria* bat, const char* caminho_img) {
    if (caminho_img) {
        bat->texture = IMG_LoadTexture(renderer, caminho_img);
        if (!bat->texture) {
            printf("Erro ao carregar imagem da bateria '%s': %s\n", caminho_img, IMG_GetError());
        }
    }
    
    // Fallback ou se caminho_img for NULL: cria um retângulo azul
    if (!bat->texture) {
        bat->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, BAT_W, BAT_H);
        SDL_SetRenderTarget(renderer, bat->texture);
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(renderer, NULL);
        SDL_SetRenderTarget(renderer, NULL);
        return false; // Indica que a imagem não foi carregada (mas o fallback foi criado)
    }
    return true;
}

void liberar_bateria(Bateria* bat) {
    if (bat->texture) {
        SDL_DestroyTexture(bat->texture);
        bat->texture = NULL;
    }
}

void inicializar_bateria(Bateria* bat, NivelDificuldade nivel) {
    bat->ativa = true;
    bat->conectada = false;
    bat->na_ponte = false;
    bat->nivel = nivel;
    
    // Inicializar movimento aleatório
    bat->velocidade = 1 + (rand() % 3); // Velocidade entre 1 e 3
    bat->direcao = (rand() % 2) ? 1 : -1; // Direção aleatória (1 ou -1)
    
    // Inicializar foguetes
    for (int i = 0; i < MAX_FOGUETES; i++) {
        bat->foguetes[i].ativo = false;
        bat->foguetes[i].velocidade = FOGUETE_VELOCIDADE;
        bat->foguetes[i].direcao = -1; // Foguetes sempre disparam para cima
    }
    bat->tempo_ultimo_disparo = 0;
    
    // Definir foguetes e tempo de recarga baseado no nível
    switch (nivel) {
        case FACIL:
            bat->foguetes_max = 5; // Poucos foguetes
            bat->foguetes_atual = 5; // Começa com todos os foguetes
            break;
        case MEDIO:
            bat->foguetes_max = 10; // Média de foguetes
            bat->foguetes_atual = 10; // Começa com todos os foguetes
            break;
        case DIFICIL:
            bat->foguetes_max = 15; // Muitos foguetes
            bat->foguetes_atual = 15; // Começa com todos os foguetes
            break;
    }
}

void desenhar_bateria(SDL_Renderer* renderer, Bateria* bat) {
    if (bat->texture) {
        SDL_Rect bat_rect = { bat->pos.x, bat->pos.y, BAT_W, BAT_H };
        SDL_RenderCopy(renderer, bat->texture, NULL, &bat_rect);
    }
}

void mover_bateria(Bateria* bat, int largura_tela) {
    if (!bat->ativa || bat->conectada || bat->na_ponte) {
        return; // Não move se não estiver ativa ou estiver conectada/na ponte
    }
    
    // Mover a bateria
    bat->pos.x += bat->velocidade * bat->direcao;
    
    // Verificar se bateu nas bordas da tela
    if (bat->pos.x <= 0) {
        bat->pos.x = 0;
        bat->direcao = 1; // Muda para direita
    } else if (bat->pos.x + BAT_W >= largura_tela) {
        bat->pos.x = largura_tela - BAT_W;
        bat->direcao = -1; // Muda para esquerda
    }
    
    // Ocasionalmente mudar direção aleatoriamente (10% de chance)
    if (rand() % 100 < 10) {
        bat->direcao *= -1;
    }
}

void disparar_foguete(Bateria* bat) {
    if (!bat->ativa || bat->conectada || bat->na_ponte || bat->foguetes_atual <= 0) {
        return; // Não dispara se não estiver ativa, conectada, ou sem foguetes
    }
    
    // Encontrar um slot livre para o foguete
    for (int i = 0; i < MAX_FOGUETES; i++) {
        if (!bat->foguetes[i].ativo) {
            // Posicionar foguete no centro da bateria
            bat->foguetes[i].pos.x = bat->pos.x + BAT_W / 2 - FOGUETE_W / 2;
            bat->foguetes[i].pos.y = bat->pos.y;
            bat->foguetes[i].ativo = true;
            bat->foguetes[i].velocidade = FOGUETE_VELOCIDADE;
            bat->foguetes[i].direcao = -1; // Para cima
            
            bat->foguetes_atual--; // Reduzir contador de foguetes
            bat->tempo_ultimo_disparo = 0; // Resetar timer
            break;
        }
    }
}

void mover_foguetes(Bateria* bat, int altura_tela) {
    for (int i = 0; i < MAX_FOGUETES; i++) {
        if (bat->foguetes[i].ativo) {
            // Mover foguete
            bat->foguetes[i].pos.y += bat->foguetes[i].velocidade * bat->foguetes[i].direcao;
            
            // Verificar se saiu da tela
            if (bat->foguetes[i].pos.y < 0 || bat->foguetes[i].pos.y > altura_tela) {
                bat->foguetes[i].ativo = false;
            }
        }
    }
}

void desenhar_foguetes(SDL_Renderer* renderer, Bateria* bat) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Amarelo para foguetes
    
    for (int i = 0; i < MAX_FOGUETES; i++) {
        if (bat->foguetes[i].ativo) {
            SDL_Rect foguete_rect = {
                bat->foguetes[i].pos.x,
                bat->foguetes[i].pos.y,
                FOGUETE_W,
                FOGUETE_H
            };
            SDL_RenderFillRect(renderer, &foguete_rect);
        }
    }
}

bool verificar_colisao_foguete_helicoptero(Foguete* foguete, Posicao heli_pos, int heli_w, int heli_h) {
    return (foguete->pos.x < heli_pos.x + heli_w &&
            foguete->pos.x + FOGUETE_W > heli_pos.x &&
            foguete->pos.y < heli_pos.y + heli_h &&
            foguete->pos.y + FOGUETE_H > heli_pos.y);
}

// Implementação da thread da bateria
void* thread_bateria(void* arg) {
    Bateria *bat = (Bateria *)arg;
    
    // Declaração externa dos mutexes e constantes (definidos em main.c)
    extern pthread_mutex_t mutex_render;
    extern bool jogo_ativo;
    #define LARGURA 800  // Constante da largura da tela
    #define ALTURA 600   // Constante da altura da tela
    
    while (bat->ativa && jogo_ativo) {
        pthread_mutex_lock(&mutex_render);
        
        // Mover a bateria
        mover_bateria(bat, LARGURA);
        
        // Mover foguetes existentes
        mover_foguetes(bat, ALTURA);
        
        // Tentar disparar foguete (se tiver foguetes e tempo suficiente)
        bat->tempo_ultimo_disparo++;
        if (bat->foguetes_atual > 0 && bat->tempo_ultimo_disparo > 30) { // Dispara a cada ~1.5 segundos
            disparar_foguete(bat);
        }
        
        pthread_mutex_unlock(&mutex_render);
        
        // Lógica de disparo, etc.
        usleep(50000); // 50ms de delay
    }
    return NULL;
}

bool detectar_colisao_bateria_recarregador(Bateria* bat, Posicao rec_pos, int rec_w, int rec_h) {
    // Lógica de colisão de retângulo AABB
    return (bat->pos.x < rec_pos.x + rec_w &&
            bat->pos.x + BAT_W > rec_pos.x &&
            bat->pos.y < rec_pos.y + rec_h &&
            bat->pos.y + BAT_H > rec_pos.y);
} 