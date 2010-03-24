#ifndef GEOMETRY_H
#define GEOMETRY_H

struct rect {
  int x, y, w, h;
};

void union_rect(const struct rect* r1, const struct rect* r2, struct rect* r);

void point_change_orientation(int x, int y, int orientation, int *rx, int *ry);

int point_in_rect(int x, int y, const struct rect* r);

#endif

