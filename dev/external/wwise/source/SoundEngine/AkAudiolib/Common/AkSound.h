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
// AkSound.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _SOUND_H_
#define _SOUND_H_

#include "AkMonitorData.h"
#include "AkSoundBase.h"
#include "AkParameters.h"
#include "AkBankMgr.h"
#include "AkCommon.h"
#include "AkSource.h"

// class corresponding to a Sound
//
// Author:  alessard
class CAkSound : public CAkSoundBase
{
public:
	//Thread safe version of the constructor
	static CAkSound* Create(AkUniqueID in_ulID = 0);

	virtual AkNodeCategory NodeCategory();

	bool IsZeroLatency() const 
{
		return m_Source.IsZeroLatency();
	}

	void IsZeroLatency( bool in_bIsZeroLatency )
	{
		m_Source.IsZeroLatency( in_bIsZeroLatency ); 
	}

	// Call a play on the definition directly
    //
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT PlayInternal( AkPBIParams& in_rPBIParams );

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction );
	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction );

	void SeekSound( CAkRegisteredObj * in_pGameObj, const SeekActionParams & in_rActionParams );
	void SeekSound( CAkRegisteredObj * in_pGameObj, const SeekActionParamsExcept & in_rActionParams );// NOTE: At this point the exception list is useless.

	void SetSource( AkUniqueID in_sourceID, AkUInt32 in_PluginID, const AkOSChar* in_pszFilePath, AkFileID in_uCacheID )
	{
		return m_Source.SetSource( in_sourceID, in_PluginID, in_pszFilePath, in_uCacheID,
				false,		// Setting source via sound: cannot be from RSX.
				false );	// Setting source via sound: Not externally supplied.
	}

	void SetSource( AkUInt32 in_PluginID, const AkMediaInformation& in_rMediaInfo )
	{
		return m_Source.SetSource( in_PluginID, in_rMediaInfo );
	}

	void SetSource( AkUniqueID in_sourceID )
	{
		return m_Source.SetSource( in_sourceID );
	}

	void FreeCurrentSource()
	{
		m_Source.FreeSource();
	}

	virtual void RemoveChild(
        CAkParameterNodeBase* /*in_pChild*/
		) {AKASSERT(!"Cannot Remove a child on a sound");}

	using CAkParameterNodeBase::RemoveChild; //This will remove warnings on 3DS, WiiU and Vita compilers
	
	// Get/Set source info and format structs.
    AkSrcTypeInfo * GetSrcTypeInfo()
	{     
		return m_Source.GetSrcTypeInfo();
	}	

	virtual AkObjectCategory Category();

	AKRESULT SetInitialValues( AkUInt8* pData, AkUInt32 ulDataSize, CAkUsageSlot* in_pUsageSlot, bool in_bIsPartialLoadOnly );

	bool HasBankSource()	{ return m_Source.HasBankSource(); }

	virtual AKRESULT PrepareData();
	virtual void UnPrepareData();

	virtual bool IsPlayable(){ return m_Source.GetSrcTypeInfo()->mediaInfo.sourceID != AK_INVALID_UNIQUE_ID; }
	bool SourceLoaded(){ return m_Source.GetSrcTypeInfo()->mediaInfo.Type != SrcTypeNone; }

protected:
	// Constructors
    CAkSound( AkUniqueID in_ulID );

	//Destructor
    virtual ~CAkSound();

	AKRESULT Init(){ return CAkParameterNode::Init(); }

// members
private:
	CAkSource					m_Source;
};
#endif
