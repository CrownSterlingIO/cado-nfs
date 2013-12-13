#include "cado.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mod_ul.c"
#include "portability.h"
#include "utils.h"

#ifdef FOR_FFS
#include "fppol.h"
#include "fq.h"
#include "utils_ffs.h"
#endif

static void declare_usage(param_list pl)
{
  param_list_decl_usage(pl, "poly", "input polynomial file");
  param_list_decl_usage(pl, "renumber", "input file for renumbering table");
  param_list_decl_usage(pl, "lpb0", "large prime bound on side 0");
  param_list_decl_usage(pl, "lpb1", "large prime bound on side 1");
  param_list_decl_usage(pl, "check", "(switch) check the renumbering table");
}

static void
usage (param_list pl, char *argv0)
{
    param_list_print_usage(pl, argv0, stderr);
    exit(EXIT_FAILURE);
}

int
main (int argc, char *argv[])
{
    int check = 0;
    char *argv0 = argv[0];
    cado_poly cpoly;
    renumber_t tab;
    unsigned long lpb[2] = {0, 0};

    param_list pl;
    param_list_init(pl);
    declare_usage(pl);
    param_list_configure_switch(pl, "check", &check);

    argv++, argc--;
    if (argc == 0)
      usage (pl, argv0);

    for( ; argc ; ) {
        if (param_list_update_cmdline(pl, &argc, &argv))
            continue;
        fprintf(stderr, "Unhandled parameter %s\n", argv[0]);
        usage (pl, argv0);
    }
    /* print command-line arguments */
    param_list_print_command_line (stdout, pl);
    fflush(stdout);

    const char *polyfilename = param_list_lookup_string(pl, "poly");
    const char *renumberfilename = param_list_lookup_string(pl, "renumber");

    param_list_parse_ulong(pl, "lpb0", &lpb[0]);
    param_list_parse_ulong(pl, "lpb1", &lpb[1]);

    if (polyfilename == NULL)
    {
      fprintf (stderr, "Error, missing -poly command line argument\n");
      usage (pl, argv0);
    }
    if (renumberfilename == NULL)
    {
      fprintf (stderr, "Error, missing -renumber command line argument\n");
      usage (pl, argv0);
    }

    cado_poly_init(cpoly);
#ifndef FOR_FFS
    if (!cado_poly_read (cpoly, polyfilename))
#else
    if (!ffs_poly_read (cpoly, polyfilename))
#endif
    {
      fprintf (stderr, "Error reading polynomial file\n");
      exit (EXIT_FAILURE);
    }

    renumber_debug_print_tab(stdout, renumberfilename, cpoly, lpb);

  /* Check for all index i if it can retrieved p and r from it and if it can
   * retrieved the same index from this p and r
   */
  if (check)
  {
    renumber_init (tab, cpoly, lpb);
    renumber_read_table (tab, renumberfilename);

    index_t i, j;
    p_r_values_t p, r;
    int side;
    for (i = 0; i < tab->size; i++)
    {
      if (tab->table[i] != RENUMBER_SPECIAL_VALUE)
      {
        renumber_get_p_r_from_index(tab, &p, &r, &side, i, cpoly);
        j = renumber_get_index_from_p_r (tab, p, r, side);
        if (i == j)
          fprintf (stderr, "## %" PRxid ": Ok\n", i);
        else
          fprintf (stderr, "#### %" PRxid ": Error, p = %" PRpr " r=%" PRpr " "
                           "give index j = %" PRxid "\n", i, p, r, j);
      }
    }

    renumber_free (tab);
  }

    cado_poly_clear (cpoly);
    param_list_clear(pl);
    return 0;
}