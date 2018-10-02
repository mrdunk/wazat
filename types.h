#ifndef WAZAT_TYPES_H
#define WAZAT_TYPES_H

#include <assert.h>

template <class T>
struct buffer {
  T   *start;
  size_t length;

  void resize(size_t length_) {
    if(length_ == length) {
      return;
    }
    delete[] (T*)start;
    length = length_;
    start = new T[length];
  }

  void clear() {
    memset(start, 0, length * sizeof(T));
  }

  void destroy() {
    if(length == 0) {
      return;
    }
    delete[] (T*)start;
    length = 0;
  }
};

struct polarCoord {
  int r;
  int16_t a;
  bool operator<(const polarCoord& rhs) const {
    assert(a >= -180 && a < 180);
    return a + 180 < rhs.a + 180 ||
      abs(r) < abs(rhs.r);
  }
};

#endif  // WAZAT_TYPES_H
