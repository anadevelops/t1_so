#include "bateria.h"
#include <stdio.h>

bool carregar_bateria(SDL_Renderer* renderer, Bateria* bat, const char* caminho_img) {
    if (caminho_img) {
        bat->texture = IMG_LoadTexture(renderer, caminho_img);
        if (bat->texture) {
            return true;
        }
        printf("Erro ao carregar imagem da bateria: %s\n", IMG_GetError());
    }
    
    // Fallback: cria um retângulo azul
    bat->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, BAT_W, BAT_H);
    if (!bat->texture) {
        printf("Erro ao criar textura da bateria: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_SetRenderTarget(renderer, bat->texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderFillRect(renderer, NULL);
    SDL_SetRenderTarget(renderer, NULL);
    return true;
}

void liberar_bateria(Bateria* bat) {
    if (bat->texture) {
        SDL_DestroyTexture(bat->texture);
        bat->texture = NULL;
    }
}

void desenhar_bateria(SDL_Renderer* renderer, Bateria* bat) {
    if (!bat->ativa) return;
    
    SDL_Rect bat_rect = { bat->pos.x, bat->pos.y, BAT_W, BAT_H };
    SDL_RenderCopy(renderer, bat->texture, NULL, &bat_rect);
    
    // Desenhar indicador de foguetes (barra verde)
    int barra_largura = (bat->foguetes_atual * BAT_W) / bat->foguetes_max;
    SDL_Rect barra_rect = { bat->pos.x, bat->pos.y - 10, barra_largura, 5 };
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &barra_rect);
}

void inicializar_bateria(Bateria* bat, NivelDificuldade nivel) {
    bat->ativa = true;
    bat->conectada = false;
    
    switch (nivel) {
        case FACIL:
            bat->foguetes_max = 3;
            bat->foguetes_atual = 1;  // Começa com poucos foguetes
            break;
        case MEDIO:
            bat->foguetes_max = 6;
            bat->foguetes_atual = 2;  // Começa com quantidade média
            break;
        case DIFICIL:
            bat->foguetes_max = 10;
            bat->foguetes_atual = 3;  // Começa com mais foguetes
            break;
    }
}

bool detectar_colisao_bateria_recarregador(Bateria* bat, Posicao pos_rec, int rec_w, int rec_h) {
    return (bat->pos.x < pos_rec.x + rec_w &&
            bat->pos.x + BAT_W > pos_rec.x &&
            bat->pos.y < pos_rec.y + rec_h &&
            bat->pos.y + BAT_H > pos_rec.y);
} 