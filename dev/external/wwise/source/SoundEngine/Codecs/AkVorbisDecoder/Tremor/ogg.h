/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2003    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: subsumed libogg includes

 ********************************************************************/
#ifndef _OGG_H
#define _OGG_H

#include "os_types.h"
#include <AK/Tools/Common/AkAssert.h>

#if defined(__SPU__)

#include "spu_printf.h"
#include "libsn_spu.h"

#include "stdlib.h"		// NULL
#include "string.h"		// memset

#ifdef _DEBUG
#include <spu_printf.h>
#endif

// Note: SPU specific code (mdct) needs AkTypes, which cannot be included directly in mdct.cpp.
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#ifdef _DEBUG
#define PRINT	spu_printf
#else
#define PRINT
#endif

#else

#include <stdio.h>

#ifdef _DEBUG
#define PRINT	printf
#else
#define PRINT
#endif

#endif

//================================================================================
// allocation using some memory block
//================================================================================
class CAkVorbisAllocator
{
public:
#ifndef __SPU__
	CAkVorbisAllocator()
	{
		pStartAddress   = NULL;
		pCurrentAddress	= NULL;
		CurrentSize		= 0;
		MaxSize			= 0;
	}

	inline bool Init(unsigned int Size)
	{
		pStartAddress = (AkUInt8*)AkAlloc( g_LEngineDefaultPoolId, Size );
		pCurrentAddress = pStartAddress;
		MaxSize = Size;
		return pCurrentAddress != NULL;
	}

	inline void Term()
	{
		if ( pStartAddress )
		{
			AkFree( g_LEngineDefaultPoolId, pStartAddress );
			pCurrentAddress	= 0;
			CurrentSize		= 0;
			MaxSize			= 0;
			pStartAddress	= NULL;
		}
	}
#endif

#ifdef SPU_CODEBOOK
	void FixUVMPtrs(AkUInt8* PpuAddress, AkUInt8* SpuAddress)
	{
		size_t delta = SpuAddress - PpuAddress;

		pStartAddress += delta;
		pCurrentAddress += delta;
	}
#endif

	inline AkUInt8* GetAddress()
	{
		return pStartAddress;
	}

	inline AkUInt32 GetCurrentSize()
	{
		return CurrentSize;
	}

	inline AkUInt32 GetMaxSize()
	{
		return MaxSize;
	}

	inline void* Alloc(unsigned int Size)
	{
		if(Size != 0)
		{
			unsigned int AlignedSize = ((Size + 3) & ~3);
			unsigned int NextSize = CurrentSize + AlignedSize;
			if(NextSize <= MaxSize)
			{
				AkUInt8* pAddress = pCurrentAddress;
				pCurrentAddress += AlignedSize;
				CurrentSize += AlignedSize;
				return pAddress;
			}
			else
			{
				AKASSERT( !"No more UVM memory");
			}
		}
		return NULL;
	}

	inline void* Calloc(unsigned int Size)
	{
		void* pAddress = Alloc(Size);
		if(pAddress != NULL)
		{
#ifdef __SPU__
			AKASSERT( ((Size&0x3)==0) && "Calloc not aligned to 4 bytes");
			AkUInt32 * pUInts = (AkUInt32 *) pAddress;
			// fixme: optimize
			for ( unsigned int i = 0; i < Size/4; ++i )
				pUInts[i] = 0;
#else
			AKPLATFORM::AkMemSet(pAddress,0,Size);
#endif
		}
		return pAddress;
	}

private:
	AkUInt8*		pStartAddress;
	AkUInt8*		pCurrentAddress;
	unsigned int	CurrentSize;
	unsigned int	MaxSize;
} AK_ALIGN_DMA;

typedef struct ogg_buffer_state
{
	struct ogg_buffer    *unused_buffers;
	struct ogg_reference *unused_references;
	int                   outstanding;
	int                   shutdown;
} ogg_buffer_state;

typedef struct ogg_buffer
{
	unsigned char*	data;
	int				size;
} ogg_buffer;

#ifdef __SPU__

typedef struct oggpack_buffer 
{
  vec_int4       headbit;
  vec_int4       headend;
  vec_uint4      headptr; // really a pointer in slot 0
} oggpack_buffer;

#else

typedef struct oggpack_buffer 
{
  int            headbit;
  unsigned char *headptr;
  int			 headend;
} oggpack_buffer;

#endif

typedef struct oggbyte_buffer
{
	ogg_reference*	baseref;

	ogg_reference*	ref;
	unsigned char*	ptr;
	int				pos;
	int				end;
} oggbyte_buffer;

typedef struct ogg_sync_state 
{
	/* decode memory management pool */
	ogg_buffer_state *bufferpool;

	/* stream buffers */
	ogg_reference    *fifo_head;
	ogg_reference    *fifo_tail;
	int				 fifo_fill;

	/* stream sync management */
	int               unsynced;
	int               headerbytes;
	int               bodybytes;
} ogg_sync_state;

typedef struct ogg_stream_state
{
	ogg_reference *header_head;
	ogg_reference *header_tail;
	ogg_reference *body_head;
	ogg_reference *body_tail;

	int            e_o_s;	/* set when we have buffered the last
							packet in the logical bitstream */
	int            b_o_s;	/* set after we've written the initial page
							of a logical bitstream */
	int			   serialno;
	int			   pageno;
	ogg_int64_t    packetno;	/* sequence number for decode; the framing
								knows where there's a hole in the data,
								but we need coupling so that the codec
								(which is in a seperate abstraction
								layer) also knows about the gap */
	ogg_int64_t    granulepos;

	int            lacing_fill;
	ogg_uint32_t   body_fill;

	/* decode-side state data */
	int            holeflag;
	int            spanflag;
	int            clearflag;
	int            laceptr;
	ogg_uint32_t   body_fill_next;

} ogg_stream_state;

