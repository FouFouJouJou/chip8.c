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
#define SCALE 15
#define WIDTH SCALE*SCREEN_WIDTH
#define HEIGHT SCALE*SCREEN_HEIGHT

#define PIXELS_PER_ROW 64
#define PIXELS_PER_COL 32
#define PADDING 1
#define PIXEL_WIDTH WIDTH/PIXELS_PER_ROW
#define PIXEL_HEIGHT HEIGHT/PIXELS_PER_COL
#define TOTAL_PIXELS (WIDTH*HEIGHT)/(PIXEL_WIDTH*PIXEL_HEIGHT)
#define INSTRUCTION_SIZE 2

#define FPS 60

// TODO: convert registers to memory pointers
struct chip8_t {
  uint8_t v[16];
  uint16_t i, pc;
  uint8_t sp, dt, st;
  uint16_t stack[12];
  uint8_t ram[1<<12];
  uint8_t frame_buffer[SCREEN_HEIGHT*SCREEN_WIDTH];
  uint8_t keypad[16];
  //uint16_t *i, *pc, *stack;
  //uint8_t *sp, *dt, *st;
  //uint16_t *v1, *v2, *v3, *v4, *v5, *v6, *v7, *v8, *v9, *va, *vb, *vc, *vd, *ve, *vf;
  //uint8_t *v_regs, *frame_buffer;
};

typedef void (*decode_entry)(struct chip8_t *chip8, uint16_t instruction);

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
    DrawRectangle(
      pos_x
      , pos_y
      , PIXEL_WIDTH-(PADDING)
      , PIXEL_HEIGHT-(PADDING)
      , pixels[i] ? WHITE : BLACK
    );
  }
  EndDrawing();
}

uint8_t exit_() {
  CloseWindow();
  return EXIT_SUCCESS;
}

