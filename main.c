#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <raylib.h>
#include <math.h>

#define WIDTH 800
#define HEIGHT 800

#define PIXELS_PER_ROW 20
#define PIXELS_PER_COL 20

#define PIXEL_WIDTH WIDTH/PIXELS_PER_COL
#define PIXEL_HEIGHT HEIGHT/PIXELS_PER_ROW


#define FPS 60

#define TOTAL_PIXELS (WIDTH*HEIGHT)/(PIXEL_WIDTH*PIXEL_HEIGHT)

void set_pixel(uint8_t pixels[], uint16_t pos_x, uint16_t pos_y, uint8_t bit) {
  pixels[(pos_y*PIXELS_PER_ROW)+pos_x]=bit;
}

void invert_pixel(uint8_t pixels[], uint16_t pos_x, uint16_t pos_y) {
  pixels[(pos_y*PIXELS_PER_ROW)+pos_x]=!pixels[(pos_y*PIXELS_PER_ROW)+pos_x];
}

void init_pixels(uint8_t pixels[]) {
  for(int i=0; i<TOTAL_PIXELS; ++i) {
    pixels[i]=(GetRandomValue(0, 10) > 5);
  }
}

void init(uint8_t pixels[]) {
  InitWindow(WIDTH, HEIGHT, "raylib [core] example - basic window");
  init_pixels(pixels);
  SetTargetFPS(FPS);
}

void render(uint8_t pixels[]) {
  BeginDrawing();
  for(int i=0; i<TOTAL_PIXELS; ++i) {
    uint16_t pos_x=(i%(PIXELS_PER_COL))*PIXEL_WIDTH;
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

int main(void) {
  uint8_t pixels[TOTAL_PIXELS]={0};
  init(pixels);
  int mouse_x=0, mouse_y=0;
  while (!WindowShouldClose()) {
    if(IsKeyPressed('R')){
      init_pixels(pixels);
    }
    if(IsMouseButtonPressed(0)) {
      mouse_x=GetMouseX();
      mouse_y=GetMouseY();
      invert_pixel(pixels, mouse_x/(PIXEL_HEIGHT), mouse_y/(PIXEL_WIDTH));
    }
    render(pixels);
  }
  return exit_();
}
