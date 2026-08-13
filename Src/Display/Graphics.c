#include <stdlib.h>
#include <Common.h>
#include <Display/Graphics.h>

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
  Pixel pixel;
  uint16_t index;

  if (x0 > x1) {
    return;
  }
  index = x0;
  for (;;) {
    pixel.x = index;
    pixel.y = y0;
    pixel.color = color;
    DrawPixel(&pixel);
    if (index == x1) {
      break;
    }
    ++index;
  }
}

// ========== 垂直线 ==========
__attribute__((weak)) void DrawVLine(uint16_t x0, uint16_t y0, uint16_t y1,
                                     COLOR color) {
  Pixel pixel;
  uint16_t index;

  if (y0 > y1) {
    return;
  }
  index = y0;
  for (;;) {
    pixel.x = x0;
    pixel.y = index;
    pixel.color = color;
    DrawPixel(&pixel);
    if (index == y1) {
      break;
    }
    ++index;
  }
}

// ========== 直线（Bresenham） ==========
__attribute__((weak)) void DrawLine(uint16_t x0, uint16_t y0, uint16_t x1,
                                    uint16_t y1, COLOR color) {
  Pixel pixel;

  if (y0 == y1) {
    DrawHLine(x0 < x1 ? x0 : x1, y0, x0 < x1 ? x1 : x0, color);
    return;
  }
  if (x0 == x1) {
    DrawVLine(x0, y0 < y1 ? y0 : y1, y0 < y1 ? y1 : y0, color);
    return;
  }

  int dx = abs((int32_t)x1 - (int32_t)x0);
  int dy = -abs((int32_t)y1 - (int32_t)y0);
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
  if ((w == 0U) || (h == 0U) ||
      (((uint32_t)x + w - 1U) > UINT16_MAX) ||
      (((uint32_t)y + h - 1U) > UINT16_MAX)) {
    return;
  }
  DrawLine(x, y, x + w - 1, y, color);
  DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
  DrawLine(x, y, x, y + h - 1, color);
  DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

// ========== 填充矩形 ==========
__attribute__((weak)) void DrawFilledRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        COLOR color) {
  Pixel pixel;
  if ((w == 0U) || (h == 0U) ||
      (((uint32_t)x + w - 1U) > UINT16_MAX) ||
      (((uint32_t)y + h - 1U) > UINT16_MAX)) {
    return;
  }
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
  Pixel pixel;
  int32_t x;
  int32_t y = 0;
  int32_t err = 0;

  if ((r > INT16_MAX) || (r > x0) || (r > y0) ||
      (((uint32_t)x0 + r) > UINT16_MAX) ||
      (((uint32_t)y0 + r) > UINT16_MAX)) {
    return;
  }
  x = r;
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
  int32_t x;
  int32_t y = 0;
  int32_t err = 0;

  if ((r > INT16_MAX) || (r > x0) || (r > y0) ||
      (((uint32_t)x0 + r) > UINT16_MAX) ||
      (((uint32_t)y0 + r) > UINT16_MAX)) {
    return;
  }
  x = r;

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
    if ((x0 > INT16_MAX) || (y0 > INT16_MAX) || (x1 > INT16_MAX) ||
        (y1 > INT16_MAX) || (x2 > INT16_MAX) || (y2 > INT16_MAX)) {
        return;
    }
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
        for (int32_t y = y0; y < y1; y++) {
            int16_t startX = e02.x;
            int16_t endX = e01.x;
            if (startX > endX) { int16_t t = startX; startX = endX; endX = t; }
            DrawHLine((uint16_t)startX, (uint16_t)y, (uint16_t)endX, color);
            StepEdge(&e02);
            StepEdge(&e01);
        }
    }

    // 4. 填充下半部分 (y1 -> y2)
    InitEdge(&e12, x1, y1, x2, y2);
    for (int32_t y = y1; y <= y2; y++) {
        int16_t startX = e02.x;
        int16_t endX = e12.x;
        if (startX > endX) { int16_t t = startX; startX = endX; endX = t; }
        DrawHLine((uint16_t)startX, (uint16_t)y, (uint16_t)endX, color);
        StepEdge(&e02);
        StepEdge(&e12);
    }
}

