#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <raylib.h>

#define ORG 0x200

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCALE 10
#define WIDTH SCALE*SCREEN_WIDTH
#define HEIGHT SCALE*SCREEN_HEIGHT

#define PIXELS_PER_ROW 64
#define PIXELS_PER_COL 32
#define PIXEL_WIDTH WIDTH/PIXELS_PER_ROW
#define PIXEL_HEIGHT HEIGHT/PIXELS_PER_COL
#define TOTAL_PIXELS (WIDTH*HEIGHT)/(PIXEL_WIDTH*PIXEL_HEIGHT)

#define FPS 60
#define INSTRUCTION_SIZE 16

struct chip8_t {
  uint8_t v[16];
  uint16_t i, pc;
  uint8_t sp, dt, st;
  uint16_t stack[12];
  uint8_t ram[1<<12];
  uint8_t frame_buffer[SCREEN_HEIGHT*SCREEN_WIDTH];
};

void set_pixel(uint8_t pixels[], uint16_t pos_x, uint16_t pos_y, uint8_t bit) {
  printf("%d %d %d\n", pos_x, pos_y, (pos_y*(PIXELS_PER_ROW))+pos_x);
  pixels[(pos_y*PIXELS_PER_ROW)+pos_x]=bit;
}

void invert_pixel(uint8_t pixels[], uint16_t pos_x, uint16_t pos_y) {
  printf("%d %d %d\n", pos_x, pos_y, (pos_y*(PIXELS_PER_ROW))+pos_x);
  pixels[(pos_y*(PIXELS_PER_ROW))+pos_x]=!pixels[(pos_y*(PIXELS_PER_ROW))+pos_x];
}

void init() {
  InitWindow(WIDTH, HEIGHT, "Chip-8 emulator");
  SetTargetFPS(FPS);
}

void render(uint8_t pixels[]) {
  BeginDrawing();
  for(int i=0; i<TOTAL_PIXELS; ++i) {
    uint16_t pos_x=(i%(PIXELS_PER_ROW))*PIXEL_WIDTH;
    uint16_t pos_y=(i/(PIXELS_PER_ROW))*PIXEL_HEIGHT;
    DrawLine(pos_x, 0, pos_x, HEIGHT, GRAY);
    DrawLine(0, pos_y, WIDTH, pos_y, GRAY);
    DrawRectangle(
      pos_x
      ,pos_y
      , PIXEL_WIDTH
      , PIXEL_HEIGHT
      , pixels[i] ? WHITE : BLACK
    );
  }
  EndDrawing();
}

uint8_t exit_() {
  CloseWindow();
  return EXIT_SUCCESS;
}

size_t read_file(const char *const file_name, char *buffer) {
  int fd=open(file_name, O_RDONLY);
  if(fd == -1) exit(80);
  off_t raw_bytes=lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  ssize_t bytes_read=read(fd, buffer, raw_bytes);
  buffer[bytes_read]='\0';
  close(fd);
  return bytes_read;
}

size_t boot(struct chip8_t *const chip8, const char *const rom_name) {
  chip8->pc=ORG;
  chip8->sp=0;
  chip8->dt=chip8->st=0;
  memset(chip8->frame_buffer, 0, SCREEN_WIDTH*SCREEN_HEIGHT);
  uint8_t fonts[5*16] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };
  memcpy(chip8->ram, fonts, 40);

  uint8_t buffer[0xFFF-0x200];
  ssize_t bytes=read_file(rom_name, buffer);
  memmove(chip8->ram+ORG, buffer, bytes);
  return bytes;
}

uint16_t get_4_bits(uint16_t instruction, uint8_t start_bit, uint8_t size) {
  return (instruction >> ((start_bit-1)*4)) & (0xFFFF >> (4-size)*4);
}

