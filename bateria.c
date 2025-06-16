#include "bateria.h"
#include "tipos.h" // Para BAT_W e BAT_H
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL_image.h> // Para IMG_LoadTexture
#include <unistd.h> // Para usleep

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
    // Definir foguetes e tempo de recarga baseado no nível (apenas para exemplo, a lógica de recarga é do Recarregador)
    switch (nivel) {
        case FACIL:
            bat->foguetes_max = 5; // Poucos foguetes
            bat->foguetes_atual = 1; // Começa com poucos foguetes
            // tempo de recarga é alto no recarregador
            break;
        case MEDIO:
            bat->foguetes_max = 10; // Média de foguetes
            bat->foguetes_atual = 3; // Começa com quantidade média
            // tempo de recarga é médio no recarregador
            break;
        case DIFICIL:
            bat->foguetes_max = 15; // Muitos foguetes
            bat->foguetes_atual = 5; // Começa com mais foguetes
            // tempo de recarga é baixo no recarregador
            break;
    }
}

void desenhar_bateria(SDL_Renderer* renderer, Bateria* bat) {
    if (bat->texture) {
        SDL_Rect bat_rect = { bat->pos.x, bat->pos.y, BAT_W, BAT_H };
        SDL_RenderCopy(renderer, bat->texture, NULL, &bat_rect);
    }
}

// Implementação da thread da bateria (placeholder por enquanto)
void* thread_bateria(void* arg) {
    // Bateria *bat = (Bateria *)arg;
    while (true) {
        // Lógica de movimentação, disparo, etc.
        usleep(100000);
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