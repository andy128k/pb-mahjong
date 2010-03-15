#include "menu.h"

#define MENU_FONT_NAME (DEFAULTFONT)
#define MENU_FONT_SIZE (18)
#define MENU_MARGIN (10)
#define MENU_ITEM_HEIGHT (30)
#define MENU_SEPARATOR_HEIGHT (5)

typedef struct {
  const ibitmap *background;
  message_id message;

  message_id *items;
  int count;
  iv_menuhandler proc;
  int current;

  int calc_required;
  int x, y, w, h;
} menu_t;

static ifont *g_menu_font = NULL;
static ifont *get_menu_font(void)
{
  if (g_menu_font == NULL)
    g_menu_font = OpenFont(MENU_FONT_NAME, MENU_FONT_SIZE, 1);
  return g_menu_font;
}

void menu_calc(menu_t *menu)
{
  if (!menu->calc_required)
    return;
  menu->calc_required = 1;

  SetFont(get_menu_font(), BLACK);

  int i;
  menu->w = 0;
  menu->h = 0;
  for (i = 0; i < menu->count; ++i)
    {
      if (menu->items[i] > MSG_NONE)
	{
	  const int lw = StringWidth((char*)get_message(menu->items[i]));
	  if (lw > menu->w)
	    menu->w = lw;
	  menu->h += MENU_ITEM_HEIGHT;
	}
      else /*separator*/
	{
	  menu->h += MENU_SEPARATOR_HEIGHT;
	}
    }

  menu->x = (ScreenWidth() - menu->w - 2 * MENU_MARGIN) / 2 + MENU_MARGIN;
  menu->y = ScreenHeight() - menu->h - MENU_MARGIN - 30;
}

static void draw_popup(menu_t *menu)
{
  int i, y;

  menu_calc(menu);

  SetFont(get_menu_font(), BLACK);

  FillArea(menu->x - MENU_MARGIN,
	   menu->y - MENU_MARGIN,
	   menu->w + 2 * MENU_MARGIN,
	   menu->h + 2 * MENU_MARGIN,
	   WHITE);
  DrawRect(menu->x - MENU_MARGIN,
	   menu->y - MENU_MARGIN,
	   menu->w + 2 * MENU_MARGIN,
	   menu->h + 2 * MENU_MARGIN,
	   BLACK);

  y = menu->y;
  for (i = 0; i < menu->count; ++i)
    {
      if (menu->items[i] > MSG_NONE)
	{
	  DrawTextRect(menu->x,
		       y,
		       menu->w,
		       MENU_ITEM_HEIGHT,
		       (char*)get_message(menu->items[i]),
		       ALIGN_LEFT | VALIGN_MIDDLE);

	  if (i == menu->current)
	    DrawSelection(menu->x - 5, y, menu->w + 10, MENU_ITEM_HEIGHT, DGRAY);

	  y += MENU_ITEM_HEIGHT;
	}
      else /*separator*/
	{
	  const int sy = y + MENU_SEPARATOR_HEIGHT / 2;
	  DrawLine(menu->x, sy, menu->x + menu->w, sy, BLACK);
	  y += MENU_SEPARATOR_HEIGHT;
	}
    }
}

static void menu_update(menu_t *menu)
{
  PartialUpdate(menu->x - MENU_MARGIN,
		menu->y - MENU_MARGIN,
		menu->w + 2 * MENU_MARGIN,
		menu->h + 2 * MENU_MARGIN);
}

static menu_t g_menu1;

static int menu_handler(int type, int par1, int par2)
{
  menu_t *menu = &g_menu1;

  switch (type)
    {
    case EVT_SHOW:
      if (menu->background != NULL || menu->message != MSG_NONE)
	ClearScreen();
      if (menu->background != NULL)
	StretchBitmap(0, 0, ScreenWidth(), ScreenHeight(), (ibitmap*)menu->background, 0);
      if (menu->message != MSG_NONE)
	{
	  const int x_margin = 100;
	  const int y_margin = 130;
	  const int height = 150;
	  FillArea(x_margin, y_margin, ScreenWidth() - 2 * x_margin, height, WHITE);
	  DrawSelection(x_margin, y_margin, ScreenWidth() - 2 * x_margin, height, DGRAY);

	  ifont *font = OpenFont(MENU_FONT_NAME, 40, 1);
	  SetFont(font, BLACK);
	  DrawTextRect(x_margin, y_margin, ScreenWidth() - 2 * x_margin, height, (char*)get_message(menu->message), ALIGN_CENTER | VALIGN_MIDDLE);

	  CloseFont(font);
	}
      draw_popup(menu);
      if (menu->background != NULL || menu->message != MSG_NONE)
	FullUpdate();
      else
	menu_update(menu);
      break;
    case EVT_KEYPRESS:
      switch (par1)
	{
	case KEY_UP:
	  do
	    {
	      menu->current = (menu->current + menu->count - 1) % menu->count;
	    }
	  while (menu->items[menu->current] <= MSG_NONE);

	  draw_popup(menu);
	  menu_update(menu);
	  break;
	case KEY_DOWN:
	  do
	    {
	      menu->current = (menu->current + 1) % menu->count;
	    }
	  while (menu->items[menu->current] <= MSG_NONE);

	  draw_popup(menu);
	  menu_update(menu);
	  break;
	case KEY_OK:
	  menu->proc(menu->items[menu->current]);
	  break;
	}
      break;
    }
  return 0;
}

void show_popup(const ibitmap* background, message_id message, message_id *items, iv_menuhandler hproc)
{
  menu_t *menu = &g_menu1;

  menu->calc_required = 1;
  menu->background = background;
  menu->message = message;
  menu->items = items;
  menu->count = 0;
  while (items[menu->count] != MSG_NONE)
    ++menu->count;
  menu->proc = hproc;
  menu->current = 0;

  SetEventHandler(menu_handler);
}

