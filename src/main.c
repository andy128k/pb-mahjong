#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "inkview.h"

#include "common.h"
#include "board.h"
#include "maps.h"
#include "bitmaps.h"
#include "geometry.h"
#include "menu.h"
#include "messages.h"

#ifdef EMULATION
#undef STATEPATH
#define STATEPATH "."
#endif

#define SAVED_GAME_PATH (STATEPATH "/pb-mahjong.saved-game")

static int orientation = ROTATE270;
static board_t g_board;
static int row_count;
static int col_count;
static int caret_pos;
static int selection_pos = -1;
static positions_t *g_selectable = NULL;
static int help_index = 0;
static int help_offset = 0;
static int game_active = 0;

extern const ibitmap background;

#define HELP_HEIGHT (20)

static void menu_handler(int index);
static void read_state(void);
static void write_state(void);
static int load_game(void);
static void save_game(void);

static struct
{
  position_t positions[144];
  chip_t chips[144];
  int count;
} undo_stack;

static int cmp_pos(const void *p1, const void *p2)
{
  const position_t *pos1 = p1;
  const position_t *pos2 = p2;
  return ((pos1->y - 1) / 2 * MAX_COL_COUNT + pos1->x)
    - ((pos2->y - 1) / 2 * MAX_COL_COUNT + pos2->x);
}

static void rebuild_selectables(void)
{
  if (g_selectable != NULL)
    free(g_selectable);

  g_selectable = get_selectable_positions(&g_board);
  qsort(&g_selectable->positions[0], g_selectable->count, sizeof(position_t), cmp_pos);

  help_index = 0;
  help_offset = 0;
}

static void undo()
{
  int i;

  if (undo_stack.count == 0)
    return;

  for (i = 0; i < 2; ++i)
    {
      board_set( &g_board, &undo_stack.positions[undo_stack.count - 1], undo_stack.chips[undo_stack.count - 1]);
      --undo_stack.count;
    }

  selection_pos = -1;
  rebuild_selectables();

  // find caret pos
  if (caret_pos >= g_selectable->count)
    caret_pos = g_selectable->count - 1;
}

static void clear_undo_stack()
{
  undo_stack.count = 0;
}

static void start_game(void)
{
  rebuild_selectables();
  caret_pos = 0;
  selection_pos = -1;
  game_active = 1;
  unlink(SAVED_GAME_PATH);
}

