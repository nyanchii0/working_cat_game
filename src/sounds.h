#ifndef SOUNDS_H
#define SOUNDS_H

#include <Arduboy2.h>
#include <Arduboy2Beep.h>

// Объекты создаются в cat.ino — здесь только extern
extern BeepPin1 beepMusic;
extern BeepPin2 beepSfx;

// Длительность в КАДРАХ. 60 кадров = 1 сек.
inline void playShieldBlock() { beepSfx.tone(beepSfx.freq(880),  3); }
inline void playMissedHit()   { beepSfx.tone(beepSfx.freq(131), 12); }
inline void playInvulnOn()    { beepSfx.tone(beepSfx.freq(1047), 6); }
inline void playInvulnEnd()   { beepSfx.tone(beepSfx.freq(523),  4); }
inline void playBoxAbility()  { beepSfx.tone(beepSfx.freq(659),  8); }
inline void playLevelUp()     { beepSfx.tone(beepSfx.freq(784),  8); }
inline void playGameOver()    { beepSfx.tone(beepSfx.freq(196), 30); }
inline void playVictory()     { beepSfx.tone(beepSfx.freq(1047),15); }
inline void playMenuSelect()  { beepSfx.tone(beepSfx.freq(659),  4); }

// Фоновая музыка (если понадобится)
inline void playBgTick()      { beepMusic.tone(beepMusic.freq(110), 25); }

#endif