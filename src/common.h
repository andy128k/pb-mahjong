#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define ROW_COUNT 18
#define COL_COUNT 32
#define MAX_HEIGHT 16

/*
  category 2-bit
  suit - 2-bit
  rank - 4-bit
*/
typedef unsigned char chip_t;

typedef struct tag_map {
  unsigned char map[ROW_COUNT][COL_COUNT];
} map_t;

extern map_t standard_map;

static inline int min_int(int x, int y)
{
  return x < y ? x : y;
}

static inline int max_int(int x, int y)
{
  return x > y ? x : y;
}

static inline int rrand(int m)
{
  return (int)((double)m * ( rand() / (RAND_MAX+1.0) ));
}

extern void shuffle(void *obj, size_t nmemb, size_t size);

#endif

