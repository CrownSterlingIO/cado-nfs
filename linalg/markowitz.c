#include "utils/utils.h"

#include "merge_opts.h"

#ifdef USE_MARKOWITZ

#include "sparse.h"
#include "dclist.h"
#include "sparse_mat.h"
#include "report.h"
#include "markowitz.h"

#define MKZ_DEBUG 0

#define MKZ_INF -1

// Again, a priority queue as a heap...!
// Q[0] contains the number of items in Q[], so that useful part of Q
// is Q[1..Q[0]]

// Q[2*i] contains j-jmin = dj
// Q[2*i+1] contains the Markowitz count for j

// get j-th component of Q[i] 
#define MkzGet(Q, i, j) (Q[((i)<<1)+(j)])
#define MkzSet(Q, i, j, val) (Q[((i)<<1)+(j)] = (val))

// (Q, A)[k1] <- (Q, A)[k2]
void
MkzAssign(INT *Q, INT *A, INT k1, INT k2)
{
    INT dj = MkzGet(Q, k2, 0);

    MkzSet(Q, k1, 0, dj);
    MkzSet(Q, k1, 1, MkzGet(Q, k2, 1)); // could be simplified...!
    A[dj] = k1;
}

void
MkzPrintQueue(INT *Q)
{
    int level = 0, imax = 1, i;

    fprintf(stderr, "L0:");
    for(i = 1; i <= Q[0]; i++){
	fprintf(stderr, " [%d, %d]", MkzGet(Q, i, 1), MkzGet(Q, i, 0));
	if(i == imax){
	    imax = (imax<<1)+1;
	    fprintf(stderr, "\nL%d:", ++level);
	}
    }
    fprintf(stderr, "\n");
}

void
MkzUpQueue(INT *Q, INT *A, INT k)
{
    INT x = MkzGet(Q, k, 0), v = MkzGet(Q, k, 1);

    while((k > 1) && (MkzGet(Q, k/2, 1) >= v)){
	// we are at level > 0 and the father is >= son
	// the father replaces the son
	MkzAssign(Q, A, k, k/2);
	k /= 2;
    }
    // we found the place of (x, v)
    MkzSet(Q, k, 0, x);
    MkzSet(Q, k, 1, v);
    A[x] = k;
}

void
MkzInsert(INT *Q, INT *A, INT dj, INT c)
{
    Q[0]++;
    MkzSet(Q, Q[0], 0, dj);
    MkzSet(Q, Q[0], 1, c);
    A[dj] = Q[0];
    MkzUpQueue(Q, A, Q[0]);
}

// Move Q[k] down.
void
MkzDownQueue(INT *Q, INT *A, INT k)
{
    INT x = MkzGet(Q, k, 0), v = MkzGet(Q, k, 1), j;

    while(k <= Q[0]/2){
	// k has at least a left son
	j = 2*k;
	if(j < Q[0])
	    // k has a right son
	    if(MkzGet(Q, j, 1) > MkzGet(Q, j+1, 1))
		j++;
	// at this point, Q[j] is the largest son
	if(v <= MkzGet(Q, j, 1))
	    break;
	else{
	    // the father takes the place of the son
	    MkzAssign(Q, A, k, j);
	    k = j;
	}
    }
    // we found the place of (x, v)
    MkzSet(Q, k, 0, x);
    MkzSet(Q, k, 1, v);
    A[x] = k;
}

void
MkzPopQueue(INT *dj, INT *mkz, INT *Q, INT *A)
{
    *dj = MkzGet(Q, 1, 0);
    *mkz = MkzGet(Q, 1, 1);
    A[*dj] = MKZ_INF;
    MkzAssign(Q, A, 1, Q[0]);
    Q[0]--;
    MkzDownQueue(Q, A, 1);
}

// (Q, A)[k] has just arrived, but we have to move it in the heap, so that
// it finds its place.
void
MkzMoveUpOrDown(INT *Q, INT *A, INT k)
{
    // move new node up or down
    if(k == 1)
	// rare event!
	MkzDownQueue(Q, A, 1);
    else{
	// k has a father
	if(MkzGet(Q, k/2, 1) > MkzGet(Q, k, 1))
	    // we have to move up
	    MkzUpQueue(Q, A, k);
	else
	    MkzDownQueue(Q, A, k);
    }
}