void exec_op_0(struct chip8_t *chip8, uint16_t instruction) {
  uint32_t remainding_bits=get_4_bits(instruction, 3, 3);
  if(remainding_bits == 0x0e0) {
    printf("cls\n");
    memset(chip8->frame_buffer, 0, SCREEN_WIDTH*SCREEN_HEIGHT);
    chip8->pc++;
  }
  else if(remainding_bits == 0x0ee) {
    chip8->pc=chip8->stack[chip8->sp];
    chip8->sp--;
    printf("ret\n");
  }
  else {
    chip8->pc=remainding_bits;
    printf("sys %d\n", chip8->pc);
  }
}

void exec_op_1(struct chip8_t *chip8, uint16_t instruction) {
  uint32_t remainding_bits=get_4_bits(instruction, 3, 3);
  chip8->pc=remainding_bits;
  printf("jp %d\n", chip8->pc);
}

void exec_op_2(struct chip8_t *chip8, uint16_t instruction) {
  uint32_t remainding_bits=get_4_bits(instruction, 3, 3);
  chip8->sp+=1;
  chip8->stack[chip8->sp]=chip8->pc;
  chip8->pc=remainding_bits;
  printf("call %d\n", chip8->pc);
}

void exec_op_3(struct chip8_t *chip8, uint16_t instruction) {
  uint32_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t value=get_4_bits(instruction, 2, 2);
  if(chip8->v[register_x] == value) {
    chip8->pc+=2;
  }
  else chip8->pc+=1;
  printf("se V%d, %x\n", register_x, value);
}

void exec_op_4(struct chip8_t *chip8, uint16_t instruction) {
  uint32_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t value=get_4_bits(instruction, 2, 2);
  if(chip8->v[register_x] != value) {
    chip8->pc+=2;
  }
  else chip8->pc+=1;
  printf("sne V%d, %x\n", register_x, value);
}

void exec_op_5(struct chip8_t *chip8, uint16_t instruction) {
  uint32_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t value=get_4_bits(instruction, 2, 2);
  printf("sne V%d, %x\n", register_x, value);

  if(chip8->v[register_x] != value) {
    chip8->pc+=2;
  }
  else chip8->pc+=1;
}

void exec_op_6(struct chip8_t *chip8, uint16_t instruction) {
  uint8_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t value=get_4_bits(instruction, 2, 2);
  chip8->v[register_x]=value;
  printf("mov V%d, %d\n", register_x, value);
  chip8->pc+=1;
}

void exec_op_7(struct chip8_t *chip8, uint16_t instruction) {
  uint8_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t value=get_4_bits(instruction, 2, 2);
  chip8->v[register_x]+=value;
  printf("add V%d, %d\n", register_x, value);
  chip8->pc+=1;
}

void exec_op_8(struct chip8_t *chip8, uint16_t instruction) {
  uint8_t least_significant_4_bits=get_4_bits(instruction, 1, 1);
  uint8_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t register_y=get_4_bits(instruction, 2, 1);
  switch(least_significant_4_bits) {
    case 0:
      chip8->v[register_x]=chip8->v[register_y];
      printf("ld V%d, V%d\n", register_x, register_y);
      chip8->pc+=1;
      break;
    case 1:
      chip8->v[register_x]|=chip8->v[register_y];
      printf("or V%d, V%d\n", register_x, register_y);
      chip8->pc+=1;
      break;
    case 2:
      chip8->v[register_x]&=chip8->v[register_y];
      printf("and V%d, V%d\n", register_x, register_y);
      chip8->pc+=1;
      break;
    case 3:
      chip8->v[register_x]^=chip8->v[register_y];
      printf("xor V%d, V%d\n", register_x, register_y);
      chip8->pc+=1;
      break;
    case 4:
      uint16_t result=chip8->v[register_x]+chip8->v[register_y];
      chip8->v[0xF]=(result>255);
      chip8->v[register_x]=result;
      printf("add V%d, V%d\n", register_x, register_y);
      chip8->pc+=1;
      break;
    case 5:
      chip8->v[0xF]=(chip8->v[register_x]>chip8->v[register_y]);
      chip8->v[register_x]-=chip8->v[register_y];
      printf("sub V%d, V%d", register_x, register_y);
      chip8->pc+=1;
      break;
    case 6:
      chip8->v[0xF]=(chip8->v[register_x] & 0x01);
      chip8->v[register_x]/=2;
      printf("shr V%d\n", register_x);
      chip8->pc+=1;
      break;
    case 7:
      chip8->v[0xF]=(chip8->v[register_y]>chip8->v[register_x]);
      chip8->v[register_x]=chip8->v[register_y]-chip8->v[register_x];
      printf("subn V%d, V%d\n", register_x, register_y);
      chip8->pc+=1;
      break;
    case 0xe:
      chip8->pc+=(chip8->v[register_x] != chip8->v[register_y]) ? 2 : 1;
      chip8->v[register_x]*=2;
      printf("shl V%d\n", register_x);
      break;
  }
}

