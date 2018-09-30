#ifndef WAZAT_TYPES_H
#define WAZAT_TYPES_H

struct buffer {
  void   *start;
  size_t length;
};

struct polarCoord {
  uint8_t a;
  int r;
  bool operator<(const polarCoord& rhs) const {
    return a < rhs.a || r < rhs.r;
  }
};

#endif  // WAZAT_TYPES_H