// Remove (Q, A)[k].
void
MkzDelete(INT *Q, INT *A, INT k)
{
    fprintf(stderr, "MKZ: deleting (Q, A)[%d]=[%d, %d]\n", k,
	    MkzGet(Q, k, 0), MkzGet(Q, k, 1));
    // we put Q[Q[0]] in Q[k]
    MkzAssign(Q, A, k, Q[0]);
    Q[0]--;
    MkzMoveUpOrDown(Q, A, k);
}

int
MkzIsHeap(INT *Q)
{
    int k;

    for(k = 1; k <= Q[0]/2; k++){
	// k has a left son
	if(MkzGet(Q, k, 1) > MkzGet(Q, 2*k, 1)){
	    fprintf(stderr, "Pb: father=%d > lson=%d\n", 
		    MkzGet(Q, k, 1), MkzGet(Q, 2*k, 1));
	    return 0;
	}
	if(k < Q[0]/2){
	    // k has a right son
	    if(MkzGet(Q, k, 1) > MkzGet(Q, 2*k+1, 1)){
		fprintf(stderr, "Pb: father=%d > rson=%d\n", 
			MkzGet(Q, k, 1), MkzGet(Q, 2*k+1, 1));
		return 0;
	    }
	}
    }
    return 1;
}

void
MkzCheck(sparse_mat_t *mat)
{
    INT dj;

    for(dj = 0; dj < mat->jmax - mat->jmin; dj++)
	if(mat->wt[dj] > 0)
	    if(MkzGet(mat->MKZQ, mat->MKZA[dj], 0) != dj)
		fprintf(stderr, "GASP: %d in MkzCheck\n", dj);
}

int
MkzCount(sparse_mat_t *mat, INT j)
{
    int mkz, k, i;

    mkz = mat->nrows;
    for(k = 1; k <= mat->R[GETJ(mat, j)][0]; k++)
	if((i = mat->R[GETJ(mat, j)][k]) != -1){
	    // this should be the weight of row i
	    if(mat->rows[i][0] < mkz)
		mkz = mat->rows[i][0];
	}
    return ((mkz-1) * mat->wt[GETJ(mat, j)]);
}

void
MkzInit(sparse_mat_t *mat)
{
    INT j, mkz;
    int sz = 0;

    fprintf(stderr, "Entering initMarkowitz\n");
    // compute number of elligible columns in the heap
    for(j = mat->jmin; j < mat->jmax; j++)
	if(mat->wt[GETJ(mat, j)] > 0)
	    sz++;
    fprintf(stderr, "Allocating heap for %d columns\n", sz);
    mat->MKZQ = (INT *)malloc((sz+1) * 2 * sizeof(INT));
    mat->MKZQ[1] = sz; // why not?
    // every j needs a pointer
    mat->MKZA = (INT *)malloc((mat->jmax - mat->jmin + 1) * sizeof(INT));
    for(j = mat->jmin; j < mat->jmax; j++)
	if(mat->wt[GETJ(mat, j)] > 0){
	    mkz = MkzCount(mat, j);
#if MKZ_DEBUG >= 2
	    printf("j=%d wt=%d", j, mat->wt[GETJ(mat, j)]);
	    printf(" => mkz=%d\n", mkz);
#endif
	    MkzInsert(mat->MKZQ, mat->MKZA, GETJ(mat, j), mkz);
	}
        else
	    mat->MKZA[GETJ(mat, j)] = MKZ_INF;
    // TODO: overflow in mkz???
    MkzCheck(mat);
#if MKZ_DEBUG >= 1
    fprintf(stderr, "Initial queue is\n");
    MkzPrintQueue(mat->MKZQ);
#endif
}

void
MkzClose(sparse_mat_t *mat)
{
    free(mat->MKZQ);
}

