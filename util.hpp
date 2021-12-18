#ifndef UTIL_HPP
#define UTIL_HPP

// Represents a 2D size.
template <typename T>
struct Size {
  T width, height;

  Size() = default;
  Size(T w, T h)
      : width(w), height(h) {
  }

};

template <typename T>
struct Position {
    T x, y;

    Position() = default;
    Position(T _x, T _y) : x(_x), y(_y) {
    }

};

// Represents a 2D vector.
template <typename T>
struct Vector2D {
  T x, y;

  Vector2D() = default;
  Vector2D(T _x, T _y)
      : x(_x), y(_y) {
  }

};

#endif
