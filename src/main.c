#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "inkview.h"

#include "common.h"
#include "board.h"
#include "maps.h"
#include "bitmaps.h"
#include "geometry.h"
#include "menu.h"

static int orientation = ROTATE270;
static board_t g_board;
static int row_count;
static int col_count;
static int caret_pos;
static int selection_pos = -1;
static positions_t* g_selectable = NULL;

extern const ibitmap background;

static void menu_handler(int index);

static int cmp_pos(const void *p1, const void *p2)
{
  const position_t* pos1 = p1;
  const position_t* pos2 = p2;
  return ((pos1->y - 1) / 2 * MAX_COL_COUNT + pos1->x)
    - ((pos2->y - 1) / 2 * MAX_COL_COUNT + pos2->x);
}

static void rebuild_selectables(void)
{
  if (g_selectable != NULL)
    free(g_selectable);
  
  g_selectable = get_selectable_positions(&g_board);
  qsort(&g_selectable->positions[0], g_selectable->count, sizeof(position_t), cmp_pos);
}

static void init_map(map_t *map)
{
  generate_board(&g_board, map);
  row_count = map->row_count;
  col_count = map->col_count;

  rebuild_selectables();
  caret_pos = 0;
}

static int fits(chip_t a, chip_t b)
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

static void cell_rect(const position_t *pos, struct rect* r)
{
#define IMG_WIDTH (55)
#define IMG_HEIGHT (79)

  const int screen_width = ScreenWidth();
  const int screen_height = ScreenHeight();
  int offset_x;
  int offset_y;

  int w, h;

  if (IMG_WIDTH * col_count * screen_height > screen_width * IMG_HEIGHT * row_count)
    {
      w = screen_width / col_count;
      h = w * IMG_HEIGHT / IMG_WIDTH;
    }
  else
    {
      h = screen_height / row_count;
      w = h * IMG_WIDTH / IMG_HEIGHT;
    }

  offset_x = (screen_width - w * col_count) / 2;
  offset_y = (screen_height - h * row_count) / 2;

  r->x = offset_x + pos->x * w + 4 * pos->k;
  r->y = offset_y + pos->y * h + 4 * pos->k;
  r->w = 2 * w;
  r->h = 2 * h;
}

static void draw_caret(const struct rect *r, int color)
{
  DrawRect(r->x + 2, r->y + 2, r->w - 4, r->h - 4, color);
  DrawRect(r->x + 3, r->y + 3, r->w - 6, r->h - 6, color);
  DrawRect(r->x + 4, r->y + 4, r->w - 8, r->h - 8, color);
  DrawRect(r->x + 5, r->y + 5, r->w - 10, r->h - 10, color);
  DrawRect(r->x + 6, r->y + 6, r->w - 12, r->h - 12, color);
}

static void draw_chip(const position_t *pos, chip_t chip)
{
  int i;
  struct rect r;

  cell_rect(pos, &r);
  DrawRect(r.x-4, r.y-4, r.w, r.h, DGRAY);

  for (i = 0; i < 3; ++i)
    {
      DrawLine(r.x - 1 - i, r.y - 1 - i, r.x - 1 - i, r.y - 1 - i + r.h - 1, LGRAY);
      DrawLine(r.x - 1 - i, r.y - 1 - i, r.x - 1 - i + r.w - 1, r.y - 1 - i, LGRAY);
    }
  DrawLine(r.x-4, r.y-4, r.x, r.y, DGRAY);
  DrawLine(r.x-4+r.w-1, r.y-4, r.x+r.w-1, r.y, DGRAY);
  DrawLine(r.x-4, r.y-4+r.h-1, r.x, r.y+r.h-1, DGRAY);
	
  FillArea(r.x, r.y, r.w, r.h, WHITE);
  DrawRect(r.x, r.y, r.w, r.h, DGRAY);

  StretchBitmap(r.x+1, r.y+1, r.w-2, r.h-2, (ibitmap*)bitmaps[chip], 0);
	
  if (caret_pos >= 0 && caret_pos < g_selectable->count)
    {
      if (position_equal(pos, &g_selectable->positions[caret_pos]))
	draw_caret(&r, DGRAY);
    }

  if (selection_pos >= 0 && selection_pos < g_selectable->count)
    {
      position_t *selection = &g_selectable->positions[selection_pos];
      if (position_equal(pos, selection))
	InvertArea(r.x+1, r.y+1, r.w-2, r.h-2);
    }
}

static int is_covered_by(const void *p1, const void *p2)
{
  const position_t *s1 = p1;
  const position_t *s2 = p2;

  if (s1->k < s2->k)
    return 1;
  if (s1->k > s2->k)
    return 0;

  if (s2->y >= s1->y + 2)
    return 0;

  if (abs(s2->y -s1->y) <= 1)
    return s2->x < s1->x;

  if (s2->y == s1->y - 2)
    return s2->x >= s1->x - 2 && s2->x < s1->x - 2;

  return 0;
}

