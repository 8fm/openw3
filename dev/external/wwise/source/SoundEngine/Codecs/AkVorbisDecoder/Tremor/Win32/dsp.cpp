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

 function: PCM data vector blocking, windowing and dis/reassembly

 ********************************************************************/

#include <stdlib.h> 
#include "ogg.h"
#include "mdct.h"
#include "ivorbiscodec.h"
#include "codec_internal.h"
#include "window_lookup.h"

int vorbis_dsp_restart(vorbis_dsp_state *v, ogg_uint16_t in_uExtraSamplesBegin, ogg_uint16_t in_uExtraSamplesEnd )
{
	v->state.out_end		= -1;
	v->state.out_begin		= -1;
	v->state.extra_samples_begin = in_uExtraSamplesBegin;
	v->state.extra_samples_end = in_uExtraSamplesEnd;

	return 0;
}

int vorbis_dsp_init(vorbis_dsp_state* v, int channels )
{
	int i;

	v->channels = channels;

	// otherwise vorbis_dsp_clear will take it for some valid address
	v->work[0]		= NULL;

	// allocate the arrays
	int WorkSize	= v->GetWorkSize();
	int DctSize		= v->GetDctSize();
	int AllocSize	= WorkSize + DctSize;

	// Do not count in UVM, do not alloc in UVM memory
	char* pWork		= (char*)_ogg_malloc(AllocSize);

	// out of memory
	if(pWork == NULL)
	{
		return -1;
	}

	char* pDct		= pWork + WorkSize;

	AkZeroMemLarge(pWork,AllocSize);

	// we need the size per channel now
	WorkSize /= channels;
	DctSize /= channels;
	for(i = 0 ; i < channels ; ++i)
	{
		v->work[i]		= (ogg_int32_t *)pWork;
		v->mdctright[i]	= (ogg_int32_t *)pDct;

		pWork += WorkSize;
		pDct += DctSize;
	}

	v->state.lW=0; /* previous window size */
	v->state.W=0;  /* current window size */

	return 0;
}

void vorbis_dsp_clear(vorbis_dsp_state *v)
{
	if(v->work[0])
	{
		// Do not count in UVM, do not alloc in UVM memory
		_ogg_free(v->work[0]);
		v->work[0] = NULL;
	}
}

static LOOKUP_T *_vorbis_window(int left){
  switch(left){
/*
  case 32:
    return vwin64;
  case 64:
    return vwin128;
*/
  case 128:
    return vwin256;
  case 256:
    return vwin512;
  case 512:
    return vwin1024;
  case 1024:
    return vwin2048;
  case 2048:
    return vwin4096;
/*
#ifndef LIMIT_TO_64kHz
  case 4096:
    return vwin8192;
#endif
*/
  default:
    return(0);
  }
}

/* pcm==0 indicates we just want the pending samples, no more */
int vorbis_dsp_pcmout(vorbis_dsp_state *v,ogg_int16_t *pcm,int samples)
{
	if(v->state.out_begin < v->state.out_end)
	{
		AKASSERT( v->state.out_begin > -1 );

		int n = v->state.out_end - v->state.out_begin;
		if(pcm)
		{
			//AkPrefetchZero(pcm, samples*sizeof(ogg_int16_t));

			codec_setup_info *ci = v->csi;
			if(n > samples)
			{
				n = samples;
			}
			LOOKUP_T *Window0 = _vorbis_window(ci->blocksizes[0]>>1);
			LOOKUP_T *Window1 = _vorbis_window(ci->blocksizes[1]>>1);
			int i = 0;
			do
			{
				mdct_unroll_lap(ci->blocksizes[0],
								ci->blocksizes[1],
								v->state.lW,
								v->state.W,
#ifdef AK_TREMOR_FIXED_POINT		
								v->work[i],
								v->mdctright[i],
#else
								(float *) v->work[i],
								(float *) v->mdctright[i],
#endif
								Window0,
								Window1,
								pcm + i,
								v->channels,
								v->state.out_begin,
								v->state.out_begin + n);
			}
			while ( ++i < v->channels );
			v->state.out_begin += n;
		}
		return(n);
	}
	return(0);
}

void vorbis_dsp_synthesis(vorbis_dsp_state *vd,ogg_packet *op)
{
	codec_setup_info* ci = vd->csi;

	oggpack_readinit(&vd->opb,&(op->buffer));

	/* read our mode */
	int mode=oggpack_read(&vd->opb,1);
	AKASSERT( mode < ci->modes );

	/* shift information we still need from last window */
	vd->state.lW=vd->state.W;
	vd->state.W=ci->mode_param[mode].blockflag;

	int iBlockSizeLW = ci->blocksizes[vd->state.lW];

	int i=0;
	do
	{
		mdct_shift_right(iBlockSizeLW,vd->work[i],vd->mdctright[i]);
	}
	while( ++i<vd->channels );

	if( vd->state.out_begin != -1)
	{
		vd->state.out_begin=0;
		vd->state.out_end=iBlockSizeLW/4+ci->blocksizes[vd->state.W]/4;

		if ( AK_EXPECT_FALSE( vd->state.extra_samples_begin ) )
		{
			/* partial first frame, or seek table overshoot. Strip the extra samples off */
			vd->state.out_begin = vd->state.extra_samples_begin;
			if ( vd->state.out_begin > vd->state.out_end )
			{
				vd->state.out_begin = vd->state.out_end;
				vd->state.extra_samples_begin -= vd->state.out_end;

				// If we are more than 2 windows away from target start, no
				// need to synthesize this packet.
				if ( vd->state.extra_samples_begin >= ( ci->blocksizes[ 1 ] / 2 ) )
					return;
			}
			else
			{
				vd->state.extra_samples_begin = 0;
			}
		}

		if ( AK_EXPECT_FALSE( op->e_o_s ) )
		{
			/* partial last frame.  Strip the extra samples off */
			vd->state.out_end -= vd->state.extra_samples_end;
			if ( vd->state.out_end < vd->state.out_begin )
				vd->state.out_end = vd->state.out_begin;
		}
	}
	else
	{
		vd->state.out_begin=0;
		vd->state.out_end=0;

		// If we are more than 2 windows away from target start, no
		// need to synthesize this packet.
		if ( vd->state.extra_samples_begin >= ( ci->blocksizes[ 1 ] / 2 ) )
			return;
	}

	/* packet decode and portions of synthesis that rely on only this block */
	mapping_inverse(vd,ci->map_param+ci->mode_param[mode].mapping);
}
