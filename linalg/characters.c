#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "mod_ul.c"

#define WANT_ASSERT

#include "cado.h"
#include "utils/utils.h"

#include "files.h"

#define DEBUG 0

int kernel(mp_limb_t* mat, mp_limb_t** ker, int nrows, int ncols,
    int limbs_per_row, int limbs_per_col);

#define TYPE_STRING 0
#define TYPE_MPZ 1
#define TYPE_INT 2
#define TYPE_ULONG 3
#define TYPE_DOUBLE 4
#define PARSE_MATCH -1
#define PARSE_ERROR 0
#define PARSE_NOMATCH 1

/* Return value: 1: tag does not match, 0: error occurred, -1 tag did match */

static int
parse_line (void *target, char *line, const char *tag, int *have, 
	    const int type)
{
  char *lineptr = line;

  if (strncmp (lineptr, tag, strlen (tag)) != 0)
    return PARSE_NOMATCH;

  if (have != NULL && *have != 0)
    {
      fprintf (stderr, "parse_line: %s appears twice\n", tag);
      return PARSE_ERROR;
    }
  if (have != NULL)
    *have = 1;
  
  lineptr += strlen (tag);
  if (type == TYPE_STRING) /* character string of length up to 256 */
    {
      strncpy ((char *)target, lineptr, 256);
      ((char *)target)[255] = '0';
    }
  else if (type == TYPE_MPZ)
    {
      mpz_init (*(mpz_t *)target);
      mpz_set_str (*(mpz_t *)target, lineptr, 0);
    }
  else if (type == TYPE_INT)
    *(int *)target = atoi (lineptr);
  else if (type == TYPE_ULONG)
    *(unsigned long *)target = strtoul (lineptr, NULL, 10);
  else if (type == TYPE_DOUBLE)
    *(double *)target = atof (lineptr);
  else return PARSE_ERROR;
  
  return PARSE_MATCH;
}

