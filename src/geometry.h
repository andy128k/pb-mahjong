#ifndef GEOMETRY_H
#define GEOMETRY_H

struct rect {
  int x, y, w, h;
};

void union_rect(const struct rect* r1, const struct rect* r2, struct rect* r);

#endif

