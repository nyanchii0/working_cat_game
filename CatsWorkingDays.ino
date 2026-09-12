#include <Arduboy2.h>
#include <Arduboy2Beep.h>
#include "sprites.h"
#include "sounds.h"

Arduboy2 arduboy;

// Два пина: музыка — пин 1, эффекты — пин 2
BeepPin1 beepMusic;
BeepPin2 beepSfx;

// ---------------- Состояния ----------------
enum GameState { STATE_MENU, STATE_GAMEPLAY, STATE_GAMEOVER, STATE_VICTORY };
GameState currentState = STATE_MENU;

// ---------------- Стрелы ----------------
struct Arrow {
  int16_t x, y;
  Direction dir;
  int8_t speed;
  bool active;
};

const uint8_t MAX_ARROWS = 8;
Arrow arrows[MAX_ARROWS];

// ---------------- Баланс ----------------
int8_t   hp           = 3;
uint16_t score        = 0;
uint8_t  currentLevel = 1;
Direction shieldDir   = DIR_UP;

// Неуязвимость
bool     isInvincible      = false;
uint32_t invincibilityTimer = 0;
uint32_t abilityCooldown    = 0;

// Спавн
uint32_t lastArrowSpawnTime = 0;
uint16_t spawnInterval      = 1500;

// Центр котика
const int16_t CAT_CX = 64;
const int16_t CAT_CY = 32;

// ============================================================
void initGame() {
  hp = 3;
  score = 0;
  currentLevel = 1;
  shieldDir = DIR_UP;
  isInvincible = false;
  invincibilityTimer = 0;
  abilityCooldown = 0;
  spawnInterval = 1500;
  lastArrowSpawnTime = millis();
  for (uint8_t i = 0; i < MAX_ARROWS; i++) arrows[i].active = false;
}

// ============================================================
void spawnArrow() {
  for (uint8_t i = 0; i < MAX_ARROWS; i++) {
    if (arrows[i].active) continue;
    Direction d = (Direction)random(0, 4);
    arrows[i].dir    = d;
    arrows[i].active = true;
    arrows[i].speed  = 1 + currentLevel;

    switch (d) {
      case DIR_UP:    arrows[i].x = CAT_CX; arrows[i].y = -8;   break;
      case DIR_DOWN:  arrows[i].x = CAT_CX; arrows[i].y = 72;   break;
      case DIR_LEFT:  arrows[i].x = -8;     arrows[i].y = CAT_CY; break;
      case DIR_RIGHT: arrows[i].x = 136;    arrows[i].y = CAT_CY; break;
    }
    return;
  }
}

// ============================================================
void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);
  arduboy.initRandomSeed();
  arduboy.audio.begin();
  beepMusic.begin();
  beepSfx.begin();
  initGame();
}

