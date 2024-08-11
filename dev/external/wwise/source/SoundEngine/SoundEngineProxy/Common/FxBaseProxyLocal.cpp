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

#include "stdafx.h"
#ifndef AK_OPTIMIZED
#ifndef PROXYCENTRAL_CONNECTED

#include "FxBaseProxyLocal.h"

#include "AkAudioLib.h"
#include "AkCritical.h"
#include "AkFxBase.h"
#include "AkURenderer.h"

FxBaseProxyLocal::FxBaseProxyLocal( AkUniqueID in_id, bool in_bShareSet )
{
	CAkIndexable* pIndexable;

	if ( in_bShareSet )
	{
		pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_FxShareSet );
		if ( !pIndexable )
			pIndexable = CAkFxShareSet::Create( in_id );
	}
	else
	{
		pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_FxCustom );
		if ( !pIndexable )
			pIndexable = CAkFxCustom::Create( in_id );
	}

	SetIndexable( pIndexable );
}

FxBaseProxyLocal::~FxBaseProxyLocal()
{
#ifndef PROXYCENTRAL_CONNECTED
	ReleaseAllPluginMedia();
#endif
}

void FxBaseProxyLocal::SetFX( 
	AkPluginID in_FXID )
{
	CAkFxBase* pIndexable = static_cast<CAkFxBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->SetFX( in_FXID, NULL, 0 );
	}

}

void FxBaseProxyLocal::SetFXParam( 
	AkFXParamBlob in_blobParams,
	AkPluginParamID in_uParamID )
{
	CAkFxBase* pIndexable = static_cast<CAkFxBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->SetFXParam( in_uParamID, in_blobParams.pBlob, in_blobParams.uBlobSize );
	}
}

void FxBaseProxyLocal::SetRTPC( 
	AkRtpcID			in_RTPC_ID,
	AkRTPC_ParameterID	in_ParamID,
	AkUniqueID			in_RTPCCurveID,
	AkCurveScaling		in_eScaling,
	AkRTPCGraphPoint* in_pArrayConversion, 
	AkUInt32 in_ulConversionArraySize
	)
{
	CAkFxBase* pIndexable = static_cast<CAkFxBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->SetRTPC( in_RTPC_ID, in_ParamID, in_RTPCCurveID, in_eScaling, in_pArrayConversion, in_ulConversionArraySize );
	}
}

void FxBaseProxyLocal::UnsetRTPC( 
	AkRTPC_ParameterID in_ParamID,
	AkUniqueID in_RTPCCurveID
	)
{
	CAkFxBase* pIndexable = static_cast<CAkFxBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->UnsetRTPC( in_ParamID, in_RTPCCurveID );
	}
}

void FxBaseProxyLocal::SetMediaID( 
		AkUInt32 in_uIdx, 
		AkUniqueID in_mediaID )
{
	CAkFxBase* pIndexable = static_cast<CAkFxBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->SetMediaID( in_uIdx, in_mediaID );
	}
}

void FxBaseProxyLocal::LoadPluginMedia( AkUInt32 in_uIdx, const AkOSChar* in_pszSourceFile, AkUniqueID in_mediaID, unsigned long in_hash )
{
#ifndef PROXYCENTRAL_CONNECTED
	// grow array accordingly. 
	while ( in_uIdx >= m_RefedPluginMedia.size() )
		m_RefedPluginMedia.push_back( AK_INVALID_UNIQUE_ID );

	AkUniqueID & ref = m_RefedPluginMedia[ in_uIdx ];
	if ( ref == in_mediaID )
	{
		ResetData( in_pszSourceFile, in_mediaID, in_hash );
	}
	else
	{
		if ( ref != AK_INVALID_UNIQUE_ID )
			DecRefOrReleaseData( ref );

		AddRefOrLoadData( in_pszSourceFile, in_mediaID, in_hash );
		ref = in_mediaID;
	}
#endif
}

void FxBaseProxyLocal::ReleaseAllPluginMedia()
{
#ifndef PROXYCENTRAL_CONNECTED
	RefPluginVector::iterator iter = m_RefedPluginMedia.begin();
	while( iter != m_RefedPluginMedia.end() )
	{
		if ( *iter != AK_INVALID_UNIQUE_ID )
			DecRefOrReleaseData( *iter );
		++iter;
	}
	m_RefedPluginMedia.clear();
#endif
}

#ifndef PROXYCENTRAL_CONNECTED
//Declare static variable
FxBaseProxyLocal::RefGlobalPluginSet FxBaseProxyLocal::m_globalRefedPluginMedia;

void FxBaseProxyLocal::AddRefOrLoadData( const AkOSChar* in_pszSourceFile, AkUniqueID in_mediaID, unsigned long in_hash )
{
	RefGlobalPluginSet::iterator iter = m_globalRefedPluginMedia.find( in_mediaID );
	if( iter != m_globalRefedPluginMedia.end() )
	{
		// Increment the refcount and then make sure the media is up to date.
		++( iter->second.refcount );

		if( in_hash != iter->second.hash )
		{
			ResetData( in_pszSourceFile, in_mediaID, in_hash );
		}
	}
	else
	{
		DataInfo info;
		info.hash = in_hash;
		info.refcount = 1;

		AK::SoundEngine::LoadMediaFileSync( in_mediaID, in_pszSourceFile, AK::SoundEngine::LoadMediaFile_Load );
		m_globalRefedPluginMedia[in_mediaID] = info;
	}

}

void FxBaseProxyLocal::ResetData( const AkOSChar* in_pszSourceFile, AkUniqueID in_mediaID, unsigned long in_hash )
{
	RefGlobalPluginSet::iterator iter = m_globalRefedPluginMedia.find( in_mediaID );
	AKASSERT( iter != m_globalRefedPluginMedia.end() );
	if( iter != m_globalRefedPluginMedia.end() )
	{
		if( iter->second.hash == in_hash )
			return; // We are already up to date, simply return.
	
		AKRESULT eResult = AK::SoundEngine::LoadMediaFileSync( in_mediaID, in_pszSourceFile, AK::SoundEngine::LoadMediaFile_Swap );
	
		if( eResult == AK_Success )
		{
			// Set the newly updated hash if load was successful, otherwise, keeping the hash dirty will 
			// make the system to retry later.
			iter->second.hash = in_hash;
		}
	}
}

void FxBaseProxyLocal::DecRefOrReleaseData( AkUniqueID in_mediaID )
{
	RefGlobalPluginSet::iterator iter = m_globalRefedPluginMedia.find( in_mediaID );
	if( iter != m_globalRefedPluginMedia.end() )
	{
		AKASSERT( iter->second.refcount > 0 );
		if( iter->second.refcount == 1 )
		{
			AK::SoundEngine::LoadMediaFileSync( in_mediaID, NULL, AK::SoundEngine::LoadMediaFile_Unload );
			m_globalRefedPluginMedia.erase( iter );
		}
		else
		{
			// Decrement refcount
			--( iter->second.refcount );
		}
	}
}

#endif
#endif
#endif // #ifndef AK_OPTIMIZED