// FIXME: why not using the one in polyfile?
cado_poly *
my_read_polynomial (char *filename)
{
  FILE *file;
  const int linelen = 512;
  char line[linelen];
  cado_poly *poly;
  int have_name = 0, have_n = 0, have_Y0 = 0, have_Y1 = 0;
  int i, ok = PARSE_ERROR;

  file = fopen (filename, "r");
  if (file == NULL)
    {
      fprintf (stderr, "read_polynomial: could not open %s\n", filename);
      return NULL;
    }

  poly = (cado_poly *) malloc (sizeof(cado_poly));
  (*poly)->f = (mpz_t *) malloc (MAXDEGREE * sizeof (mpz_t));
  (*poly)->g = (mpz_t *) malloc (2 * sizeof (mpz_t));

  (*poly)->name[0] = '\0';
  (*poly)->degree = -1;
  (*poly)->type[0] = '\0';

  while (!feof (file))
    {
      ok = 1;
      if (fgets (line, linelen, file) == NULL)
	break;
      if (line[0] == '#')
	continue;

      ok *= parse_line (&((*poly)->name), line, "name: ", &have_name, TYPE_STRING);
      ok *= parse_line (&((*poly)->n), line, "n: ", &have_n, TYPE_MPZ);
      ok *= parse_line (&((*poly)->skew), line, "skew: ", NULL, TYPE_DOUBLE);
      ok *= parse_line (&((*poly)->g[0]), line, "Y0: ", &have_Y0, TYPE_MPZ);
      ok *= parse_line (&((*poly)->g[1]), line, "Y1: ", &have_Y1, TYPE_MPZ);
      ok *= parse_line (&((*poly)->type), line, "type: ", NULL, TYPE_STRING);
      ok *= parse_line (&((*poly)->rlim), line, "rlim: ", NULL, TYPE_ULONG);
      ok *= parse_line (&((*poly)->alim), line, "alim: ", NULL, TYPE_ULONG);
      ok *= parse_line (&((*poly)->lpbr), line, "lpbr: ", NULL, TYPE_INT);
      ok *= parse_line (&((*poly)->lpba), line, "lpba: ", NULL, TYPE_INT);
      ok *= parse_line (&((*poly)->mfbr), line, "mfbr: ", NULL, TYPE_INT);
      ok *= parse_line (&((*poly)->mfba), line, "mfba: ", NULL, TYPE_INT);
      ok *= parse_line (&((*poly)->rlambda), line, "rlambda: ", NULL, TYPE_DOUBLE);
      ok *= parse_line (&((*poly)->alambda), line, "alambda: ", NULL, TYPE_DOUBLE);
      ok *= parse_line (&((*poly)->qintsize), line, "qintsize: ", NULL, TYPE_INT);


      if (ok == 1 && line[0] == 'c' && isdigit (line[1]) && line[2] == ':' &&
	       line[3] == ' ')
	{
	  int index = line[1] - '0', i;
	  for (i = (*poly)->degree + 1; i <= index; i++)
	    mpz_init ((*poly)->f[i]);
	  if (index > (*poly)->degree)
	    (*poly)->degree = index;
	  mpz_set_str ((*poly)->f[index], line + 4, 0);
	  ok = -1;
	}

      if (ok == PARSE_NOMATCH)
	{
	  fprintf (stderr, 
		   "read_polynomial: Cannot parse line %s\nIgnoring.\n", 
		   line);
	  continue;
	}
      
      if (ok == PARSE_ERROR)
	break;
    }
  
  if (ok != PARSE_ERROR)
    {
      if (have_n == 0)
	{
	  fprintf (stderr, "n ");
	  ok = PARSE_ERROR;
	}
      if (have_Y0 == 0)
	{
	  fprintf (stderr, "Y0 ");
	  ok = PARSE_ERROR;
	}
      if (have_Y1 == 0)
	{
	  fprintf (stderr, "Y1 ");
	  ok = PARSE_ERROR;
	}
      if (ok == PARSE_ERROR)
	fprintf (stderr, "are missing in polynomial file\n");
    }

  if (ok == PARSE_ERROR)
    {
      for (i = 0; i <= (*poly)->degree; i++)
	mpz_clear ((*poly)->f[i]);
      if (have_n)
	mpz_clear ((*poly)->n);
      if (have_Y0)
	mpz_clear ((*poly)->g[0]);
      if (have_Y1)
	mpz_clear ((*poly)->g[1]);
      free ((*poly)->f);
      free ((*poly)->g);
      free (poly);
      poly = NULL;
    }
  
  return poly;
}



// Data structure for one algebraic prime and for a table of those.
typedef struct {
  unsigned long prime;
  unsigned long root;
} rootprime_t;


int eval_char(long a, unsigned long b, rootprime_t ch) {
  unsigned long ua, aux;
  int res;
#if DEBUG >= 1
  printf("a := %ld; b := %lu; p := %lu; r := %lu;", a, b, ch.prime, ch.root);
#endif
  if (a < 0) {
    ua = ((unsigned long) (-a)) % ch.prime;
    b = b % ch.prime;
    modul_mul(&aux, &b, &ch.root, &ch.prime);
    modul_add(&aux, &ua, &aux, &ch.prime);
    modul_neg(&aux, &aux, &ch.prime);
    res = modul_jacobi(&aux, &ch.prime);
  } else {
    ua = ((unsigned long) (a)) % ch.prime;
    b = b % ch.prime;
    modul_mul(&aux, &b, &ch.root, &ch.prime);
    modul_sub(&aux, &ua, &aux, &ch.prime);
    res = modul_jacobi(&aux, &ch.prime);
  }
#if DEBUG >= 1
  printf("res := %d; assert (JacobiSymbol(a-b*r, p) eq res);\n", res);
#endif
  return res;
}