size_t read_file(const char *const file_name, uint8_t *buffer) {
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
  //chip8->pc=chip8->memory+ORG;
  chip8->pc=ORG;
  chip8->sp=0;
  chip8->dt=chip8->st=0;
  memset(chip8->frame_buffer, 0, SCREEN_WIDTH*SCREEN_HEIGHT);
  memset(chip8->keypad, 0, 16);
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

void increment_pc(uint16_t *const pc, uint16_t increment) {
  (*pc)+=(increment*INSTRUCTION_SIZE);
}

uint16_t get_4_bits(uint16_t instruction, uint8_t start_bit, uint8_t size) {
  return (instruction >> ((start_bit-1)*4)) & (0xFFFF >> (4-size)*4);
}

uint8_t process_input(struct chip8_t *const chip8) {
  int key=-1;
  char keyboard[17]="1234qwerasdfzxcv";
  uint8_t keypad[17]={
    0x1, 0x2, 0x3, 0xc
    ,0x4, 0x5, 0x6, 0xd
    ,0x7, 0x8, 0x9, 0xe
    ,0xa, 0x0, 0xb, 0xf
  };

  for(size_t i=0; i<strlen(keyboard); ++i) {
    if(IsKeyDown(keyboard[i])) {
      chip8->keypad[keypad[i]]=1;
      key=keypad[i];
    }
    else if (IsKeyUp(keyboard[i])) {
      chip8->keypad[keypad[i]]=0;
    }
  }
  return key;
}


void exec_op_0(struct chip8_t *chip8, uint16_t instruction) {
  uint32_t remainding_bits=get_4_bits(instruction, 1, 3);
  if(remainding_bits == 0x0e0) {
    memset(chip8->frame_buffer, 0, SCREEN_WIDTH*SCREEN_HEIGHT);
    increment_pc(&(chip8->pc), 1);
    printf("cls\n");
  }
  else if(remainding_bits == 0x0ee) {
    chip8->pc=chip8->stack[chip8->sp];
    chip8->sp--;
    printf("ret\n");
  }
  else {
    chip8->pc=remainding_bits;
    printf("sys 0x%04X\n", chip8->pc);
  }
}

void exec_op_1(struct chip8_t *chip8, uint16_t instruction) {
  uint16_t remainding_bits=get_4_bits(instruction, 1, 3);
  chip8->pc=remainding_bits;
  printf("jp 0x%04X\n", chip8->pc);
}

void exec_op_2(struct chip8_t *chip8, uint16_t instruction) {
  uint32_t remainding_bits=get_4_bits(instruction, 1, 3);
  increment_pc(&(chip8->pc), 1);
  chip8->stack[chip8->sp]=chip8->pc;
  chip8->pc=remainding_bits;
  printf("call %d\n", chip8->pc);
}

void exec_op_3(struct chip8_t *chip8, uint16_t instruction) {
  uint32_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t value=get_4_bits(instruction, 1, 2);
  if(chip8->v[register_x] == value) {
    increment_pc(&(chip8->pc), 2);
  }
  else increment_pc(&(chip8->pc), 1);
  printf("se V%d, 0x%04X\n", register_x, value);
}

void exec_op_4(struct chip8_t *chip8, uint16_t instruction) {
  uint32_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t value=get_4_bits(instruction, 1, 2);
  if(chip8->v[register_x] != value) {
    increment_pc(&(chip8->pc), 2);
  }
  else increment_pc(&(chip8->pc), 1);
  printf("sne V%d, 0x%04X\n", register_x, value);
}

void exec_op_5(struct chip8_t *chip8, uint16_t instruction) {
  uint32_t register_x=get_4_bits(instruction, 3, 1);
  uint32_t register_y=get_4_bits(instruction, 2, 1);
  if(chip8->v[register_x] == chip8->v[register_y])
    increment_pc(&(chip8->pc), 2);
  else
    increment_pc(&(chip8->pc), 1);
  printf("se V%d, V%d\n", register_x, register_y);
}

void exec_op_6(struct chip8_t *chip8, uint16_t instruction) {
  uint8_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t value=get_4_bits(instruction, 1, 2);
  chip8->v[register_x]=value;
  printf("ld V%d, 0x%04X\n", register_x, value);
  increment_pc(&chip8->pc, 1);
}

void exec_op_7(struct chip8_t *chip8, uint16_t instruction) {
  uint8_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t value=get_4_bits(instruction, 1, 2);
  chip8->v[register_x]+=value;
  printf("add V%d, 0x%04X\n", register_x, value);
  increment_pc(&(chip8->pc), 1);
}

void exec_op_8(struct chip8_t *chip8, uint16_t instruction) {
  uint8_t least_significant_4_bits=get_4_bits(instruction, 1, 1);
  uint8_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t register_y=get_4_bits(instruction, 2, 1);
  switch(least_significant_4_bits) {
  case 0:
    chip8->v[register_x]=chip8->v[register_y];
    printf("ld V%d, V%d\n", register_x, register_y);
    increment_pc(&(chip8->pc), 1);
    break;
  case 1:
    chip8->v[register_x]|=chip8->v[register_y];
    printf("or V%d, V%d\n", register_x, register_y);
    increment_pc(&(chip8->pc), 1);
    break;
  case 2:
    chip8->v[register_x]&=chip8->v[register_y];
    printf("and V%d, V%d\n", register_x, register_y);
    increment_pc(&(chip8->pc), 1);
    break;
  case 3:
    chip8->v[register_x]^=chip8->v[register_y];
    printf("xor V%d, V%d\n", register_x, register_y);
    increment_pc(&(chip8->pc), 1);
    break;
  case 4: {
    uint16_t result=chip8->v[register_x]+chip8->v[register_y];
    chip8->v[0xF]=(result>255);
    chip8->v[register_x]=result;
    printf("add V%d, V%d\n", register_x, register_y);
    increment_pc(&(chip8->pc), 1);
    break;
  }
  case 5:
    chip8->v[0xF]=(chip8->v[register_x]>chip8->v[register_y]);
    chip8->v[register_x]-=chip8->v[register_y];
    printf("sub V%d, V%d\n", register_x, register_y);
    increment_pc(&(chip8->pc), 1);
    break;
  case 6:
    chip8->v[0xF]=(chip8->v[register_x] & 0x1);
    chip8->v[register_x]>>=1;
    printf("shr V%d(%d)\n", register_x, chip8->v[register_x]);
    increment_pc(&(chip8->pc), 1);
    break;
  case 7:
    chip8->v[0xF]=(chip8->v[register_y]>chip8->v[register_x]);
    chip8->v[register_x]=chip8->v[register_y]-chip8->v[register_x];
    printf("subn V%d, V%d\n", register_x, register_y);
    increment_pc(&(chip8->pc), 1);
    break;
  case 0xe:
    chip8->v[0xF]=chip8->v[register_x] & (1 << 7);
    chip8->v[register_x]<<=1;
    printf("shl V%d\n", register_x);
    increment_pc(&(chip8->pc), 1);
    break;
  }
}

void exec_op_9(struct chip8_t *chip8, uint16_t instruction) {
  uint8_t register_x=get_4_bits(instruction, 3, 1);
  uint8_t register_y=get_4_bits(instruction, 2, 1);
  printf("sne V%d, V%d\n", register_x, register_y);
  increment_pc(&(chip8->pc), (chip8->v[register_x] != chip8->v[register_y]) ? 2 : 1);
}

void exec_op_a(struct chip8_t *chip8, uint16_t instruction) {
  uint16_t addr=get_4_bits(instruction, 1, 3);
  chip8->i=addr;
  printf("ld I, %d\n", addr);
  increment_pc(&(chip8->pc), 1);
}

void exec_op_b(struct chip8_t *chip8, uint16_t instruction) {
  uint16_t addr=get_4_bits(instruction, 1, 3);
  increment_pc(&(chip8->pc), chip8->v[0]+addr);
  printf("jp V0, %d\n", chip8->pc);
}

void exec_op_c(struct chip8_t *const chip8, const uint16_t instruction) {
  const uint8_t register_x=get_4_bits(instruction, 3, 1);
  const uint8_t value=get_4_bits(instruction, 2, 2);
  const uint8_t random_value=GetRandomValue(0, 255);
  chip8->v[register_x]=random_value & value;
  increment_pc(&(chip8->pc), 1);
  printf("rnd V%d, 0x%04X\n", register_x, random_value);
}

void exec_op_d(struct chip8_t *const chip8, const uint16_t instruction) {
  uint8_t x_pos=chip8->v[get_4_bits(instruction, 3, 1)], original_x=x_pos;
  uint8_t y_pos=chip8->v[get_4_bits(instruction, 2, 1)];
  const uint8_t n_bytes=get_4_bits(instruction, 1, 1);
  uint8_t data[n_bytes];
  memcpy(data, chip8->ram+chip8->i, n_bytes);
  for(int i=0; i<n_bytes; ++i) printf("%2x ", data[i]);
  printf("\n");
  chip8->v[0x0F]=0;
  for(int i=0; i<n_bytes; ++i) {
    x_pos=original_x;
    for(int k=7; k>=0; --k) {
      const uint8_t bit = (data[i]>>k) & 0x1;
      uint8_t *const bit_on_screen = chip8->frame_buffer+(y_pos*(SCREEN_WIDTH)+x_pos);
      // Or just xOr but still need to set Vf
      if(*bit_on_screen == 1 && bit) {
        chip8->v[0x0F]=1;
        *bit_on_screen=0;
      }
      else if(bit){
        *bit_on_screen=1;
      }
      x_pos+=1;
      if(x_pos == SCREEN_WIDTH) break;
    }
    y_pos+=1;
    if(y_pos == SCREEN_HEIGHT) break;
  }
  printf("drw %d, %d, %x\n", x_pos, y_pos, n_bytes);
  increment_pc(&chip8->pc, 1);
}

void exec_op_e(struct chip8_t *const chip8, const uint16_t instruction) {
  const uint8_t register_x=get_4_bits(instruction, 3, 1); 
  const uint8_t least_significant_byte=get_4_bits(instruction, 1, 2);
  switch(least_significant_byte) {
    case 0x9e:
      increment_pc(&chip8->pc, chip8->keypad[chip8->v[register_x]] ? 2 : 1);
      printf("skp V%d\n", register_x);
      break;
    case 0xa1:
      increment_pc(&chip8->pc, !chip8->keypad[chip8->v[register_x]] ? 2 : 1);
      printf("sknp V%d\n", register_x);
      break;
  }
}

void exec_op_f(struct chip8_t *const chip8, const uint16_t instruction) {
  const uint8_t least_significant_byte = get_4_bits(instruction, 1, 2);
  const uint8_t register_x = get_4_bits(instruction, 3, 1);
  switch(least_significant_byte) {
  case 0x07:
    chip8->v[register_x]=chip8->dt;
    printf("ld V%d, DT\n", register_x);
    break;
  case 0x0a: {
    int8_t key_pressed=-1;
    while((key_pressed=process_input(chip8)) != -1);
    chip8->v[register_x]=key_pressed;
    printf("ld V%d, %d\n", register_x, key_pressed);
    break;
  }
  case 0x15:
    chip8->dt=chip8->v[register_x];
    printf("ld DT, V%d\n", register_x);
    break;
  case 0x18:
    chip8->st=chip8->v[register_x];
    printf("ld ST, V%d\n", register_x);
    break;
  case 0x1E:
    chip8->i+=chip8->v[register_x];
    printf("add I, V%d\n", register_x);
    break;
  case 0x29:
    chip8->i=5*chip8->v[register_x];
    printf("ld %d, V%d\n", chip8->v[register_x], register_x );
    break;
  case 0x33: {
    const uint8_t bcd=chip8->v[register_x];
    const uint8_t hundreds=(bcd/100);
    const uint8_t tens=(bcd%100)/10;
    const uint8_t ones=(bcd%10);
    chip8->ram[chip8->i]=hundreds;
    chip8->ram[chip8->i+1]=tens;
    chip8->ram[chip8->i+2]=ones;
    printf("ld %d, V%d\n", bcd, register_x);
    break;
  }
  case 0x55:
    memcpy(chip8->ram+chip8->i, chip8->v, register_x+1);
    printf("ld [%d], V%d\n", chip8->i, register_x);
    break;
  case 0x65:
    memcpy(chip8->v, chip8->ram+chip8->i, register_x+1);
    printf("ld V%d, [%d]\n", register_x, chip8->i);

    break;
  }
  increment_pc(&chip8->pc, 1);
}

void cycle(struct chip8_t *const chip8) {
  uint16_t instruction=(chip8->ram[(chip8->pc)]<<8)|chip8->ram[(chip8->pc)+1];
  printf("0x%04X 0x%04X => ", chip8->pc, instruction);
  decode_entry routines[16]={
    exec_op_0 ,exec_op_1 ,exec_op_2 ,exec_op_3
    ,exec_op_4 ,exec_op_5 ,exec_op_6 ,exec_op_7
    ,exec_op_8 ,exec_op_9 ,exec_op_a ,exec_op_b
    ,exec_op_c ,exec_op_d ,exec_op_e ,exec_op_f
  };
  routines[get_4_bits(instruction, 4, 1)](chip8, instruction);
}

void help() {
  printf("Help: ./main <path_to_rom_file>.ch8\n");
}

int main(int argc, char **argv) {
  struct chip8_t chip8;
  if(argc != 2) {
    fprintf(stderr, "[ERROR] no rom file was specified\n");
    help();
    exit(68);
  }
  boot(&chip8, argv[1]);
  init();
  printf("%d\n", TOTAL_PIXELS);
  while(!WindowShouldClose()) {
    cycle(&chip8);
    WaitTime(0.001667);
    process_input(&chip8);
    render(chip8.frame_buffer);
  }
  return exit_();
}
