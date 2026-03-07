#include <stdlib.h>
#include <Common.h>
#include <Display/Graphics.h>
static Pixel pixel;

// 边缘追踪器
typedef struct {
    int16_t x;
    int16_t dx, dy;
    int16_t err;
    int16_t stepX;
} EdgeState;

// 初始化边缘追踪器
void InitEdge(EdgeState *e, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    e->x = x1;
    e->dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    e->dy = y2 - y1; // 已经排序过，所以 y2 >= y1
    e->stepX = (x2 > x1) ? 1 : -1;
    e->err = e->dx / 2; // 初始误差偏移
}

// 沿 Y 轴更新 X 坐标
void StepEdge(EdgeState *e) {
    if (e->dy <= 0) return;
    e->err += e->dx;
    while (e->err >= e->dy) {
        e->err -= e->dy;
        e->x += e->stepX;
    }
}

// ========== 水平直线 ==========
__attribute__((weak)) void DrawHLine(uint16_t x0, uint16_t y0, uint16_t x1,
                                     COLOR color) {
  for (uint16_t index = x0; index <= x1; ++index) {
    pixel.x = index;
    pixel.y = y0;
    pixel.color = color;
    DrawPixel(&pixel);
  }
}

// ========== 垂直线 ==========
__attribute__((weak)) void DrawVLine(uint16_t x0, uint16_t y0, uint16_t y1,
                                     COLOR color) {
  for (uint16_t index = y0; index <= y1; ++index) {
    pixel.x = x0;
    pixel.y = index;
    pixel.color = color;
    DrawPixel(&pixel);
  }
}

// ========== 直线（Bresenham） ==========
__attribute__((weak)) void DrawLine(uint16_t x0, uint16_t y0, uint16_t x1,
                                    uint16_t y1, COLOR color) {

  if (y0 == y1) {
    DrawHLine(x0 < x1 ? x0 : x1, y0, x0 < x1 ? x1 : x0, color);
    return;
  }
  if (x0 == x1) {
    DrawVLine(x0, y0 < y1 ? y0 : y1, y0 < y1 ? y1 : y0, color);
    return;
  }

  int dx = abs(x1 - x0);
  int dy = -abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy;
  pixel.x = x0;
  pixel.y = y0;
  pixel.color = color;
  while (1) {
    DrawPixel(&pixel);
    if (pixel.x == x1 && pixel.y == y1)
      break;
    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      pixel.x += sx;
    }
    if (e2 <= dx) {
      err += dx;
      pixel.y += sy;
    }
  }
}

// ========== 矩形 ==========
__attribute__((weak)) void DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  COLOR color) {
  DrawLine(x, y, x + w - 1, y, color);
  DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
  DrawLine(x, y, x, y + h - 1, color);
  DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

// ========== 填充矩形 ==========
__attribute__((weak)) void DrawFilledRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        COLOR color) {
  pixel.y = y;
  pixel.color = color;
  for (uint16_t i = 0; i < h; i++, pixel.y++) {
    pixel.x = x;
    for (uint16_t j = 0; j < w; j++, pixel.x++) {
      DrawPixel(&pixel);
    }
  }
}

// ========== 圆形（中点算法） ==========
__attribute__((weak)) void DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, COLOR color) {
  int16_t x = r;
  int16_t y = 0;
  int16_t err = 0;
  pixel.color = color;

  while (x >= y) {
    pixel.x = x0 + x;
    pixel.y = y0 + y;
    DrawPixel(&pixel);
    pixel.x = x0 + y;
    pixel.y = y0 + x;
    DrawPixel(&pixel);
    pixel.x = x0 - y;
    DrawPixel(&pixel);
    pixel.x = x0 - x;
    pixel.y = y0 + y;
    DrawPixel(&pixel);
    pixel.y = y0 - y;
    DrawPixel(&pixel);
    pixel.x = x0 - y;
    pixel.y = y0 - x;
    DrawPixel(&pixel);
    pixel.x = x0 + y;
    DrawPixel(&pixel);
    pixel.x = x0 + x;
    pixel.y = y0 - y;
    DrawPixel(&pixel);

    y++;
    if (err <= 0)
      err += 2 * y + 1;
    else {
      x--;
      err += 2 * (y - x + 1);
    }
  }
}