/* norm <- |f(a,b)| = |f[d]*a^d + ... + f[0]*b^d| */
void
eval_algebraic (mpz_t norm, mpz_t *f, int d, long a, unsigned long b)
{
  mpz_t B; /* powers of b */
  mpz_init_set_ui (B, 1);
  mpz_set (norm, f[d]);
  while (d-- > 0)
    {
      mpz_mul_si (norm, norm, a);
      mpz_mul_ui (B, B, b);
      mpz_addmul (norm, f[d], B);
    }
  mpz_clear (B);
  mpz_abs (norm, norm);
}

void
handleOneKer(int *charval, int k, rootprime_t * tabchar, FILE * purgedfile, FILE * kerfile, int nlimbs, cado_poly pol)
{  
  int i, j, jj;
  int ret;
  unsigned long w;
  long a;
  unsigned long b;
  char str[1024];
  mpz_t prd, z;

  mpz_init_set_ui(prd, 1);
  mpz_init(z);

  rewind(purgedfile);
  fgets(str, 1024, purgedfile); // skip first line

  for (i = 0; i < k; ++i) 
    charval[i] = 1;

  for (i = 0; i < nlimbs; ++i) {
    ret = fscanf(kerfile, "%lx", &w);
//    printf("%lx ", w);
    ASSERT (ret == 1);
    for (j = 0; j < GMP_NUMB_BITS; ++j) {
      if (fgets(str, 1024, purgedfile)) {
	if (w & 1UL) {
	  ret = gmp_sscanf(str, "%ld %lu", &a, &b);
	  ASSERT (ret == 2);
	  for (jj = 0; jj < k; ++jj)
	    charval[jj] *= eval_char(a, b, tabchar[jj]);

	  eval_algebraic(z, pol->f, pol->degree, a, b);
#if 0
	  if (mpz_divisible_ui_p(z, 40009)) {
	    fprintf(stderr, "%ld %lu\n", a, b);
	    gmp_fprintf(stderr, "norm = %Zd\n", z);
	  }
#endif
	  mpz_mul(prd, prd, z);
	}
      }
      w >>= 1;
    }
  }

#if 0
  {
    mpz_t q, r;
    mpz_init(q);
    mpz_init(r);
    for(;;) {
      long p;
      int val;
      ret = scanf("%ld", &p);
      if (ret != 1) 
	break;
      val = 0;
      while (mpz_divisible_ui_p(prd, p)) {
	val++;
	mpz_divexact_ui(prd, prd, p);
      }
      fprintf(stderr, "%d\n", val);
    }
    mpz_clear(q);
    mpz_clear(r);
  }
#endif

  mpz_sqrtrem(prd, z, prd);
  if (mpz_cmp_ui(z, 0)!=0)
    fprintf(stderr, "Problem: norm is not a square...\n");

  mpz_clear(prd);
  mpz_clear(z);
}

/* allocate a polynomial of degree d, and initialize it */
mpz_t*
alloc_poly (int d)
{
  mpz_t *f;
  int i;
  
  f = (mpz_t*) malloc ((d + 1) * sizeof (mpz_t));
  for (i = 0; i <= d; i++)
    mpz_init (f[i]);
  return f;
} 

/* free a polynomial of degree d */
void
poly_clear (mpz_t *f, int d)
{
  int i;
  
  for (i = 0; i <= d; i++)
    mpz_clear (f[i]); 
  free (f);
} 

/* f <- g */
void
copy_poly (mpz_t *f, mpz_t *g, int d)
{
  int i;
  
  for (i = 0; i <= d; i++)
    mpz_set (f[i], g[i]);
}   

/* f <- diff(g,x) */
void
diff_poly (mpz_t *f, mpz_t *g, int d)
{
  int i;
  
  for (i = 0; i < d; i++)
    mpz_mul_ui (f[i], g[i + 1], i + 1);
}   
/* Returns the number of roots of f(x) mod p, where f has degree d.
   Assumes p does not divide disc(f).
   In case where nb of root is 1, the root is put in r.
   TODO: share this code with polyselect.c 
   */
