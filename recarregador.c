#include "recarregador.h"
#include "tipos.h" // Para REC_W e REC_H
#include <stdio.h>
#include <SDL2/SDL_image.h> // Para IMG_LoadTexture

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
    // Definir tempo de recarga baseado no nível
    switch (nivel) {
        case FACIL: rec->tempo_recarga = 500; break; // 0.5s
        case MEDIO: rec->tempo_recarga = 300; break; // 0.3s
        case DIFICIL: rec->tempo_recarga = 100; break; // 0.1s
    }
    rec->tempo_atual = 0;
}

void atualizar_recarregador(Recarregador* rec) {
    if (rec->ocupado) {
        rec->tempo_atual += 100; // Incrementa tempo a cada 100ms de verificação (do thread_recarregador)
        if (rec->tempo_atual >= rec->tempo_recarga) {
            // Recarga completa
            // Aqui você chamaria uma função para notificar a bateria que ela foi recarregada
            // Por enquanto, apenas libera o recarregador
            // desconectar_bateria(rec);
            rec->ocupado = false;
            rec->bateria_conectada = NULL;
            rec->tempo_atual = 0;
            printf("Recarregador liberado!\n");
        }
    }
}

void conectar_bateria(Recarregador* rec, void* bateria) {
    rec->ocupado = true;
    rec->bateria_conectada = bateria;
    rec->tempo_atual = 0;
    printf("Bateria conectada ao recarregador!\n");
}

// void desconectar_bateria(Recarregador* rec) {
//     rec->ocupado = false;
//     rec->bateria_conectada = NULL;
//     rec->tempo_atual = 0;
// }