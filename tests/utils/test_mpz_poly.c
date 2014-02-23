#include "cado.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mpz_poly.h"
#include "tests_common.h"

/* put random coefficients of k bits in a polynomial (already initialized) */
static void
mpz_poly_random (mpz_poly_t f, int d, int k)
{
  int i;
  mpz_t u;

  ASSERT_ALWAYS (k > 0);
  ASSERT_ALWAYS (d < f->alloc);
  mpz_init_set_ui (u, 1);
  mpz_mul_2exp (u, u, k - 1); /* u = 2^(k-1) */
  for (i = 0; i <= d; i++)
    {
      mpz_rrandomb (f->coeff[i], state, k); /* 0 to 2^k-1 */
      mpz_sub (f->coeff[i], f->coeff[i], u); /* -2^(k-1) to 2^(k-1)-1 */
    }
  mpz_clear (u);
  while (d >= 0 && mpz_cmp_ui (f->coeff[d], 0) == 0)
    d--;
  f->deg = d;
}

static void
mpz_poly_mul_xk (mpz_poly_t f, int k)
{
  int i;

  ASSERT_ALWAYS (k >= 0);

  if (k == 0)
    return;

  mpz_poly_realloc (f, f->deg + k + 1);
  for (i = f->deg; i >= 0; i--)
    mpz_set (f->coeff[i + k], f->coeff[i]);
  for (i = 0; i < k; i++)
    mpz_set_ui (f->coeff[i], 0);
  f->deg += k;
}

void
test_polymodF_mul ()
{
  int d1, d2, d;
  mpz_poly_t F, T, U;
  polymodF_t P1, P2, Q, P1_saved;
  int k = 2 + (lrand48 () % 128), count = 0;
  mpz_t c;

  mpz_poly_init (T, -1);
  mpz_poly_init (U, -1);
  mpz_init (c);
  for (d = 1; d <= 10; d++)
    {
      mpz_poly_init (F, d);
      mpz_poly_init (Q->p, d-1);
      do mpz_poly_random (F, d, k); while (F->deg == -1);
      for (d1 = 1; d1 <= 10; d1++)
        {
          mpz_poly_init (P1->p, d1);
          mpz_poly_init (P1_saved->p, d1);
          mpz_poly_random (P1->p, d1, k);
          mpz_poly_copy (P1_saved->p, P1->p);
          P1->v = 0;
          for (d2 = 1; d2 <= 10; d2++)
            {
              mpz_poly_init (P2->p, d2);
              mpz_poly_random (P2->p, d2, k);
              P2->v = 0;
              if ((++count % 3) == 0)
                polymodF_mul (Q, P1, P2, F);
              else if ((count % 3) == 1)
                {
                  polymodF_mul (P1, P1, P2, F);
                  mpz_poly_copy (Q->p, P1->p);
                  Q->v = P1->v;
                  mpz_poly_copy (P1->p, P1_saved->p);
                  P1->v = 0;
                }
              else
                {
                  polymodF_mul (P1, P2, P1, F);
                  mpz_poly_copy (Q->p, P1->p);
                  Q->v = P1->v;
                  mpz_poly_copy (P1->p, P1_saved->p);
                  P1->v = 0;
                }
              /* check that Q->p = lc(F)^Q->v * P1 * P1 mod F */
              ASSERT_ALWAYS (Q->p->deg < F->deg);
              mpz_poly_mul (T, P1->p, P2->p);
              mpz_pow_ui (c, F->coeff[F->deg], Q->v);
              mpz_poly_mul_mpz (T, T, c);
              mpz_poly_sub (T, T, Q->p);
              /* T should be a multiple of F */
              while (T->deg >= F->deg)
                {
                  int oldd = T->deg;
                  if (!mpz_divisible_p (T->coeff[T->deg], F->coeff[F->deg]))
                    {
                      printf ("Error in test_polymodF_mul\n");
                      printf ("F="); mpz_poly_fprintf (stdout, F);
                      printf ("P1="); mpz_poly_fprintf (stdout, P1->p);
                      printf ("P2="); mpz_poly_fprintf (stdout, P2->p);
                      printf ("Q="); mpz_poly_fprintf (stdout, Q->p);
                      exit (1);
                    }
                  mpz_divexact (c, T->coeff[T->deg], F->coeff[F->deg]);
                  mpz_poly_mul_mpz (U, F, c);
                  /* multiply U by x^(T->deg - F->deg) */
                  mpz_poly_mul_xk (U, T->deg - F->deg);
                  mpz_poly_sub (T, T, U);
                  ASSERT_ALWAYS (T->deg < oldd);
                }
              if (T->deg != -1)
                {
                  printf ("count=%d\n", count);
                }
              ASSERT_ALWAYS (T->deg == -1);
              mpz_poly_clear (P2->p);
            }
          mpz_poly_clear (P1->p);
        }
      mpz_poly_clear (F);
      mpz_poly_clear (Q->p);
    }
  mpz_poly_clear (T);
  mpz_poly_clear (U);
  mpz_clear (c);
}

