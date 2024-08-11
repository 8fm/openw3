/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: basic shared codebook operations

 ********************************************************************/

#ifndef _V_CODEBOOK_H_
#define _V_CODEBOOK_H_

#include "ogg.h"

typedef struct codebook{
  int	dim;             /* codebook dimensions (elements per vector) */
  int	entries;         /* codebook entries */
  int	used_entries;    /* populated codebook entries */

  int   dec_maxlength;
  void *dec_table;
  int   dec_nodeb;      
  int   dec_leafw;      
  int   dec_type;        /* 0 = entry number
			    1 = packed vector of values
			    2 = packed vector of column offsets, maptype 1 
			    3 = scalar offset into value array,  maptype 2 (UNUSED IN ENCODER, STRIPPED FROM AK DECODER)  */

  ogg_int32_t q_min;  
  int         q_minp;  
  ogg_int32_t q_del;
  int         q_delp;
//  int         q_seq; UNUSED IN ENCODER, STRIPPED FROM AK DECODER
  int         q_bits;
  int         q_pack;
  void       *q_val;   

} codebook;

extern void vorbis_book_unpack(oggpack_buffer *b,codebook *c, CAkVorbisAllocator& VorbisAllocator);
extern long ak_vorbis_book_decode(codebook *book, oggpack_buffer *b); // signature change to avoid symbol clash with standard vorbis
extern void vorbis_book_decodev_add(codebook *book, ogg_int32_t *a, 
				    oggpack_buffer *b,int n,int point);
extern void vorbis_book_decodevv_add_2ch(codebook *book, ogg_int32_t **a,
				     long off,
				    oggpack_buffer *b,int n,int point);
/*extern void vorbis_book_decodevv_add(codebook *book, ogg_int32_t **a,
				     long off,int ch, 
				    oggpack_buffer *b,int n,int point);
*/
#endif
