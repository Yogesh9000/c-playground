#include <SDL2/SDL.h>
#include <fmt/core.h>

#include <chrono>
#include <cstdint>
#include <fstream>
#include <random>
#include <string_view>

constexpr unsigned int START_ADDRESS = 0x200;
constexpr unsigned int FONTSET_START_ADDRESS = 0x50;
constexpr unsigned int FONTSET_SIZE = 80;
constexpr unsigned int VIDEO_WIDTH{64};
constexpr unsigned int VIDEO_HEIGHT{32};

constexpr uint8_t fontset[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

class Chip8 {
public:
  using Chip8Func = void (Chip8::*)();

  Chip8();
  Chip8(const Chip8 &) = delete;
  Chip8 operator=(const Chip8 &) = delete;
  Chip8(const Chip8 &&) = delete;
  Chip8 operator=(const Chip8 &&) = delete;
  ~Chip8() = default;

  void LoadROM(std::string_view fileHandle);
  void Cycle();

public:
  // Instructions
  void OP_NULL();
  void OP_00E0();
  void OP_00EE();
  void OP_1nnn();
  void OP_2nnn();
  void OP_3xkk();
  void OP_4xkk();
  void OP_5xy0();
  void OP_6xkk();
  void OP_7xkk();
  void OP_8xy0();
  void OP_8xy1();
  void OP_8xy2();
  void OP_8xy3();
  void OP_8xy4();
  void OP_8xy5();
  void OP_8xy6();
  void OP_8xy7();
  void OP_8xyE();
  void OP_9xy0();
  void OP_Annn();
  void OP_Bnnn();
  void OP_Cxkk();
  void OP_Dxyn();
  void OP_Ex9E();
  void OP_ExA1();
  void OP_Fx07();
  void OP_Fx0A();
  void OP_Fx15();
  void OP_Fx18();
  void OP_Fx1E();
  void OP_Fx29();
  void OP_Fx33();
  void OP_Fx55();
  void OP_Fx65();

  void Table0();
  void Table8();
  void TableE();
  void TableF();

public:
  uint8_t registers[16]{};  // Chip8 has 16 8bit registers
  uint8_t memory[4096]{};   // Chip8 has 4kb of ram
  uint16_t index{};         // Chip8 has a 16bit index register to store address
  uint16_t pc{};            // pc stores address of next instruction
  uint16_t stack[16]{};     // Chip8 uses stack to store the return address
  uint8_t sp{};             // sp points to top of stack
  uint8_t delayTimer{};
  uint8_t soundTimer{};
  uint8_t keypad[16]{};  // Chip8 has a keyboard with inputs from 0 to F
  uint32_t video[VIDEO_WIDTH * VIDEO_HEIGHT]{};  // 64*32 pixel diasplay
  uint16_t opcode;                               // Current instruction opcode

  // Random number generator
  std::default_random_engine randGen{};
  std::uniform_int_distribution<uint8_t> randByte{};

  // Opcode Table
  Chip8Func table[0xF + 1]{};
  Chip8Func table0[0xE + 1]{};
  Chip8Func table8[0xE + 1]{};
  Chip8Func tableE[0xE + 1]{};
  Chip8Func tableF[0x65 + 1]{};
};

Chip8::Chip8()
    : pc{START_ADDRESS},
      randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
  for (int i{0}; i < FONTSET_SIZE; ++i) {
    memory[FONTSET_START_ADDRESS + i] = fontset[i];
  }

  randByte = std::uniform_int_distribution<uint8_t>(0, 255U);

  table[0x0] = &Chip8::Table0;
  table[0x1] = &Chip8::OP_1nnn;
  table[0x2] = &Chip8::OP_2nnn;
  table[0x3] = &Chip8::OP_3xkk;
  table[0x4] = &Chip8::OP_4xkk;
  table[0x5] = &Chip8::OP_5xy0;
  table[0x6] = &Chip8::OP_6xkk;
  table[0x7] = &Chip8::OP_7xkk;
  table[0x8] = &Chip8::Table8;
  table[0x9] = &Chip8::OP_9xy0;
  table[0xA] = &Chip8::OP_Annn;
  table[0xB] = &Chip8::OP_Bnnn;
  table[0xC] = &Chip8::OP_Cxkk;
  table[0xD] = &Chip8::OP_Dxyn;
  table[0xE] = &Chip8::TableE;
  table[0xF] = &Chip8::TableF;

  for (int i{0}; i <= 0xE; ++i) {
    table0[i] = &Chip8::OP_NULL;
    table8[i] = &Chip8::OP_NULL;
    tableE[i] = &Chip8::OP_NULL;
  }

  table0[0x0] = &Chip8::OP_00E0;
  table0[0xE] = &Chip8::OP_00EE;

  table8[0x0] = &Chip8::OP_8xy0;
  table8[0x1] = &Chip8::OP_8xy1;
  table8[0x2] = &Chip8::OP_8xy2;
  table8[0x3] = &Chip8::OP_8xy3;
  table8[0x4] = &Chip8::OP_8xy4;
  table8[0x5] = &Chip8::OP_8xy5;
  table8[0x6] = &Chip8::OP_8xy6;
  table8[0x7] = &Chip8::OP_8xy7;
  table8[0xE] = &Chip8::OP_8xyE;

  tableE[0x1] = &Chip8::OP_ExA1;
  tableE[0xE] = &Chip8::OP_Ex9E;

  for (int i{0}; i <= 0x65; ++i) {
    tableF[i] = &Chip8::OP_NULL;
  }

  tableF[0x07] = &Chip8::OP_Fx07;
  tableF[0x0A] = &Chip8::OP_Fx0A;
  tableF[0x15] = &Chip8::OP_Fx15;
  tableF[0x18] = &Chip8::OP_Fx18;
  tableF[0x1E] = &Chip8::OP_Fx1E;
  tableF[0x29] = &Chip8::OP_Fx29;
  tableF[0x33] = &Chip8::OP_Fx33;
  tableF[0x55] = &Chip8::OP_Fx55;
  tableF[0x65] = &Chip8::OP_Fx65;
}