static void main_repaint(void)
{
  int i, j, k;
  position_t chips[144];
  int chip_count = 0;

  for (i = 0; i < MAX_ROW_COUNT; ++i)
    for (j = 0; j < MAX_COL_COUNT; ++j)
      for (k = 0; k < MAX_HEIGHT; ++k)
	{
	  const chip_t chip = g_board.columns[i][j].chips[k];
	  if (chip)
	    {
	      chips[chip_count].x = j;
	      chips[chip_count].y = i;
	      chips[chip_count].k = k;
	      ++chip_count;
	    }
	}
  topological_sort(chips, chip_count, sizeof(position_t), is_covered_by);

  ClearScreen();
  
  for (i = chip_count - 1; i >=0; --i)
    {
      const position_t *pos = &chips[i];
      draw_chip(pos, board_get(&g_board, pos));
    }
  
  /* help */
  /*
  {
    int h = 20;
    DrawTextRect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, h, "←, ↑, →, ↓ - выбор, OK - afas", ALIGN_FIT | ALIGN_CENTER);
  }
  */
}

static int move_left(void)
{
  if (caret_pos > 0)
    {
      --caret_pos;
      return 1;
    }
  return 0;
}

static int move_right(void)
{
  if (caret_pos < g_selectable->count - 1)
    {
      ++caret_pos;
      return 1;
    }
  return 0;
}

static int move_up(void)
{
  int i;
  int min_dist = (MAX_ROW_COUNT + 1) * (MAX_COL_COUNT + 1);
  int new_pos = caret_pos;

  const position_t *caret = &g_selectable->positions[caret_pos];

  for (i = 0; i < g_selectable->count; ++i)
    if (i != caret_pos)
      {
	const position_t *pos = &g_selectable->positions[i];
	int dist;

	if (caret->y <= pos->y)
	  continue;

	dist = abs((caret->y - 1) / 2 - (pos->y - 1) / 2) * MAX_COL_COUNT + abs(caret->x - pos->x);
	if (dist < min_dist)
	  {
	    new_pos = i;
	    min_dist = dist;
	  }
      }

  if (new_pos != caret_pos)
    {
      caret_pos = new_pos;
      return 1;
    }
  return 0;
}

static int move_down(void)
{
  int i;
  int min_dist = (MAX_ROW_COUNT + 1) * (MAX_COL_COUNT + 1);
  int new_pos = caret_pos;

  const position_t *caret = &g_selectable->positions[caret_pos];

  for (i = 0; i < g_selectable->count; ++i)
    if (i != caret_pos)
      {
	const position_t *pos = &g_selectable->positions[i];
	int dist;
	if (caret->y >= pos->y)
	  continue;

	dist = abs((caret->y - 1) / 2 - (pos->y - 1) / 2) * MAX_COL_COUNT + abs(caret->x - pos->x);
	if (dist < min_dist)
	  {
	    new_pos = i;
	    min_dist = dist;
	  }
      }

  if (new_pos != caret_pos)
    {
      caret_pos = new_pos;
      return 1;
    }
  return 0;
}

static void generic_move(int (*move_func)(void))
{
  int prev_caret_pos = caret_pos;
  if (move_func())
    {
      struct rect r1;
      struct rect r2;
      struct rect r;

      main_repaint();
      cell_rect(&g_selectable->positions[prev_caret_pos], &r1);
      cell_rect(&g_selectable->positions[caret_pos], &r2);
      union_rect(&r1, &r2, &r);

      PartialUpdate(r.x, r.y, r.w, r.h);
    }
}

static int finished(void)
{
  int i, j, k;

  for (i = 0; i < MAX_ROW_COUNT; ++i)
    for (j = 0; j < MAX_COL_COUNT; ++j)
      for (k = 0; k < MAX_HEIGHT; ++k)
	{
	  position_t pos;
	  pos.y = i;
	  pos.x = j;
	  pos.k = k;
	  if (board_get(&g_board, &pos) != 0)
	    return 0;
	}
  return 1;
}

static int pair_exists(board_t *board)
{
  int i, j;

  for (i = 0; i < g_selectable->count - 1; ++i)
    {
      const chip_t chip1 = board_get(board, &g_selectable->positions[i]);
      for (j = i + 1; j < g_selectable->count; ++j)
	{
	  const chip_t chip2 = board_get(board, &g_selectable->positions[j]);
	  if (fits(chip1, chip2))
	    return 1;
	}
    }
  return 0;
}

static imenu game_menu_en[] = {
  { ITEM_ACTIVE, 1, "Continue", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 2, "New game (Easy)", NULL },
  { ITEM_ACTIVE, 3, "New game (Difficult)", NULL },
  { ITEM_ACTIVE, 4, "New game (Four Bridges)", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 999, "Exit", NULL },
  { 0, 0, NULL, NULL }
};

static imenu game_menu_ru[] = {
  { ITEM_ACTIVE, 1, "Продолжить", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 2, "Новая игра (Лёгкая)", NULL },
  { ITEM_ACTIVE, 3, "Новая игра (Тяжёлая)", NULL },
  { ITEM_ACTIVE, 4, "Новая игра (Четыре моста)", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 999, "Выход", NULL },
  { 0, 0, NULL, NULL }
};

