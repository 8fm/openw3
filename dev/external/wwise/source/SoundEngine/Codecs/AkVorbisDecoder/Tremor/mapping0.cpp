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

 function: channel mapping 0 implementation

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "os.h"
#include "ivorbiscodec.h"
#include "mdct.h"
#include "codec_internal.h"
#include "codebook.h"
#include "misc.h"
#include "AkRuntimeEnvironmentMgr.h"

#if !defined(AK_PS3) || defined(SPU_CODEBOOK)

static int ilog(unsigned int v){
  int ret=0;
  if(v)--v;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

/* also responsible for range checking */
int mapping_info_unpack(vorbis_info_mapping*	info,
						codec_setup_info*		ci,
						int channels,
						oggpack_buffer*			opb,
						CAkVorbisAllocator&		VorbisAllocator)
{
	int i;
	int AllocSize;
	memset(info,0,sizeof(*info));

	if(oggpack_read(opb,1))
		info->submaps=oggpack_read(opb,4)+1;
	else
		info->submaps=1;

	if(oggpack_read(opb,1))
	{
		info->coupling_steps=oggpack_read(opb,8)+1;
		int AllocSize = info->coupling_steps*sizeof(*info->coupling);
		info->coupling = (coupling_step*)VorbisAllocator.Alloc(AllocSize);

		for(i=0;i<info->coupling_steps;i++)
		{
			int testM=info->coupling[i].mag=oggpack_read(opb,ilog(channels));
			int testA=info->coupling[i].ang=oggpack_read(opb,ilog(channels));

			if(	testM<0 || 
				testA<0 || 
				testM==testA || 
				testM>=channels ||
				testA>=channels)
			{
				goto err_out;
			}
		}
	}

	if(oggpack_read(opb,2)>0)
	{
		goto err_out; /* 2,3:reserved */
	}

	if(info->submaps>1)
	{
		int AllocSize = sizeof(*info->chmuxlist)*channels;
		info->chmuxlist = (unsigned char*)VorbisAllocator.Alloc(AllocSize);

		for(i=0;i<channels;i++)
		{
			info->chmuxlist[i]=oggpack_read(opb,4);
			if(info->chmuxlist[i]>=info->submaps)
			{
				goto err_out;
			}
		}
	}

	AllocSize = sizeof(*info->submaplist)*info->submaps;
	info->submaplist = (submap*)VorbisAllocator.Alloc(AllocSize);

	for(i=0;i<info->submaps;i++)
	{
		oggpack_read(opb,8);
		info->submaplist[i].floor=oggpack_read(opb,8);
		if(info->submaplist[i].floor>=ci->floors)
		{
			goto err_out;
		}
		info->submaplist[i].residue=oggpack_read(opb,8);
		if(info->submaplist[i].residue>=ci->residues)
		{
			goto err_out;
		}
  }

  return 0;

 err_out:
  return -1;
}
#endif

#if !defined(AK_PS3) || defined(SPU_DECODER)

int mapping_inverse(vorbis_dsp_state *vd,vorbis_info_mapping *info)
{
	codec_setup_info     *ci= vd->csi;

	int                   i,j;
	long                  n=ci->blocksizes[vd->state.W];

	ogg_int32_t **pcmbundle = (ogg_int32_t**)alloca(sizeof(*pcmbundle)*vd->channels);
	int          *zerobundle = (int*)alloca(sizeof(*zerobundle)*vd->channels);
	int          *nonzero	= (int*)alloca(sizeof(*nonzero)*vd->channels);
	ogg_int32_t **floormemo = (ogg_int32_t**)alloca(sizeof(*floormemo)*vd->channels);

	/* recover the spectral envelope; store it in the PCM vector for now */
	for(i=0;i<vd->channels;i++)
	{
		int submap=0;
		int floorno;

		if(info->submaps>1)
		{
			submap=info->chmuxlist[i];
		}
		floorno=info->submaplist[submap].floor;

		/* floor 1 */
		floormemo[i] = (ogg_int32_t*)alloca(sizeof(*floormemo[i])*floor1_memosize(&ci->floor_param[floorno]));

		floormemo[i]=floor1_inverse1(vd,&ci->floor_param[floorno],floormemo[i]);

		if(floormemo[i])
		{
			nonzero[i]=1;
		}
		else
		{
			nonzero[i]=0;
		}
		AkZeroMemLarge(vd->work[i],sizeof(*vd->work[i])*n/2);
	}

	/* channel coupling can 'dirty' the nonzero listing */
	for(i=0;i<info->coupling_steps;i++)
	{
		if(nonzero[info->coupling[i].mag] ||
			nonzero[info->coupling[i].ang])
		{
				nonzero[info->coupling[i].mag]=1; 
				nonzero[info->coupling[i].ang]=1; 
		}
	}

	/* recover the residue into our working vectors */
	for(i=0;i<info->submaps;i++)
	{
		int ch_in_bundle	= 0;

		for(j = 0 ; j < vd->channels ; j++)
		{
			if(!info->chmuxlist || info->chmuxlist[j]==i)
			{
				if(nonzero[j])
				{
					zerobundle[ch_in_bundle]=1;
				}
				else
				{
					zerobundle[ch_in_bundle]=0;
				}
				pcmbundle[ch_in_bundle++]=vd->work[j];
			}
		}

		res_inverse(vd,ci->residue_param+info->submaplist[i].residue,pcmbundle,zerobundle,ch_in_bundle);
	}

	//for(j=0;j<vd->channels;j++)
	//_analysis_output("coupled",seq+j,vb->pcm[j],-8,n/2,0,0);

	/* channel coupling */
#ifdef AK_PS3
	for(i=info->coupling_steps-1;i>=0;i--)
	{
		ogg_int32_t *pcmM=vd->work[info->coupling[i].mag];
		ogg_int32_t *pcmA=vd->work[info->coupling[i].ang];
		static const vec_int4 kvZero = {0, 0, 0, 0};

		vec_int4 *vPM = (vec_int4 *)pcmM;
		vec_int4 *vPA = (vec_int4 *)pcmA;

		for(j=0; j < n/8; j++)
		{	
			vec_int4 vMag		= vPM[j];
			vec_int4 vAng		= vPA[j];
			
			vec_uint4 vMagGT0	= spu_cmpgt(vMag, kvZero);
			vec_uint4 vAngGT0	= spu_cmpgt(vAng, kvZero);

			vec_int4 vMagAngAdd = spu_add(vMag, vAng);
			vec_int4 vMagAngSub = spu_sub(vMag, vAng);

			vec_int4 TempAdd0	= spu_sel(vMagAngAdd,vMag,vAngGT0);
			vec_int4 TempAdd1	= spu_sel(vMag,vMagAngAdd,vAngGT0);

			vec_int4 TempSub0	= spu_sel(vMagAngSub,vMag,vAngGT0);
			vec_int4 TempSub1	= spu_sel(vMag,vMagAngSub,vAngGT0);

			vPM[j]				= spu_sel(TempSub0, TempAdd0, vMagGT0);
			vPA[j]				= spu_sel(TempAdd1, TempSub1, vMagGT0);
		}
	}
#else
	for(i=info->coupling_steps-1;i>=0;i--)
	{
		ogg_int32_t *pcmM=vd->work[info->coupling[i].mag];
		ogg_int32_t *pcmA=vd->work[info->coupling[i].ang];

		for(j=0;j<n/2;j++)
		{
			ogg_int32_t mag=pcmM[j];
			ogg_int32_t ang=pcmA[j];

			if(mag>0)
			{
				if(ang>0)
				{
					pcmM[j]=mag;
					pcmA[j]=mag-ang;
				}
				else
				{
					pcmA[j]=mag;
					pcmM[j]=mag+ang;
				}
			}
			else
			{
				if(ang>0)
				{
					pcmM[j]=mag;
					pcmA[j]=mag+ang;
				}
				else
				{
					pcmA[j]=mag;
					pcmM[j]=mag-ang;
				}
			}
		}
	}
#endif
	//for(j=0;j<vd->channels;j++)
	//_analysis_output("residue",seq+j,vb->pcm[j],-8,n/2,0,0);

	/* compute and apply spectral envelope */
	for(i=0;i<vd->channels;i++)
	{
		ogg_int32_t *pcm = vd->work[i];
		int submap=0;
		int floorno;

		if(info->submaps>1)
		{
			submap=info->chmuxlist[i];
		}
		floorno=info->submaplist[submap].floor;

		/* floor 1 */
		floor1_inverse2(vd,&ci->floor_param[floorno],floormemo[i],pcm);
	}

	//for(j=0;j<vd->channels;j++)
	//_analysis_output("mdct",seq+j,vb->pcm[j],-24,n/2,0,1);

	/* transform the PCM data; takes PCM vector, vb; modifies PCM vector */
	/* only MDCT right now.... */
	for(i=0;i<vd->channels;i++)
	{
#ifdef AK_TREMOR_FIXED_POINT		
		mdct_backward(n, vd->work[i]);
#else
		mdct_backward(n, (float *) vd->work[i]);
#endif
	}

	//for(j=0;j<vd->channels;j++)
	//_analysis_output("imdct",seq+j,vb->pcm[j],-24,n,0,0);

	/* all done! */

	return(0);
}
#endif