int
roots_mod (unsigned long *r, mpz_t *f, int d, const unsigned long p)
{
  mpz_t *fp, *g, *h;
  int i, j, k, df, dg, dh;

  /* the number of roots is the degree of gcd(x^p-x,f) */

  /* we first compute fp = f/lc(f) mod p */
  while (d >= 0 && mpz_divisible_ui_p (f[d], p))
    d --;
  ASSERT (d >= 0); /* f is 0 mod p: should not happen since otherwise p would
		      divide N, because f(m)=N */
  fp = alloc_poly (d);
  mpz_set_ui (fp[d], p);
  mpz_invert (fp[d], f[d], fp[d]); /* 1/f[d] mod p */
  for (i = 0; i < d; i++)
    {
      mpz_mul (fp[i], f[i], fp[d]);
      mpz_mod_ui (fp[i], fp[i], p);
    }
  mpz_set_ui (fp[d], 1); /* useless? */
  df = d;

  /* we first compute x^p mod fp; since fp has degree d, all operations can
     be done with polynomials of degree < 2d */
  g = alloc_poly (2 * d - 1);
  h = alloc_poly (2 * d - 1);

  /* initialize g to x */
  mpz_set_ui (g[1], 1);
  dg = 1;

  mpz_set_ui (g[2], p);
  k = mpz_sizeinbase (g[2], 2); /* number of bits of p: bit 0 to k-1 */

  for (k -= 2; k >= 0; k--)
    {
      /* square g into h: g has degree dg -> h has degree 2*dg */
      for (i = 0; i <= dg; i++)
	for (j = i + 1; j <= dg; j++)
	  if (i == 0 || j == dg)
	    mpz_mul (h[i + j], g[i], g[j]);
	  else
	    mpz_addmul (h[i + j], g[i], g[j]);
      for (i = 1; i < 2 * dg; i++)
	mpz_mul_2exp (h[i], h[i], 1);
      mpz_mul (h[0], g[0], g[0]);
      mpz_mul (h[2 * dg], g[dg], g[dg]);
      for (i = 1; i < dg; i++)
	mpz_addmul (h[2 * i], g[i], g[i]);
      dh = 2 * dg;

      /* reduce mod p */
      ASSERT (dh < 2 * d);
      for (i = 0; i <= dh; i++)
	mpz_mod_ui (h[i], h[i], p);
      
      /* multiply h by x if bit k of p is set */
      if (p & (1 << k))
	{
	  for (i = dh; i >= 0; i--)
	    mpz_swap (h[i+1], h[i]);
	  mpz_set_ui (h[0], 0);
	  dh ++;
	  ASSERT (dh < 2 * d);
	}

      /* reduce mod fp */
      while (dh >= d)
	{
	  /* subtract h[dh]*fp*x^(dh-d) from h */
	  mpz_mod_ui (h[dh], h[dh], p);
	  for (i = 0; i < d; i++)
	    mpz_submul (h[dh - d + i], h[dh], fp[i]);
	  /* it is not necessary to reduce h[j] for j < dh */
	  dh --;
          ASSERT (dh < 2 * d);
	}

      /* reduce h mod p and copy to g */
      for (i = 0; i <= dh; i++)
	mpz_mod_ui (g[i], h[i], p);
      dg = dh;
      ASSERT (dg < 2 * d);
    }

  /* subtract x */
  mpz_sub_ui (g[1], g[1], 1);
  mpz_mod_ui (g[1], g[1], p);

  while (dg >= 0 && mpz_cmp_ui (g[dg], 0) == 0)
    dg --;

  /* take the gcd with fp */
  while (dg > 0)
    {
      while (df >= dg)
	{
	  /* divide f by g */
	  mpz_set_ui (h[0], p);
	  mpz_invert (h[0], g[dg], h[0]); /* 1/g[dg] mod p */
	  mpz_mul (h[0], h[0], fp[df]);
	  mpz_mod_ui (h[0], h[0], p); /* fp[df]/g[dg] mod p */
	  for (i = 0; i < dg; i++)
	    {
	      mpz_submul (fp[df - dg + i], h[0], g[i]);
	      mpz_mod_ui (fp[df - dg + i], fp[df - dg + i], p);
	    }
	  df --;
	  while (df >= 0 && mpz_cmp_ui (fp[df], 0) == 0)
	    df --;
	}
      /* now d < dg: swap f and g */
      for (i = 0; i <= dg; i++)
	mpz_swap (fp[i], g[i]);
      i = df;
      df = dg;
      dg = i;
    }

  /* if g=0 now, gcd is f, otherwise if g<>0, gcd is 1 */
  if (mpz_cmp_ui (g[0], 0) != 0)
    df = 0;

  if (df == 1) {
    mpz_set_ui (h[0], p);
    mpz_invert(h[0], fp[1], h[0]);
    mpz_mul(h[0], h[0], fp[0]);
    mpz_neg(h[0], h[0]);
    mpz_mod_ui(h[0], h[0], p);
    *r = mpz_get_ui(h[0]);
  }

  poly_clear (g, 2 * d - 1);
  poly_clear (h, 2 * d - 1);
  poly_clear (fp, d);

  return df;
}

