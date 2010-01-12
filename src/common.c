#include <string.h>

#include "common.h"

void shuffle(void *obj, size_t nmemb, size_t size)
{
  void *temp = malloc(size);
  size_t n = nmemb;
  while (n > 1)
    {
      size_t k = rrand(n--);
      memcpy(temp, (char*)obj + n*size, size);
      memcpy((char*)obj + n*size, (char*)obj + k*size, size);
      memcpy((char*)obj + k*size, temp, size);
    }
  free(temp);
}

