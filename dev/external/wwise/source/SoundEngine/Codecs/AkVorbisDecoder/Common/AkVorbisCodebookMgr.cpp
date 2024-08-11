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

#include "AkVorbisCodebookMgr.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "AkPBI.h"

#ifdef AK_PS3
	#include "AkVorbisCodecPS3.h"
#else
	#include "AkVorbisCodec.h"
#endif

AkVorbisCodebookMgr g_VorbisCodebookMgr;

AkVorbisCodebookMgr::AkVorbisCodebookMgr()
{
}

AkVorbisCodebookMgr::~AkVorbisCodebookMgr()
{
#ifdef _DEBUG
	AKASSERT( m_codebooks.Length() == 0 );
#endif
}

CAkVorbisAllocator * AkVorbisCodebookMgr::Decodebook( 
	AkVorbisSourceState & in_VorbisState, 
	CAkPBI * in_pPBI,		// For error monitoring
#ifdef AK_PS3
	CodebookDecodeState & out_eDecodeState
#else
	ogg_packet *op
#endif
	)
{
	// Look for existing codebook

	Codebook * pCodebook = m_codebooks.Exists( in_VorbisState.VorbisInfo.uHashCodebook );
	if ( pCodebook )
	{
		++pCodebook->cRef;
#ifdef AK_PS3
		// It might be in the list because a task has just been launched -- in which case
		// the codebooks are not ready yet and we need to wait.
		codec_setup_info * csi = (codec_setup_info *) pCodebook->allocator.GetAddress();
		out_eDecodeState = csi->book_param ? Codebook_Decoded : Codebook_NeedsWait;
#endif
		return &pCodebook->allocator;
	}

	// Not found; create new codebook entry and decode

	AkNew2( pCodebook, g_LEngineDefaultPoolId, Codebook, Codebook() );
	if ( !pCodebook ) 
		return NULL;

#if defined( AK_CPU_X86_64 )
	AkUInt32 uAllocSize = in_VorbisState.VorbisInfo.dwDecodeX64AllocSize;
#else
	AkUInt32 uAllocSize = in_VorbisState.VorbisInfo.dwDecodeAllocSize;
#endif

	// Placed here to calm the PS3 compiler
	codec_setup_info * csi;
	int iResult;
	int channels = AK::GetNumChannels(in_VorbisState.TremorInfo.uChannelMask);

#ifdef AK_PS3
	if ( !pCodebook->allocator.Init( (uAllocSize + 0xf) & ~0xf ) )
#else
	if ( !pCodebook->allocator.Init( uAllocSize ) )
#endif
		goto error;

	// Many things depend on this being the first allocation in the UVM
	csi = (codec_setup_info *) pCodebook->allocator.Alloc( sizeof( codec_setup_info ) );

	iResult = vorbis_info_init(
		csi,  
		in_VorbisState.VorbisInfo.uBlockSizes[0],
		in_VorbisState.VorbisInfo.uBlockSizes[1]);
	if ( iResult )
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader, in_pPBI );
		goto error;
	}

#ifndef AK_PS3
	oggpack_buffer opb;
	oggpack_readinit(&opb,&(op->buffer));

	iResult = vorbis_unpack_books(
		csi,
		channels,
		&opb,
		pCodebook->allocator);
	if ( iResult )
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader, in_pPBI );
		goto error;
	}
#else
	out_eDecodeState = Codebook_NeedsDecode; // decoding goes into a job on the PS3
#endif

	// Success

	++pCodebook->cRef;

	pCodebook->key = in_VorbisState.VorbisInfo.uHashCodebook;
	m_codebooks.Set( pCodebook );

	return &pCodebook->allocator;

error:
	pCodebook->allocator.Term( );
	AkDelete( g_LEngineDefaultPoolId, pCodebook );
	return NULL;
}

void AkVorbisCodebookMgr::ReleaseCodebook( AkVorbisSourceState & in_VorbisState )
{
	CodebookList::IteratorEx it = m_codebooks.FindEx( in_VorbisState.VorbisInfo.uHashCodebook );
	if ( it != m_codebooks.End() )
	{
		Codebook * pCodebook = *it;
		if ( --pCodebook->cRef <= 0 )
		{
			m_codebooks.Erase( it );

			pCodebook->allocator.Term( );
			AkDelete( g_LEngineDefaultPoolId, pCodebook );
		}
		return;
	}

	AKASSERT( false && "Vorbis Codebook not found" );
}
