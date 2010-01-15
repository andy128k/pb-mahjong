#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "inkview.h"

#include "common.h"
#include "board.h"
#include "maps.h"
#include "bitmaps.h"
#include "geometry.h"
#include "helpers.h"

static board_t g_board;
static int row_count;
static int col_count;
static int caret_pos;
static int selection_pos = -1;
static positions_t* g_selectable = NULL;

static int cmp_pos(const void *p1, const void *p2)
{
  const position_t* pos1 = p1;
  const position_t* pos2 = p2;

  return (pos1->y * MAX_COL_COUNT + pos1->x)
    - (pos2->y * MAX_COL_COUNT + pos2->x);
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

static void cell_rect(const position_t *pos, struct rect* r)
{
#define IMG_WIDTH (55)
#define IMG_HEIGHT (79)

  const int screen_width = ScreenWidth();
  const int screen_height = ScreenHeight();

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

  int offset_x = (screen_width - w * col_count) / 2;
  int offset_y = (screen_height - h * row_count) / 2;

  r->x = offset_x + pos->x * w + 4 * pos->k;
  r->y = offset_y + pos->y * h + 4 * pos->k;
  r->w = 2 * w;
  r->h = 2 * h;
}

static void draw_chip(position_t *pos, chip_t chip, int selected)
{
  struct rect r;
  cell_rect(pos, &r);

  int i;

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
	
  if (selected)
    InvertArea(r.x+1, r.y+1, r.w-2, r.h-2);
}

static void draw_caret(int color)
{
  struct rect r;
  
  cell_rect(&g_selectable->positions[caret_pos], &r);

  DrawRect(r.x + 2, r.y + 2, r.w - 4, r.h - 4, color);
  DrawRect(r.x + 3, r.y + 3, r.w - 6, r.h - 6, color);
  DrawRect(r.x + 4, r.y + 4, r.w - 8, r.h - 8, color);
  DrawRect(r.x + 5, r.y + 5, r.w - 10, r.h - 10, color);
  DrawRect(r.x + 6, r.y + 6, r.w - 12, r.h - 12, color);
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
	  chip_t chip = g_board.columns[i][j].chips[k];
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

  position_t *caret = &g_selectable->positions[caret_pos];
  position_t *selection = &g_selectable->positions[selection_pos];
  for (i = chip_count - 1; i >=0; --i)
    {
      position_t *pos = &chips[i];

      draw_chip(pos, board_get(&g_board, pos), position_equal(pos, selection));
      
      if (position_equal(pos, caret))
	draw_caret(DGRAY);
    }
  
  int h = 20;
  DrawTextRect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, h, "влево, вправо - навигация, OK - выбор", ALIGN_FIT | ALIGN_CENTER);
}

static int move_left(void)
{
  int new_pos = caret_pos;
  if (caret_pos > 0)
    new_pos = caret_pos - 1;
  else
    new_pos = g_selectable->count - 1;
  
  if (new_pos != caret_pos)
    {
      caret_pos = new_pos;
      return 1;
    }
  return 0;
}

static int move_right(void)
{
  int new_pos = caret_pos;
  if (caret_pos < g_selectable->count - 1)
    new_pos = caret_pos + 1;
  else
    new_pos = 0;
  
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

static imenu game_menu[] = {
  { ITEM_HEADER, 0, "Mahjong", NULL },
  { ITEM_ACTIVE, 1, "Continue", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 2, "New game (Standard)", NULL },
  { ITEM_ACTIVE, 3, "New game (Difficult)", NULL },
  { ITEM_ACTIVE, 4, "New game (Four Bridges)", NULL },
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
      init_map(&standard_map);
      main_repaint();
      FullUpdate();
      break;

    case 3:
      init_map(&difficult_map);
      main_repaint();
      FullUpdate();
      break;

    case 4:
      init_map(&four_bridges_map);
      main_repaint();
      FullUpdate();
      break;

    case 999:
      CloseApp();
      break;
    }
}

static void game_finished_dialog_handler(int button)
{
  show_popup_strict(game_menu + 2, game_menu_handler); /* skip 'continue' item */
}

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
      if (caret_pos >= g_selectable->count)
	caret_pos = g_selectable->count - 1;
      
      main_repaint();
      FullUpdate();

      if (finished())
	{
	  Dialog(ICON_INFORMATION, "Mahjong", "You won", "OK", NULL, game_finished_dialog_handler);
	}
      /*
      else if (no_solutions())
        {
        }
      */
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

	default:
	  if (par1 == KEY_MENU) // or any other key
	    {
	      show_popup_strict(game_menu, game_menu_handler);
	      return 1;
	    }
	  break;
	}
      break;
    }
  return 0;
}

static imenu main_menu[] = {
  { ITEM_HEADER, 0, "Mahjong", NULL },
  { ITEM_ACTIVE, 2, "New game (Standard)", NULL },
  { ITEM_ACTIVE, 3, "New game (Difficult)", NULL },
  { ITEM_ACTIVE, 4, "New game (Four Bridges)", NULL },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 999, "Exit", NULL },
  { 0, 0, NULL, NULL }
};

static void main_menu_handler(int index)
{
  switch (index)
    {
    case 2:
      init_map(&standard_map);
      SetEventHandler(game_handler);
      break;
      
    case 3:
      init_map(&difficult_map);
      SetEventHandler(game_handler);
      break;
      
    case 4:
      init_map(&four_bridges_map);
      SetEventHandler(game_handler);
      break;
      
    case 999:
      CloseApp();
      break;
      
    default:
      printf("%d\n", index);
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
      
      show_popup_strict(main_menu, main_menu_handler);
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
  // srand(time(NULL));
  srand(12345);
  
  bitmaps_init();
	
  InkViewMain(main_handler);
  return 0;
}

