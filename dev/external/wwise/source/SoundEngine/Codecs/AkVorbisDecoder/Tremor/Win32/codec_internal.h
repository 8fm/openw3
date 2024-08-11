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

 function: libvorbis codec headers

 ********************************************************************/

#ifndef _V_CODECI_H_
#define _V_CODECI_H_

#include "ivorbiscodec.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>

#define VI_TRANSFORMB 1
#define VI_WINDOWB 1
#define VI_TIMEB 1
#define VI_FLOORB 2
#define VI_RESB 3
#define VI_MAPB 1

/* Floor backend generic *****************************************/

inline int floor1_memosize(vorbis_info_floor1 *info)
{
  return info->posts;
}

extern  int floor1_info_unpack(vorbis_info_floor1 *,codec_setup_info*,oggpack_buffer *, CAkVorbisAllocator& VorbisAllocator);
extern ogg_int32_t *floor1_inverse1(struct vorbis_dsp_state *,
				    vorbis_info_floor1 *,ogg_int32_t *);
extern int floor1_inverse2 (struct vorbis_dsp_state *,vorbis_info_floor1 *,
			    ogg_int32_t *buffer,ogg_int32_t *);
/* Residue backend generic *****************************************/

typedef struct vorbis_info_residue
{
	int type;
	unsigned char *stagemasks;
	unsigned char *stagebooks;

/* block-partitioned VQ coded straight residue */
	int begin;
	int end;

  /* first stage (lossless partitioning) */
	int           grouping;         /* group n vectors per partition */
	char          partitions;       /* possible codebooks for a partition */
	unsigned char groupbook;        /* huffbook for partitioning */
	char          stages;
} vorbis_info_residue;

extern int res_unpack(vorbis_info_residue *info,
		      codec_setup_info *ci,oggpack_buffer *opb, CAkVorbisAllocator& VorbisAllocator);
extern int res_inverse(vorbis_dsp_state *,vorbis_info_residue *info,
		       ogg_int32_t **in,int *nonzero,int ch);

/* mode ************************************************************/
//typedef struct {
//  unsigned char blockflag;
//  unsigned char mapping;
//} vorbis_info_mode;

/* Mapping backend generic *****************************************/
typedef struct coupling_step{
  unsigned char mag;
  unsigned char ang;
} coupling_step;

typedef struct submap{
  char floor;
  char residue;
} submap;

typedef struct vorbis_info_mapping{
  int            submaps; 
  
  unsigned char *chmuxlist;
  submap        *submaplist;

  int            coupling_steps;
  coupling_step *coupling;
} vorbis_info_mapping;

extern int mapping_info_unpack(vorbis_info_mapping *,codec_setup_info *, int channels,
			       oggpack_buffer *, CAkVorbisAllocator& VorbisAllocator);
extern int mapping_inverse(struct vorbis_dsp_state *,vorbis_info_mapping *);

/* vorbis_dsp_state buffers the current vorbis audio
   analysis/synthesis state.  The DSP state belongs to a specific
   logical bitstream ****************************************************/
struct vorbis_dsp_state2
{
	int			out_begin;
	int			out_end;
	int			lW;
	int			W;
	ogg_uint16_t	extra_samples_begin;
	ogg_uint16_t	extra_samples_end;
};

#define VORBIS_MAX_NUM_CHANNELS (AK_VOICE_MAX_NUM_CHANNELS)

struct vorbis_dsp_state
{
	inline int GetWorkSize()
	{
		int WorkSize	= (csi->blocksizes[1] >> 1) * sizeof(*work[0]);
		WorkSize		= channels * WorkSize;
		WorkSize		= (WorkSize + 15) & ~15;
		return WorkSize;
	}

	inline int GetDctSize()
	{
		int DctSize	= (csi->blocksizes[1] >> 2) * sizeof(*mdctright[0]);
		DctSize		= channels * DctSize;
		DctSize		= (DctSize + 15) & ~15;
		return DctSize;
	}

	oggpack_buffer		opb;

	int channels;
	codec_setup_info	* csi;

	ogg_int32_t			*work[ VORBIS_MAX_NUM_CHANNELS ];
	ogg_int32_t			*mdctright[ VORBIS_MAX_NUM_CHANNELS ];

	vorbis_dsp_state2	state;
#ifdef AK_WIIU
	void *pUnalignedWork;
#endif
};

extern int					vorbis_dsp_init(vorbis_dsp_state* in_pVorbisDspState, int channels);
extern void					vorbis_dsp_clear(vorbis_dsp_state *v );
extern int					vorbis_unpack_books(codec_setup_info *ci,int channels,oggpack_buffer *opb, CAkVorbisAllocator& VorbisAllocator);
extern int					vorbis_dsp_restart(vorbis_dsp_state *v, ogg_uint16_t in_uExtraSamplesBegin, ogg_uint16_t in_uExtraSamplesEnd );

extern void					vorbis_dsp_synthesis(vorbis_dsp_state *vd,ogg_packet *op);
extern int					vorbis_dsp_pcmout(vorbis_dsp_state *v,ogg_int16_t *pcm,int samples);

#endif
