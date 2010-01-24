/* complete_rels: same as check_rels, but completes small factors if omitted */

#define _POSIX_C_SOURCE 2  /* popen/pclose with -std=c99, -pedantic or -ansi */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <time.h>
#include <gmp.h>
#include "cado.h"
#include "utils.h"

void
norm (mpz_t *f, int deg, mpz_t r, long a, unsigned long b)
{
  int i;
  mpz_t tmp;

  mpz_init_set_ui (tmp, 1);
  mpz_set (r, f[deg]);
  for (i = deg - 1; i >= 0; i--)
    {
      mpz_mul_si (r, r, a);
      mpz_mul_ui (tmp, tmp, b);
      mpz_addmul (r, tmp, f[i]);
    }
  mpz_abs (r, r);
  mpz_clear (tmp);
}

int
is_gzip (const char * s)
{
    unsigned int l = strlen(s);
    return l >= 3 && strcmp(s + l - 3, ".gz") == 0;
}

void
add_alg (relation_t *rel, unsigned long p)
{
  int n = rel->nb_ap;

  rel->ap = realloc (rel->ap, (n + 1) * sizeof (alg_prime_t));
  (rel->ap[n]).p = p;
  (rel->ap[n]).e = 1;
  rel->nb_ap = n + 1;
}

void
add_rat (relation_t *rel, unsigned long p)
{
  int n = rel->nb_rp;

  rel->rp = realloc (rel->rp, (n + 1) * sizeof (rat_prime_t));
  (rel->rp[n]).p = p;
  (rel->rp[n]).e = 1;
  rel->nb_rp = n + 1;
}

int
check_relation (relation_t *rel, cado_poly_ptr cpoly)
{
  mpz_t no;
  int i, j;
  unsigned long p;

  mpz_init (no);

  // algebraic side
  norm (cpoly->f, cpoly->degree, no, rel->a, rel->b);
  for (i = 0; i < rel->nb_ap; ++i)
    {
      for (j = 0; j < (rel->ap[i]).e; ++j)
	if (mpz_divisible_ui_p (no, (rel->ap[i]).p) == 0)
	  {
	    fprintf (stderr, "Wrong algebraic side for (%ld, %lu)\n",
		     rel->a, rel->b);
	    fprintf (stderr, "Given factor %lu does not divide norm\n",
		     (rel->ap[i]).p);
	    mpz_clear (no);
	    return 0;
	  }
	else
	  mpz_divexact_ui (no, no, (rel->ap[i]).p);
    }
  for (p = 2; mpz_cmp_ui (no, 1) != 0; p += 1 + (p != 2))
    {
      while (mpz_divisible_ui_p (no, p))
	{
	  add_alg (rel, p);
	  mpz_divexact_ui (no, no, p);
	}
    }

  // rational side
  norm (cpoly->g, 1, no, rel->a, rel->b);
  for (i = 0; i < rel->nb_rp; ++i)
    {
      for (j = 0; j < (rel->rp[i]).e; ++j)
	if (mpz_divisible_ui_p (no, (rel->rp[i]).p) == 0)
	  {
	    fprintf (stderr, "Wrong rational side for (%ld, %lu)\n",
		     rel->a, rel->b);
	    fprintf (stderr, "Given factor %lu does not divide norm\n",
		     (rel->rp[i]).p);
	    mpz_clear (no);
	    return 0;
	  }
	else
	  mpz_divexact_ui (no, no, (rel->rp[i]).p);
    }
  for (p = 2; mpz_cmp_ui (no, 1) != 0; p += 1 + (p != 2))
    {
      while (mpz_divisible_ui_p (no, p))
	{
	  add_rat (rel, p);
	  mpz_divexact_ui (no, no, p);
	}
    }

    return 1;
}

#define MAX_LENGTH 512

int check_stream(const char *name, FILE * stream, cado_poly_ptr cpoly)
{
    int lnum;
    int nrels = 0;

    for (lnum = 1;; lnum++) {
        char line[MAX_LENGTH];
        if (fgets(line, sizeof(line), stream) == NULL)
            break;
        if (line[strlen(line) - 1] != '\n')
          {
            fprintf (stderr, "Line %d of %s is buggy or too long, please check or increase MAX_LENGTH in check_rels.c\n", lnum, name);
            exit (1);
          }
        if (line[0] == '#')
            continue;

        relation_t rel;
        if (!read_relation(&rel, line)) {
            fprintf(stderr, "Failed on line %d in %s: %s\n", lnum, name, line);
            fprintf(stderr,
                    "File check aborted. Please fix this file and re-run.\n");
            fprintf(stderr, "Continuing with next file\n");
            return 1;
        }
        if (!check_relation(&rel, cpoly)) {
            fprintf(stderr, "Failed on line %d in %s: %s\n", lnum, name, line);
            fprintf(stderr,
                    "File check aborted. Please fix this file and re-run.\n");
            fprintf(stderr, "Continuing with next file\n");
            return 1;
        }

	fprint_relation (stdout, rel);

        clear_relation(&rel);
        nrels++;
    }
    fprintf(stderr, "%s : ok %d rels\n", name, nrels);
    return 0;
}


void usage_and_die(char *str) {
    fprintf(stderr, "usage: %s -poly <polyfile> <relfile1> <relfile2> ...\n",
            str);
    exit(3);
}

int main(int argc, char * argv[])
{
    cado_poly cpoly;
    int i, had_error = 0;

    if (argc < 4 || strcmp(argv[1], "-poly") != 0) {
        usage_and_die(argv[0]);
    }
    cado_poly_init(cpoly);
    if (!cado_poly_read(cpoly, argv[2])) 
        return 2;

    for(i = 3 ; i < argc ; i++) {
        FILE * f;
        if (is_gzip(argv[i])) {
            char command[1024];
            snprintf(command, sizeof(command), "gzip -dc %s", argv[i]);
            f = popen(command, "r");
            if (check_stream(argv[i], f, cpoly) != 0)
                had_error = 1;
            pclose(f);
        } else {
            f = fopen(argv[i], "r");
            if (check_stream(argv[i], f, cpoly) != 0)
                had_error = 1;
            fclose(f);
        }
    }

    cado_poly_clear(cpoly);

    return had_error;
}