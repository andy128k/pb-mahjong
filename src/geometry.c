#include "common.h"

#include "geometry.h"

void union_rect(const struct rect* r1, const struct rect* r2, struct rect* r)
{
  r->x = min_int(r1->x, r2->x);
  r->y = min_int(r1->y, r2->y);
  r->w = max_int(r1->x + r1->w, r2->x + r2->w) - r->x;
  r->h = max_int(r1->y + r1->h, r2->y + r2->h) - r->y;
}

