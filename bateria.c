#include "bateria.h"
#include "tipos.h" // Para BAT_W e BAT_H
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL_image.h> // Para IMG_LoadTexture
#include <unistd.h> // Para usleep
#include <time.h> // Para rand()
#include <pthread.h> // Para pthread_mutex_t
#include <SDL2/SDL.h>
  extern SDL_Texture* foguete_texture;
extern pthread_mutex_t mutex_ponte;

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
    bat->recarregando = false;
    bat->voltando_para_area_original = false;
    bat->nivel = nivel;
    
    // Inicializar movimento com velocidade aleatória (±20% da velocidade base)
    float velocidade_base = 2.0f;  // Reduzida pela metade
    float variacao = 0.2f; // 20% de variação
    float fator_aleatorio = 0.8f + (rand() % 41) / 100.0f; // Entre 0.8 e 1.2
    bat->velocidade = (int)(velocidade_base * fator_aleatorio);
    if (bat->velocidade < 1) bat->velocidade = 1; // Velocidade mínima de 1
    
    // Inicializar tempo de disparo personalizado (±50% do tempo base)
    int tempo_disparo_base = 60; // Tempo base de disparo dobrado (metade da frequência)
    float fator_disparo = 0.5f + (rand() % 101) / 100.0f; // Entre 0.5 e 1.5
    bat->tempo_disparo_personalizado = (int)(tempo_disparo_base * fator_disparo);
    if (bat->tempo_disparo_personalizado < 20) bat->tempo_disparo_personalizado = 20; // Mínimo de 20
    
    // Inicializar foguetes
    for (int i = 0; i < MAX_FOGUETES; i++) {
        bat->foguetes[i].ativo = false;
        bat->foguetes[i].velocidade = FOGUETE_VELOCIDADE / 2;
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
    if (!bat->ativa) {
        return; // Não move se não estiver ativa
    }
    
    // Verificar se a bateria está sendo recarregada
    if (bat->recarregando) {
        // Debug: informar que está recarregando
        static int last_state = -1;
        if (last_state != bat->id) {
            printf("Bateria %d: Está recarregando (parada no recarregador)\n", bat->id);
            last_state = bat->id;
        }
        return; // Não move durante a recarga; recarregador controla o fim
    }
    
    // Verificar se a bateria está voltando para sua área original
    if (bat->voltando_para_area_original) {
        int ponte_fim = 370;  // Atualizado para a nova posição da ponte
        if (bat->pos.x < ponte_fim) {
            bat->direcao = 1;
            printf("Bateria %d voltando: indo para direita, pos=%d\n", bat->id, bat->pos.x);
        } else {
            bat->voltando_para_area_original = false;
            bat->direcao = (bat->id == 0) ? 1 : -1;
            bat->tempo_ultimo_disparo = 0;
            printf("Bateria %d chegou na área original! Direção restaurada: %d\n", bat->id, bat->direcao);
        }
    }
    // Verificar se a bateria está sem munição e precisa ir para o recarregador
    else if (bat->foguetes_atual <= 0 && !bat->na_ponte && !bat->voltando_para_area_original) {
        int recarregador_x = 100;
        static bool debug_sem_municao = false;
        if (!debug_sem_municao) {
            printf("Bateria %d ficou sem munição! Indo para recarregador em X=%d\n", bat->id, recarregador_x);
            debug_sem_municao = true;
        }
        if (bat->pos.x > recarregador_x) {
            bat->direcao = -1;
        } else {
            bat->direcao = 1;
        }
        if (bat->pos.x <= recarregador_x + 50 && bat->pos.x + BAT_W >= recarregador_x) {
            bat->recarregando = true;
            printf("Bateria %d chegou ao recarregador! Pos: %d, Rec: %d\n", bat->id, bat->pos.x, recarregador_x);
            debug_sem_municao = false;
            return; // Parar de mover
        }
    }
    
    // Mover a bateria
    bat->pos.x += bat->velocidade * bat->direcao;
    
    // Verificar se bateu nos limites da área central (400 pixels centrais)
    int limite_esquerdo = 200;  // Início da área central
    int limite_direito = 600 - BAT_W;  // Fim da área central (considerando largura da bateria)
    
    // Posições da ponte (atualizadas para a nova posição)
    int ponte_inicio = 220;  // Posição X da ponte (movida 50px para direita)
    int ponte_fim = 370;     // Fim da ponte (ponte_inicio + PONTE_LARGURA)
    
    // Verificar limites apenas se a bateria tem munição ou está voltando
    if (bat->foguetes_atual > 0 || bat->voltando_para_area_original) {
        if (bat->pos.x <= limite_esquerdo) {
            bat->pos.x = limite_esquerdo;
            if (bat->foguetes_atual > 0 && !bat->voltando_para_area_original) {
                bat->direcao = 1; // Muda para direita apenas se tiver munição e não estiver voltando
            }
            // Se não tem munição e não está voltando, pode continuar para esquerda (para o recarregador)
        } else if (bat->pos.x >= limite_direito) {
            bat->pos.x = limite_direito;
            if (bat->foguetes_atual > 0 && !bat->voltando_para_area_original) {
                bat->direcao = -1; // Muda para esquerda apenas se tiver munição e não estiver voltando
            }
        }
    }
    
    if (bat->pos.x + BAT_W >= ponte_inicio && bat->pos.x < ponte_fim) {
        // Bateria está tentando atravessar a ponte
        if (bat->foguetes_atual > 0 && !bat->voltando_para_area_original) {
            // Bateria com munição - não pode atravessar (exceto se estiver voltando)
            if (bat->direcao == 1) {
                bat->pos.x = ponte_inicio - BAT_W;
            } else {
                bat->pos.x = ponte_fim;
            }
            bat->direcao *= -1;
        } else {
            // Bateria sem munição ou voltando - pode atravessar se ponte estiver livre
            if (!bat->na_ponte) {
                pthread_mutex_lock(&mutex_ponte);
                bat->na_ponte = true;
                printf("Bateria %d atravessando a ponte\n", bat->id);
            }
        }
    } else if (bat->na_ponte && (bat->pos.x < ponte_inicio || bat->pos.x >= ponte_fim)) {
        // Bateria saiu da ponte
        bat->na_ponte = false;
        pthread_mutex_unlock(&mutex_ponte);
        printf("Bateria %d saiu da ponte\n", bat->id);
    }
}

