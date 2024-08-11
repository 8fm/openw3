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

//////////////////////////////////////////////////////////////////////
//
// AkSource.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkSource.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include "AkFXMemAlloc.h"
#include "AkBankMgr.h"
#include "AkEffectsMgr.h"
#include "AkMonitor.h"
#include "AkURenderer.h"
#include "AkBankMgr.h"

#define SOURCE_DEFAULT_FILE_FORMAT					eFileWAV	//eFileInvalidFormat
#define SOURCE_DEFAULT_CHANNELSMASK					AK_SPEAKER_SETUP_MONO
#define SOURCE_DEFAULT_BITSPERSAMPLE				AK_LE_NATIVE_BITSPERSAMPLE
#define SOURCE_DEFAULT_BLOCKALIGN					(SOURCE_DEFAULT_BITSPERSAMPLE/8)
#define SOURCE_DEFAULT_TYPEID						AK_INT
#define SOURCE_DEFAULT_INTERLEAVEID					AK_INTERLEAVED
#define SOURCE_DEFAULT_DATAOFFSET					0
#define SOURCE_DEFAULT_DATASIZE						0

#define SOURCE_DEFAULT_TYPE							SrcTypeNone

CAkSource::CAkSource()
{
	SetDefaultParams();
}

void CAkSource::SetDefaultParams()
{
	// Default source type.
	m_sSrcTypeInfo.dwID             = AK_INVALID_SOURCE_ID;	// NOTE: dwID is a FileID. 
	m_sSrcTypeInfo.pInMemoryMedia	= NULL;
	m_sSrcTypeInfo.mediaInfo.SetDefault( SOURCE_DEFAULT_TYPE, AK_INVALID_UNIQUE_ID );
}

CAkSource::~CAkSource()
{
	FreeSource();
}

// File sources (pushed by Wwise, or by the game using external source info).
void CAkSource::SetSource(
	AkUniqueID in_sourceID,			// Source ID.
	AkUInt32  in_PluginID,			// Conversion plug-in id.
	const AkOSChar* in_pszFilePath,	// Filename.
	AkFileID in_uCacheID,			// Unique cache ID.
	bool in_bIsFromRSX,				// Is it from RSX?
	bool in_bExternallySupplied		// Source data is supplied by the game ("external source").
	)
{
	FreeSource();

	m_sSrcTypeInfo.dwID				= in_PluginID;
	
	if( in_pszFilePath != NULL )
	{
		size_t uNumChars = AKPLATFORM::OsStrLen( in_pszFilePath ) + 1;

		m_sSrcTypeInfo.pszFilename = (AkOSChar*)AkAlloc( g_DefaultPoolId, uNumChars * sizeof( AkOSChar ) );

		if( m_sSrcTypeInfo.pszFilename != NULL )
		{
			AKPLATFORM::SafeStrCpy( m_sSrcTypeInfo.pszFilename, in_pszFilePath, uNumChars );			 
		}
	}

	m_sSrcTypeInfo.mediaInfo.SetDefault( SrcTypeFile, in_sourceID );
	m_sSrcTypeInfo.mediaInfo.uFileID = in_uCacheID;	// use uFileID as cacheID when file is specified by string.
    m_sSrcTypeInfo.mediaInfo.bHasSource = true;
	m_sSrcTypeInfo.mediaInfo.bIsFromRSX = in_bIsFromRSX;
	m_sSrcTypeInfo.mediaInfo.bExternallySupplied = in_bExternallySupplied;
	m_sSrcTypeInfo.mediaInfo.bUseFilenameString = true;
}

void CAkSource::SetSource( AkUInt32 in_PluginID, AkMediaInformation in_MediaInfo )
{
	FreeSource();

	m_sSrcTypeInfo.dwID					= in_PluginID;
	m_sSrcTypeInfo.mediaInfo			= in_MediaInfo;
	m_sSrcTypeInfo.mediaInfo.bHasSource = true;
}

