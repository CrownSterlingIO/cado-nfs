#ifndef ABASE_U64K1_T_H_
#define ABASE_U64K1_T_H_

/* MPFQ generated file -- do not edit */

#include "abase_u64k1.h"
#include "abase_u64k2.h"
/* Active handler: Mpfq::defaults::vec */
/* Active handler: Mpfq::defaults */
/* Active handler: Mpfq::engine::defaults::mpi_flat */
/* Active handler: simd_dotprod */
/* Active handler: io */
/* Active handler: trivialities */
/* Active handler: simd_flat */
/* Active handler: simd_u64k */
/* Automatically generated code  */
/* Options used: k=1 tag=u64k1 choose_by_groupsize=CODE(0x1581cf8) prefix=abase_ family=[u64k1, u64k2] virtual_base={
                  'filebase' => 'abase_vbase',
                  'substitutions' => [
                                       [
                                         qr/(?-xism:abase_u64k1_elt \*)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_src_elt\b)/,
                                         'const void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_elt\b)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_dst_elt\b)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_elt_ur \*)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_src_elt_ur\b)/,
                                         'const void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_elt_ur\b)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_dst_elt_ur\b)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_vec \*)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_src_vec\b)/,
                                         'const void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_vec\b)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_dst_vec\b)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_vec_ur \*)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_src_vec_ur\b)/,
                                         'const void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_vec_ur\b)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_dst_vec_ur\b)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_poly \*)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_src_poly\b)/,
                                         'const void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_poly\b)/,
                                         'void *'
                                       ],
                                       [
                                         qr/(?-xism:abase_u64k1_dst_poly\b)/,
                                         'void *'
                                       ]
                                     ],
                  'name' => 'abase_vbase'
                };
 */


/* Functions operating on the field structure */

/* Element allocation functions */

/* Elementary assignment functions */

/* Assignment of random values */

/* Arithmetic operations on elements */

/* Operations involving unreduced elements */

/* Comparison functions */

/* Input/output functions */

/* Vector functions */

/* Functions related to SIMD operation */

/* Member templates related to SIMD operation */

/* MPI interface */

/* Object-oriented interface */
void abase_u64k1_u64k1_dotprod(abase_u64k1_dst_field, abase_u64k1_dst_field, abase_u64k1_dst_vec, abase_u64k1_src_vec, abase_u64k1_src_vec, unsigned int);
void abase_u64k1_u64k2_dotprod(abase_u64k1_dst_field, abase_u64k2_dst_field, abase_u64k1_dst_vec, abase_u64k2_src_vec, abase_u64k1_src_vec, unsigned int);
void abase_u64k1_u64k1_addmul_tiny(abase_u64k1_dst_field, abase_u64k1_dst_field, abase_u64k1_dst_vec, abase_u64k1_src_vec, abase_u64k1_dst_vec, unsigned int);
void abase_u64k1_u64k2_addmul_tiny(abase_u64k1_dst_field, abase_u64k2_dst_field, abase_u64k2_dst_vec, abase_u64k1_src_vec, abase_u64k2_dst_vec, unsigned int);
void abase_u64k1_u64k1_transpose(abase_u64k1_dst_field, abase_u64k1_dst_field, abase_u64k1_dst_vec, abase_u64k1_src_vec);
void abase_u64k1_u64k2_transpose(abase_u64k1_dst_field, abase_u64k2_dst_field, abase_u64k1_dst_vec, abase_u64k2_src_vec);

#endif  /* ABASE_U64K1_T_H_ */

/* vim:set ft=cpp: */