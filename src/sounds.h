#ifndef SOUNDS_H
#define SOUNDS_H

#include <Arduboy2.h>
#include <Arduboy2Beep.h>

// Объекты создаются в .ino — здесь только extern
extern BeepPin1 beepMusic;
extern BeepPin2 beepSfx;

// Длительность в КАДРАХ. 60 кадров = 1 сек.
//
// ВАЖНО: все звуки временно переведены на beepMusic (пин 5, BeepPin1) —
// на большинстве самодельных Arduboy реально распаян только один динамик,
// именно на этом пине. beepSfx (пин 13, BeepPin2) оставлен объявленным
// на случай, если у вашей платы физически два динамика — тогда можно
// будет часть функций вернуть обратно на beepSfx.tone(...).

inline void playShieldBlock() { beepMusic.tone(beepMusic.freq(880),  3); }
inline void playMissedHit()   { beepMusic.tone(beepMusic.freq(131), 12); }
inline void playInvulnOn()    { beepMusic.tone(beepMusic.freq(1047), 6); }
inline void playInvulnEnd()   { beepMusic.tone(beepMusic.freq(523),  4); }
inline void playBoxAbility()  { beepMusic.tone(beepMusic.freq(659),  8); }
inline void playLevelUp()     { beepMusic.tone(beepMusic.freq(784),  8); }
inline void playGameOver()    { beepMusic.tone(beepMusic.freq(196), 30); }
inline void playVictory()     { beepMusic.tone(beepMusic.freq(1047),15); }
inline void playMenuSelect()  { beepMusic.tone(beepMusic.freq(659),  4); }

// Фоновая музыка (если понадобится) — тоже на beepMusic
inline void playBgTick()      { beepMusic.tone(beepMusic.freq(110), 25); }

#endif
