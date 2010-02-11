#include <string.h>

#include "common.h"

static inline void swap_b(void *e1, void *e2, size_t size, void *b)
{
  if (e1 != e2)
    {
      memcpy(b, e1, size);
      memcpy(e1, e2, size);
      memcpy(e2, b, size);
    }
}

int rrand(int m)
{
  return rand() % m;
}

void shuffle(void *array, size_t nmemb, size_t size)
{
  void *temp = malloc(size);
  size_t n = nmemb;
  while (n > 1)
    {
      size_t k = rrand(n);
      swap_b((char*)array + (n-1)*size, (char*)array + k*size, size, temp);
      --n;
    }
  free(temp);
}

struct ts_data {
  void *array;
  size_t nmemb;
  size_t size;
  int (*has_edge)(const void*, const void*);

  void *result;
  int result_count;
  char *visited;
};

static void visit(struct ts_data *ts_data, int n)
{
  size_t m;
  void *nth_ptr;

  if (ts_data->visited[n])
    return;
  ts_data->visited[n] = 1;

  nth_ptr = (char*)ts_data->array + n * ts_data->size;
  for (m = 0; m < ts_data->nmemb; ++m)
    {
      void *mth_ptr = (char*)ts_data->array + m * ts_data->size;
      if (ts_data->has_edge(nth_ptr, mth_ptr))
	visit(ts_data, m);
    }

  memcpy((char*)ts_data->result + ts_data->result_count * ts_data->size, nth_ptr, ts_data->size);
  ++ts_data->result_count;
}

void topological_sort(void *array, size_t nmemb, size_t size, int (*has_edge)(const void*, const void*))
{
  size_t i;

  struct ts_data ts_data;

  ts_data.array = array;
  ts_data.nmemb = nmemb;
  ts_data.size = size;
  ts_data.has_edge = has_edge;
  ts_data.result = malloc(nmemb * size);
  ts_data.result_count = 0;
  ts_data.visited = calloc(nmemb, sizeof(char));
  
  for (i = 0; i < nmemb; ++i)
    visit(&ts_data, i);
  
  memcpy(array, ts_data.result, nmemb * size);

  free(ts_data.result);
  free(ts_data.visited);
}