void Chip8::LoadROM(std::string_view fileHandle) {
  // open the rom and seek to end
  std::ifstream file(fileHandle.data(), std::ios::binary | std::ios::ate);
  if (file.is_open()) {
    std::streampos size = file.tellg();
    char *buffer = new char[size];  // temporary buffer to hold data
    file.seekg(0, std::ios::beg);
    file.read(buffer, size);
    file.close();

    for (int i{0}; i < size; ++i) {
      memory[START_ADDRESS + i] = buffer[i];  // copy the rom to memory
    }

    delete[] buffer;
  }
}

void Chip8::OP_NULL() {}

void Chip8::OP_00E0() {
  memset(video, 0, sizeof video);
}

void Chip8::OP_00EE() {
  --sp;
  pc = stack[sp];
}

void Chip8::OP_1nnn() {
  uint16_t address = opcode & 0x0FFFU;
  pc = address;
}

void Chip8::OP_2nnn() {
  uint16_t address = opcode & 0x0FFFU;
  stack[sp] = pc;
  ++sp;
  pc = address;
}

void Chip8::OP_3xkk() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t kk = opcode & 0x00FFU;
  if (registers[Vx] == kk) {
    pc += 2;
  }
}

void Chip8::OP_4xkk() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t kk = opcode & 0x00FFU;
  if (registers[Vx] != kk) {
    pc += 2;
  }
}

void Chip8::OP_5xy0() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t Vy = (opcode & 0x00F0U) >> 4U;
  if (registers[Vx] == registers[Vy]) {
    pc += 2;
  }
}

void Chip8::OP_6xkk() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t kk = opcode & 0x00FFU;

  registers[Vx] = kk;
}

void Chip8::OP_7xkk() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t kk = opcode & 0x00FFU;

  registers[Vx] = registers[Vx] + kk;
}

