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
  chip8->i=chip8->sp+1;
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

// TODO: argc and argv checking
int main(int argc, char **argv) {
  struct chip8_t chip8;
  boot(&chip8, argv[1]);
  return EXIT_SUCCESS;
}
