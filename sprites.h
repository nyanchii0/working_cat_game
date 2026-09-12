#ifndef SPRITES_H
#define SPRITES_H

#include <Arduboy2.h>

// ============================================================
// ВСЕ СПРАЙТЫ ИГРЫ
// Формат Arduboy: массив в PROGMEM, [W, H, байты...]
// Или просто байты, если W/H заданы константами ниже.
// Заменить любой блок — просто подставь новые байты.
// ============================================================

// Направление (нужно для стрел) — определено здесь,
// чтобы sprites.h не зависел от .ino
enum Direction { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

// ---------------- КОТИК-ПРОГРАММИСТ (8x8) ----------------
// ЗАМЕНИТЬ на спрайт от Вадима (можно 16x16 — тогда поменяй CAT_W/CAT_H)
#define CAT_W 8
#define CAT_H 8
const uint8_t PROGMEM catSprite[] = {
  0x3C, 0x7E, 0xDB, 0xFF, 0xDB, 0xBD, 0x42, 0x3C
};

// ---------------- СТРЕЛА / БАГ (8x8) ----------------
// ЗАМЕНИТЬ на спрайт от Вадима
#define ARROW_W 8
#define ARROW_H 8
const uint8_t PROGMEM arrowSprite[] = {
  0x18, 0x3C, 0x7E, 0xFF, 0x18, 0x18, 0x18, 0x18
};

// ---------------- СЕРДЕЧКО HP (8x8) ----------------
#define HEART_W 8
#define HEART_H 8
const uint8_t PROGMEM heartFull[] = {
  0x66, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00
};
const uint8_t PROGMEM heartEmpty[] = {
  0x66, 0x99, 0x81, 0x81, 0x42, 0x24, 0x18, 0x00
};

// ============================================================
// ФУНКЦИИ ОТРИСОВКИ — вызывай их из .ino
// ============================================================

inline void drawCat(int16_t cx, int16_t cy, bool invincible) {
  if (invincible) arduboy.drawCircle(cx, cy, 12, WHITE);
  arduboy.drawBitmap(cx - CAT_W / 2, cy - CAT_H / 2,
                     catSprite, CAT_W, CAT_H, WHITE);
}

inline void drawArrow(int16_t cx, int16_t cy, Direction d) {
  // Спрайт рисуется всегда вертикально. Поворот делаем через
  // выбор одной из 4 копий — Вадим может нарисовать 4 варианта.
  // Пока просто точка-заглушка для любой стороны:
  arduboy.drawBitmap(cx - ARROW_W / 2, cy - ARROW_H / 2,
                     arrowSprite, ARROW_W, ARROW_H, WHITE);
}

inline void drawShield(int16_t cx, int16_t cy, Direction d) {
  // Щит рисуем как линию со стороны, куда смотрит котик.
  // Вадим может заменить на спрайт-«лапку» или «< / >».
  const int16_t R = 12;
  switch (d) {
    case DIR_UP:    arduboy.drawFastHLine(cx - R, cy - R, R * 2, WHITE); break;
    case DIR_DOWN:  arduboy.drawFastHLine(cx - R, cy + R, R * 2, WHITE); break;
    case DIR_LEFT:  arduboy.drawFastVLine(cx - R, cy - R, R * 2, WHITE); break;
    case DIR_RIGHT: arduboy.drawFastVLine(cx + R, cy - R, R * 2, WHITE); break;
  }
}

inline void drawHeart(int16_t x, int16_t y, bool filled) {
  arduboy.drawBitmap(x, y, filled ? heartFull : heartEmpty,
                     HEART_W, HEART_H, WHITE);
}

#endif