void Chip8::OP_8xy0() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t Vy = (opcode & 0x00F0U) >> 4U;

  registers[Vx] = registers[Vy];
}

void Chip8::OP_8xy1() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t Vy = (opcode & 0x00F0U) >> 4U;

  registers[Vx] = registers[Vx] or registers[Vy];
}

void Chip8::OP_8xy2() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t Vy = (opcode & 0x00F0U) >> 4U;

  registers[Vx] = registers[Vx] and registers[Vy];
}

void Chip8::OP_8xy3() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t Vy = (opcode & 0x00F0U) >> 4U;

  registers[Vx] = registers[Vx] xor registers[Vy];
}

void Chip8::OP_8xy4() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t Vy = (opcode & 0x00F0U) >> 4U;
  constexpr uint8_t Vf = 0xFU;

  uint16_t sum = registers[Vx] + registers[Vy];

  if (sum > 255U) {
    registers[Vf] = 1U;
  } else {
    registers[Vf] = 0U;
  }

  registers[Vx] = sum & 0x00FFU;
}

void Chip8::OP_8xy5() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t Vy = (opcode & 0x00F0U) >> 4U;
  constexpr uint8_t Vf = 0xFU;

  if (registers[Vx] > registers[Vy]) {
    registers[Vf] = 1;
  } else {
    registers[Vf] = 0;
  }

  registers[Vx] = registers[Vx] - registers[Vy];
}

void Chip8::OP_8xy6() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  constexpr uint8_t Vf = 0xFU;

  registers[Vf] = (registers[Vx] & 0x1u);
  registers[Vx] >>= 1;
}

void Chip8::OP_8xy7() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t Vy = (opcode & 0x00F0U) >> 4U;
  constexpr uint8_t Vf = 0xFU;

  registers[Vf] = (registers[Vy] > registers[Vx]) ? 1u : 0u;

  registers[Vx] = registers[Vy] - registers[Vx];
}

void Chip8::OP_8xyE() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  constexpr uint8_t Vf = 0xFU;

  registers[Vf] = (registers[Vx] & 0x80U) >> 7U;
  registers[Vx] <<= 1;
}

void Chip8::OP_9xy0() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t Vy = (opcode & 0x00F0U) >> 4U;

  if (registers[Vx] != registers[Vy]) {
    pc += 2;
  }
}

void Chip8::OP_Annn() {
  index = opcode & 0x0FFFU;
}

void Chip8::OP_Bnnn() {
  uint16_t address = opcode & 0x0FFFU;
  pc = registers[0] + address;
}

void Chip8::OP_Cxkk() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t kk = opcode & 0x00FFU;

  registers[Vx] = randByte(randGen) and kk;
}

void Chip8::OP_Dxyn() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t Vy = (opcode & 0x00F0U) >> 4U;
  uint8_t height = opcode & 0x000FU;
  constexpr uint8_t Vf = 0xFU;

  uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
  uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

  for (int row{0}; row < height; ++row) {
    uint8_t spriteByte = memory[index + row];
    for (int col{0}; col < 8; ++col) {
      uint8_t spritePixel = spriteByte & (0x80u >> col);
      uint32_t *screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

      // Sprite pixel is on
      if (spritePixel) {
        // Screen pixel also on - collision
        if (*screenPixel == 0xFFFFFFFF) {
          registers[Vf] = 1;
        }

        // Effectively XOR with the sprite pixel
        *screenPixel ^= 0xFFFFFFFF;
      }
    }
  }
}

void Chip8::OP_Ex9E() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t key = registers[Vx];
  if (keypad[key]) {
    pc += 2;
  }
}

void Chip8::OP_ExA1() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t key = registers[Vx];
  if (!keypad[key]) {
    pc += 2;
  }
}

void Chip8::OP_Fx07() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  registers[Vx] = delayTimer;
}

