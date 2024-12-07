#include <SDL2/SDL.h>
#include <SDL2/SDL_messagebox.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <ostream>

constexpr int ROW_SIZE = 3;
constexpr int COL_SIZE = 3;
constexpr size_t WINDOW_WIDTH = 500;
constexpr size_t WINDOW_HEIGHT = 500;

enum SquareType {
  X_SQUARE,
  O_SQUARE,
  FREE_SQUARE,
};

enum Players {
  PLAYER_X,
  PLAYER_O,
};

enum ButtonAction {
  RESTART,
  QUIT,
  NONE,
};

enum GameState {
  X_WIN,
  O_WIN,
  DRAW,
  PROGRESS,
};

class Board {
public:
  Board();
  void printBoard();
  void startGui();

private:
  enum SquareType board[COL_SIZE][ROW_SIZE];
  enum Players currPlayer;
  enum GameState gameState;
  GameState getGameState();
  void move(size_t row, size_t col);
  void restart();
};

Board::Board() {
  currPlayer = PLAYER_X;
  for (int i = 0; i < COL_SIZE; ++i) {
    for (int j = 0; j < ROW_SIZE; ++j) {
      board[i][j] = FREE_SQUARE;
    }
  }
  gameState = PROGRESS;
}

void Board::restart() {
  currPlayer = PLAYER_X;
  for (int i = 0; i < COL_SIZE; ++i) {
    for (int j = 0; j < ROW_SIZE; ++j) {
      board[i][j] = FREE_SQUARE;
    }
  }
  gameState = PROGRESS;
}

const char *gameStateToString(GameState state) {
  switch (state) {
  case X_WIN:
    return "X has won!";
  case O_WIN:
    return "O has won!";
  case DRAW:
    return "You've drawn";
  default:
    return "Unknown State";
  }
}
void Board::startGui() {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error("Failed to initialise SDL: " + std::string(TTF_GetError()));
  }

  if (TTF_Init() < 0) {
    SDL_Quit();
    throw std::runtime_error("Failed to initialise SDL_ttf: " + std::string(TTF_GetError()));
  }

  // Create a window
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
      SDL_CreateWindow("TicTacToe", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                       SDL_WINDOW_SHOWN),
      SDL_DestroyWindow);
  if (window == nullptr) {
    TTF_Quit();
    SDL_Quit();
    throw std::runtime_error("Failed to create window: " +
                             std::string(SDL_GetError()));
  }

  // Create a renderer
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
      SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED),
      SDL_DestroyRenderer);
  if (renderer == nullptr) {
    TTF_Quit();
    SDL_Quit();
    throw std::runtime_error("Failed to create renderer: " +
                             std::string(SDL_GetError()));
  }

  // Load font
  std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> font(
      TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24),
      TTF_CloseFont);
  if (!font) {
    TTF_Quit();
    SDL_Quit();
    throw std::runtime_error("Failed to load font: " +
                             std::string(SDL_GetError()));
  }

  // Render characters
  const SDL_Color color = {0, 0, 0, 255};
  const char *X = "X";
  const char *O = "O";
  const std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> X_surface(
      TTF_RenderText_Solid(font.get(), X, color), SDL_FreeSurface);
  const std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> O_surface(
      TTF_RenderText_Solid(font.get(), O, color), SDL_FreeSurface);
  if (!X_surface || !O_surface) {
    TTF_Quit();
    SDL_Quit();
    throw std::runtime_error("Failed to render text surface: " + std::string(TTF_GetError()));
  }

  std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> X_texture(
      SDL_CreateTextureFromSurface(renderer.get(), X_surface.get()),
      SDL_DestroyTexture);

  std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> O_texture(
      SDL_CreateTextureFromSurface(renderer.get(), O_surface.get()),
      SDL_DestroyTexture);

  if (!X_texture || !O_texture) {
    TTF_Quit();
    SDL_Quit();
    throw std::runtime_error("Failed to create texture: " +
                             std::string(SDL_GetError()));
  }

  // Main loop
  const size_t SQUARE_WIDTH = WINDOW_WIDTH / ROW_SIZE;
  const size_t SQUARE_HEIGHT = WINDOW_HEIGHT / COL_SIZE;

  bool running = true;
  SDL_Event event;
  while (running) {
    // Handle events
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        int buttonRow = floor(mouseX / SQUARE_WIDTH);
        int buttonCol = floor(mouseY / SQUARE_HEIGHT);
        move(buttonRow, buttonCol);
      } else if (gameState != PROGRESS) {
        SDL_MessageBoxButtonData buttons[2]{
            {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Restart"},
            {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Quit"},
        };

        SDL_MessageBoxData messageBoxData = {};
        messageBoxData.flags = SDL_MESSAGEBOX_INFORMATION;
        messageBoxData.title = "Game Over!";
        messageBoxData.message = gameStateToString(gameState);
        messageBoxData.numbuttons = 2;
        messageBoxData.buttons = buttons;

        int buttonPressed;
        if (SDL_ShowMessageBox(&messageBoxData, &buttonPressed) < 0) {
          TTF_Quit();
          SDL_Quit();
          throw std::runtime_error("SDL_ShowMessageBox failed: " + std::string(SDL_GetError()));
        }

        if (buttonPressed == 0) {
          restart();
        } else {
          running = false;
        }
      }
    }

    // Draw Squares
    for (int i = 0; i < COL_SIZE; ++i) {
      for (int j = 0; j < ROW_SIZE; ++j) {
        SDL_Rect squareRect = {static_cast<int>(SQUARE_WIDTH * i),
                               static_cast<int>(SQUARE_HEIGHT * j),
                               SQUARE_WIDTH, SQUARE_HEIGHT};
        SDL_SetRenderDrawColor(renderer.get(), 207, 185, 151, 255);
        SDL_RenderFillRect(renderer.get(), &squareRect);
        SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer.get(), &squareRect);

        SDL_Rect textRect = {
            static_cast<int>((squareRect.x) + (SQUARE_WIDTH / 4)),
            static_cast<int>(squareRect.y + (SQUARE_HEIGHT / 4)),
            SQUARE_WIDTH / 2, SQUARE_HEIGHT / 2};
        switch (board[i][j]) {
        case X_SQUARE:
          SDL_RenderCopy(renderer.get(), X_texture.get(), nullptr, &textRect);
          break;
        case O_SQUARE:
          SDL_RenderCopy(renderer.get(), O_texture.get(), nullptr, &textRect);
          break;
        default:
          break;
        }
      }
    }

    // Present the renderer
    SDL_RenderPresent(renderer.get());
  }

  // Cleanup
  TTF_Quit();
  SDL_Quit();
}

