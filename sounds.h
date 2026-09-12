#ifndef SOUNDS_H
#define SOUNDS_H

#include <Arduboy2.h>
#include <Arduboy2Beep.h>

// Объекты создаются в .ino, здесь — только extern-объявления
extern BeepPin1 beepMusic;
extern BeepPin2 beepSfx;

// ============================================================
// ЗВУКОВЫЕ ЭФФЕКТЫ
// Формат: beepSfx.tone(beepSfx.freq(Гц), КАДРЫ)
// 60 кадров = 1 секунда. Примеры:
//   3 кадра ≈ 50 мс,  6 ≈ 100 мс,  15 ≈ 250 мс,  30 ≈ 500 мс
// Все эффекты идут на beepSfx (пин 2), музыка — на beepMusic (пин 1)
// ============================================================

// Успешный блок щитом — короткий приятный «дзинь»
inline void playShieldBlock() { beepSfx.tone(beepSfx.freq(880), 3); }

// Пропущенный удар — низкий грубый звук
inline void playMissedHit()   { beepSfx.tone(beepSfx.freq(131), 12); }

// Активация неуязвимости — высокий «взлёт»
inline void playInvulnOn()    { beepSfx.tone(beepSfx.freq(1047), 6); }

// Конец неуязвимости
inline void playInvulnEnd()   { beepSfx.tone(beepSfx.freq(523), 4); }

// Переход на новый уровень
inline void playLevelUp()     { beepSfx.tone(beepSfx.freq(784), 8); }

// Game Over — мрачный низкий
inline void playGameOver()    { beepSfx.tone(beepSfx.freq(196), 30); }

// Победа — торжественный писк
inline void playVictory()     { beepSfx.tone(beepSfx.freq(1047), 15); }

// Нажатие в меню
inline void playMenuSelect()  { beepSfx.tone(beepSfx.freq(659), 4); }

// ============================================================
// ФОНОВАЯ МУЗЫКА (пин 1, beepMusic)
// Вызывать playBgTick() раз в ~30 кадров из loop, если нужен
// фоновый гул во время игры.
// ============================================================
inline void playBgTick() { beepMusic.tone(beepMusic.freq(110), 25); }

#endif