// ========== 填充圆形 ==========
__attribute__((weak)) void DrawFilledCircle(uint16_t x0, uint16_t y0, uint16_t r,
                          COLOR color) {
  int16_t x = r;
  int16_t y = 0;
  int16_t err = 0;

  while (x >= y) {
    DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
    DrawLine(x0 - y, y0 + x, x0 + y, y0 + x, color);
    DrawLine(x0 - x, y0 - y, x0 + x, y0 - y, color);
    DrawLine(x0 - y, y0 - x, x0 + y, y0 - x, color);

    y++;
    if (err <= 0)
      err += 2 * y + 1;
    else {
      x--;
      err += 2 * (y - x + 1);
    }
  }
}

// ========== 三角形 ==========
__attribute__((weak)) void DrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  uint16_t x2, uint16_t y2, COLOR color) {
  DrawLine(x0, y0, x1, y1, color);
  DrawLine(x1, y1, x2, y2, color);
  DrawLine(x2, y2, x0, y0, color);
}

// ========== 填充三角形 ==========
__attribute__((weak)) void DrawFilledTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                        uint16_t x2, uint16_t y2, COLOR color) {
    // 1. 排序 (y0 <= y1 <= y2)
    if (y0 > y1) { Swap(&y0, &y1); Swap(&x0, &x1); }
    if (y1 > y2) { Swap(&y1, &y2); Swap(&x1, &x2); }
    if (y0 > y1) { Swap(&y0, &y1); Swap(&x0, &x1); }

    if (y0 == y2) return;
    EdgeState e02, e01, e12;

    // 2. 初始化长边 (0->2)
    InitEdge(&e02, x0, y0, x2, y2);

    // 3. 填充上半部分 (y0 -> y1)
    if (y1 > y0) {
        InitEdge(&e01, x0, y0, x1, y1);
        for (int16_t y = y0; y < y1; y++) {
            int16_t startX = e02.x;
            int16_t endX = e01.x;
            if (startX > endX) { int16_t t = startX; startX = endX; endX = t; }
            DrawHLine(startX, y, endX - startX + 1, color);
            StepEdge(&e02);
            StepEdge(&e01);
        }
    }

    // 4. 填充下半部分 (y1 -> y2)
    InitEdge(&e12, x1, y1, x2, y2);
    for (int16_t y = y1; y <= y2; y++) {
        int16_t startX = e02.x;
        int16_t endX = e12.x;
        if (startX > endX) { int16_t t = startX; startX = endX; endX = t; }
        DrawHLine(startX, y, endX - startX + 1, color);
        StepEdge(&e02);
        StepEdge(&e12);
    }
}

// ========== 5x7 字体 ==========
static const uint8_t font5x7[] = {
    // 每个字符5x7，ASCII 32~127
    0x00, 0x00, 0x00, 0x00, 0x00, // ' '
    0x00, 0x00, 0x5F, 0x00, 0x00, // '!'
    0x00, 0x07, 0x00, 0x07, 0x00, // '"'
    0x14, 0x7F, 0x14, 0x7F, 0x14, // '#'
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // '$'
                                  // ...可自行补充（此处省略部分）
};

// 绘制单个字符
__attribute__((weak)) void EPD_DrawChar(uint16_t x, uint16_t y, char c, COLOR color) {
  if (c < 32 || c > 126)
    c = '?';
  const uint8_t *chr = &font5x7[(c - 32) * 5];
  pixel.x = x;
  pixel.color = color;
  for (uint8_t i = 0; i < 5; i++, pixel.x++) {
    pixel.y = y;
    for (uint8_t j = 0, line = chr[i]; j < 7; j++, pixel.y++) {
      if (line & 0x01) {
        DrawPixel(&pixel);
      }
      line >>= 1;
    }
  }
}

// 绘制字符串
__attribute__((weak)) void DrawString(uint16_t x, uint16_t y, const char *str, COLOR color) {
  while (*str) {
    DrawChar(x, y, *str, color);
    x += 6; // 字间距
    str++;
  }
}