// Row and col are 0-indexed
void Board::move(size_t row, size_t col) {
  assert(row < ROW_SIZE && col < COL_SIZE);
  if (board[row][col] != FREE_SQUARE || gameState != PROGRESS)
    return;
  board[row][col] = currPlayer == PLAYER_X ? X_SQUARE : O_SQUARE;
  GameState currGameState = getGameState();
  if (currGameState != PROGRESS) {
    gameState = currGameState;
  } else {
    currPlayer = currPlayer == PLAYER_X ? PLAYER_O : PLAYER_X;
  }
}

void Board::printBoard() {
  for (int i = 0; i < COL_SIZE; ++i) {
    for (int j = 0; j < ROW_SIZE; ++j) {
      switch (board[i][j]) {
      case X_SQUARE:
        std::cout << 'X';
        break;
      case O_SQUARE:
        std::cout << 'O';
        break;
      default:
        std::cout << '-';
        break;
      }
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

bool areEqualSquares(SquareType square1, SquareType square2,
                     SquareType square3) {
  return square1 == square2 && square1 == square3 && square1 != FREE_SQUARE;
}

GameState Board::getGameState() {

  if (areEqualSquares(board[0][2], board[1][1], board[2][0]) ||
      areEqualSquares(board[0][0], board[1][1], board[2][2]))
    return currPlayer == PLAYER_X ? X_WIN : O_WIN;
  for (int i = 0; i < COL_SIZE; ++i) {
    if (areEqualSquares(board[i][0], board[i][1], board[i][2]) ||
        areEqualSquares(board[0][i], board[1][i], board[2][i]))
      return currPlayer == PLAYER_X ? X_WIN : O_WIN;
  }
  for (int i = 0; i < COL_SIZE; ++i) {
    for (int j = 0; j < ROW_SIZE; ++j) {
      if (board[i][j] == FREE_SQUARE)
        return PROGRESS;
    }
  }
  return DRAW;
}

int main() {
  std::unique_ptr<Board> board = std::make_unique<Board>();
  board->startGui();
  return EXIT_SUCCESS;
}
