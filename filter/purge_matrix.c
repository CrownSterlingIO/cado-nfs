#include "cado.h"
#include <stdio.h>
#include <stdlib.h>

#include "portability.h"

#include "filter_common.h"
#include "purge_matrix.h"


void
purge_matrix_init (purge_matrix_ptr mat, uint64_t nrows_init,
                   uint64_t col_min_index, uint64_t col_max_index)
{
  size_t cur_alloc;
  mat->nrows_init = nrows_init;
  mat->col_max_index = col_max_index;
  mat->nrows = mat->ncols = 0;
  mat->col_min_index = col_min_index;
  mat->tot_alloc_bytes = 0;

  /* Malloc cols_weight ans set to 0 */
  cur_alloc = mat->col_max_index * sizeof (weight_t);
  mat->cols_weight = (weight_t *) malloc(cur_alloc);
  ASSERT_ALWAYS(mat->cols_weight != NULL);
  mat->tot_alloc_bytes += cur_alloc;
  fprintf(stdout, "# MEMORY: Allocated cols_weight of %zuMB (total %zuMB "
                  "so far)\n", cur_alloc >> 20, mat->tot_alloc_bytes >> 20);
  memset(mat->cols_weight, 0, cur_alloc);

  /* Malloc row_used and set to 1 */
  bit_vector_init(mat->row_used, mat->nrows_init);
  cur_alloc = bit_vector_memory_footprint(mat->row_used);
  mat->tot_alloc_bytes += cur_alloc;
  fprintf(stdout, "# MEMORY: Allocated row_used of %zuMB (total %zuMB "
                  "so far)\n", cur_alloc >> 20, mat->tot_alloc_bytes >> 20);
  bit_vector_set(mat->row_used, 1);

  /* Malloc sum2_row */
  cur_alloc = mat->col_max_index * sizeof (uint64_t);
  mat->sum2_row = (uint64_t *) malloc(cur_alloc);
  ASSERT_ALWAYS(mat->sum2_row != NULL);
  mat->tot_alloc_bytes += cur_alloc;
  fprintf(stdout, "# MEMORY: Allocated sum2_row of %zuMB (total %zuMB "
                  "so far)\n", cur_alloc >> 20, mat->tot_alloc_bytes >> 20);

  /* Malloc row_compact */
  cur_alloc = mat->nrows_init * sizeof (index_t *);
  mat->row_compact = (index_t **) malloc (cur_alloc);
  ASSERT_ALWAYS(mat->row_compact != NULL);
  mat->tot_alloc_bytes += cur_alloc;
  fprintf(stdout, "# MEMORY: Allocated row_compact of %zuMB (total %zuMB "
                  "so far)\n", cur_alloc >> 20, mat->tot_alloc_bytes >> 20);
}

/* Field row_compact has its own clear function so we can free the memory as
 * soon as we do not need it anymore */
void
purge_matrix_clear_row_compact (purge_matrix_ptr mat)
{
  if (mat->row_compact != NULL)
  {
    size_t cur_free_size = get_my_malloc_bytes();
    mat->tot_alloc_bytes -= cur_free_size;
    my_malloc_free_all();
    fprintf(stdout, "# MEMORY: Freed row_compact[i] %zuMB (total %zuMB so "
                    "far)\n", cur_free_size >> 20, mat->tot_alloc_bytes >> 20);

    cur_free_size = (mat->nrows_init * sizeof(index_t *));
    mat->tot_alloc_bytes -= cur_free_size;
    free(mat->row_compact);
    mat->row_compact = NULL;
    fprintf(stdout, "# MEMORY: Freed row_compact %zuMB (total %zuMB so far)\n",
                    cur_free_size >> 20, mat->tot_alloc_bytes >> 20);
  }
}

/* Free everything and set everythin to 0. */
void
purge_matrix_clear (purge_matrix_ptr mat)
{
  free (mat->cols_weight);
  bit_vector_clear(mat->row_used);
  free (mat->sum2_row);
  purge_matrix_clear_row_compact (mat);

  memset (mat, 0, sizeof (purge_matrix_t));
}

void
purge_matrix_clear_row_compact_update_mem_usage (purge_matrix_ptr mat)
{
  size_t cur_alloc = get_my_malloc_bytes();
  mat->tot_alloc_bytes += cur_alloc;
  fprintf(stdout, "# MEMORY: Allocated row_compact[i] %zuMB (total %zuMB so "
                  "far)\n", cur_alloc >> 20, mat->tot_alloc_bytes >> 20);
}


/* Set a row of a purge_matrix_t from a read relation.
 * The row that is set is decided by the relation number rel->num
 * We put in mat->row_compact only primes such that their index h is greater or
 * equal to mat->col_min_index.
 * A row in mat->row_compact is ended by a -1 (= UMAX(index_t))
 * The number of columns mat->ncols is updated is necessary.
 * The number of rows mat->ncols is increased by 1.
 *
 * The return type of this function is void * instead of void so we can use it
 * as a callback function for filter_rels.
 *
 * Not thread-safe.
 */

void *
purge_matrix_set_row_from_rel (purge_matrix_t mat, earlyparsed_relation_ptr rel)
{
  ASSERT_ALWAYS(rel->num < mat->nrows_init);

  unsigned int nb_above_min_index = 0;
  for (weight_t i = 0; i < rel->nb; i++)
    nb_above_min_index += (rel->primes[i].h >= mat->col_min_index);

  index_t *tmp_row = index_my_malloc (1 + nb_above_min_index);
  unsigned int next = 0;
  for (weight_t i = 0; i < rel->nb; i++)
  {
    index_t h = rel->primes[i].h;
    if (mat->cols_weight[h] == 0)
    {
      mat->cols_weight[h] = 1;
      (mat->ncols)++;
    }
    else if (mat->cols_weight[h] != UMAX(weight_t))
      mat->cols_weight[h]++;

    if (h >= mat->col_min_index)
      tmp_row[next++] = h;
  }

  tmp_row[next] = UMAX(index_t); /* sentinel */
  mat->row_compact[rel->num] = tmp_row;
  (mat->nrows)++;

  return NULL;
}


/* Delete a row: set mat->row_used[i] to 0, update the count of the columns
 * appearing in that row, mat->nrows and mat->ncols.
 * Warning: we only update the count of columns that we consider, i.e.,
 * columns with index >= mat->col_min_index.
 * CAREFUL: no multithread compatible with " !(--(*o))) " and
 * bit_vector_clearbit.
 */
void
purge_matrix_delete_row (purge_matrix_ptr mat, uint64_t i)
{
  index_t *row;
  weight_t *w;

  for (row = mat->row_compact[i]; *row != UMAX(*row); row++)
  {
    w = &(mat->cols_weight[*row]);
    ASSERT(*w);
    /* Decrease only if not equal to the maximum value */
    /* If weight becomes 0, we just remove a column */
    if (*w < UMAX(*w) && !(--(*w)))
      mat->ncols--;
  }
  /* We do not free mat->row_compact[i] as it is freed later with
     my_malloc_free_all */
  bit_vector_clearbit (mat->row_used, (size_t) i);
  mat->nrows--;
}