void
test_mpz_poly_roots_mpz (unsigned long iter)
{
  mpz_t r[10], f[10], p, res;
  unsigned long i, n, d;
  mpz_poly_t F;

  for (i = 0; i < 10; i++)
    {
      mpz_init (r[i]);
      mpz_init (f[i]);
    }
  mpz_init (p);
  mpz_init (res);

  /* -16*x^2 - x - 2 mod 17 */
  mpz_set_si (f[2], -16);
  mpz_set_si (f[1], -1);
  mpz_set_si (f[0], -2);
  mpz_set_ui (p, 17);
  n = mpz_poly_roots_mpz (r, f, 2, p);
  ASSERT_ALWAYS(n == 2);
  ASSERT_ALWAYS(mpz_cmp_ui (r[0], 2) == 0);
  ASSERT_ALWAYS(mpz_cmp_ui (r[1], 16) == 0);

  /* try random polynomials */
  for (i = 0; i < iter; i++)
    {
      d = 1 + (lrand48 () % 7);
      for (n = 0; n <= d; n++)
        mpz_set_si (f[n], mrand48 ());
      mpz_urandomb (p, state, 128);
      mpz_nextprime (p, p);
      ASSERT_ALWAYS (mpz_cmp_ui (f[d], 0) != 0);
      n = mpz_poly_roots_mpz (r, f, d, p);
      ASSERT_ALWAYS (n <= d);
      while (n-- > 0)
        {
          F->coeff = f;
          F->deg = d;
          mpz_poly_eval (res, F, r[n]);
          ASSERT_ALWAYS (mpz_divisible_p (res, p));
        }
    }

  for (i = 0; i < 10; i++)
    {
      mpz_clear (r[i]);
      mpz_clear (f[i]);
    }
  mpz_clear (p);
  mpz_clear (res);
}

void
test_mpz_poly_sqr_mod_f_mod_mpz (unsigned long iter)
{
  while (iter--)
    {
      mpz_poly_t Q, P, f;
      mpz_t m, invm;
      int k = 2 + (lrand48 () % 127);
      int d = 1 + (lrand48 () % 7);

      mpz_init (m);
      do mpz_urandomb (m, state, k); while (mpz_tstbit (m, 0) == 0);
      mpz_poly_init (f, d);
      mpz_init (invm);
      while (1)
        {
          mpz_poly_random (f, d, k);
          if (f->deg < d)
            continue;
          mpz_gcd (invm, m, f->coeff[d]);
          if (mpz_cmp_ui (invm, 1) == 0)
            break;
        }
      barrett_init (invm, m);
      mpz_poly_init (P, d - 1);
      if (iter)
        mpz_poly_random (P, d - 1, k);
      else
        P->deg = -1; /* P=0 */
      mpz_poly_init (Q, d - 1);
      mpz_poly_sqr_mod_f_mod_mpz (Q, P, f, m, invm);
      mpz_poly_clear (f);
      mpz_poly_clear (P);
      mpz_poly_clear (Q);
      mpz_clear (m);
      mpz_clear (invm);
    }
}

int
main (int argc, const char *argv[])
{
  unsigned long iter = 500;
  tests_common_cmdline(&argc, &argv, PARSE_SEED | PARSE_ITER);
  tests_common_get_iter(&iter);
  test_polymodF_mul ();
  test_mpz_poly_roots_mpz (iter);
  test_mpz_poly_sqr_mod_f_mod_mpz (iter);
  tests_common_clear ();
  exit (EXIT_SUCCESS);
}
