#pragma once
#include <math.h>

inline float move_to(float from, float to, float dt, float vel)
{
  float d = vel * dt;
  if (fabsf(from - to) < d)
    return to;

  if (to < from)
    return from - d;
  else
    return from + d;
}

inline float clamp(float in, float min, float max)
{
  return in < min ? min : in > max ? max : in;
}

inline float sign(float in)
{
  return in > 0.f ? 1.f : in < 0.f ? -1.f : 0.f;
}

/**
* to > from.
* r < from - to.
*/
template<typename T>
inline T rotr(T X, uint8_t from, uint8_t to, uint8_t r)
{
    T ones = ~(X & 0);
    T mask_unchanged = (~(ones >> from)) | (ones >> to);
    T mask_changed = ~mask_unchanged;
    T unchanged = X & mask_unchanged;
    T changed = X & mask_changed;
    changed = (changed >> r) | (changed << (to - from - r));
    changed = changed & mask_changed;
    return changed | unchanged;
}

constexpr float PI = 3.141592654f;