void create_characters(rootprime_t * tabchar, int k, cado_poly pol) {
  unsigned long p, r;
  int ret;
  int i = 0;
  mpz_t pp;

  p = 1<<(pol->lpba+1);
  
  do {
    mpz_init_set_ui(pp, p);
    mpz_nextprime(pp, pp);
    p = mpz_get_ui(pp);
    ret = roots_mod(&r, pol->f, pol->degree, p);
    if (ret != 1)
      continue;
    tabchar[i].prime = p;
    tabchar[i].root = r;
    i++;
  } while (i < k);
  mpz_clear(pp);
}

void
readOneKer(mp_limb_t *vec, FILE *file, int nlimbs) {
  unsigned long w;
  int ret, i;
  for (i = 0; i < nlimbs; ++i) {
    ret = fscanf(file, "%lx", &w);
    ASSERT (ret == 1);
    *vec = w;
    vec++;
  }
}

typedef struct {
  unsigned int nrows;
  unsigned int ncols;
  mp_limb_t *data;
  unsigned int limbs_per_row;
  unsigned int limbs_per_col;
} dense_mat_t;

// str belongs to the [i, j]-th relation-set in the ker-ified matrix.
void
treatOneabpair(int **charval, char *str, int n, int k, int i, int j, mp_limb_t **ker, rootprime_t * tabchar)
{
    long a;
    unsigned long b;
    int ret, ii, jj;

    ret = gmp_sscanf(str, "%ld %lu", &a, &b);
#if DEBUG >= 1
    fprintf(stderr, "str=[%s], a=%ld, b=%lu\n", str, a, b);
#endif
    ASSERT (ret == 2);
    for (jj = 0; jj < n; ++jj) { // for each vector in ker...
	if ((ker[jj][i]>>j) & 1UL) {
	    for (ii = 0; ii < k; ++ii) { // for each character...
		charval[jj][ii] *= eval_char(a, b, tabchar[ii]);
	    }
	}
    }
}

// charmat is small_nrows x k
void
computeAllCharacters(char **charmat, int i, int k, rootprime_t * tabchar, long a, unsigned long b)
{
    int j;
    
    for(j = 0; j < k; j++)
	charmat[i][j] *= (char)eval_char(a, b, tabchar[j]);
}

