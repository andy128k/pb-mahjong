#include "common.h"

#include "geometry.h"

void union_rect(const struct rect* r1, const struct rect* r2, struct rect* r)
{
  r->x = min_int(r1->x, r2->x);
  r->y = min_int(r1->y, r2->y);
  r->w = max_int(r1->x + r1->w, r2->x + r2->w) - r->x;
  r->h = max_int(r1->y + r1->h, r2->y + r2->h) - r->y;
}

void point_change_orientation(int x, int y, int orientation, int *rx, int *ry)
{
  const int w = 600;
  const int h = 800;
  switch (orientation)
    {
    case 0: /* portrait */
      *rx = x;
      *ry = y;
      break;
    case 1: /* landscape 90 */
      *rx = y;
      *ry = w - x;
      break;
    case 2: /* landscape 270 */
      *rx = h - y;
      *ry = x;
      break;
    case 3: /* portrait 180 */
      *rx = w - x;
      *ry = h - y;
      break;
    }
}

int point_in_rect(int x, int y, const struct rect* r)
{
  return
    (x >= r->x && x < r->x + r->w) &&
    (y >= r->y && y < r->y + r->h);
}

