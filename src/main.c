#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "inkview.h"

#include "common.h"
#include "bitmaps.h"

typedef struct {
  chip_t chips[MAX_HEIGHT];
} column_t;

typedef struct {
  column_t columns[ROW_COUNT][COL_COUNT];
} board_t;

static board_t g_board;
static int caret_x, caret_y;
static int selection_x = -1, selection_y = -1;

static int column_height(int y, int x);

static void find_position(void)
{
  int i, j;
  int m_dist = COL_COUNT * ROW_COUNT;
  int y = -1, x = -1;

  if (column_height(caret_y, caret_x) != 0)
    return;

  for (i = 0; i < ROW_COUNT; ++i)
    for (j = 0; j < COL_COUNT; ++j)
      {
	int dist;
		
	if (column_height(i, j) == 0)
	  continue;

	dist = abs(i - caret_y) * 2 + abs(j - caret_x);
	if (dist < m_dist)
	  {
	    m_dist = dist;
	    y = i;
	    x = j;
	  }
      }

  caret_y = y;
  caret_x = x;
}

static void init_map(void)
{
  int i, j, k;
  int pile[144];
  int c;

  for (i = 0; i < ROW_COUNT; ++i)
    for (j = 0; j < COL_COUNT; ++j)
      for (k = 0; k < MAX_HEIGHT; ++k)
	g_board.columns[i][j].chips[k] = 0;

  c = 0;
				
  /**suits**/
  for (i = 0; i < 4; ++i)
    {
      /*stones*/
      for (j = 0; j < 9; ++j)
	pile[c++] = 0x51 + j;
      /*bamboos*/
      for (j = 0; j < 9; ++j)
	pile[c++] = 0x61 + j;
      /*characters*/
      for (j = 0; j < 9; ++j)
	pile[c++] = 0x71 + j;
    }
  /**honors**/
  for (i = 0; i < 4; ++i)
    {
      /*winds*/
      for (j = 0; j < 4; ++j)
	pile[c++] = 0x91 + j;
      /*dragons*/
      for (j = 0; j < 3; ++j)
	pile[c++] = 0xA1 + j;
    }
  /**flowers**/
  /*plants*/
  for (j = 0; j < 4; ++j)
    pile[c++] = 0xD1 + j;
  /*seasons*/
  for (j = 0; j < 4; ++j)
    pile[c++] = 0xE1 + j;
		
  shuffle(pile, 144, sizeof(pile[0]));
	
  /* fill */
  c = 0;
  for (i = 0; i < ROW_COUNT; ++i)
    for (j = 0; j < COL_COUNT; ++j)
      {
	int start = standard_map.map[i][j] >> 4;
	int height = standard_map.map[i][j] & 15;

	for (k = 0; k < height; ++k)
	  g_board.columns[i][j].chips[start + k] = pile[c++];
      }

  caret_x = 0;
  caret_y = 0;
  find_position();
}

static int fits(chip_t a,  chip_t b)
{
  int category = a & 0xC0;
  if (category == 0xC0) /*flowers*/
    {
      return (a & 0xF0) == (b & 0xF0);
    }
  else
    {
      return a == b;
    }
}

static void draw_chip(int x, int y, chip_t chip, int selected)
{
  int i;
  const int width = 2 * SCREEN_WIDTH / COL_COUNT;
  const int height = 2 * SCREEN_HEIGHT / ROW_COUNT;

  if (!chip)
    return;

  DrawRect(x-4, y-4, width, height, DGRAY);

  for (i = 0; i < 3; ++i)
    {
      DrawLine(x - 1 - i, y - 1 - i, x - 1 - i, y - 1 - i + height - 1, LGRAY);
      DrawLine(x - 1 - i, y - 1 - i, x - 1 - i + width - 1, y - 1 - i, LGRAY);
    }
  DrawLine(x-4, y-4, x, y, DGRAY);
  DrawLine(x-4+width-1, y-4, x+width-1, y, DGRAY);
  DrawLine(x-4, y-4+height-1, x, y+height-1, DGRAY);
	
  FillArea(x, y, width, height, WHITE);
  DrawRect(x, y, width, height, DGRAY);

  StretchBitmap(x+1, y+1, width-2, height-2, (ibitmap*)bitmaps[chip], 0);
	
  if (selected)
    InvertArea(x+1, y+1, width-2, height-2);
}