// ========== 5x7 字体（ASCII 32~126，列优先，低位在上）==========
static const uint8_t font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // ' '
    0x00, 0x00, 0x5F, 0x00, 0x00, // '!'
    0x00, 0x07, 0x00, 0x07, 0x00, // '"'
    0x14, 0x7F, 0x14, 0x7F, 0x14, // '#'
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // '$'
    0x23, 0x13, 0x08, 0x64, 0x62, // '%'
    0x36, 0x49, 0x55, 0x22, 0x50, // '&'
    0x00, 0x05, 0x03, 0x00, 0x00, // '\''
    0x00, 0x1C, 0x22, 0x41, 0x00, // '('
    0x00, 0x41, 0x22, 0x1C, 0x00, // ')'
    0x08, 0x2A, 0x1C, 0x2A, 0x08, // '*'
    0x08, 0x08, 0x3E, 0x08, 0x08, // '+'
    0x00, 0x50, 0x30, 0x00, 0x00, // ','
    0x08, 0x08, 0x08, 0x08, 0x08, // '-'
    0x00, 0x60, 0x60, 0x00, 0x00, // '.'
    0x20, 0x10, 0x08, 0x04, 0x02, // '/'
    0x3E, 0x51, 0x49, 0x45, 0x3E, // '0'
    0x00, 0x42, 0x7F, 0x40, 0x00, // '1'
    0x42, 0x61, 0x51, 0x49, 0x46, // '2'
    0x21, 0x41, 0x45, 0x4B, 0x31, // '3'
    0x18, 0x14, 0x12, 0x7F, 0x10, // '4'
    0x27, 0x45, 0x45, 0x45, 0x39, // '5'
    0x3C, 0x4A, 0x49, 0x49, 0x30, // '6'
    0x01, 0x71, 0x09, 0x05, 0x03, // '7'
    0x36, 0x49, 0x49, 0x49, 0x36, // '8'
    0x06, 0x49, 0x49, 0x29, 0x1E, // '9'
    0x00, 0x36, 0x36, 0x00, 0x00, // ':'
    0x00, 0x56, 0x36, 0x00, 0x00, // ';'
    0x00, 0x08, 0x14, 0x22, 0x41, // '<'
    0x14, 0x14, 0x14, 0x14, 0x14, // '='
    0x41, 0x22, 0x14, 0x08, 0x00, // '>'
    0x02, 0x01, 0x51, 0x09, 0x06, // '?'
    0x32, 0x49, 0x79, 0x41, 0x3E, // '@'
    0x7E, 0x11, 0x11, 0x11, 0x7E, // 'A'
    0x7F, 0x49, 0x49, 0x49, 0x36, // 'B'
    0x3E, 0x41, 0x41, 0x41, 0x22, // 'C'
    0x7F, 0x41, 0x41, 0x22, 0x1C, // 'D'
    0x7F, 0x49, 0x49, 0x49, 0x41, // 'E'
    0x7F, 0x09, 0x09, 0x01, 0x01, // 'F'
    0x3E, 0x41, 0x41, 0x51, 0x32, // 'G'
    0x7F, 0x08, 0x08, 0x08, 0x7F, // 'H'
    0x00, 0x41, 0x7F, 0x41, 0x00, // 'I'
    0x20, 0x40, 0x41, 0x3F, 0x01, // 'J'
    0x7F, 0x08, 0x14, 0x22, 0x41, // 'K'
    0x7F, 0x40, 0x40, 0x40, 0x40, // 'L'
    0x7F, 0x02, 0x04, 0x02, 0x7F, // 'M'
    0x7F, 0x04, 0x08, 0x10, 0x7F, // 'N'
    0x3E, 0x41, 0x41, 0x41, 0x3E, // 'O'
    0x7F, 0x09, 0x09, 0x09, 0x06, // 'P'
    0x3E, 0x41, 0x51, 0x21, 0x5E, // 'Q'
    0x7F, 0x09, 0x19, 0x29, 0x46, // 'R'
    0x46, 0x49, 0x49, 0x49, 0x31, // 'S'
    0x01, 0x01, 0x7F, 0x01, 0x01, // 'T'
    0x3F, 0x40, 0x40, 0x40, 0x3F, // 'U'
    0x1F, 0x20, 0x40, 0x20, 0x1F, // 'V'
    0x7F, 0x20, 0x18, 0x20, 0x7F, // 'W'
    0x63, 0x14, 0x08, 0x14, 0x63, // 'X'
    0x03, 0x04, 0x78, 0x04, 0x03, // 'Y'
    0x61, 0x51, 0x49, 0x45, 0x43, // 'Z'
    0x00, 0x00, 0x7F, 0x41, 0x41, // '['
    0x02, 0x04, 0x08, 0x10, 0x20, // '\'
    0x41, 0x41, 0x7F, 0x00, 0x00, // ']'
    0x04, 0x02, 0x01, 0x02, 0x04, // '^'
    0x40, 0x40, 0x40, 0x40, 0x40, // '_'
    0x00, 0x01, 0x02, 0x04, 0x00, // '`'
    0x20, 0x54, 0x54, 0x54, 0x78, // 'a'
    0x7F, 0x48, 0x44, 0x44, 0x38, // 'b'
    0x38, 0x44, 0x44, 0x44, 0x20, // 'c'
    0x38, 0x44, 0x44, 0x48, 0x7F, // 'd'
    0x38, 0x54, 0x54, 0x54, 0x18, // 'e'
    0x08, 0x7E, 0x09, 0x01, 0x02, // 'f'
    0x08, 0x14, 0x54, 0x54, 0x3C, // 'g'
    0x7F, 0x08, 0x04, 0x04, 0x78, // 'h'
    0x00, 0x44, 0x7D, 0x40, 0x00, // 'i'
    0x20, 0x40, 0x44, 0x3D, 0x00, // 'j'
    0x00, 0x7F, 0x10, 0x28, 0x44, // 'k'
    0x00, 0x41, 0x7F, 0x40, 0x00, // 'l'
    0x7C, 0x04, 0x18, 0x04, 0x78, // 'm'
    0x7C, 0x08, 0x04, 0x04, 0x78, // 'n'
    0x38, 0x44, 0x44, 0x44, 0x38, // 'o'
    0x7C, 0x14, 0x14, 0x14, 0x08, // 'p'
    0x08, 0x14, 0x14, 0x18, 0x7C, // 'q'
    0x7C, 0x08, 0x04, 0x04, 0x08, // 'r'
    0x48, 0x54, 0x54, 0x54, 0x20, // 's'
    0x04, 0x3F, 0x44, 0x40, 0x20, // 't'
    0x3C, 0x40, 0x40, 0x20, 0x7C, // 'u'
    0x1C, 0x20, 0x40, 0x20, 0x1C, // 'v'
    0x3C, 0x40, 0x30, 0x40, 0x3C, // 'w'
    0x44, 0x28, 0x10, 0x28, 0x44, // 'x'
    0x0C, 0x50, 0x50, 0x50, 0x3C, // 'y'
    0x44, 0x64, 0x54, 0x4C, 0x44, // 'z'
    0x00, 0x08, 0x36, 0x41, 0x00, // '{'
    0x00, 0x00, 0x7F, 0x00, 0x00, // '|'
    0x00, 0x41, 0x36, 0x08, 0x00, // '}'
    0x08, 0x08, 0x2A, 0x1C, 0x08, // '~'
};

// 绘制单个字符
__attribute__((weak)) void DrawChar(uint16_t x, uint16_t y, char c, COLOR color) {
  Pixel pixel;
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
  if (str == NULL) {
    return;
  }
  while (*str) {
    DrawChar(x, y, *str, color);
    if (x > (UINT16_MAX - 6U)) {
      break;
    }
    x += 6; // 字间距
    str++;
  }
}
