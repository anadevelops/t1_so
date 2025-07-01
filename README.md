# Resgate de Soldados

Este é um jogo 2D onde o objetivo é resgatar soldados utilizando um helicóptero. O jogo utiliza a biblioteca SDL2 para gráficos, além de SDL2_image e SDL2_ttf para manipulação de imagens e fontes.

## Objetivo do Jogo
Você controla um helicóptero que deve resgatar soldados do lado esquerdo da tela e levá-los em segurança para o lado direito, evitando foguetes disparados por baterias inimigas. As baterias precisam ser recarregadas periodicamente criando oportunidades para o jogador, exigindo estratégia e atenção.

## Controles
- **W, A, S, D ou Setas**: Movimentam o helicóptero
- **ESC**: Sai do jogo

## Compilação
O projeto depende das bibliotecas SDL2, SDL2_image e SDL2_ttf. Certifique-se de tê-las instaladas no seu sistema.

### Linux
```sh
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
gcc main.c bateria.c helicoptero.c recarregador.c -I. -lSDL2 -lSDL2_image -lSDL2_ttf -o jogo -pthread
```

### macOS
Instale as dependências via Homebrew:
```sh
brew install sdl2 sdl2_image sdl2_ttf
gcc main.c bateria.c helicoptero.c recarregador.c -I. -lSDL2 -lSDL2_image -lSDL2_ttf -o jogo -pthread
```

### Windows (MinGW/MSYS2)
Certifique-se de ter o MinGW/MSYS2 instalado e as bibliotecas SDL2, SDL2_image e SDL2_ttf disponíveis.
No terminal MSYS2:
```sh
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_ttf
gcc main.c bateria.c helicoptero.c recarregador.c -I. -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o jogo.exe -pthread -mwindows
```

**Obs:**
- No Windows, coloque as DLLs das bibliotecas SDL2, SDL2_image e SDL2_ttf na mesma pasta do executável, se necessário.
- O executável será gerado na raiz do projeto (`jogo` ou `jogo.exe`).

## Créditos
- Imagens e fontes: pasta `assets/`