#pragma once
enum class Difficulty { Easy=0, Normal=1, Hard=2 };
inline const char *DifficultyLabel(Difficulty d) {
  return d==Difficulty::Easy?"EASY":d==Difficulty::Hard?"HARD":"NORMAL";
}
inline Difficulty CycleDifficulty(Difficulty d,int delta) {
  return Difficulty((int(d)+delta+3)%3);
}
inline bool DifficultyStartsArmed(Difficulty d) {return d==Difficulty::Hard;}
inline bool DifficultyCanDropBomb(Difficulty d,int threat) {
  return d==Difficulty::Hard||threat>0;
}
