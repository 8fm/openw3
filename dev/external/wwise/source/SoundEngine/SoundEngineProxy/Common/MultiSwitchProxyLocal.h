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

#pragma once

#ifndef AK_OPTIMIZED
#ifndef PROXYCENTRAL_CONNECTED

#include "AkAudioLib.h"
#include "ObjectProxyLocal.h"
#include "IMultiSwitchProxy.h"
#include "AkDialogueEvent.h"

template< typename tBase, typename tSoundEngineObj, AkUInt32 eAkIdxType>
class MultiSwitchProxyLocal : public tBase, public IMultiSwitchProxy
{
public:
	MultiSwitchProxyLocal( AkUniqueID in_id )
	{
		CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, (AkIndexType)eAkIdxType );
		SetIndexable( pIndexable != NULL ? pIndexable : tSoundEngineObj::Create( in_id ) );
	}

	virtual ~MultiSwitchProxyLocal(){};

	virtual void SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax )
	{
		tSoundEngineObj* pIndexable = static_cast<tSoundEngineObj*>( GetIndexable() );
		if( pIndexable )
		{
			CAkFunctionCritical SpaceSetAsCritical;
			pIndexable->SetAkProp( in_eProp, in_fValue, in_fMin, in_fMax );
		}
	}

	virtual void SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax )
	{
		tSoundEngineObj* pIndexable = static_cast<tSoundEngineObj*>( GetIndexable() );
		if( pIndexable )
		{
			CAkFunctionCritical SpaceSetAsCritical;
			pIndexable->SetAkProp( in_eProp, in_iValue, in_iMin, in_iMax );
		}
	}

	virtual void SetDecisionTree( void* in_pData, AkUInt32 in_uSize, AkUInt32 in_uDepth )
	{
		tSoundEngineObj* pIndexable = static_cast<tSoundEngineObj*>( GetIndexable() );
		if( pIndexable )
		{
			CAkFunctionCritical SpaceSetAsCritical;
			pIndexable->SetDecisionTree( in_pData, in_uSize, in_uDepth );
		}
	}

	virtual void SetArguments( AkUInt32* in_pArgs, AkUInt8* in_pGroupTypes, AkUInt32 in_uNumArgs )
	{
		tSoundEngineObj* pIndexable = static_cast<tSoundEngineObj*>( GetIndexable() );
		if( pIndexable )
		{
			CAkFunctionCritical SpaceSetAsCritical;
			pIndexable->SetArguments( in_pArgs, in_pGroupTypes, in_uNumArgs );
		}
	}
};

#endif // #ifndef PROXYCENTRAL_CONNECTED
#endif // #ifndef AK_OPTIMIZED