void Chip8::OP_Fx0A() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  if (keypad[0]) {
    registers[Vx] = 0;
  } else if (keypad[1]) {
    registers[Vx] = 1;
  } else if (keypad[2]) {
    registers[Vx] = 2;
  } else if (keypad[3]) {
    registers[Vx] = 3;
  } else if (keypad[4]) {
    registers[Vx] = 4;
  } else if (keypad[5]) {
    registers[Vx] = 5;
  } else if (keypad[6]) {
    registers[Vx] = 6;
  } else if (keypad[7]) {
    registers[Vx] = 7;
  } else if (keypad[8]) {
    registers[Vx] = 8;
  } else if (keypad[9]) {
    registers[Vx] = 9;
  } else if (keypad[10]) {
    registers[Vx] = 10;
  } else if (keypad[11]) {
    registers[Vx] = 11;
  } else if (keypad[12]) {
    registers[Vx] = 12;
  } else if (keypad[13]) {
    registers[Vx] = 13;
  } else if (keypad[14]) {
    registers[Vx] = 14;
  } else if (keypad[15]) {
    registers[Vx] = 15;
  } else {
    pc -= 2;
  }
}

void Chip8::OP_Fx15() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  delayTimer = registers[Vx];
}

void Chip8::OP_Fx18() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  soundTimer = registers[Vx];
}

void Chip8::OP_Fx1E() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  index += registers[Vx];
}

void Chip8::OP_Fx29() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t digit = registers[Vx];
  index = FONTSET_START_ADDRESS + (digit * 5);
}

void Chip8::OP_Fx33() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  uint8_t value = registers[Vx];
  memory[index + 2] = value % 10;
  value /= 10;
  memory[index + 1] = value % 10;
  value /= 10;
  memory[index] = value % 10;
}

void Chip8::OP_Fx55() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  for (int i{0}; i <= Vx; ++i) {
    memory[index + i] = registers[i];
  }
}

void Chip8::OP_Fx65() {
  uint8_t Vx = (opcode & 0x0F00U) >> 8U;
  for (int i{0}; i <= Vx; ++i) {
    registers[i] = memory[index + i];
  }
}

void Chip8::Table0() {
  ((*this).*(table0[opcode & 0x000Fu]))();
}

void Chip8::Table8() {
  ((*this).*(table8[opcode & 0x000Fu]))();
}

void Chip8::TableE() {
  ((*this).*(tableE[opcode & 0x000FU]))();
}

void Chip8::TableF() {
  ((*this).*(tableF[opcode & 0x00FFU]))();
}

void Chip8::Cycle() {
  opcode = ((memory[pc] << 8U) | memory[pc + 1]);
  pc += 2;  // increment pc to next instruction
  ((*this).*(table[(opcode & 0xF000U) >> 12U]))();

  if (delayTimer > 0) {
    --delayTimer;
  }

  if (soundTimer > 0) {
    --soundTimer;
  }
}

class Platform {
public:
  Platform(std::string_view title, int windowWidth, int windowHeight,
           int texturWidth, int textureHeight);
  Platform(const Platform &) = delete;
  Platform(const Platform &&) = delete;
  Platform &operator=(const Platform &) = delete;
  Platform &operator=(const Platform &&) = delete;
  ~Platform();

public:
  void Update(void const *buffer, int pitch);
  bool ProcessInput(uint8_t *keys);

private:
  SDL_Window *window{};
  SDL_Renderer *renderer{};
  SDL_Texture *texture{};
};

Platform::Platform(std::string_view title, int windowWidth, int windowHeight,
                   int textureWidth, int textureHeight)
    : window{nullptr}, renderer{nullptr}, texture{nullptr} {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fmt::println(stderr, "failed to initialize sdl: {}", SDL_GetError());
  } else {
    window = SDL_CreateWindow(title.data(), SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, windowWidth,
                              windowHeight, SDL_WINDOW_SHOWN);
    if (!window) {
      fmt::println(stderr, "failed to create sdl window: {}", SDL_GetError());
    } else {
      renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
      if (!renderer) {
        fmt::println(stderr, "failed to create sdl renderer: {}",
                     SDL_GetError());
      } else {
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_STREAMING, textureWidth,
                                    textureHeight);
      }
    }
  }
}