void exec_op_9(struct chip8_t *chip8, uint16_t instruction) {
  uint8_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t register_y=get_4_bits(instruction, 2, 1);
  printf("sne V%d, V%d\n", register_x, register_y);
  chip8->pc+=(chip8->v[register_x] != chip8->v[register_y]) ? 2 : 1;
}

void exec_op_a(struct chip8_t *chip8, uint16_t instruction) {
  uint16_t addr=get_4_bits(instruction, 1, 3);
  chip8->i=addr;
  printf("ld I, %d\n", addr);
  chip8->pc+=1;
}

void exec_op_b(struct chip8_t *chip8, uint16_t instruction) {
  uint16_t addr=get_4_bits(instruction, 1, 3);
  chip8->pc+=chip8->v[0]+addr;
  printf("jp V0, %d\n", chip8->pc);
}

void exec_op_c(struct chip8_t *chip8, uint16_t instruction) {
  uint8_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t value=get_4_bits(instruction, 2, 2);
  uint8_t random_value=GetRandomValue(0, 255);
  chip8->v[register_x]=random_value & value;
  chip8->pc+=1;
  printf("rnd V%d, %x\n", register_x, random_value);
}

// TODO
void exec_op_d(struct chip8_t *chip8, uint16_t instruction) {}
void exec_op_e(struct chip8_t *chip8, uint16_t instruction) {}
void exec_op_f(struct chip8_t *chip8, uint16_t instruction) {}

void cycle(struct chip8_t *const chip8) {
  uint16_t instruction=(chip8->ram[chip8->pc]<<((INSTRUCTION_SIZE)/2)) | chip8->ram[chip8->pc+1];
  switch(get_4_bits(instruction, 4, 1)) {
    case 0:
      exec_op_0(chip8, instruction);
      break;
    case 1:
      exec_op_1(chip8, instruction);
      break;
    case 2:
      exec_op_2(chip8, instruction);
      break;
    case 3:
      exec_op_3(chip8, instruction);
      break;
    case 4:
      exec_op_4(chip8, instruction);
      break;
    case 5:
      exec_op_5(chip8, instruction);
      break;
    case 6:
      exec_op_6(chip8, instruction);
      break;
    case 7:
      printf("7 opcode\n");
      break;
    case 8:
      exec_op_8(chip8, instruction);
      break;
    case 9:
      exec_op_9(chip8, instruction);
      break;
    case 0xa:
      printf("0xa opcode\n");
      break;
    case 0xb:
      printf("0xb opcode\n");
      break;
    case 0xc:
      printf("0xc opcode\n");
      break;
    case 0xd:
      printf("0xd opcode\n");
      break;
    case 0xe:
      printf("0xe opcode\n");
      break;
    case 0xf:
      printf("0xf opcode\n");
      break;
    default:
      exit(81);
  }
}


// TODO: argc and argv checking
int main(int argc, char **argv) {
  struct chip8_t chip8;
  boot(&chip8, argv[1]);
  cycle(&chip8);
  cycle(&chip8);
  cycle(&chip8);
  cycle(&chip8);
  return EXIT_SUCCESS;
}