static int column_height(int y, int x)
{
  int k;

  if (y < 0 || y >= ROW_COUNT)
    return 0;
  if (x < 0 || x >= COL_COUNT)
    return 0;

  for (k = MAX_HEIGHT-1; k >= 0; --k)
    {
      chip_t chip = g_board.columns[y][x].chips[k];
      if (chip)
	return k + 1;
    }
  return 0;
}

static void draw_caret(int color)
{
  const int k = column_height(caret_y, caret_x) - 1;

  const int width = 2 * SCREEN_WIDTH / COL_COUNT;
  const int height = 2 * SCREEN_HEIGHT / ROW_COUNT;

  int x = caret_x * SCREEN_WIDTH / COL_COUNT + 4 * k;
  int y = caret_y * SCREEN_HEIGHT / ROW_COUNT + 4 * k;
  DrawRect(x + 2, y + 2, width - 4, height - 4, color);
  DrawRect(x + 3, y + 3, width - 6, height - 6, color);
  DrawRect(x + 4, y + 4, width - 8, height - 8, color);
  DrawRect(x + 5, y + 5, width - 10, height - 10, color);
  DrawRect(x + 6, y + 6, width - 12, height - 12, color);
}

static void main_repaint(void)
{
  int i, j, k;
  const int caret_k = column_height(caret_y, caret_x) - 1;

  ClearScreen();

  for (k = 0; k < MAX_HEIGHT; ++k)
    {
      for (j = COL_COUNT - 1; j >= 0; --j)
	{
	  int x = j * SCREEN_WIDTH / COL_COUNT;
	  for (i = ROW_COUNT-1; i >= 0; --i)
	    {
	      int y = i * SCREEN_HEIGHT / ROW_COUNT;

	      chip_t chip = g_board.columns[i][j].chips[k];
	      draw_chip(x + 4 * k, y + 4 * k, chip, (i == selection_y && j == selection_x));
				
	      if (i == caret_y && j == caret_x && k == caret_k)
		draw_caret(DGRAY);
	    }
	}
    }
}

static int move_left(void)
{
  int x = caret_x;
  while (x - 1 >= 0)
    {
      if (column_height(caret_y, x - 1) != 0)
	{
	  caret_x = x - 1;
	  return 1;
	}

      if (caret_y - 1 >= 0 && column_height(caret_y - 1, x - 1) != 0)
	{
	  --caret_y;
	  caret_x = x - 1;
	  return 1;
	}

      if (caret_y + 1 < ROW_COUNT && column_height(caret_y + 1, x - 1) != 0)
	{
	  ++caret_y;
	  caret_x = x - 1;
	  return 1;
	}
		
      --x;
    }
  return 0;
}

static int move_right(void)
{
  int x = caret_x;
  while (x + 1 < COL_COUNT)
    {
      if (column_height(caret_y, x + 1) != 0)
	{
	  caret_x = x + 1;
	  return 1;
	}

      if (caret_y + 1 < ROW_COUNT && column_height(caret_y + 1, x + 1) != 0)
	{
	  ++caret_y;
	  caret_x = x + 1;
	  return 1;
	}

      if (caret_y - 1 >= 0 && column_height(caret_y - 1, x + 1) != 0)
	{
	  --caret_y;
	  caret_x = x + 1;
	  return 1;
	}
		
      ++x;
    }
  return 0;
}