struct ogg_packet
{
	ogg_buffer		buffer;
	unsigned char	e_o_s;
}
#ifdef __SPU__
AK_ALIGN_DMA
#endif
;

/* Ogg BITSTREAM PRIMITIVES: bitstream ************************/

extern long  oggpack_read(oggpack_buffer *b, int bits);

#ifdef __SPU__

static const vec_uchar16 v16ShuffleBSwap40 = { 4,3,2,1, 0,0x80,0x80,0x80, 0x80,0x80,0x80,0x80, 0x80,0x80,0x80,0x80 }; // byte swap of the 40 first bits

inline void oggpack_readinit(oggpack_buffer *b, unsigned char * data, int size )
{
  b->headbit = (vec_int4) spu_splats( (unsigned char) 0 );
  b->headend = spu_splats( (int) size );
  b->headptr = spu_splats( (unsigned) data );
}

inline long oggpack_look(oggpack_buffer *b, int bits)
{
	AkUInt8 * headptr = (AkUInt8 *) spu_extract( b->headptr, 0 );

	vec_uchar16 qw0 = *((vec_uchar16 *) headptr );
	vec_uchar16 qw1 = *((vec_uchar16 *) headptr + 1 );
	unsigned int shift = ( (unsigned) headptr & 0xf );

	vec_uchar16 qw2 = spu_or( spu_slqwbyte(qw0, shift), spu_rlmaskqwbyte(qw1, shift-16) );
	vec_uchar16 qw3 = spu_shuffle( qw2, qw2, v16ShuffleBSwap40);

	unsigned int shiftBits = 8 - spu_extract( b->headbit, 0 );
	vec_uint4 qw4 = (vec_uint4) spu_slqwbyte( spu_slqw( qw3, shiftBits ), shiftBits >> 3 );

	unsigned int m = (1<<bits)-1;
	unsigned int ret = spu_extract( qw4, 0 ) & m;

	return ret;
}

/* limited to 32 at a time */
inline void oggpack_adv(oggpack_buffer *b, int bits)
{
	vec_int4 vBits = spu_splats( bits );
	vBits += b->headbit;
	b->headbit= spu_and( vBits, 7 );
	vec_int4 vBytes = spu_rlmask( vBits, -3 );
	b->headend-= vBytes;
	b->headptr+= vBytes;
}

inline int oggpack_eop(oggpack_buffer *b)
{
	if(spu_extract(b->headend,0)<0)return -1;
	return 0;
}

#else

static AkForceInline void oggpack_readinit( oggpack_buffer *b,ogg_buffer *r )
{
  b->headbit = 0;
  b->headptr = r->data;
  b->headend = r->size;
}

extern void  oggpack_readinit(oggpack_buffer *b,ogg_buffer *r);
extern long  oggpack_look(oggpack_buffer *b,int bits);
extern void  oggpack_adv(oggpack_buffer *b,int bits);

inline int oggpack_eop(oggpack_buffer *b)
{
	if(b->headend<0)return -1;
	return 0;
}

#endif

/* Ogg BITSTREAM PRIMITIVES: decoding **************************/

extern ogg_sync_state *ogg_sync_create(void);
extern int      ogg_sync_destroy(ogg_sync_state *oy);
extern int      ogg_sync_reset(ogg_sync_state *oy);

extern unsigned char *ogg_sync_bufferin(ogg_sync_state *oy, long size);
extern int      ogg_sync_wrote(ogg_sync_state *oy, long bytes);
//extern long     ogg_sync_pageseek(ogg_sync_state *oy,ogg_page *og);
//extern int      ogg_sync_pageout(ogg_sync_state *oy, ogg_page *og);
//extern int      ogg_stream_pagein(ogg_stream_state *os, ogg_page *og);
extern int      ogg_stream_packetout(ogg_stream_state *os,ogg_packet *op);
extern int      ogg_stream_packetpeek(ogg_stream_state *os,ogg_packet *op);

/* Ogg BITSTREAM PRIMITIVES: general ***************************/

extern ogg_stream_state *ogg_stream_create(int serialno);
extern int      ogg_stream_destroy(ogg_stream_state *os);
extern int      ogg_stream_reset(ogg_stream_state *os);
extern int      ogg_stream_reset_serialno(ogg_stream_state *os,int serialno);
extern int      ogg_stream_eos(ogg_stream_state *os);
/*
extern int      ogg_page_checksum_set(ogg_page *og);

extern int      ogg_page_version(ogg_page *og);
extern int      ogg_page_continued(ogg_page *og);
extern int      ogg_page_bos(ogg_page *og);
extern int      ogg_page_eos(ogg_page *og);
extern ogg_int64_t  ogg_page_granulepos(ogg_page *og);
extern ogg_uint32_t ogg_page_serialno(ogg_page *og);
extern ogg_uint32_t ogg_page_pageno(ogg_page *og);
extern int      ogg_page_packets(ogg_page *og);
extern int      ogg_page_getbuffer(ogg_page *og, unsigned char **buffer);
*/
extern int      ogg_packet_release(ogg_packet *op);
/*
extern int      ogg_page_release(ogg_page *og);

extern void     ogg_page_dup(ogg_page *d, ogg_page *s);
*/
/* Ogg BITSTREAM PRIMITIVES: return codes ***************************/

#define  OGG_SUCCESS   0

#define  OGG_HOLE     -10
#define  OGG_SPAN     -11
#define  OGG_EVERSION -12
#define  OGG_ESERIAL  -13
#define  OGG_EINVAL   -14
#define  OGG_EEOS     -15

#endif  /* _OGG_H */