// charmat is small_nrows x k
// relfile is the big rough relation files;
// purgedfile contains crunched rows, with their label referring to relfile;
// indexfile contains the coding "row[i] uses rows i_0...i_r in purgedfile".
void
buildCharacterMatrix(char **charmat, int k, rootprime_t * tabchar, FILE *purgedfile, FILE *indexfile, FILE *relfile)
{
    relation_t rel;
    int i, j, r, small_nrows, nr, nrows, ncols, irel, kk;
    char str[1024];
    char **charbig;

    // let's dump purgedfile which is a nrows x ncols matrix
    rewind(purgedfile);
    fgets(str, 1024, purgedfile);
    sscanf(str, "%d %d", &nrows, &ncols);
    fprintf(stderr, "Reading indices in purgedfile\n");

    fprintf(stderr, "Reading all (a, b)'s just once\n");
    // charbig is nrows x k
    charbig = (char **)malloc(nrows * sizeof(char *));
    for(i = 0; i < nrows; i++){
	charbig[i] = (char *)malloc(k * sizeof(char));
	for(j = 0; j < k; j++)
	    charbig[i][j] = 1;
    }
    irel = 0;
    //    rewind(relfile); // useless?
    for(i = 0; i < nrows; i++){
	fgets(str, 1024, purgedfile);
	sscanf(str, "%d", &nr);
	jumpToRelation(&rel, relfile, irel, nr);
	irel = nr+1;
	computeAllCharacters(charbig, i, k, tabchar, rel.a, rel.b);
    }
    fprintf(stderr, "Reading index file to reconstruct the characters\n");
    rewind(indexfile);
    // skip small_nrows, small_ncols
    fscanf(indexfile, "%d %d", &small_nrows, &r);
    // read all relation-sets and update charmat accordingly
    for(i = 0; i < small_nrows; i++){
	if(!(i % 10000))
	    fprintf(stderr, "Treating relation #%d / %d\n", i, small_nrows);
	fscanf(indexfile, "%d", &nr);
	for(j = 0; j < nr; j++){
	    fscanf(indexfile, "%d", &r);
	    for(kk = 0; kk < k; kk++)
		charmat[i][kk] *= charbig[r][kk];
	}
    }
    for(i = 0; i < nrows; i++)
	free(charbig[i]);
    free(charbig);
}

void
printCompactMatrix(mp_limb_t **A, int nrows, int ncols)
{
    int i, j;

    fprintf(stderr, "array([\n");
    for(i = 0; i < nrows; i++){
        fprintf(stderr, "[");
	for(j = 0; j < ncols; j++){
	    int j0 = j/GMP_NUMB_BITS;
            int j1 = j - j0*GMP_NUMB_BITS;
	    if((A[i][j0]>>j1) & 1UL)
		fprintf(stderr, "1");
	    else
		fprintf(stderr, "0");
	    if(j < ncols-1)
		fprintf(stderr, ", ");
	}
	fprintf(stderr, "]");
	if(i < nrows-1)
	    fprintf(stderr, ", ");
	fprintf(stderr, "\n");
    }
    fprintf(stderr, "])");
}

void
printTabMatrix(dense_mat_t *mat, int nrows, int ncols)
{
    int i, j;

    fprintf(stderr, "array([\n");
    for(i = 0; i < nrows; i++){
        fprintf(stderr, "[");
	for(j = 0; j < ncols; j++){
	    int j0 = j/GMP_NUMB_BITS;
            int j1 = j - j0*GMP_NUMB_BITS;
	    if((mat->data[i*mat->limbs_per_row+j0]>>j1) & 1UL)
		fprintf(stderr, "1");
	    else
		fprintf(stderr, "0");
	    if(j < ncols-1)
		fprintf(stderr, ", ");
	}
	fprintf(stderr, "]");
	if(i < nrows-1)
	    fprintf(stderr, ", ");
	fprintf(stderr, "\n");
    }
    fprintf(stderr, "])");
}

