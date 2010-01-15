#ifndef BOARD_H
#define BOARD_H

/*
  category 2-bit
  suit - 2-bit
  rank - 4-bit
*/
typedef unsigned char chip_t;

#define MAX_ROW_COUNT 18
#define MAX_COL_COUNT 32
#define MAX_HEIGHT 16

typedef struct {
  chip_t chips[MAX_HEIGHT];
} column_t;

typedef struct {
  column_t columns[MAX_ROW_COUNT][MAX_COL_COUNT];
} board_t;

typedef struct {
  int y, x, k;
} position_t;

int position_equal(const position_t *pos1, const position_t *pos2);

typedef struct {
  position_t positions[144];
  int count;
} positions_t;

positions_t* get_selectable_positions(board_t *board);

chip_t safe_get(board_t *board, int y, int x, int k);
int column_height(board_t *board, int y, int x);
chip_t board_get(board_t *board, position_t *pos);
void board_set(board_t *board, position_t *pos, chip_t chip);

/*******************************************************/

typedef struct tag_map {
  const char *name;
  int row_count;
  int col_count;
  struct {
    int x, y, z;
  } map[144];
} map_t;

void generate_board(board_t *board, map_t *map);

#endif