static imenu *game_menu = game_menu_en;

static const char * const win_text_en = "Solitaire is solved. Congratulations!";
static const char * const win_text_ru = "Пасьянс решён. Поздравляем!";
static const char *win_text;

static const char * const lose_text_en = "There is no more free pair. You lose";
static const char * const lose_text_ru = "Свободных пар больше нет. Вы проиграли.";
static const char *lose_text;

static void select_cell(void)
{
  if (selection_pos == caret_pos)
    {
      struct rect r;
      cell_rect(&g_selectable->positions[selection_pos], &r);
      selection_pos = -1;
      main_repaint();
      PartialUpdate(r.x, r.y, r.w, r.h);
      return;
    }
  
  if (selection_pos >= 0
      && fits( board_get(&g_board, &g_selectable->positions[selection_pos]),
	       board_get(&g_board, &g_selectable->positions[caret_pos])))
    {
      board_set( &g_board, &g_selectable->positions[selection_pos], 0);
      board_set( &g_board, &g_selectable->positions[caret_pos], 0);
      
      selection_pos = -1;
      
      rebuild_selectables();
      // find caret pos
      if (caret_pos >= g_selectable->count)
	caret_pos = g_selectable->count - 1;
      
      if (finished())
	{
	  show_popup(&background, win_text, game_menu + 2, menu_handler);
	}
      else if (!pair_exists(&g_board))
        {
	  show_popup(&background, lose_text, game_menu + 2, menu_handler);
        }
      else
	{
	  main_repaint();
	  FullUpdate();
	}
    }
  else
    {
      struct rect r;
      int prev_selection_pos = selection_pos;

      selection_pos = caret_pos;
      main_repaint();

      cell_rect(&g_selectable->positions[selection_pos], &r);
		
      if (prev_selection_pos != -1)
	{
	  struct rect r1;
	  struct rect r2;
			
	  r1 = r;
	  cell_rect(&g_selectable->positions[prev_selection_pos], &r2);
	  union_rect(&r1, &r2, &r);
	}
      
      PartialUpdate(r.x, r.y, r.w, r.h);
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

	case KEY_PREV:
	case KEY_NEXT:
	case KEY_MENU:
	  show_popup(NULL, NULL, game_menu, menu_handler);
	  return 1;
	}
      break;
    }
  return 0;
}

static imenu main_menu_en[] = {
  { ITEM_ACTIVE, 2, "New game (Easy)", NULL },
  { ITEM_ACTIVE, 3, "New game (Difficult)", NULL },
  { ITEM_ACTIVE, 4, "New game (Four Bridges)", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 102, "Русский", NULL },
  { ITEM_ACTIVE, 201, "Change screen orientation", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 999, "Exit", NULL },
  { 0, 0, NULL, NULL }
};

static imenu main_menu_ru[] = {
  { ITEM_ACTIVE, 2, "Новая игра (Лёгкая)", NULL },
  { ITEM_ACTIVE, 3, "Новая игра (Тяжёлая)", NULL },
  { ITEM_ACTIVE, 4, "Новая игра (Четыре моста)", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 101, "English", NULL },
  { ITEM_ACTIVE, 201, "Сменить ориентацию экрана", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 999, "Выход", NULL },
  { 0, 0, NULL, NULL }
};

static imenu *main_menu = main_menu_en;

static void menu_handler(int index)
{
  switch (index)
    {
    case 1: /*continue*/
      SetEventHandler(game_handler);
      break;

    case 2:/*new standard*/
      init_map(&standard_map);
      SetEventHandler(game_handler);
      break;

    case 3:/*new difficult*/
      init_map(&difficult_map);
      SetEventHandler(game_handler);
      break;

    case 4:/*new bridges*/
      init_map(&four_bridges_map);
      SetEventHandler(game_handler);
      break;

    case 101:/*lang: english*/
      main_menu = main_menu_en;
      game_menu = game_menu_en;
      win_text = win_text_en;
      lose_text = lose_text_en;
      show_popup(&background, NULL, main_menu, menu_handler);
      break;
      
    case 102:/*lang: russian*/
      main_menu = main_menu_ru;
      game_menu = game_menu_ru;
      win_text = win_text_ru;
      lose_text = lose_text_ru;
      show_popup(&background, NULL, main_menu, menu_handler);
      break;
      
    case 201:/*change orientation*/
      if (orientation == ROTATE270)
        orientation = ROTATE90;
      else
        orientation = ROTATE270;
      SetOrientation(orientation);
      ClearScreen();
      StretchBitmap(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (ibitmap*)&background, 0);
      FullUpdate();
      show_popup(&background, NULL, main_menu, menu_handler);
      break;

    case 999:/*exit*/
      CloseApp();
      break;
    }
}

static int main_handler(int type, int par1, int par2)
{
  switch (type)
    {
    case EVT_INIT:
      SetOrientation(orientation);
      show_popup(&background, NULL, main_menu, menu_handler);
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

  win_text = win_text_en;
  lose_text = lose_text_en;

  bitmaps_init();
	
  InkViewMain(main_handler);
  return 0;
}

