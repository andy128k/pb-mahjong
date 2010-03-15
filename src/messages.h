#ifndef MESSAGES_H
#define MESSAGES_H

typedef enum {
  MSG_SEPARATOR = -1,
  MSG_NONE = 0,
#define MESSAGE(c, e, r) MSG_##c,
#include "messages.inc"
#undef MESSAGE
  MSG_COUNT
} message_id;

typedef enum {
  ENGLISH,
  RUSSIAN
} language_t;

extern language_t current_language;

const char * get_message_ex(message_id id, language_t language);

#define get_message(id) (get_message_ex((id), current_language))
  
#endif

