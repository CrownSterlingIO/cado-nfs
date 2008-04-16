#ifndef BL_READWRITE_H_
#define BL_READWRITE_H_

#include <stdio.h>
#include "struct.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void displayMatrixV(unsigned long **M, unsigned long m,
			   unsigned long n, char c);
extern void displayMatrix(const unsigned long *matrix, unsigned long m,
			  unsigned long n, char c);
extern void displaySMatrix(const unsigned long *matrix, unsigned long m,
			   unsigned long n, char c);
extern void displaySMatrixNew(const unsigned long *matrix, unsigned long m,
			      unsigned long n, char c);
extern void DisplaySMatrix(unsigned long m, unsigned long *A);
extern unsigned long SizeSMatrixFile(char *f);
extern void ReadSMatrixFile(char *f, unsigned long *M);
extern void WriteSMatrix(unsigned long *A, unsigned long m, unsigned long n,
			 char c, char *f);
extern void WriteMatrix(const unsigned long *matrix, unsigned long m,
			unsigned long n, char c, char *f);
extern void WriteSMatrixSparse(unsigned long *A, unsigned long m, char *f);
extern void WriteMatrixDense(const unsigned long *matrix, unsigned long m,
			     unsigned long n, char *f);
extern void ReadDMatrixFile(char *f, unsigned long n, unsigned long *M);
extern void ReadSMatrixFileBlock(char *f, unsigned long *M, unsigned long k,
				 unsigned long size);
extern void WriteSMatrixSparseNew(unsigned long *A, unsigned long m,
				  unsigned long n, unsigned long w, char *f);
extern void WriteFileSMatrixNew(const unsigned long *matrix, unsigned long m,
				unsigned long n, char c);
extern void WriteFileMatrix(const unsigned long *matrix, unsigned long m,
			    unsigned long n, char c);
extern void ReadSMatrixDimensions(char *f, unsigned long *data);
extern void ReadSMatrixFileNew(char *f, unsigned long *M);
extern void ReadSMatrixFileBlockNew(char *f, unsigned long *M,
				    unsigned long k, unsigned long size);
extern unsigned long SizeBlock(unsigned long Nproc, unsigned long proc,
			       unsigned long m);
extern void displayMatrixScreen(const unsigned long *matrix,
				unsigned long m, unsigned long n);

extern void WriteBlockMatrix(DenseMatrix Ker, char *f);

extern void ReadSMatrixSlice(SparseMatrix M, char *filename);
extern void PrepareMatrixSlices(SparseMatrix, char *);

#ifdef __cplusplus
}
#endif

#endif	/* BL_READWRITE_H_ */