void disparar_foguete(Bateria* bat) {
    if (!bat->ativa || bat->conectada || bat->na_ponte || bat->recarregando || bat->voltando_para_area_original || bat->foguetes_atual <= 0) {
        return; // Não dispara se não estiver ativa, conectada, na ponte, recarregando, voltando, ou sem foguetes
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
    for (int i = 0; i < MAX_FOGUETES; i++) {
        if (bat->foguetes[i].ativo) {
            SDL_Rect dst = {
                bat->foguetes[i].pos.x,
                bat->foguetes[i].pos.y,
                FOGUETE_W,
                FOGUETE_H
            };
            if (foguete_texture) {
                SDL_RenderCopy(renderer, foguete_texture, NULL, &dst);
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                SDL_RenderFillRect(renderer, &dst);
            }
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
    extern bool jogo_ativo;
    #define LARGURA 800
    #define ALTURA 600
    Bateria *bat = (Bateria *)arg;
    while (bat->ativa && jogo_ativo) {
        mover_bateria(bat, LARGURA);
        mover_foguetes(bat, ALTURA);
        bat->tempo_ultimo_disparo++;
        if (bat->foguetes_atual > 0 && bat->tempo_ultimo_disparo > bat->tempo_disparo_personalizado) {
            disparar_foguete(bat);
        }
        usleep(16000); // 16ms para 60fps
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