void
handleKer(dense_mat_t *mat, rootprime_t * tabchar, FILE * purgedfile,
          mp_limb_t ** ker, /*int nlimbs, cado_poly pol, */
          FILE *indexfile, FILE *relfile)
{  
    int i, n;
    unsigned int j, k;
    char str[1024];
    char **charmat;
    int small_nrows, small_ncols;

    rewind(purgedfile);
    fgets(str, 1024, purgedfile);
    sscanf(str, "%d %d", &small_nrows, &small_ncols);

    n = mat->nrows;
    k = mat->ncols;
    charmat = (char **) malloc(small_nrows * sizeof(char *));
    ASSERT (charmat != NULL);
    for (i = 0; i < small_nrows; ++i) {
	charmat[i] = (char *) malloc(k*sizeof(char));
	ASSERT (charmat[i] != NULL);
	for (j = 0; j < k; ++j)
	    charmat[i][j] = 1;
    }
    buildCharacterMatrix(charmat, k, tabchar, purgedfile, indexfile, relfile);
#ifdef WANT_ASSERT
    for (i = 0; i < small_nrows; ++i)
	for(j = 0; j < k; j++)
	    ASSERT((charmat[i][j] == 1) || (charmat[i][j] == -1));
#endif
#if DEBUG >= 1
    fprintf(stderr, "charmat:=array([");
    for (i = 0; i < small_nrows; ++i){
	fprintf(stderr, "[");
	for(j = 0; j < k; j++){
	    fprintf(stderr, "%d", (charmat[i][j] == 1 ? 0 : 1));
	    if(j < k-1)
		fprintf(stderr, ", ");
	}
	fprintf(stderr, "]");
	if(i < small_nrows-1)
	    fprintf(stderr, ", ");
	fprintf(stderr, "\n");
    }
    fprintf(stderr, "]);\n");
    fprintf(stderr, "ker:=");
    printCompactMatrix(ker, n, small_nrows);
    fprintf(stderr, ";\n");
#endif

    // now multiply: ker * charmat 
    for (i = 0; i < n; ++i)
	for (j = 0; j < mat->limbs_per_row; ++j)
	    mat->data[i*mat->limbs_per_row+j] = 0UL;

    // mat[i, j] = sum ker[i][u] * charmat[u][j]
    for(i = 0; i < n; ++i){
	for(j = 0; j < k; ++j){
	    int j0 = j/GMP_NUMB_BITS;
	    int j1 = j - j0*GMP_NUMB_BITS;
	    int u;
	    for(u = 0; u < small_nrows; u++){
		int u0 = u/GMP_NUMB_BITS;
		int u1 = u - u0*GMP_NUMB_BITS;
		if((ker[i][u0]>>u1) & 1UL)
		    if(charmat[u][j] == -1)
			mat->data[i*mat->limbs_per_row+j0] ^= (1UL<<j1);
	    }
	}
    }
#if DEBUG >= 1
    fprintf(stderr, "prod:=");
    printTabMatrix(mat, n, k);
    fprintf(stderr, ";\n");
#endif
    // TODO: clean data
}

