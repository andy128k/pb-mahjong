#include "messages.h"

static struct {
  message_id id;
  const char * en;
  const char * ru;
} g_message_table[] = {
#define MESSAGE(c, e, r) { MSG_##c, e, r },
#include "messages.inc"
#undef MESSAGE
  { MSG_NONE, 0, 0 }
};

language_t current_language = ENGLISH;

const char * get_message_ex(message_id id, language_t language)
{
  if (id <= MSG_NONE || id >= MSG_COUNT)
    return 0;
  if (language == ENGLISH)
    return g_message_table[id - 1].en;
  else
    return g_message_table[id - 1].ru;
}

