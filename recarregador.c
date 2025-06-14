#include "recarregador.h"
#include <stdio.h>

bool carregar_recarregador(SDL_Renderer* renderer, Recarregador* rec, const char* caminho_img) {
    if (caminho_img) {
        rec->texture = IMG_LoadTexture(renderer, caminho_img);
        if (rec->texture) {
            return true;
        }
        printf("Erro ao carregar imagem do recarregador: %s\n", IMG_GetError());
    }
    
    // Fallback: cria um retângulo vermelho
    rec->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, REC_W, REC_H);
    if (!rec->texture) {
        printf("Erro ao criar textura do recarregador: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_SetRenderTarget(renderer, rec->texture);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, NULL);
    SDL_SetRenderTarget(renderer, NULL);
    return true;
}

void liberar_recarregador(Recarregador* rec) {
    if (rec->texture) {
        SDL_DestroyTexture(rec->texture);
        rec->texture = NULL;
    }
}

void desenhar_recarregador(SDL_Renderer* renderer, Recarregador* rec) {
    SDL_Rect rec_rect = { rec->pos.x, rec->pos.y, REC_W, REC_H };
    
    // Cor baseada no estado
    if (rec->ocupado) {
        // Verde quando ocupado (recarregando)
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    } else {
        // Vermelho quando livre
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    }
    
    SDL_RenderFillRect(renderer, &rec_rect);
    
    // Desenhar barra de progresso se estiver recarregando
    if (rec->ocupado && rec->bateria_conectada) {
        int progresso = (rec->tempo_atual * REC_W) / rec->tempo_recarga;
        SDL_Rect barra_rect = { rec->pos.x, rec->pos.y - 15, progresso, 8 };
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderFillRect(renderer, &barra_rect);
    }
}

void inicializar_recarregador(Recarregador* rec, NivelDificuldade nivel) {
    rec->ativo = true;
    rec->ocupado = false;
    rec->nivel = nivel;
    rec->bateria_conectada = NULL;
    rec->tempo_atual = 0;
    
    switch (nivel) {
        case FACIL:
            rec->tempo_recarga = 5000;  // 5 segundos (tempo alto)
            break;
        case MEDIO:
            rec->tempo_recarga = 3000;  // 3 segundos (tempo médio)
            break;
        case DIFICIL:
            rec->tempo_recarga = 1500;  // 1.5 segundos (tempo baixo)
            break;
    }
}

void atualizar_recarregador(Recarregador* rec) {
    if (!rec->ocupado || !rec->bateria_conectada) return;
    
    rec->tempo_atual += 16; // ~60 FPS
    
    if (rec->tempo_atual >= rec->tempo_recarga) {
        // Recarga completa!
        if (rec->bateria_conectada->foguetes_atual < rec->bateria_conectada->foguetes_max) {
            rec->bateria_conectada->foguetes_atual++;
            printf("Bateria recarregada! Foguetes: %d/%d\n", 
                   rec->bateria_conectada->foguetes_atual, 
                   rec->bateria_conectada->foguetes_max);
        }
        
        // Se a bateria está cheia, desconecta
        if (rec->bateria_conectada->foguetes_atual >= rec->bateria_conectada->foguetes_max) {
            desconectar_bateria(rec);
        } else {
            // Reinicia o timer para o próximo foguete
            rec->tempo_atual = 0;
        }
    }
}

bool conectar_bateria(Recarregador* rec, Bateria* bat) {
    if (rec->ocupado || bat->conectada) return false;
    
    rec->bateria_conectada = bat;
    rec->ocupado = true;
    bat->conectada = true;
    rec->tempo_atual = 0;
    
    printf("Bateria conectada ao recarregador! Nível: %d\n", rec->nivel);
    return true;
}

void desconectar_bateria(Recarregador* rec) {
    if (rec->bateria_conectada) {
        rec->bateria_conectada->conectada = false;
        rec->bateria_conectada = NULL;
    }
    rec->ocupado = false;
    rec->tempo_atual = 0;
    printf("Bateria desconectada do recarregador!\n");
}