// ============================================================
void loop() {
  if (!arduboy.nextFrame()) return;
  arduboy.pollButtons();

  // ОБЯЗАТЕЛЬНО каждый кадр
  beepMusic.timer();
  beepSfx.timer();

  arduboy.clear();

  switch (currentState) {

    // ---------------- МЕНЮ ----------------
    case STATE_MENU:
      arduboy.setCursor(8, 10);  arduboy.print("CAT'S WORKING DAYS");
      arduboy.setCursor(20, 35); arduboy.print("Press A to Start");
      arduboy.setCursor(0, 50);  arduboy.print("Happy Programmer Day!");
      if (arduboy.justPressed(A_BUTTON)) {
        playMenuSelect();
        initGame();
        currentState = STATE_GAMEPLAY;
      }
      break;

    // ---------------- ИГРА ----------------
    case STATE_GAMEPLAY: {
      // 1. Щит — 4 стрелки
      if (arduboy.pressed(UP_BUTTON))    shieldDir = DIR_UP;
      if (arduboy.pressed(DOWN_BUTTON))  shieldDir = DIR_DOWN;
      if (arduboy.pressed(LEFT_BUTTON))  shieldDir = DIR_LEFT;
      if (arduboy.pressed(RIGHT_BUTTON)) shieldDir = DIR_RIGHT;

      // 2. Неуязвимость (B)
      if (arduboy.justPressed(B_BUTTON) && millis() > abilityCooldown) {
        isInvincible = true;
        invincibilityTimer = millis() + 3000;
        abilityCooldown    = millis() + 10000;
        playInvulnOn();
      }
      if (isInvincible && millis() > invincibilityTimer) {
        isInvincible = false;
        playInvulnEnd();
      }

      // 3. Спавн стрел
      if (millis() - lastArrowSpawnTime > spawnInterval) {
        spawnArrow();
        lastArrowSpawnTime = millis();
      }

      // 4. Движение + коллизии
      for (uint8_t i = 0; i < MAX_ARROWS; i++) {
        if (!arrows[i].active) continue;

        if (arrows[i].dir == DIR_UP)    arrows[i].y += arrows[i].speed;
        if (arrows[i].dir == DIR_DOWN)  arrows[i].y -= arrows[i].speed;
        if (arrows[i].dir == DIR_LEFT)  arrows[i].x += arrows[i].speed;
        if (arrows[i].dir == DIR_RIGHT) arrows[i].x -= arrows[i].speed;

        // Попадание в котика (радиус ~10)
        bool nearCat = (abs(arrows[i].x - CAT_CX) < 10 &&
                        abs(arrows[i].y - CAT_CY) < 10);

        if (nearCat) {
          arrows[i].active = false;
          bool blocked = isInvincible || (shieldDir == arrows[i].dir);
          if (blocked) {
            score += 10;
            playShieldBlock();
          } else {
            hp--;
            playMissedHit();
          }
        }

        // Ушла за экран — удалить
        if (arrows[i].x < -16 || arrows[i].x > 144 ||
            arrows[i].y < -16 || arrows[i].y > 80) {
          arrows[i].active = false;
        }
      }

      // 5. Уровни
      if (score >= 100 && currentLevel == 1) { currentLevel = 2; spawnInterval = 1000; playLevelUp(); }
      if (score >= 250 && currentLevel == 2) { currentLevel = 3; spawnInterval = 600;  playLevelUp(); }
      if (score >= 500 && currentLevel == 3) { playVictory(); currentState = STATE_VICTORY; }

      // 6. Смерть
      if (hp <= 0) { playGameOver(); currentState = STATE_GAMEOVER; }

      // 7. Отрисовка
      // HUD — сердечки
      for (uint8_t i = 0; i < 3; i++) {
        drawHeart(2 + i * 10, 2, i < hp);
      }
      arduboy.setCursor(45, 2); arduboy.print("LVL:"); arduboy.print(currentLevel);
      arduboy.setCursor(90, 2); arduboy.print("PTS:"); arduboy.print(score);

      drawCat(CAT_CX, CAT_CY, isInvincible);
      drawShield(CAT_CX, CAT_CY, shieldDir);

      for (uint8_t i = 0; i < MAX_ARROWS; i++) {
        if (arrows[i].active) {
          drawArrow(arrows[i].x, arrows[i].y, arrows[i].dir);
        }
      }
      break;
    }

    // ---------------- GAME OVER ----------------
    case STATE_GAMEOVER:
      arduboy.setCursor(35, 15); arduboy.print("GAME OVER");
      arduboy.setCursor(25, 35); arduboy.print("SCORE: "); arduboy.print(score);
      arduboy.setCursor(20, 50); arduboy.print("Press A to Restart");
      if (arduboy.justPressed(A_BUTTON)) {
        playMenuSelect();
        initGame();
        currentState = STATE_GAMEPLAY;
      }
      break;

    // ---------------- ПОБЕДА ----------------
    case STATE_VICTORY:
      arduboy.setCursor(25, 15); arduboy.print("SENIOR CODER!");
      arduboy.setCursor(4, 35);  arduboy.print("You survived the day");
      arduboy.setCursor(20, 50); arduboy.print("Press A for Menu");
      if (arduboy.justPressed(A_BUTTON)) {
        playMenuSelect();
        currentState = STATE_MENU;
      }
      break;
  }

  arduboy.display();
}