Platform::~Platform() {
  SDL_DestroyTexture(texture);
  texture = nullptr;
  SDL_DestroyRenderer(renderer);
  renderer = nullptr;
  SDL_DestroyWindow(window);
  window = nullptr;
}

void Platform::Update(void const *buffer, int pitch) {
  SDL_UpdateTexture(texture, nullptr, buffer, pitch);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

bool Platform::ProcessInput(uint8_t *keys) {
  bool quit{false};
  SDL_Event event{};
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        quit = true;
        break;
      case SDL_KEYDOWN: {
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            quit = true;
            break;
          case SDLK_x:
            keys[0] = 1;
            break;
          case SDLK_1:
            keys[1] = 1;
            break;
          case SDLK_2:
            keys[2] = 1;
            break;
          case SDLK_3:
            keys[3] = 1;
            break;
          case SDLK_q:
            keys[4] = 1;
            break;
          case SDLK_w:
            keys[5] = 1;
            break;
          case SDLK_e:
            keys[6] = 1;
            break;
          case SDLK_a:
            keys[7] = 1;
            break;
          case SDLK_s:
            keys[8] = 1;
            break;
          case SDLK_d:
            keys[9] = 1;
            break;
          case SDLK_z:
            keys[0xA] = 1;
            break;
          case SDLK_c:
            keys[0xB] = 1;
            break;
          case SDLK_4:
            keys[0xC] = 1;
            break;
          case SDLK_r:
            keys[0xD] = 1;
            break;
          case SDLK_f:
            keys[0xE] = 1;
            break;
          case SDLK_v:
            keys[0xF] = 1;
            break;
        }
      } break;
      case SDL_KEYUP: {
        switch (event.key.keysym.sym) {
          case SDLK_x:
            keys[0] = 0;
            break;
          case SDLK_1:
            keys[1] = 0;
            break;
          case SDLK_2:
            keys[2] = 0;
            break;
          case SDLK_3:
            keys[3] = 0;
            break;
          case SDLK_q:
            keys[4] = 0;
            break;
          case SDLK_w:
            keys[5] = 0;
            break;
          case SDLK_e:
            keys[6] = 0;
            break;
          case SDLK_a:
            keys[7] = 0;
            break;
          case SDLK_s:
            keys[8] = 0;
            break;
          case SDLK_d:
            keys[9] = 0;
            break;
          case SDLK_z:
            keys[0xA] = 0;
            break;
          case SDLK_c:
            keys[0xB] = 0;
            break;
          case SDLK_4:
            keys[0xC] = 0;
            break;
          case SDLK_r:
            keys[0xD] = 0;
            break;
          case SDLK_f:
            keys[0xE] = 0;
            break;
          case SDLK_v:
            keys[0xF] = 0;
            break;
        }
      } break;
    }
  }
  return quit;
}

int main(int argc, char **argv) {
  if (argc != 4) {
    fmt::println(stderr, "Usage: {} <Scale> <Delay> <ROM>", argv[0]);
    std::exit(EXIT_FAILURE);
  }

  int videoScale = std::stoi(argv[1]);
  int cycleDelay = std::stoi(argv[2]);
  std::string_view romFileName = argv[3];
  Platform platform("CHIP-8 Emulator", VIDEO_WIDTH * videoScale,
                    VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);

  Chip8 chip8;
  chip8.LoadROM(romFileName);
  int videoPitch = sizeof(chip8.video[0]) * VIDEO_WIDTH;

  auto lastCycleTime = std::chrono::high_resolution_clock::now();
  bool quit = false;

  while (!quit) {
    quit = platform.ProcessInput(chip8.keypad);

    auto currentTime = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(
                   currentTime - lastCycleTime)
                   .count();

    if (dt > cycleDelay) {
      lastCycleTime = currentTime;

      chip8.Cycle();

      platform.Update(chip8.video, videoPitch);
    }
  }
  return 0;
}
