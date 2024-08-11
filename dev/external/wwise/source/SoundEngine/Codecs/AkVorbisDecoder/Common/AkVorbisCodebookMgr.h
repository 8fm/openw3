/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#ifndef _AK_VORBIS_CODEBOOK_MGR_H_
#define _AK_VORBIS_CODEBOOK_MGR_H_

#include "AkSrcVorbis.h"
#include "AkHashList.h"
#include "AkPrivateTypes.h"
#include "packed_codebooks.h"

struct AkVorbisSourceState;

class AkVorbisCodebookMgr
{
public:
	AkVorbisCodebookMgr();
	~AkVorbisCodebookMgr();

	enum CodebookDecodeState
	{
		Codebook_NeedsDecode,
		Codebook_NeedsWait,
		Codebook_Decoded
	};

	CAkVorbisAllocator * Decodebook( 
		AkVorbisSourceState & in_VorbisState, 
		class CAkPBI * in_pPBI,		// For error monitoring
#ifdef AK_PS3
		CodebookDecodeState & out_eDecodeState
#else
		ogg_packet *op
#endif
		);

	void ReleaseCodebook( AkVorbisSourceState & in_VorbisState );

private:
	struct Codebook
	{
		AkUInt32 key;
		Codebook * pNextItem;
		CAkVorbisAllocator allocator;
		AkInt32 cRef;

		Codebook() : cRef( 0 ) {}
	};

	typedef AkHashListBare<AkUInt32, Codebook, AK_SMALL_HASH_SIZE> CodebookList;
	CodebookList m_codebooks;
};

extern AkVorbisCodebookMgr g_VorbisCodebookMgr;

#endif // _AK_VORBIS_CODEBOOK_MGR_H_
