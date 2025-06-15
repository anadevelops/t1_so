#ifndef HELICOPTERO_H
#define HELICOPTERO_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "tipos.h"

#define HELI_W 60
#define HELI_H 30

// Estrutura do helicóptero
typedef struct {
    Posicao pos;
    bool ativo;
    SDL_Texture* texture;
} Helicoptero;

// Funções do helicóptero
bool carregar_helicoptero(SDL_Renderer* renderer, Helicoptero* heli, const char* caminho_img);
void liberar_helicoptero(Helicoptero* heli);
void desenhar_helicoptero(SDL_Renderer* renderer, Helicoptero* heli);
void mover_helicoptero(Helicoptero* heli, char direcao);
bool helicopero_fora_da_tela(Helicoptero* heli, int largura_tela, int altura_tela);

#endif // HELICOPTERO_H 