int
MkzIncrCol(sparse_mat_t *mat, INT j)
{
    int ind;

#if MKZ_DEBUG >= 1
    fprintf(stderr, "Incr: wt(%d) was %d\n", j, mat->wt[GETJ(mat, j)]);
#endif
    ind = mat->wt[GETJ(mat, j)] = incrS(mat->wt[GETJ(mat, j)]);
    return ind;
}

void
MkzUpdate(sparse_mat_t *mat, INT j)
{
    INT adr = mat->MKZA[GETJ(mat, j)];
    int mkz;

    if(adr == -1){
#if MKZ_DEBUG >= 1
	fprintf(stderr, "Prevented use of adr[%d]=-1 in MkzUpdate\n", j);
#endif
	return;
    }
#if MKZ_DEBUG >= 1
    if((mat->wt[GETJ(mat, j)] == 0) || (mat->wt[GETJ(mat, j)] == 1))
	fprintf(stderr, "W: wt[%d] = %d\n", j, mat->wt[GETJ(mat, j)]);
#endif
    mkz = MkzCount(mat, j);
#if MKZ_DEBUG >= 1
    fprintf(stderr, "Updating j=%d (old=%d, new=%d)\n", j,
	    MkzGet(mat->MKZQ, adr, 0), mkz);
#endif
    // nothing to do if new count == old count
    if(mkz != MkzGet(mat->MKZQ, adr, 0)){
	// add new count
	MkzSet(mat->MKZQ, adr, 1, mkz);
	// a variant of delete is needed...!
	MkzMoveUpOrDown(mat->MKZQ, mat->MKZA, adr);
    }
}

/*    
   Updates:
   - mat->wt[j] (weight of column j)

   We arrive here when mat->wt[j] > 0.

*/
void
MkzDecreaseColWeight(sparse_mat_t *mat, INT j)
{
    INT dj = GETJ(mat, j);

#if MKZ_DEBUG >= 1
    fprintf(stderr, "Decreasing col %d; was %d\n", j, mat->wt[dj]);
#endif
    mat->wt[dj] = decrS(mat->wt[dj]);
}

void
MkzRemoveJ(sparse_mat_t *mat, INT j)
{
    INT dj = GETJ(mat, j);

    if(mat->MKZA[dj] == MKZ_INF){
#if MKZ_DEBUG >= 1
	fprintf(stderr, "Prevented use of adr[%d]=-1 in MkzRemoveJ\n", j);
#endif
	return;
    }
    fprintf(stderr, "Removing col %d; was %d\n", j, mat->wt[dj]);
    mat->wt[dj] = 0;
    // remove j from the QA structure
    MkzDelete(mat->MKZQ, mat->MKZA, mat->MKZA[dj]);
    mat->MKZA[dj] = MKZ_INF;
#if MKZ_DEBUG >= 1
    MkzIsHeap(mat->MKZQ);
#endif
}

// let's say we remove some columns with the highest Mkz count.
// Not very pertinent right now.
int
MkzDeleteHeavyColumns(report_t *rep, sparse_mat_t *mat)
{
#if 1
    fprintf(stderr, "Max Markowitz count: %d\n", 
	    MkzGet(mat->MKZQ, mat->MKZQ[0], 1));
    return 0;
#else
    INT j;
    int nj, njmax, w;

    if(MkzQueueCardinality(mat->MKZQ) < 5000)
	return 0;
    njmax = 10; // humf
    for(nj = 0; nj < njmax; nj++){
	j = MkzGet(mat->MKZQ, Q[0], 0);
	w = mat->wt[GETJ(mat, j)]; // make a copy of the weight
	MkzRemoveJ(mat, j);
        // mat->wt[j] was put to 0...
        mat->wt[GETJ(mat, j)] = -w; // restore and update
	Q[0]--;
    }
    return njmax;
#endif
}

int
MkzRemoveCols(report_t *rep, sparse_mat_t *mat, int wmin, int wmax)
{
    INT j;
    int w;

    for(j = mat->jmin; j < mat->jmax; j++){
	w = mat->wt[GETJ(mat, j)] == 1;
        if((w >= wmin) && (w <= wmax)){
	    fprintf(stderr, "w[%d]=%d\n", j, w);
	}
    }
    return 0;
}

#endif
