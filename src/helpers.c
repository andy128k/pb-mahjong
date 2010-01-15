#include "helpers.h"

imenu *g_menu;
iv_menuhandler g_hproc;

static void popup_handler(int index)
{
  if (index <= 0)
    OpenMenu(g_menu, 0, -1, -1, popup_handler);
  else
    g_hproc(index);
}

void show_popup_strict(imenu *menu, iv_menuhandler hproc)
{
  g_menu = menu;
  g_hproc = hproc;
  OpenMenu(menu, 0, -1, -1, popup_handler);
}