// matrix M_purged is nrows x ncols
// matrix M_small is small_nrows x small_ncols, the kernel of which
// is contained in ker and is n x small_nrows; small_[nrows,ncols] are
// accessible in file indexfile.
// charmat is small_nrows x k; ker * charmat will be n x k, yielding M_tiny.
int main(int argc, char **argv) {
  FILE *purgedfile, *kerfile, *indexfile, *relfile;
  int ret;
  int k, isz;
  unsigned int i, j, n, nlimbs;
  rootprime_t *tabchar;
  int *charval;
  cado_poly *pol;
  mp_limb_t **ker;
  mp_limb_t *newker;
  dense_mat_t mymat;
  mp_limb_t **myker;
  unsigned int dim;

  if (argc != 8) {
    fprintf(stderr, "usage: %s purgedfile kerfile polyfile", argv[0]);
    fprintf(stderr, "indexfile relfile n k\n");
    fprintf(stderr, "  where n is the number of kernel vector to deal with\n");
    fprintf(stderr, "    and k is the number of characters you want to use\n");
    exit(1);
  }

  purgedfile = fopen(argv[1], "r");
  ASSERT (purgedfile != NULL);
  kerfile = fopen(argv[2], "r");
  ASSERT (kerfile != NULL);

  pol = my_read_polynomial(argv[3]);

  indexfile = fopen(argv[4], "r");
  ASSERT (indexfile != NULL);
  relfile = fopen(argv[5], "r");
  ASSERT (relfile != NULL);

  ret = gmp_sscanf(argv[6], "%d", &n);
  ASSERT (ret == 1);
  ret = gmp_sscanf(argv[7], "%d", &k);
  ASSERT (ret == 1);

  tabchar = (rootprime_t *)malloc(k*sizeof(rootprime_t));
  ASSERT (tabchar != NULL);
  charval = (int *)malloc(k*sizeof(int));
  ASSERT (charval != NULL);

  create_characters(tabchar, k, *pol);

#if DEBUG >= 2
  fprintf(stderr, "using characters (p,r):\n");
  for (i = 0; i < k; ++i)
      fprintf(stderr, "\t%lu %lu\n", tabchar[i].prime, tabchar[i].root);
#endif

  {
    int small_nrows, small_ncols;
    ret = fscanf(indexfile, "%d %d", &small_nrows, &small_ncols);
    ASSERT (ret == 2);
    nlimbs = (small_nrows / GMP_NUMB_BITS) + 1;
  }

  ker = (mp_limb_t **)malloc(n*sizeof(mp_limb_t *));
  ASSERT (ker != NULL);
  for (j = 0; j < n; ++j) {
    ker[j] = (mp_limb_t *) malloc (nlimbs*sizeof(mp_limb_t));
    ASSERT (ker[j] != NULL);
    readOneKer(ker[j], kerfile, nlimbs);
  }
  fprintf(stderr, "finished reading kernel file\n");

  mymat.nrows = n;
  mymat.ncols = k;
  mymat.limbs_per_row =  ((mymat.ncols-1) / GMP_NUMB_BITS) + 1;
  mymat.limbs_per_col =  ((mymat.nrows-1) / GMP_NUMB_BITS) + 1;

  mymat.data = (mp_limb_t *) malloc(mymat.limbs_per_row*mymat.nrows*sizeof(mp_limb_t));
  ASSERT (mymat.data != NULL);

  fprintf(stderr, "start computing characters...\n");

  handleKer(&mymat, tabchar, purgedfile, ker, /*nlimbs, *pol, */
            indexfile, relfile);

  myker = (mp_limb_t **)malloc(mymat.nrows*sizeof(mp_limb_t *));
  ASSERT (myker != NULL);
  for (i = 0; i < mymat.nrows; ++i) {
    myker[i] = (mp_limb_t *)malloc(mymat.limbs_per_col*sizeof(mp_limb_t));
    ASSERT (myker[i] != NULL);
    for (j = 0; j < mymat.limbs_per_col; ++j) 
      myker[i][j] = 0UL;
  }
  fprintf(stderr, "Computing tiny kernel\n");
  dim = kernel(mymat.data, myker, mymat.nrows, mymat.ncols, mymat.limbs_per_row, mymat.limbs_per_col);
  fprintf(stderr, "dim of ker = %d\n", dim);

#if DEBUG >= 1
  fprintf(stderr, "newker:=");
  printCompactMatrix(myker, dim, k);
  fprintf(stderr, ";\n");
#endif
    
  newker = (mp_limb_t *) malloc (nlimbs*sizeof(mp_limb_t));
  ASSERT (newker != NULL);
  for (i = 0; i < dim; ++i) {
    for (j = 0; j < nlimbs; ++j)
      newker[j] = 0;
    for (j = 0; j < mymat.limbs_per_col; ++j) {
      int jj;
      unsigned int kk;
      unsigned long w = myker[i][j];
      for (jj = 0; jj < GMP_NUMB_BITS; ++jj) {
	if (w & 1UL) {
	  for (kk = 0; kk < nlimbs; ++kk)
	    newker[kk] ^= ker[j*GMP_NUMB_BITS+jj][kk];
	}
	w >>= 1;
      }
    }
    // do not print zero vector...!
    isz = 1;
    for(j = 0; j < nlimbs; ++j)
	if(newker[j]){
	    isz = 0;
	    break;
	}
    if(!isz){
	for(j = 0; j < nlimbs; ++j)
	    printf("%lx ", newker[j]);
	printf("\n");
    }
  }

  free(pol);

  return 0;
}