static void init_map(map_t *map)
{
  clear_undo_stack();

  generate_board(&g_board, map);
  row_count = map->row_count;
  col_count = map->col_count;

  start_game();
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

static void cell_rect(const position_t *pos, struct rect *r)
{
#define IMG_WIDTH (55)
#define IMG_HEIGHT (79)

  const int screen_width = ScreenWidth();
  const int screen_height = ScreenHeight() - HELP_HEIGHT;
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
  DrawRect(r.x - 4, r.y - 4, r.w, r.h, DGRAY);

  for (i = 0; i < 3; ++i)
    {
      DrawLine(r.x - 1 - i, r.y - 1 - i, r.x - 1 - i, r.y - 1 - i + r.h - 1, LGRAY);
      DrawLine(r.x - 1 - i, r.y - 1 - i, r.x - 1 - i + r.w - 1, r.y - 1 - i, LGRAY);
    }
  DrawLine(r.x - 4, r.y - 4, r.x, r.y, DGRAY);
  DrawLine(r.x - 4 + r.w - 1, r.y - 4, r.x + r.w - 1, r.y, DGRAY);
  DrawLine(r.x - 4, r.y - 4 + r.h - 1, r.x, r.y + r.h - 1, DGRAY);

  FillArea(r.x, r.y, r.w, r.h, WHITE);
  DrawRect(r.x, r.y, r.w, r.h, DGRAY);

  StretchBitmap(r.x + 1, r.y + 1, r.w - 2, r.h - 2, (ibitmap*)bitmaps[chip], 0);

  if (caret_pos >= 0 && caret_pos < g_selectable->count)
    {
      if (position_equal(pos, &g_selectable->positions[caret_pos]))
	draw_caret(&r, DGRAY);
    }

  if (selection_pos >= 0 && selection_pos < g_selectable->count)
    {
      position_t *selection = &g_selectable->positions[selection_pos];
      if (position_equal(pos, selection))
	InvertArea(r.x + 1, r.y + 1, r.w - 2, r.h - 2);
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

  if (abs(s2->y - s1->y) <= 1)
    return s2->x < s1->x;

  if (s2->y == s1->y - 2)
    return s2->x >= s1->x - 2 && s2->x < s1->x - 2;

  return 0;
}

static ifont *g_help_font = NULL;
static ifont *get_help_font(void)
{
  if (g_help_font == NULL)
    g_help_font = OpenFont(DEFAULTFONTB, 16, 1);
  return g_help_font;
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

  for (i = chip_count - 1; i >= 0; --i)
    {
      const position_t *pos = &chips[i];
      draw_chip(pos, board_get(&g_board, pos));
    }

  /* status bar */
  {
    struct rect r;
    r.x = 0;
    r.y = ScreenHeight() - HELP_HEIGHT;
    r.w = ScreenWidth();
    r.h = HELP_HEIGHT;

    DrawLine(r.x, r.y, r.x + r.w, r.y, BLACK);
    FillArea(r.x, r.y + 2, r.w, r.h - 2, DGRAY);

    SetFont(get_help_font(), WHITE);

    r.x += 10;
    r.w -= 20;
    r.y += 2;
    r.h -= 2;

    {
      int pairs = 0;
      for (i = 0; i < g_selectable->count - 1; ++i)
	{
	  const chip_t chip1 = board_get(&g_board, &g_selectable->positions[i]);
	  for (j = i + 1; j < g_selectable->count; ++j)
	    {
	      const chip_t chip2 = board_get(&g_board, &g_selectable->positions[j]);
	      if (fits(chip1, chip2))
		++pairs;
	    }
	}

      char buffer[256];
      snprintf(buffer, 256, get_message(MSG_MOVES_LEFT), pairs);
      DrawTextRect(r.x, r.y, r.w, r.h, buffer, ALIGN_FIT | ALIGN_LEFT);
    }

    DrawTextRect(r.x, r.y, r.w, r.h, (char*)get_message(MSG_HELP), ALIGN_FIT | ALIGN_RIGHT);
  }
}

static int move_left(void)
{
  if (caret_pos > 0)
    --caret_pos;
  else
    caret_pos = g_selectable->count - 1;
  return 1;
}

static int move_right(void)
{
  if (caret_pos < g_selectable->count - 1)
    ++caret_pos;
  else
    caret_pos = 0;
  return 1;
}

static int move_up(void)
{
  int i;
  int min_dist = INT_MAX;
  int new_pos = caret_pos;

  const position_t *caret = &g_selectable->positions[caret_pos];
  const int caret_row = (caret->y - 1) / 2;

  for (i = 0; i < g_selectable->count; ++i)
    if (i != caret_pos)
      {
	const position_t *pos = &g_selectable->positions[i];
	int pos_row = (pos->y - 1) / 2;
	if (caret_row <= pos_row)
	  pos_row -= MAX_COL_COUNT;

	const int dist = abs(caret_row - pos_row) * MAX_COL_COUNT + abs(caret->x - pos->x);
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
  int min_dist = INT_MAX;
  int new_pos = caret_pos;

  const position_t *caret = &g_selectable->positions[caret_pos];
  const int caret_row = (caret->y - 1) / 2;

  for (i = 0; i < g_selectable->count; ++i)
    if (i != caret_pos)
      {
	const position_t *pos = &g_selectable->positions[i];
        int pos_row = (pos->y - 1) / 2;
	if (caret_row >= pos_row)
	  pos_row += MAX_COL_COUNT;

	const int dist = abs(caret_row - pos_row) * MAX_COL_COUNT + abs(caret->x - pos->x);
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

  const chip_t chip1 = selection_pos >= 0 ? board_get(&g_board, &g_selectable->positions[selection_pos]) : 0;
  const chip_t chip2 = board_get(&g_board, &g_selectable->positions[caret_pos]);

  if (chip1 != 0 && fits(chip1, chip2))
    {
      undo_stack.positions[undo_stack.count] = g_selectable->positions[selection_pos];
      undo_stack.chips[undo_stack.count] = chip1;
      ++undo_stack.count;
      undo_stack.positions[undo_stack.count] = g_selectable->positions[caret_pos];
      undo_stack.chips[undo_stack.count] = chip2;
      ++undo_stack.count;

      board_set(&g_board, &g_selectable->positions[selection_pos], 0);
      board_set(&g_board, &g_selectable->positions[caret_pos], 0);

      selection_pos = -1;

      rebuild_selectables();
      // find caret pos
      if (caret_pos >= g_selectable->count)
	caret_pos = g_selectable->count - 1;

      static message_id finish_menu[] = {
	MSG_NEW_GAME_EASY,
	MSG_NEW_GAME_DIFFICULT,
	MSG_NEW_GAME_FOUR_BRIDGES,
	MSG_SEPARATOR,
	MSG_EXIT,
	MSG_NONE
      };

      if (finished())
	{
	  game_active = 0;
	  clear_undo_stack();
	  show_popup(&background, MSG_WIN, finish_menu, menu_handler);
	}
      else if (!pair_exists(&g_board))
        {
	  game_active = 0;
	  clear_undo_stack();
	  show_popup(&background, MSG_LOSE, finish_menu, menu_handler);
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
	  {
	    static message_id game_menu[] = {
	      MSG_CONTINUE,
	      MSG_HINT,
	      MSG_SEPARATOR,
	      MSG_NEW_GAME_EASY,
	      MSG_NEW_GAME_DIFFICULT,
	      MSG_NEW_GAME_FOUR_BRIDGES,
	      MSG_SEPARATOR,
	      MSG_EXIT,
	      MSG_NONE
	    };

	    static message_id game_menu_with_undo[] = {
	      MSG_CONTINUE,
	      MSG_HINT,
	      MSG_UNDO,
	      MSG_SEPARATOR,
	      MSG_NEW_GAME_EASY,
	      MSG_NEW_GAME_DIFFICULT,
	      MSG_NEW_GAME_FOUR_BRIDGES,
	      MSG_SEPARATOR,
	      MSG_EXIT,
	      MSG_NONE
	    };
	    if (undo_stack.count != 0)
	      show_popup(NULL, MSG_NONE, game_menu_with_undo, menu_handler);
	    else
	      show_popup(NULL, MSG_NONE, game_menu, menu_handler);
	    return 1;
	  }
	}
      break;
    case EVT_POINTERDOWN:
      {
	int i;
	int rx, ry;

	point_change_orientation(par1, par2, GetOrientation(), &rx, &ry);

	for (i = 0; i < g_selectable->count; ++i)
	  {
	    struct rect r;
	    cell_rect(&g_selectable->positions[i], &r);
	    if (point_in_rect(rx, ry, &r))
	      {
		int prev_caret_pos = caret_pos;
		struct rect prev_r;

		caret_pos = i;

		main_repaint();
		cell_rect(&g_selectable->positions[prev_caret_pos], &prev_r);
		PartialUpdate(prev_r.x, prev_r.y, prev_r.w, prev_r.h);

		select_cell();
		break;
	      }
	  }
      }
      break;
    }
  return 0;
}

static void make_hint(void)
{
  int i, j;

  while (1)
    {
      for (i = help_index; i < g_selectable->count - 1; ++i)
	{
	  const chip_t chip1 = board_get(&g_board, &g_selectable->positions[i]);

	  for (j = i + 1 + help_offset; j < g_selectable->count; ++j)
	    {
	      const chip_t chip2 = board_get(&g_board, &g_selectable->positions[j]);

	      if (fits(chip1, chip2))
		{
		  help_index = i;
		  help_offset = j - i - 1 + 1;
		  selection_pos = i;
		  caret_pos = j;
		  return;
		}
	    }
	  help_offset = 0;
	}
      help_index = 0;
      help_offset = 0;
    }
}

static message_id main_menu_wo_load[] = {
  MSG_NEW_GAME_EASY,
  MSG_NEW_GAME_DIFFICULT,
  MSG_NEW_GAME_FOUR_BRIDGES,
  MSG_SEPARATOR,
  MSG_TOGGLE_LANGUAGE,
  MSG_CHANGE_ORIENTATION,
  MSG_SEPARATOR,
  MSG_EXIT,
  MSG_NONE
};

static message_id main_menu_w_load[] = {
  MSG_NEW_GAME_EASY,
  MSG_NEW_GAME_DIFFICULT,
  MSG_NEW_GAME_FOUR_BRIDGES,
  MSG_SEPARATOR,
  MSG_LOAD,
  MSG_SEPARATOR,
  MSG_TOGGLE_LANGUAGE,
  MSG_CHANGE_ORIENTATION,
  MSG_SEPARATOR,
  MSG_EXIT,
  MSG_NONE
};

static message_id *main_menu;

static void menu_handler(int index)
{
  switch (index)
    {
    case MSG_CONTINUE:
      SetEventHandler(game_handler);
      break;

    case MSG_HINT:
      make_hint();
      SetEventHandler(game_handler);
      break;

    case MSG_UNDO:
      undo();
      SetEventHandler(game_handler);
      break;

    case MSG_NEW_GAME_EASY:
      init_map(&standard_map);
      SetEventHandler(game_handler);
      break;

    case MSG_NEW_GAME_DIFFICULT:
      init_map(&difficult_map);
      SetEventHandler(game_handler);
      break;

    case MSG_NEW_GAME_FOUR_BRIDGES:
      init_map(&four_bridges_map);
      SetEventHandler(game_handler);
      break;

    case MSG_LOAD:
      if (load_game())
	{
 	  start_game();
	  SetEventHandler(game_handler);
	}
      break;

    case MSG_TOGGLE_LANGUAGE:
      if (current_language == ENGLISH)
	current_language = RUSSIAN;
      else
	current_language = ENGLISH;
      show_popup(&background, MSG_NONE, main_menu, menu_handler);
      break;

    case MSG_CHANGE_ORIENTATION:
      if (orientation == ROTATE270)
        orientation = ROTATE90;
      else
        orientation = ROTATE270;
      SetOrientation(orientation);
      ClearScreen();
      StretchBitmap(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (ibitmap*)&background, 0);
      FullUpdate();
      show_popup(&background, MSG_NONE, main_menu, menu_handler);
      break;

    case MSG_EXIT:
      write_state();
      if (game_active)
	save_game();
      CloseApp();
      break;
    }
}

static int main_handler(int type, int par1, int par2)
{
  switch (type)
    {
    case EVT_INIT:
      srand(time(NULL));
      bitmaps_init();
      read_state();
      SetOrientation(orientation);
      if (!access(SAVED_GAME_PATH, R_OK))
	main_menu = main_menu_w_load;
      else
	main_menu = main_menu_wo_load;
      
      show_popup(&background, MSG_NONE, main_menu, menu_handler);
      break;
    case EVT_EXIT:
      if (game_active)
	save_game();
      break;
    }
  return 0;
}

static void read_state(void)
{
  FILE *f = fopen(STATEPATH "/pb-mahjong", "r");
  if (!f)
    return;

  regex_t reg;
  regcomp(&reg, "^([a-z]+)([ \t]*)=([ \t]*)([^ \t\r\n]+)", REG_EXTENDED);

  char line[128];
  regmatch_t pmatch[16];

  while (fgets(line, sizeof(line), f))
    {
      if (regexec(&reg, line, 16, pmatch, 0))
	continue;

      const char *key = &line[pmatch[1].rm_so];
      line[pmatch[1].rm_eo] = '\0';

      const char *value = &line[pmatch[4].rm_so];
      line[pmatch[4].rm_eo] = '\0';

      if (!strcmp(key, "language"))
	{
	  if (!strcmp(value, "en"))
	    current_language = ENGLISH;
	  else if (!strcmp(value, "ru"))
	    current_language = RUSSIAN;
	}
      else if (!strcmp(key, "orientation"))
	{
	  if (!strcmp(value, "90"))
	    orientation = ROTATE90;
	  else if (!strcmp(value, "270"))
	    orientation = ROTATE270;
	}
    }
  fclose(f);

  regfree(&reg);
}

static void write_state(void)
{
  FILE *f = fopen(STATEPATH "/pb-mahjong", "w");
  if (!f)
    return;

  if (current_language == ENGLISH)
    fprintf(f, "language = en\n");
  else if (current_language == RUSSIAN)
    fprintf(f, "language = ru\n");

  if (orientation == ROTATE90)
    fprintf(f, "orientation = 90\n");
  else if (orientation == ROTATE270)
    fprintf(f, "orientation = 270\n");

  fclose(f);
}

static int load_game(void)
{
  int i, j, k;

  FILE *f = fopen(SAVED_GAME_PATH, "r");
  if (!f)
    return 0;

  fscanf(f, "%d %d\n", &row_count, &col_count);

  for (i = 0; i < MAX_ROW_COUNT; ++i)
    for (j = 0; j < MAX_COL_COUNT; ++j)
      for (k = 0; k < MAX_HEIGHT; ++k)
	{
	  int ch;
	  position_t pos;
	  pos.y = i;
	  pos.x = j;
	  pos.k = k;

	  fscanf(f, "%d\n", &ch);

	  board_set(&g_board, &pos, ch);
	}

  fscanf(f, "%d\n", &undo_stack.count);
  for (i = 0; i < undo_stack.count; ++i)
    fscanf(f, "%d %d %d %d\n",
	   &undo_stack.positions[i].y,
	   &undo_stack.positions[i].x,
	   &undo_stack.positions[i].k,
	   &undo_stack.chips[i]);

  fclose(f);

  return 1;
}

static void save_game(void)
{
  int i, j, k;

  FILE *f = fopen(SAVED_GAME_PATH, "w");
  if (!f)
    return;

  fprintf(f, "%d %d\n", row_count, col_count);

  for (i = 0; i < MAX_ROW_COUNT; ++i)
    for (j = 0; j < MAX_COL_COUNT; ++j)
      for (k = 0; k < MAX_HEIGHT; ++k)
	{
	  chip_t ch;
	  position_t pos;
	  pos.y = i;
	  pos.x = j;
	  pos.k = k;
	  ch = board_get(&g_board, &pos);

	  fprintf(f, "%d\n", ch);
	}

  fprintf(f, "%d\n", undo_stack.count);
  for (i = 0; i < undo_stack.count; ++i)
    fprintf(f, "%d %d %d %d\n",
	    undo_stack.positions[i].y,
	    undo_stack.positions[i].x,
	    undo_stack.positions[i].k,
	    undo_stack.chips[i]);

  fclose(f);
}

int main(int argc, char **argv)
{
  InkViewMain(main_handler);
  return 0;
}