static int move_up(void)
{
  int y = caret_y;
  while (y - 1 >= 0)
    {
      if (column_height(y - 1, caret_x) != 0)
	{
	  caret_y = y - 1;
	  return 1;
	}

      if (caret_x - 1 >= 0 && column_height(y - 1, caret_x - 1) != 0)
	{
	  --caret_x;
	  caret_y = y - 1;
	  return 1;
	}

      if (caret_x + 1 < COL_COUNT && column_height(y - 1, caret_x + 1) != 0)
	{
	  ++caret_x;
	  caret_y = y - 1;
	  return 1;
	}

      --y;
    }
  return 0;
}

static int move_down(void)
{
  int y = caret_y;
  while (y + 1 < ROW_COUNT)
    {
      if (column_height(y + 1, caret_x) != 0)
	{
	  caret_y = y + 1;
	  return 1;
	}

      if (caret_x + 1 < COL_COUNT && column_height(y + 1, caret_x + 1) != 0)
	{
	  ++caret_x;
	  caret_y = y + 1;
	  return 1;
	}
		
      if (caret_x - 1 >= 0 && column_height(y + 1, caret_x - 1) != 0)
	{
	  --caret_x;
	  caret_y = y + 1;
	  return 1;
	}

      ++y;
    }
  return 0;
}

struct rect {
  int x, y, w, h;
};

void union_rect(const struct rect* r1, const struct rect* r2, struct rect* r)
{
  r->x = min_int(r1->x, r2->x);
  r->y = min_int(r1->y, r2->y);
  r->w = max_int(r1->x + r1->w, r2->x + r2->w) - r->x;
  r->h = max_int(r1->y + r1->h, r2->y + r2->h) - r->y;
}

void cell_rect(int y, int x, struct rect* r)
{
  int k = column_height(y, x) - 1;

  r->x = x * SCREEN_WIDTH / COL_COUNT + 4 * k;
  r->y = y * SCREEN_HEIGHT / ROW_COUNT + 4 * k;
  r->w = 2 * SCREEN_WIDTH / COL_COUNT;
  r->h = 2 * SCREEN_HEIGHT / ROW_COUNT;
}

static void generic_move(int (*move_func)(void))
{
  int prev_caret_x = caret_x;
  int prev_caret_y = caret_y;
  if (move_func())
    {
      struct rect r1;
      struct rect r2;
      struct rect r;

      main_repaint();
      cell_rect(prev_caret_y, prev_caret_x, &r1);
      cell_rect(caret_y, caret_x, &r2);
      union_rect(&r1, &r2, &r);

      PartialUpdate(r.x, r.y, r.w, r.h);
    }
}

static int safe_get(int y, int x, int k)
{
  if (y < 0 || y >= ROW_COUNT)
    return 0;
  if (x < 0 || x >= COL_COUNT)
    return 0;
  if (k < 0 || k >= MAX_HEIGHT)
    return 0;

  return g_board.columns[y][x].chips[k];
}

static inline int has_chip(int y, int x, int k)
{
  return safe_get(y, x, k) != 0;
}

static int selectable(int y, int x)
{
  int i, j;
  int k = column_height(y, x) - 1;

  for (i = -1; i <= 1; ++i)
    for (j = -1; j <= 1; ++j)
      if (has_chip(y+i, x+j, k+1))
	return 0;

  int l = has_chip(y-1, x-2, k)
    || has_chip(y, x-2, k)
    || has_chip(y+1, x-2, k);

  int r = has_chip(y-1, x+2, k)
    || has_chip(y, x+2, k)
    || has_chip(y+1, x+2, k);
	
  if (l && r)
    return 0;

  return 1;
}

static int finished(void)
{
  int i, j;

  for (i = 0; i < ROW_COUNT; ++i)
    for (j = 0; j < COL_COUNT; ++j)
      if (column_height(i, j) != 0)
	return 0;
  return 1;
}

static void dummy_dialog_handler(int button)
{
}

