#pragma once

#include <vector>

typedef struct {
  unsigned x;
  unsigned y;
} coord_t;

unsigned interpolate(std::vector<coord_t> c, unsigned x) {
  unsigned min_x = c[0].x;
  unsigned max_x = c[c.size() - 1].x;

  if (x <= min_x) {
    return c[0].y;
  } else if (x >= max_x) {
    return c[c.size() - 1].y;
  } else {
    for (int i = 0; i < c.size() - 1; i++) {
      if (c[i].x <= x && c[i + 1].x >= x) {
        unsigned diffx = x - c[i].x;
        unsigned diffn = c[i + 1].x - c[i].x;

        return c[i].y + (c[i + 1].y - c[i].y) * diffx / diffn;
      }
    }
  }

  return 0;  // this should never occur
}
