#include <Arduboy2.h>
#include <Arduboy2Beep.h>
#include "cat.h"
#include "enemy.h"
#include "hud_sprites.h"  // сердечки HP + иконка способности B + заглушка GAME OVER
// Ramka.h убрана по решению команды — рамка больше не используется

Arduboy2 arduboy;
BeepPin1 beepMusic;   // канал 1: музыка/фон
BeepPin2 beepSfx;     // канал 2: эффекты

#include "sounds.h"   // подключать ПОСЛЕ объектов beep

// ---------------- Состояния ----------------
enum GameState {
  STATE_MENU,
  STATE_CONTROLS,
  STATE_GAMEPLAY,
  STATE_LEVELUP,
  STATE_RANK,       // экран "твой ранг" — показывается ПЕРЕД game over
  STATE_GAMEOVER,
  STATE_VICTORY
};
GameState currentState = STATE_MENU;

enum Direction { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

struct Arrow {
  int16_t x, y;
  Direction dir;
  int8_t speed;
  bool active;
};

// ---------------- Константы ----------------
#define CAT_W   32
#define CAT_H   32
#define ENEMY_W 32
#define ENEMY_H 32

const int16_t CAT_CX = 64;
const int16_t CAT_CY = 32;
const int16_t HIT_RADIUS = 12;

const uint8_t  MAX_ARROWS         = 6;
const uint16_t LEVEL_DURATION_MS  = 30000;  // 30 сек на уровень (было 20)
const uint16_t INVULN_DURATION_MS = 2000;   // 2 сек невидимости
const uint16_t INVULN_COOLDOWN_MS = 8000;
const uint16_t BOX_DURATION_MS    = 2000;   // 2 сек коробки
const uint16_t BOX_COOLDOWN_MS    = 12000;  // было 10000
const uint16_t LEVELUP_PAUSE_MS   = 1500;

// Скорость врагов по уровням (было: 1 + currentLevel = 2/3/4 — слишком быстро)
// index 0 не используется, уровни 1..3
const int8_t ARROW_SPEED_BY_LEVEL[4] = { 0, 1, 2, 3 };

// Интервал спавна по уровням, мс (слегка увеличен на 1 lvl, чтобы дать простор на реакцию)
const uint16_t SPAWN_INTERVAL_BY_LEVEL[4] = { 0, 1800, 1200, 700 };

// Анимация попадания: 2 мигания кота
const uint16_t HIT_FLASH_TOGGLE_MS = 100;  // длительность одной фазы (видим/не видим)
const uint8_t  HIT_FLASH_TOGGLES   = 4;    // 4 переключения = 2 полных мигания

Arrow arrows[MAX_ARROWS];

// ---------------- Игровые переменные ----------------
int8_t   hp            = 3;
uint16_t score         = 0;
uint16_t blockedCount  = 0;
uint8_t  currentLevel  = 1;
Direction shieldDir    = DIR_UP;

// Таймеры способностей
uint32_t invulnEndMs      = 0;
uint32_t invulnCooldownMs = 0;
uint32_t boxEndMs         = 0;
uint32_t boxCooldownMs    = 0;

// Таймеры уровня / спавна
uint32_t levelStartMs     = 0;
uint32_t levelupPauseMs   = 0;
uint32_t lastArrowSpawnMs = 0;
uint16_t spawnInterval    = 1800;

// Анимация попадания
uint32_t hitFlashStartMs  = 0;
bool     hitFlashActive   = false;

// ============================================================
void initGame() {
  hp = 3;
  score = 0;
  blockedCount = 0;
  currentLevel = 1;
  shieldDir = DIR_UP;

  invulnEndMs = invulnCooldownMs = 0;
  boxEndMs    = boxCooldownMs    = 0;

  hitFlashActive = false;

  spawnInterval    = SPAWN_INTERVAL_BY_LEVEL[1];
  levelStartMs     = millis();
  lastArrowSpawnMs = millis();

  for (uint8_t i = 0; i < MAX_ARROWS; i++) arrows[i].active = false;
}

void resetArrows() {
  for (uint8_t i = 0; i < MAX_ARROWS; i++) arrows[i].active = false;
}

// ============================================================
void spawnArrow() {
  for (uint8_t i = 0; i < MAX_ARROWS; i++) {
    if (arrows[i].active) continue;
    Direction d = (Direction)random(0, 4);
    arrows[i].dir    = d;
    arrows[i].active = true;
    arrows[i].speed  = ARROW_SPEED_BY_LEVEL[currentLevel];

    switch (d) {
      case DIR_UP:    arrows[i].x = CAT_CX;          arrows[i].y = -ENEMY_H/2;      break;
      case DIR_DOWN:  arrows[i].x = CAT_CX;          arrows[i].y = 64 + ENEMY_H/2;  break;
      case DIR_LEFT:  arrows[i].x = -ENEMY_W/2;      arrows[i].y = CAT_CY;          break;
      case DIR_RIGHT: arrows[i].x = 128 + ENEMY_W/2; arrows[i].y = CAT_CY;          break;
    }
    return;
  }
}

// ============================================================
bool invulnActive() { return millis() < invulnEndMs; }
bool boxActive()    { return millis() < boxEndMs; }
bool canUseA()      { return currentLevel >= 2 && millis() > invulnCooldownMs; }
bool canUseB()      { return currentLevel >= 3 && millis() > boxCooldownMs; }

// ============================================================
void startLevel(uint8_t lvl) {
  currentLevel = lvl;
  hp = 3;                    // HP восстанавливается
  shieldDir = DIR_UP;
  hitFlashActive = false;
  resetArrows();
  levelStartMs     = millis();
  lastArrowSpawnMs = millis();
  spawnInterval    = SPAWN_INTERVAL_BY_LEVEL[lvl];
}

// ============================================================
const char* rankTitle(uint16_t blocked) {
  if (blocked < 10) return "TRAINEE";
  if (blocked < 15) return "JUNIOR CODER";
  if (blocked < 20) return "MIDDLE CODER";
  if (blocked < 26) return "SENIOR CODER";
  return "ARCHITECT";
}

// ============================================================
void setup() {
  arduboy.begin();
  arduboy.setFrameRate(60);
  arduboy.initRandomSeed();
  arduboy.audio.begin();
  arduboy.audio.on();   // форсируем звук — в свежем EEPROM/некоторых эмуляторах бывает mute по умолчанию
  beepMusic.begin();
  beepSfx.begin();
  initGame();
}

// ============================================================
void loop() {
  if (!arduboy.nextFrame()) return;
  arduboy.pollButtons();
  beepMusic.timer();
  beepSfx.timer();

  arduboy.clear();

  switch (currentState) {

    // ---------------- МЕНЮ ----------------
    case STATE_MENU:
      arduboy.setCursor(8, 0);   arduboy.print("CAT'S WORKING DAYS");
      arduboy.drawBitmap(64 - CAT_W/2, 10, catSprite, CAT_W, CAT_H, WHITE);
      arduboy.setCursor(14, 46); arduboy.print("A: Start");
      arduboy.setCursor(14, 56); arduboy.print("B: Controls");

      if (arduboy.justPressed(A_BUTTON)) {
        playMenuSelect();
        initGame();
        currentState = STATE_GAMEPLAY;
      }
      if (arduboy.justPressed(B_BUTTON)) {
        playMenuSelect();
        currentState = STATE_CONTROLS;
      }
      break;

    // ---------------- КОНТРОЛС ----------------
    case STATE_CONTROLS:
      arduboy.setCursor(32, 4);  arduboy.print("CONTROLS");
      arduboy.setCursor(2, 20);  arduboy.print("Arrows: aim shield");
      arduboy.setCursor(2, 30);  arduboy.print("A: invuln (lvl 2+)");
      arduboy.setCursor(2, 40);  arduboy.print("B: box (lvl 3+)");
      arduboy.setCursor(2, 56);  arduboy.print("Press B to back");

      if (arduboy.justPressed(B_BUTTON)) {
        playMenuSelect();
        currentState = STATE_MENU;
      }
      break;

    // ---------------- ИГРА ----------------
    case STATE_GAMEPLAY: {
      uint32_t now = millis();

      // 1. Щит
      if (arduboy.pressed(UP_BUTTON))    shieldDir = DIR_UP;
      if (arduboy.pressed(DOWN_BUTTON))  shieldDir = DIR_DOWN;
      if (arduboy.pressed(LEFT_BUTTON))  shieldDir = DIR_LEFT;
      if (arduboy.pressed(RIGHT_BUTTON)) shieldDir = DIR_RIGHT;

      // 2. Способности (A: инвиз с 2 lvl, B: коробка с 3 lvl)
      if (arduboy.justPressed(A_BUTTON) && canUseA()) {
        invulnEndMs      = now + INVULN_DURATION_MS;
        invulnCooldownMs = now + INVULN_COOLDOWN_MS;
        playInvulnOn();
      }
      if (arduboy.justPressed(B_BUTTON) && canUseB()) {
        boxEndMs      = now + BOX_DURATION_MS;
        boxCooldownMs = now + BOX_COOLDOWN_MS;
        playBoxAbility();
      }

      // 3. Спавн
      if (now - lastArrowSpawnMs > spawnInterval) {
        spawnArrow();
        lastArrowSpawnMs = now;
      }

      // 4. Движение + коллизии
      for (uint8_t i = 0; i < MAX_ARROWS; i++) {
        if (!arrows[i].active) continue;

        if (arrows[i].dir == DIR_UP)    arrows[i].y += arrows[i].speed;
        if (arrows[i].dir == DIR_DOWN)  arrows[i].y -= arrows[i].speed;
        if (arrows[i].dir == DIR_LEFT)  arrows[i].x += arrows[i].speed;
        if (arrows[i].dir == DIR_RIGHT) arrows[i].x -= arrows[i].speed;

        bool nearCat = (abs(arrows[i].x - CAT_CX) < HIT_RADIUS &&
                        abs(arrows[i].y - CAT_CY) < HIT_RADIUS);

        if (nearCat) {
          arrows[i].active = false;
          bool defended = invulnActive() ||
                          boxActive()     ||
                          (shieldDir == arrows[i].dir);
          if (defended) {
            score += 10;
            blockedCount++;
            playShieldBlock();
          } else {
            hp--;
            playMissedHit();
            hitFlashActive  = true;
            hitFlashStartMs = now;
          }
        }

        if (arrows[i].x < -ENEMY_W || arrows[i].x > 128 + ENEMY_W ||
            arrows[i].y < -ENEMY_H || arrows[i].y > 64  + ENEMY_H) {
          arrows[i].active = false;
        }
      }

      // 5. Смерть
      if (hp <= 0) {
        playGameOver();
        currentState = STATE_RANK;
        break;
      }

      // 6. Уровень пройден?
      if (now - levelStartMs > LEVEL_DURATION_MS) {
        if (currentLevel >= 3) {
          playVictory();
          currentState = STATE_VICTORY;
        } else {
          playLevelUp();
          levelupPauseMs = now;
          currentState = STATE_LEVELUP;
        }
        break;
      }

      // 7. Отрисовка

      // Прогресс-бар уровня
      uint16_t progress = (uint16_t)((now - levelStartMs) * 128UL / LEVEL_DURATION_MS);
      arduboy.fillRect(0, 0, progress > 128 ? 128 : progress, 2, WHITE);

      // Котик (с миганием при попадании)
      bool catVisible = true;
      if (hitFlashActive) {
        uint32_t elapsed = now - hitFlashStartMs;
        uint8_t toggle = elapsed / HIT_FLASH_TOGGLE_MS;
        if (toggle >= HIT_FLASH_TOGGLES) {
          hitFlashActive = false;
        } else {
          catVisible = (toggle % 2 == 0);
        }
      }

      if (invulnActive()) arduboy.drawCircle(CAT_CX, CAT_CY, 18, WHITE);
      if (catVisible) {
        arduboy.drawBitmap(CAT_CX - CAT_W/2, CAT_CY - CAT_H/2,
                           catSprite, CAT_W, CAT_H, WHITE);
      }

      // Коробка-щит
      if (boxActive()) {
        arduboy.drawRect(CAT_CX - 18, CAT_CY - 18, 36, 36, WHITE);
        arduboy.drawRect(CAT_CX - 16, CAT_CY - 16, 32, 32, WHITE);
      }

      // Щит-линия по направлению
      switch (shieldDir) {
        case DIR_UP:    arduboy.drawFastHLine(CAT_CX - 16, CAT_CY - 16, 32, WHITE); break;
        case DIR_DOWN:  arduboy.drawFastHLine(CAT_CX - 16, CAT_CY + 16, 32, WHITE); break;
        case DIR_LEFT:  arduboy.drawFastVLine(CAT_CX - 16, CAT_CY - 16, 32, WHITE); break;
        case DIR_RIGHT: arduboy.drawFastVLine(CAT_CX + 16, CAT_CY - 16, 32, WHITE); break;
      }

      // Враги
      for (uint8_t i = 0; i < MAX_ARROWS; i++) {
        if (arrows[i].active) {
          arduboy.drawBitmap(arrows[i].x - ENEMY_W/2,
                             arrows[i].y - ENEMY_H/2,
                             enemySprite, ENEMY_W, ENEMY_H, WHITE);
        }
      }

      // HUD
      for (uint8_t i = 0; i < 3; i++) {
        arduboy.drawBitmap(2 + i * 10, 2, (i < hp) ? heartFull : heartEmpty, HEART_W, HEART_H, WHITE);
      }
      arduboy.setCursor(45, 4);  arduboy.print("LVL:"); arduboy.print(currentLevel);
      arduboy.setCursor(88, 4);  arduboy.print("PTS:"); arduboy.print(score);

      // Иконка способности B — показываем, только когда её реально можно применить
      if (currentLevel >= 3 && canUseB()) {
        arduboy.drawBitmap(108, 46, boxIconSprite, BOX_ICON_W, BOX_ICON_H, WHITE);
      }
      break;
    }

    // ---------------- ПАУЗА МЕЖДУ УРОВНЯМИ ----------------
    case STATE_LEVELUP:
      arduboy.setCursor(28, 14); arduboy.print("LEVEL UP!");
      arduboy.setCursor(34, 30); arduboy.print("LVL "); arduboy.print(currentLevel + 1);
      arduboy.setCursor(14, 46); arduboy.print("HP restored!");

      if (millis() - levelupPauseMs > LEVELUP_PAUSE_MS) {
        startLevel(currentLevel + 1);
        currentState = STATE_GAMEPLAY;
      }
      break;

    // ---------------- ЭКРАН РАНГА (перед Game Over) ----------------
    case STATE_RANK:
      arduboy.setCursor(20, 6);  arduboy.print("YOUR RANK:");
      arduboy.setCursor(2, 22);  arduboy.print(rankTitle(blockedCount));
      arduboy.setCursor(2, 38);  arduboy.print("Blocked: "); arduboy.print(blockedCount);
      arduboy.setCursor(2, 56);  arduboy.print("Press A to continue");

      if (arduboy.justPressed(A_BUTTON)) {
        currentState = STATE_GAMEOVER;
      }
      break;

    // ---------------- GAME OVER ----------------
    case STATE_GAMEOVER:
      // TODO: когда Вадим дорисует gameOverSprite (сейчас заглушка из нулей в hud_sprites.h),
      // заменить строку ниже на:
      // arduboy.drawBitmap(64 - GAMEOVER_W/2, 4, gameOverSprite, GAMEOVER_W, GAMEOVER_H, WHITE);
      arduboy.setCursor(35, 10); arduboy.print("GAME OVER");
      arduboy.setCursor(20, 26); arduboy.print("SCORE: ");   arduboy.print(score);
      arduboy.setCursor(20, 38); arduboy.print("BLOCKED: "); arduboy.print(blockedCount);
      arduboy.setCursor(2, 54);  arduboy.print("A: continue  B: menu");

      if (arduboy.justPressed(A_BUTTON)) {
        playMenuSelect();
        initGame();
        currentState = STATE_GAMEPLAY;
      }
      if (arduboy.justPressed(B_BUTTON)) {
        playMenuSelect();
        currentState = STATE_MENU;
      }
      break;

    // ---------------- ПОБЕДА ----------------
    case STATE_VICTORY:
      arduboy.setCursor(34, 4);  arduboy.print("VICTORY!");
      arduboy.setCursor(2, 18);  arduboy.print("Rank: ");    arduboy.print(rankTitle(blockedCount));
      arduboy.setCursor(2, 30);  arduboy.print("Blocked: "); arduboy.print(blockedCount);
      arduboy.setCursor(2, 42);  arduboy.print("Score: ");   arduboy.print(score);
      arduboy.setCursor(2, 56);  arduboy.print("Press A for menu");

      if (arduboy.justPressed(A_BUTTON)) {
        playMenuSelect();
        currentState = STATE_MENU;
      }
      break;
  }

  arduboy.display();
}
