#include <string.h>

#include "common.h"

void swap(void *array, int i1, int i2, size_t size)
{
  void *temp;

  if (i1 == i2)
    return;

  temp = malloc(size);
  
  memcpy(temp, (char*)array + i1*size, size);
  memcpy((char*)array + i1*size, (char*)array + i2*size, size);
  memcpy((char*)array + i2*size, temp, size);

  free(temp);
}

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
  int m;
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
  int i;

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

