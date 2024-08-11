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

 function: maintain the info structure, info <-> header packets

 ********************************************************************/

/* general handling of the header and the vorbis_info structure (and
   substructures) */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ogg.h"
#include "ivorbiscodec.h"
#include "codec_internal.h"
#include "packed_codebooks.h"
#include "codebook.h"
#include "os.h"

#ifdef __SPU__

#include <AK/Plugin/PluginServices/PS3/SPUServices.h>
#include "libsn_spu.h"

#else

/* helpers */
/* used by synthesis, which has a full, alloced vi */
int vorbis_info_init(codec_setup_info *csi, long in_blocksize0, long in_blocksize1 )
{
	memset(csi,0,sizeof(*csi));

	csi->blocksizes[0] = 1 << in_blocksize0;
	csi->blocksizes[1] = 1 << in_blocksize1;

	if(csi->blocksizes[0]<64)
	{
	  goto err_out; 
	}
	if(csi->blocksizes[1]<csi->blocksizes[0])
	{
	  goto err_out;
	}
	if(csi->blocksizes[1]>8192)
	{
	  goto err_out;
	}

	return(0);

err_out:
	return(OV_EBADHEADER);
}

#endif

#if !defined(AK_PS3) || defined(SPU_CODEBOOK)

/* all of the real encoding details are here.  The modes, books,
   everything */
#ifdef AK_PS3
int vorbis_unpack_books(codec_setup_info *ci,int channels,oggpack_buffer *opb, AkUIntPtr * ppPackedCodebooks, void * pScratch, AkUInt32 DmaTag, CAkVorbisAllocator& VorbisAllocator)
#else
int vorbis_unpack_books(codec_setup_info *ci,int channels,oggpack_buffer *opb, CAkVorbisAllocator& VorbisAllocator)
#endif
{
	int i;
//--------------------------------------------------------------------------------
// codebooks
//--------------------------------------------------------------------------------
	ci->books = oggpack_read(opb,8)+1;
	int AllocSize = sizeof(*ci->book_param)*ci->books;
	ci->book_param = (codebook *)VorbisAllocator.Calloc(AllocSize);

	for(i = 0 ; i < ci->books ; i++)
	{
		unsigned int idx = oggpack_read(opb,10);
		oggpack_buffer opbBook;

#ifdef AK_PS3
		AkUIntPtr uCodebookAddr = ppPackedCodebooks[ idx ];
		AkUIntPtr uCodebookAddrAligned = uCodebookAddr & ~0xf;
		AkUIntPtr uCodebookSize = ( ppPackedCodebooks[ idx+1 ] - uCodebookAddrAligned + 0xf ) & ~0xf;

		AkDmaGet( "VorbisCodebook::Book", pScratch, uCodebookAddrAligned, uCodebookSize, DmaTag, 0, 0 );
		AkDmaWait(1<<DmaTag);

		unsigned char * pData = (unsigned char *) pScratch + ( uCodebookAddr - uCodebookAddrAligned );
		oggpack_readinit( &opbBook, pData, MAX_PACKED_CODEBOOK_SIZE );
#else
		ogg_buffer bufBook;
		bufBook.data = (unsigned char *) g_packedCodebooks[ idx ];
		bufBook.size = MAX_PACKED_CODEBOOK_SIZE;
		oggpack_readinit( &opbBook, &bufBook );
#endif

		vorbis_book_unpack(&opbBook,ci->book_param + i,VorbisAllocator);
	}
//--------------------------------------------------------------------------------
// floor backend settings
//--------------------------------------------------------------------------------
	ci->floors = oggpack_read(opb,6)+1;

	AllocSize = sizeof(vorbis_info_floor1) * ci->floors;

	ci->floor_param	= (vorbis_info_floor1*)VorbisAllocator.Calloc(AllocSize);

	for(i = 0 ; i < ci->floors ; i++)
	{
		if(floor1_info_unpack(&ci->floor_param[i],ci,opb,VorbisAllocator) != 0)
		{
			goto err_out;
		}
	}
//--------------------------------------------------------------------------------
// residue backend settings
//--------------------------------------------------------------------------------
	ci->residues = oggpack_read(opb,6)+1;
	AllocSize = sizeof(*ci->residue_param)*ci->residues;
	ci->residue_param	= (vorbis_info_residue*)VorbisAllocator.Alloc(AllocSize);

	for(i = 0 ; i < ci->residues ; ++i)
	{
		if(res_unpack(ci->residue_param + i,ci,opb,VorbisAllocator))
		{
			goto err_out;
		}
	}
//--------------------------------------------------------------------------------
// map backend settings
//--------------------------------------------------------------------------------
	ci->maps = oggpack_read(opb,6)+1;
	AllocSize = sizeof(*ci->map_param)*ci->maps;
	ci->map_param	= (vorbis_info_mapping*)VorbisAllocator.Alloc(AllocSize);

	for(i = 0 ; i < ci->maps ; ++i)
	{
		if(mapping_info_unpack(ci->map_param+i,ci,channels,opb,VorbisAllocator))
		{
			goto err_out;
		}
	}
//--------------------------------------------------------------------------------
// mode settings
//--------------------------------------------------------------------------------
	ci->modes=oggpack_read(opb,6)+1;
	AllocSize = ci->modes*sizeof(*ci->mode_param);
	ci->mode_param	= (vorbis_info_mode *)VorbisAllocator.Alloc(AllocSize);

	for(i = 0 ; i < ci->modes ; ++i)
	{
		ci->mode_param[i].blockflag = oggpack_read(opb,1);
		ci->mode_param[i].mapping = oggpack_read(opb,8);
		if(ci->mode_param[i].mapping>=ci->maps)
		{
			goto err_out;
		}
	}

	return(0);

err_out:
	return(OV_EBADHEADER);
}

#endif