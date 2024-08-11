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
// AkSource.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AKSOURCE_H_
#define _AKSOURCE_H_

#include "AkCommon.h"
#include <AK/Tools/Common/AkArray.h>
#include "AkPoolSizes.h"
#include "AudiolibDefs.h"

class CAkRegisteredObj;
class CAkUsageSlot;

//-----------------------------------------------------------------------------
// Name: struct AkSrcTypeInfo
// Desc: Defines the parameters of an audio source: e.g. file, physical 
//       modelling, or memory.
//-----------------------------------------------------------------------------

struct AkMediaInformation
{
	// Not using constructor as we don't want the bank reader to uselessly init those values before filling them for real.
	void SetDefault( AkSrcType in_SrcType, AkUniqueID in_sourceID )
	{
		sourceID			= in_sourceID;
		uFileID				= AK_INVALID_FILE_ID;
		uFileOffset			= 0;
		uInMemoryMediaSize	= 0;
		bIsLanguageSpecific = false;
		bPrefetch			= false;
		Type				= in_SrcType;
        bHasSource          = false;
		bExternallySupplied	= false;
		bIsFromRSX			= false;
		bUseFilenameString	= false;
	}

	AkForceInline bool operator<(const AkMediaInformation& other)		
	{
		if( uFileID == other.uFileID )
		{
			return uFileOffset < other.uFileOffset;
		}
		return uFileID < other.uFileID;
	}

	AkForceInline bool operator>(const AkMediaInformation& other)		
	{
		if( uFileID == other.uFileID )
		{
			return uFileOffset > other.uFileOffset;
		}
		return uFileID > other.uFileID;
	}

	AkUniqueID	sourceID;
	AkFileID	uFileID;
	AkUInt32	uFileOffset;
	AkUInt32	uInMemoryMediaSize;
	AkUInt32	bIsLanguageSpecific : 1;
	AkUInt32	bPrefetch    : 1;       // True=data in-memory, false=data not prefetched.
	AkUInt32	/*AkSrcType*/  Type    :SRC_TYPE_NUM_BITS; // File, physical modeling, etc source.

    AkUInt32	bHasSource: 1;// Only significant when in use by the source itself.	
	AkUInt32	bExternallySupplied: 1; //This media is supplied by the game.
	AkUInt32	bIsFromRSX: 1;	// Streamed fom RSX.
	AkUInt32	bUseFilenameString :1;	// True when source set by authoring tool. Source will use pszFilename and uFileID as unique caching ID.
};

struct AkSrcTypeInfo
{
	inline void *			GetMedia() { AKASSERT( mediaInfo.Type != SrcTypeFile ); return pInMemoryMedia; }
	inline bool				UseFilenameString() { return mediaInfo.bUseFilenameString; }
	inline const AkOSChar *	GetFilename() { AKASSERT( mediaInfo.Type == SrcTypeFile ); return pszFilename; }
	inline AkFileID			GetFileID() { AKASSERT( mediaInfo.Type == SrcTypeFile ); return mediaInfo.uFileID; }
	inline AkFileID			GetCacheID() { AKASSERT( mediaInfo.Type == SrcTypeFile ); return ( !mediaInfo.bExternallySupplied ) ? mediaInfo.uFileID : AK_INVALID_FILE_ID; }

	AkMediaInformation		mediaInfo;

	union
	{
		void *				pInMemoryMedia;		// Pointer to in memory data, OR...
		AkOSChar *			pszFilename;		// File name string.
	};
    AkUInt32				dwID;             		// Device ID for stream files, custom shareset id for source plugins
};

class CAkSource
{
public:
	// Constructor and destructor
	CAkSource();
	~CAkSource();

	void SetSource(			            // File sources (pushed by Wwise, or by the game using external source info).
		AkUniqueID in_sourceID,			// Source ID.
		AkUInt32  in_PluginID,			// Conversion plug-in id.
		const AkOSChar* in_szFileName,	// Filename.
		AkFileID in_uCacheID,			// Unique cache ID.
		bool in_bIsFromRSX,				// Is it from RSX?
		bool in_bExternallySupplied		// Source data is supplied by the game ("external source").
		);

	void SetSource(			            // All file sources specified by file ID
		AkUInt32  in_PluginID,			// Conversion plug-in id.
		AkMediaInformation in_MediaInfo
		);

	void SetSource(						// External in-memory sources
		AkUInt32 in_PluginID,			// Conversion plug-in id.
		void* in_pInMemoryMedia,		// In-memory media
		AkMediaInformation in_MediaInfo 
		);

	void SetSource(			            // Physical modelling source.
		AkUniqueID		in_sourceID		// Source ID.
		);

	void FreeSource();

	CAkSource* Clone();

	// Get/Set source info and format structs.
	AkUniqueID			GetSourceID() { return m_sSrcTypeInfo.mediaInfo.sourceID; }
	AkSrcTypeInfo *		GetSrcTypeInfo() { return &m_sSrcTypeInfo; }

	void LockDataPtr( void *& out_ppvBuffer, AkUInt32 & out_ulDataSize, CAkUsageSlot*& out_rpSlot );
	void UnLockDataPtr();

	bool IsZeroLatency() const 
	{
		return m_sSrcTypeInfo.mediaInfo.bPrefetch;
	}

	bool IsFromRSX() const 
	{
		return m_sSrcTypeInfo.mediaInfo.bIsFromRSX;
	}

	void IsZeroLatency( bool in_bIsZeroLatency )
	{
		m_sSrcTypeInfo.mediaInfo.bPrefetch = in_bIsZeroLatency; 
	}

	// External sources management.
	// IMPORTANT: IsExternal() returns true if a "template" external source specifies that the source should 
	// be filled at run-time by an external source (which info is passed with the AkExternalSourceInfo structure).
	// When resolving an external source, the "IsExternal" source is cloned and m_bIsExternallySupplied is
	// set to true, thus IsExternallySupplied() would return true. This is an indicator for PBIs that the source is
	// temporary and should be destroyed when it has finished playing.
	bool IsExternal()
	{
		return m_sSrcTypeInfo.dwID >> (4 + 12) == AKCODECID_EXTERNAL_SOURCE;
	}
	bool IsExternallySupplied()
	{
		return m_sSrcTypeInfo.mediaInfo.bExternallySupplied;
	}

    bool HasBankSource()	{ return m_sSrcTypeInfo.mediaInfo.bHasSource; }

	AKRESULT PrepareData();
	void UnPrepareData();

	AkUInt32	GetExternalSrcCookie() const {return (AkUInt32)(AkUIntPtr)m_sSrcTypeInfo.mediaInfo.uFileID;}

private:

	void SetDefaultParams();

	AkSrcTypeInfo				m_sSrcTypeInfo;			// Source info description.	
};

#endif //_AKSOURCE_H_