void CAkSource::SetSource( AkUInt32 in_PluginID, void* in_pInMemoryMedia, AkMediaInformation in_MediaInfo )
{
	FreeSource();

	m_sSrcTypeInfo.dwID					= in_PluginID;
	m_sSrcTypeInfo.pInMemoryMedia		= in_pInMemoryMedia;

	m_sSrcTypeInfo.mediaInfo			= in_MediaInfo;
	m_sSrcTypeInfo.mediaInfo.bHasSource = true;
}

void CAkSource::SetSource( 
	AkUniqueID in_sourceID )
{
	FreeSource();

    // Store in source info.
	m_sSrcTypeInfo.dwID = AK_INVALID_SOURCE_ID;
	m_sSrcTypeInfo.mediaInfo.SetDefault( SrcTypeModelled, in_sourceID ); 
}

void CAkSource::FreeSource()
{
	// We own the file name string, but not in-memory media. Free only if we are a SrcTypeFile!
	if ( SrcTypeFile == m_sSrcTypeInfo.mediaInfo.Type
		&& m_sSrcTypeInfo.pszFilename != NULL )
    {
        AkFree( g_DefaultPoolId, m_sSrcTypeInfo.pszFilename );
		m_sSrcTypeInfo.pszFilename = NULL;
    }

	m_sSrcTypeInfo.mediaInfo.SetDefault( SrcTypeNone, AK_INVALID_UNIQUE_ID );
	m_sSrcTypeInfo.pInMemoryMedia = NULL;
	m_sSrcTypeInfo.mediaInfo.bHasSource = false;
}

//-----------------------------------------------------------------------------
// Name: BANK ACCESS INTERFACE : LockDataPtr()
// Desc: Gives access to bank data pointer. Pointer should be released.
//
// Parameters:
//	void *& out_ppvBuffer		: Returned pointer to data.
//	AkUInt32 * out_ulSize		: Size of data returned.
//-----------------------------------------------------------------------------
void CAkSource::LockDataPtr( void *& out_ppvBuffer, AkUInt32 & out_ulSize, CAkUsageSlot*& out_rpSlot )
{
	if (m_sSrcTypeInfo.pInMemoryMedia == NULL)
	{
		AkMediaInfo mediaInfo = g_pBankManager->GetMedia( m_sSrcTypeInfo.mediaInfo.sourceID, out_rpSlot );

		out_ulSize = mediaInfo.uInMemoryDataSize;
		out_ppvBuffer = mediaInfo.pInMemoryData;
	}
	else
	{
		out_ulSize = m_sSrcTypeInfo.mediaInfo.uInMemoryMediaSize;
		out_ppvBuffer = m_sSrcTypeInfo.pInMemoryMedia;
	}
}

void CAkSource::UnLockDataPtr()
{
	if (m_sSrcTypeInfo.pInMemoryMedia == NULL)
		g_pBankManager->ReleaseMedia( m_sSrcTypeInfo.mediaInfo.sourceID );
}

AKRESULT CAkSource::PrepareData()
{
	if( m_sSrcTypeInfo.mediaInfo.uInMemoryMediaSize != 0 && m_sSrcTypeInfo.pInMemoryMedia == NULL )
	{
		return g_pBankManager->PrepareSingleMedia( m_sSrcTypeInfo );
	}
	else
	{
		return AK_Success;
	}
}

void CAkSource::UnPrepareData()
{
	if( m_sSrcTypeInfo.mediaInfo.uInMemoryMediaSize != 0 && m_sSrcTypeInfo.pInMemoryMedia == NULL )
	{
		g_pBankManager->UnprepareSingleMedia( m_sSrcTypeInfo.mediaInfo.sourceID );
	}
}

CAkSource* CAkSource::Clone()
{
	AKASSERT(IsExternal());

	CAkSource* pClone = (CAkSource*)AkNew(g_DefaultPoolId, CAkSource());
	if (pClone == NULL)
		return NULL;

	*pClone = *this;
	return pClone;
}
