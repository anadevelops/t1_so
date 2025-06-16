# Resgate de Soldados

Trabalho de Sistemas Operacionais: Sincronização de Threads com Jogo em C e SDL2

## Descrição

Um helicóptero, guiado pelo jogador, deve resgatar soldados enquanto desvia de baterias antiaéreas inimigas que disparam foguetes. O jogo utiliza múltiplas threads para simular os elementos do cenário e sincronização para garantir a integridade das ações.

## Requisitos

- **macOS** (mas pode ser adaptado para Linux/Windows)
- **SDL2** instalada  
  No macOS, instale via Homebrew:

  ```bash
  brew install sdl2
  ```

  No Linux:

  ```bash
  sudo apt-get install libsdl2-dev
  ```

  No Windows, para instalar através de linha de código é necessário instalar algum software como o MSYS2. Tendo ele instalado, execute no MSYS MinGW 64-bit:

  ```bash
  pacman -S mingw-w64-x86_64-SDL2
  ```

- **SDL2_image** instalada  
  No macOS, instale via Homebrew:

  ```bash
  brew install sdl2_image
  ```

  No Linux:

  ```bash
  sudo apt-get install libsdl2-image-dev
  ```

  No Windows:

  ```bash
  pacman -S mingw-w64-x86_64-SDL2_image
  ```

- **SDL2_ttf** instalada (para renderizar texto na tela)  
  No macOS, instale via Homebrew:

  ```bash
  brew install sdl2_ttf
  ```

## Compilação

No terminal, dentro da pasta do projeto, execute:

macOS:

```bash
gcc main.c helicoptero.c bateria.c recarregador.c -o jogo -pthread -I/opt/homebrew/include -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf
```

Linux (não sei se funciona porque não consegui testar):

```bash
gcc main.c helicoptero.c bateria.c recarregador.c -o jogo -pthread $(sdl2-config --cflags --libs) -lSDL2_image
```

Windows (não sei se funciona porque não consegui testar):

```bash
gcc main.c -o jogo.exe -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -mwindows -pthread
```

> Se estiver em outro sistema, ajuste os caminhos de acordo com a instalação da SDL2.

## Execução

No terminal, execute:

```bash
./jogo
```

## Controles

- **W**: Move o helicóptero para cima
- **A**: Move o helicóptero para a esquerda
- **S**: Move o helicóptero para baixo
- **D**: Move o helicóptero para a direita
- **Q**: Fecha o jogo

## Estrutura do Projeto

- `main.c`: Código-fonte principal do jogo
- `helicoptero.c/.h`: Módulo do Helicóptero
- `bateria.c/.h`: Módulo das Baterias Antiaéreas
- `recarregador.c/.h`: Módulo do Recarregador
- `tipos.h`: Definições de tipos comuns (e.g., `Posicao`, `NivelDificuldade`)
- `helicoptero.png`: Imagem do sprite do helicóptero
- `DejaVuSans.ttf` (ou similar): Arquivo de fonte para renderização de texto na tela
- `definicao_das_threads.md`: Documento de definição das threads e estratégias de sincronização
- `README.md`: Este arquivo

## Observações

- Não faça push do arquivo `jogo` (executável). Ele é gerado localmente ao compilar.
- O projeto está em desenvolvimento e novas funcionalidades serão adicionadas.
