#include "helicoptero.h"
#include <stdio.h>
#include <SDL2/SDL.h>

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
    SDL_Rect dst = {heli->pos.x, heli->pos.y, HELI_W, HELI_H};
    SDL_RendererFlip flip = (heli->direcao == -1) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(renderer, heli->texture, NULL, &dst, 0, NULL, flip);
}

void mover_helicoptero(Helicoptero* heli, TeclasMovimento* teclas) {
    if (teclas->a) { heli->pos.x -= 5; heli->direcao = -1; }
    if (teclas->d) { heli->pos.x += 5; heli->direcao = 1; }
    if (teclas->w) { heli->pos.y -= 5; }
    if (teclas->s) { heli->pos.y += 5; }
}

bool helicopero_fora_da_tela(Helicoptero* heli, int largura_tela, int altura_tela) {
    return (
        heli->pos.x < 0 ||
        heli->pos.x + HELI_W > largura_tela ||
        heli->pos.y < 0 ||
        heli->pos.y + HELI_H > altura_tela
    );
} 