static void select_cell(void)
{
  int k1, k2;

  if (selection_y == caret_y && selection_x == caret_x)
    {
      struct rect r;
      cell_rect(selection_y, selection_x, &r);
      selection_x = -1;
      selection_y = -1;
      main_repaint();
      PartialUpdate(r.x, r.y, r.w, r.h);
      return;
    }

  if (!selectable(caret_y, caret_x))
    return;

  k1 = column_height(selection_y, selection_x);
  k2 = column_height(caret_y, caret_x);

  if (k1 != 0 && k2 != 0
      && fits(g_board.columns[selection_y][selection_x].chips[k1-1],
	      g_board.columns[caret_y][caret_x].chips[k2-1]))
    {
      g_board.columns[selection_y][selection_x].chips[k1-1] = 0;
      g_board.columns[caret_y][caret_x].chips[k2-1] = 0;
      selection_x = -1;
      selection_y = -1;
		
      if (column_height(caret_y, caret_x) == 0)
	find_position();
		
      main_repaint();
      FullUpdate();

      if (finished())
	{
	  Dialog(ICON_INFORMATION, "Mahjong", "You won", "OK", NULL, dummy_dialog_handler);
	}
    }
  else
    {
      struct rect r;
      int prev_selection_x = selection_x;
      int prev_selection_y = selection_y;

      selection_x = caret_x;
      selection_y = caret_y;
      main_repaint();

      cell_rect(selection_y, selection_x, &r);
		
      if (prev_selection_x != -1 && prev_selection_y != -1)
	{
	  struct rect r1;
	  struct rect r2;
			
	  r1 = r;
	  cell_rect(prev_selection_y, prev_selection_x, &r2);
	  union_rect(&r1, &r2, &r);
	}

      PartialUpdate(r.x, r.y, r.w, r.h);
    }
}

static imenu game_menu[] = {
  { ITEM_HEADER, 0, "Mahjong", NULL },
  { ITEM_ACTIVE, 1, "Continue", NULL },
  { ITEM_ACTIVE, 2, "New game", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 999, "Exit", NULL },
  { 0, 0, NULL, NULL }
};

static void game_menu_handler(int index)
{
  switch (index)
    {
    case 1:
      break;

    case 2:
      init_map();
      main_repaint();
      FullUpdate();
      break;

    case 999:
      CloseApp();
      break;
    }
}

static int game_handler(int type, int par1, int par2)
{
  switch (type)
    {
    case EVT_SHOW:
      main_repaint();
      FullUpdate();
      break;
    case EVT_KEYPRESS:
      switch (par1) 
	{
	case KEY_OK:
	  select_cell();
	  break;

	case KEY_BACK:
	  CloseApp();
	  break;

	case KEY_LEFT:
	  generic_move(move_left);
	  break;

	case KEY_RIGHT:
	  generic_move(move_right);
	  break;

	case KEY_UP:
	  generic_move(move_up);
	  break;

	case KEY_DOWN:
	  generic_move(move_down);
	  break;

	case KEY_MENU:
	  OpenMenu(game_menu, 0, -1, -1, game_menu_handler);
	  break;
	}
      break;
    }
  return 0;
}

static imenu main_menu[] = {
  { ITEM_HEADER, 0, "Mahjong", NULL },
  { ITEM_ACTIVE, 1, "New game", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 999, "Exit", NULL },
  { 0, 0, NULL, NULL }
};

static void main_menu_handler(int index)
{
  switch (index)
    {
    case 1:
      init_map();
      SetEventHandler(game_handler);
      break;

    case 999:
      CloseApp();
      break;
    }
}

static int main_handler(int type, int par1, int par2)
{
  switch (type)
    {
    case EVT_INIT:
      SetOrientation(ROTATE270);
      ClearScreen();
      extern const ibitmap background;
      StretchBitmap(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (ibitmap*)&background, 0);
      FullUpdate();
      OpenMenu(main_menu, 0, -1, -1, main_menu_handler);
      break;
    case EVT_EXIT:
      /*
	occurs only in main handler when exiting or when SIGINT received.
	save configuration here, if needed
      */
      break;
    }
  return 0;
}

int main(int argc, char **argv)
{
  srand(time(NULL));

  bitmaps_init();
	
  InkViewMain(main_handler);
  return 0;
}

