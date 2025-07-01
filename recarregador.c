#include "headers/recarregador.h"
#include "headers/tipos.h" // Para REC_W e REC_H
#include <stdio.h>
#include <SDL2/SDL_image.h> // Para IMG_LoadTexture
#include "headers/bateria.h"
#include <SDL2/SDL.h>
#include <pthread.h>

// Adiciona o extern para o mutex do recarregador
extern pthread_mutex_t mutex_recarregador;

//------------------------------------------
// FUNÇÕES DE ATUALIZAÇÃO, CONEXÃO E DESCONEXÃO DO RECARREGADOR
//------------------------------------------

bool carregar_recarregador(SDL_Renderer* renderer, Recarregador* rec, const char* caminho_img) {
    if (caminho_img) {
        rec->texture = IMG_LoadTexture(renderer, caminho_img);
        if (!rec->texture) {
            printf("Erro ao carregar imagem do recarregador '%s': %s\n", caminho_img, IMG_GetError());
        }
    }
    
    // Fallback ou se caminho_img for NULL: cria um retângulo vermelho
    if (!rec->texture) {
        rec->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, REC_W, REC_H);
        SDL_SetRenderTarget(renderer, rec->texture);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, NULL);
        SDL_SetRenderTarget(renderer, NULL);
        return false; // Indica que a imagem não foi carregada (mas o fallback foi criado)
    }
    return true;
}

void liberar_recarregador(Recarregador* rec) {
    if (rec->texture) {
        SDL_DestroyTexture(rec->texture);
        rec->texture = NULL;
    }
}

void desenhar_recarregador(SDL_Renderer* renderer, Recarregador* rec) {
    if (rec->texture) {
        SDL_Rect rec_rect = { rec->pos.x, rec->pos.y, REC_W, REC_H };
        SDL_RenderCopy(renderer, rec->texture, NULL, &rec_rect);
    }
}

void inicializar_recarregador(Recarregador* rec, NivelDificuldade nivel) {
    rec->ocupado = false;
    rec->bateria_conectada = NULL;
    // Definir tempo de recarga baseado no nível (em milissegundos)
    switch (nivel) {
        case FACIL: rec->tempo_recarga = 1000; break;   // 1.0s
        case MEDIO: rec->tempo_recarga = 600; break;    // 0.6s
        case DIFICIL: rec->tempo_recarga = 200; break;  // 0.2s
    }
    rec->tempo_atual = 0;
    rec->tempo_ultimo_tick = SDL_GetTicks();
}

void atualizar_recarregador(Recarregador* rec) {
    if (rec->ocupado && rec->bateria_conectada) {
        Uint32 now = SDL_GetTicks();
        Uint32 elapsed = now - rec->tempo_ultimo_tick;
        rec->tempo_ultimo_tick = now;
        rec->tempo_atual += elapsed;
        Bateria* bat = (Bateria*)rec->bateria_conectada;
        
        // Debug: imprimir progresso a cada 200ms
        if (rec->tempo_atual % 200 < elapsed) {
            printf("[DEBUG] Recarregador: Bateria %d - Progresso: %d/%d ms (%.1f%%)\n", 
                   bat->id, rec->tempo_atual, rec->tempo_recarga, 
                   (float)rec->tempo_atual / rec->tempo_recarga * 100.0f);
        }
        
        // Verificar se a bateria ainda está válida e conectada
        if (!bat->ativa || !bat->recarregando) {
            printf("[DEBUG] Recarregador: Bateria %d inválida ou não está mais recarregando, liberando\n", bat->id);
            rec->ocupado = false;
            rec->bateria_conectada = NULL;
            rec->tempo_atual = 0;
            return;
        }
        
        if (rec->tempo_atual >= rec->tempo_recarga) {
            // Recarga completa
            printf("[DEBUG] Recarregador: Bateria %d recarregada! Liberando após %d ms\n", bat->id, rec->tempo_recarga);
            bat->conectada = false;
            bat->recarregando = false;
            bat->voltando_para_area_original = true;
            bat->foguetes_atual = bat->foguetes_max;
            // Mover a bateria para uma posição segura fora do recarregador
            bat->pos.x = rec->pos.x + REC_W + 5; // 5 pixels à direita do recarregador
            rec->ocupado = false;
            rec->bateria_conectada = NULL;
            rec->tempo_atual = 0;
        }
    }
}

void conectar_bateria(Recarregador* rec, void* bateria) {
    pthread_mutex_lock(&mutex_recarregador); // Protege início da seção crítica
    Bateria* bat = (Bateria*)bateria;
    if (!bat->ativa || bat->conectada || bat->recarregando) {
        printf("[DEBUG] Tentativa de conectar bateria %d inválida ou já conectada\n", bat->id);
        pthread_mutex_unlock(&mutex_recarregador);
        return;
    }
    if (rec->ocupado) {
        printf("[DEBUG] Recarregador já está ocupado\n");
        pthread_mutex_unlock(&mutex_recarregador);
        return;
    }
    rec->ocupado = true;
    rec->bateria_conectada = bateria;
    rec->tempo_atual = 0;
    rec->tempo_ultimo_tick = SDL_GetTicks();
    bat->conectada = true;
    bat->recarregando = true;
    printf("[DEBUG] Bateria %d conectada ao recarregador!\n", bat->id);
    pthread_mutex_unlock(&mutex_recarregador); // Libera ao final
}

void desconectar_bateria(Recarregador* rec) {
    pthread_mutex_lock(&mutex_recarregador); // Protege início da seção crítica
    if (rec->ocupado && rec->bateria_conectada) {
        Bateria* bat = (Bateria*)rec->bateria_conectada;
        printf("Forçando desconexão da bateria %d\n", bat->id);
        // Resetar estado da bateria
        bat->conectada = false;
        bat->recarregando = false;
        bat->voltando_para_area_original = true;
        bat->foguetes_atual = bat->foguetes_max;
    }
    rec->ocupado = false;
    rec->bateria_conectada = NULL;
    rec->tempo_atual = 0;
    pthread_mutex_unlock(&mutex_recarregador); // Libera ao final
}