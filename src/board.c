#include <string.h>
#include "board.h"
#include "common.h"

int position_equal(const position_t *pos1, const position_t *pos2)
{
  return pos1->x == pos2->x
    && pos1->y == pos2->y
    && pos1->k == pos2->k;
}

chip_t board_get(const board_t *board, const position_t *pos)
{
  if (pos->y < 0 || pos->y >= MAX_ROW_COUNT)
    return 0;
  if (pos->x < 0 || pos->x >= MAX_COL_COUNT)
    return 0;
  if (pos->k < 0 || pos->k >= MAX_HEIGHT)
    return 0;

  return board->columns[pos->y][pos->x].chips[pos->k];
}

void board_set(board_t *board, const position_t *pos, chip_t chip)
{
  if (pos->y < 0 || pos->y >= MAX_ROW_COUNT)
    return;
  if (pos->x < 0 || pos->x >= MAX_COL_COUNT)
    return;
  if (pos->k < 0 || pos->k >= MAX_HEIGHT)
    return;

  board->columns[pos->y][pos->x].chips[pos->k] = chip;
}

static int column_height(board_t *board, int y, int x)
{
  int k;

  if (y < 0 || y >= MAX_ROW_COUNT)
    return 0;
  if (x < 0 || x >= MAX_COL_COUNT)
    return 0;

  for (k = MAX_HEIGHT-1; k >= 0; --k)
    {
      chip_t chip = board->columns[y][x].chips[k];
      if (chip)
	return k + 1;
    }
  return 0;
}

static chip_t safe_get(const board_t *board, int y, int x, int k)
{
  if (y < 0 || y >= MAX_ROW_COUNT)
    return 0;
  if (x < 0 || x >= MAX_COL_COUNT)
    return 0;
  if (k < 0 || k >= MAX_HEIGHT)
    return 0;

  return board->columns[y][x].chips[k];
}

static int selectable(board_t *board, int y, int x)
{
  int i, j;
  int h = column_height(board, y, x);
  int l, r;

  if (h == 0)
    return 0;

  for (i = -1; i <= 1; ++i)
    for (j = -1; j <= 1; ++j)
      if (safe_get(board, y+i, x+j, h) != 0)
	return 0;

  l = safe_get(board, y-1, x-2, h-1)
    || safe_get(board, y, x-2, h-1)
    || safe_get(board, y+1, x-2, h-1);

  r = safe_get(board, y-1, x+2, h-1)
    || safe_get(board, y, x+2, h-1)
    || safe_get(board, y+1, x+2, h-1);
	
  if (l && r)
    return 0;

  return h;
}

positions_t* get_selectable_positions(board_t *board)
{
  int i, j;

  positions_t* positions = malloc(sizeof(positions_t));
  positions->count = 0;
  
  for (i = 0; i < MAX_ROW_COUNT; ++i)
    for (j = 0; j < MAX_COL_COUNT; ++j)
      {
	int h = selectable(board, i, j);
	if (h)
	  {
	    positions->positions[positions->count].y = i;
	    positions->positions[positions->count].x = j;
	    positions->positions[positions->count].k = h - 1;
	    ++positions->count;
	  }
      }

  return positions;
}

static int colorize(board_t *board, chip_t *pairs, int pile_size, board_t *result_board)
{
  int i, j;

  positions_t *positions = get_selectable_positions(board);

  if (positions->count < 2)
    {
      free(positions);
      return 0;
    }

  shuffle(positions->positions, positions->count, sizeof(position_t));
  
  if (pile_size == 2)
    {
      board_set(result_board, &positions->positions[0], pairs[0]);
      board_set(result_board, &positions->positions[1], pairs[1]);

      free(positions);
      return 1;
    }

  for (i = 0; i < positions->count - 1; ++i)
    for (j = i + 1; j < positions->count; ++j)
      {
	const position_t* p1 = &positions->positions[i];
	const position_t* p2 = &positions->positions[j];

	board_set(board, p1, 0);
	board_set(board, p2, 0);

	if (colorize(board, &pairs[2], pile_size - 2, result_board))
	  {
	    board_set(result_board, p1, pairs[0]);
	    board_set(result_board, p2, pairs[1]);
	    
	    free(positions);
	    return 1;
	  }

	board_set(board, p1, 0xFF);
	board_set(board, p2, 0xFF);
      }
  
  free(positions);
  return 0;
}

static void get_pile(chip_t pile[144])
{
  int i, j;
  int c = 0;

  /**suits**/
  for (j = 0; j < 9; ++j)
    {
      /*stones*/
      for (i = 0; i < 4; ++i)
	pile[c++] = 0x51 + j;
      /*bamboos*/
      for (i = 0; i < 4; ++i)
	pile[c++] = 0x61 + j;
      /*characters*/
      for (i = 0; i < 4; ++i)
	pile[c++] = 0x71 + j;
    }
  /**honors**/
  for (j = 0; j < 4; ++j)
    {
      /*winds*/
      for (i = 0; i < 4; ++i)
	pile[c++] = 0x91 + j;
    }
  for (j = 0; j < 3; ++j)
    {
      /*dragons*/
      for (i = 0; i < 4; ++i)
	pile[c++] = 0xA1 + j;
    }
  /**flowers**/
  /*plants*/
  for (j = 0; j < 4; ++j)
    pile[c++] = 0xD1 + j;
  /*seasons*/
  for (j = 0; j < 4; ++j)
    pile[c++] = 0xE1 + j;
}		

static void clear_board(board_t *board)
{
  int i, j;

  for (i = 0; i < MAX_ROW_COUNT; ++i)
    for (j = 0; j < MAX_COL_COUNT; ++j)
      memset(&board->columns[i][j].chips[0], 0, sizeof(chip_t) * MAX_HEIGHT);
}

void generate_board(board_t *board, map_t *map)
{
  int i;
  board_t tmp;
  chip_t pile[144];
  
  /* prepare pile */
  get_pile(pile);
  shuffle(&pile[136], 4, sizeof(chip_t));
  shuffle(&pile[140], 4, sizeof(chip_t));
  shuffle(pile, 72, 2 * sizeof(chip_t));

  clear_board(&tmp);
  for (i = 0; i < 144; ++i)
    {
      const int x = map->map[i].x;
      const int y = map->map[i].y;
      const int z = map->map[i].z;
      
      tmp.columns[y][x].chips[z] = 0xFF;
    }
  
  clear_board(board);
  colorize(&tmp, pile, 144, board);
}

