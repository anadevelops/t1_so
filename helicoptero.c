#include "helicoptero.h"
#include <stdio.h>

bool carregar_helicoptero(SDL_Renderer* renderer, Helicoptero* heli, const char* caminho_img) {
    heli->texture = IMG_LoadTexture(renderer, caminho_img);
    if (!heli->texture) {
        printf("Erro ao carregar imagem do helicóptero: %s\n", IMG_GetError());
        // Fallback: cria um retângulo vermelho
        heli->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, HELI_W, HELI_H);
        SDL_SetRenderTarget(renderer, heli->texture);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, NULL);
        SDL_SetRenderTarget(renderer, NULL);
        return false;
    }
    return true;
}

void liberar_helicoptero(Helicoptero* heli) {
    if (heli->texture) {
        SDL_DestroyTexture(heli->texture);
        heli->texture = NULL;
    }
}

void desenhar_helicoptero(SDL_Renderer* renderer, Helicoptero* heli) {
    SDL_Rect heli_rect = { heli->pos.x, heli->pos.y, HELI_W, HELI_H };
    SDL_RenderCopy(renderer, heli->texture, NULL, &heli_rect);
}

void mover_helicoptero(Helicoptero* heli, const TeclasMovimento* teclas) {
    int dx = 0, dy = 0;
    int velocidade = 5;
    if (teclas->w) dy -= 1;
    if (teclas->s) dy += 1;
    if (teclas->a) dx -= 1;
    if (teclas->d) dx += 1;

    // Normalização de velocidade
    if (dx != 0 && dy != 0) {
        float norm = 0.7071f; // 1/sqrt(2)
        heli->pos.x += (int)((dx * norm) * velocidade);
        heli->pos.y += (int)((dy * norm) * velocidade);
    } else {
        heli->pos.x += dx * velocidade;
        heli->pos.y += dy * velocidade;
    }
}

bool helicopero_fora_da_tela(Helicoptero* heli, int largura_tela, int altura_tela) {
    return (
        heli->pos.x < 0 ||
        heli->pos.x + HELI_W > largura_tela ||
        heli->pos.y < 0 ||
        heli->pos.y + HELI_H > altura_tela
    );
} 