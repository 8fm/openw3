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
// AkRegisteredObj.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _REGISTERED_OBJ_H_
#define _REGISTERED_OBJ_H_

#include "AkKeyArray.h"
#include "AkList2.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include "AudiolibDefs.h"
#include "AkPositionKeeper.h"

#define MIN_SIZE_REG_ENTRIES 8  //ListPolldID Size / 4bytes
#define MAX_SIZE_REG_ENTRIES AK_NO_MAX_LIST_SIZE

class CAkParameterNodeBase;

struct AkPackedZeroToOneValue
{
	AkReal32 Get()
	{
		// Load hit store implied.
		return ( m_ByteValue / 255.0f );
	}

	void Set( AkReal32 in_FloatValue )
	{
		AKASSERT( in_FloatValue <= 1 );
		m_ByteValue = (AkUInt8) (in_FloatValue * 255.0f);
	}

private:
	AkUInt8 m_ByteValue;
};

struct AkSwitchHistItem
{
	AkForceInline void IncrementPlayback( AkUInt32 in_Switch )
	{
		if( LastSwitch == in_Switch )
		{
			// If the switch is the same than last time, simply increment it
			++NumPlayBack;
		}
		else
		{
			LastSwitch = in_Switch;
			NumPlayBack = 1;
		}
	}

	AkUInt32 LastSwitch;
	AkUInt32 NumPlayBack;
};

typedef CAkKeyArray<AkUniqueID, AkSwitchHistItem> AkSwitchHist; // Switch Container to SwitchHist
typedef CAkList2<WwiseObjectIDext, const WwiseObjectIDext&, AkAllocAndFree> AkListNode;

//Class representing a registered Object
class CAkRegisteredObj
{
public:
	CAkRegisteredObj( AkGameObjectID in_GameObjID );
	AKSOUNDENGINE_API ~CAkRegisteredObj();

	AkForceInline void AddRef()
	{
		++m_refCount;
	}

	AkForceInline void Release()
	{
		AKASSERT( m_refCount > 0 );
		if ( --m_refCount <= 0 )
			AkDelete( g_DefaultPoolId, this );
	}

	AkForceInline bool IsRegistered() const { return m_bRegistered; }
	AkForceInline void Unregister() { m_bRegistered = false; }

	// This function is designed to be called from the querry only, 
	// and would return invalid value if called fro the audio thread.
	AkForceInline bool IsActive()
	{
		// If the object is referenced, it means that it is under process
		// When m_refCount == 1, the object is valid, but not active ( sleeping )
		return m_refCount > 1;
	}

	AkForceInline AkGameObjectID ID() { return m_GameObjID; }

	bool CreateModifiedNodeList();
	void DestroyModifiedNodeList();

	// Set the specified Node as modified (this node has an SIS that will require to be deleted)
	AKRESULT SetNodeAsModified(
		CAkParameterNodeBase* in_pNode// Node modified
		);

	const AkListNode* GetModifiedElementList(){ return m_pListModifiedNodes; }

	void SetPosition( 
		const AkSoundPosition* in_aPositions,
		AkUInt16 in_uNumPositions,
		AK::SoundEngine::MultiPositionType in_eMultiPositionType
		);

	void SetActiveListeners(
		AkUInt32 in_uListenerMask	///< Bitmask representing active listeners. LSB = Listener 0, set to 1 means active.
		);

#if defined AK_WII_FAMILY
	void SetActiveControllers(
		AkUInt32 in_uActiveControllerMask	///< Bitmask representing active listeners. LSB = Controller 0, set to 1 means active.
		);
#endif

	const AkSoundPositionRef & GetPosition() const
	{
		return m_PosKeep;
	}

	AkUInt32 GetListenerMask() const
	{
		return m_PosKeep.GetListenerMask();
	}

	AKRESULT SetGameObjectAuxSendValues( 
		AkAuxSendValue*	in_aEnvironmentValues,
		AkUInt32			in_uNumEnvValues
		);

	void SetDryLevelValue( AkReal32 in_fControlValue )
	{
		m_fDryLevelValue = in_fControlValue;
	}

	void SetScalingFactor( AkReal32 in_fScalingFactor )
	{
		m_fScalingFactor = in_fScalingFactor;
		m_bPositionDirty = true;
	}

	inline const AkAuxSendValue* GetGameDefinedAuxSends() 
	{ 
		return m_EnvironmentValues; 
	}

	inline AkReal32 GetDryLevelValue() 
	{ 
		return m_fDryLevelValue; 
	}

	inline AkReal32 GetScalingFactor()
	{
		return m_fScalingFactor;
	}

	inline bool IsPositionCached() { return !m_bPositionDirty; }

	// Get number of emitter-listener pairs for this game object.
	AkUInt32 GetNumEmitterListenerPairs() const;

	// Get cached emitter listener pairs. Use them only if
	// this game object's position is not dirty
	inline const AkEmitterListenerPairArray & GetCachedEmitListenPairs() { return m_arCachedEmitListPairs; }

	// Push emitter-listener pairs computed by a PBI for this game object.
	void CacheEmitListenPairs( const AkVolumeDataArray & in_arEmitList );

	// Cache a single emitter-listener pair.
	// Note: Caching will only work if there is one and only one emitter-listener pair in this game object;
	// partial caching (of subset of pairs or mismatching features) is not supported.
	void CacheEmitListenPair( const AkEmitterListenerPairEx & in_emitListPair );

	inline void NotifyListenerPosDirty( 
		AkUInt32 in_uListenerChangedMask	// Bitmask of listeners that changed.
		)
	{
		// if position is not already marked as dirty.
		m_bPositionDirty = m_bPositionDirty || ( ( in_uListenerChangedMask & m_PosKeep.GetListenerMask() ) > 0 );
	}

	AKRESULT SetObjectObstructionAndOcclusion( 
		AkUInt32			in_uListener,
		AkReal32			in_fObstructionValue,
		AkReal32			in_fOcclusionValue
		);

	AkReal32 GetObjectObstructionValue( 
		AkUInt32			in_uListener
		)
	{
		return m_fObstructionValue[ in_uListener ].Get();
	}

	AkReal32 GetObjectOcclusionValue( 
		AkUInt32			in_uListener
		)
	{
		return m_fOcclusionValue[ in_uListener ].Get();
	}

	AkSwitchHist & GetSwitchHist() { return m_SwitchHist; }

#if defined AK_WII_FAMILY
	void IncrementGameObjectPlayCount();
	void DecrementGameObjectPlayCount();
#endif

private:

	// Function called once all the PBIs are deactivated and the object is unregistered
	// this function frees the SIS allocated and destroy this*
	void FreeModifiedNodes();

	AkListNode* m_pListModifiedNodes;

	AkPositionKeeper	m_PosKeep;
	AkAuxSendValue	m_EnvironmentValues[AK_MAX_AUX_PER_OBJ];

	AkSwitchHist m_SwitchHist;

	AkPackedZeroToOneValue	m_fObstructionValue[AK_NUM_LISTENERS];
	AkPackedZeroToOneValue	m_fOcclusionValue[AK_NUM_LISTENERS];
	AkReal32				m_fDryLevelValue; 
	AkReal32				m_fScalingFactor; // 0 to infinite value, not to be stored on AkPackedZeroToOneValue 0 to 1 type.

	AkEmitterListenerPairArray m_arCachedEmitListPairs;

	AkGameObjectID			m_GameObjID;
	AkUInt32				m_refCount			:30;
	AkUInt32				m_bPositionDirty	:1;
	AkUInt32				m_bRegistered		:1; // Becomes false when game object is unregistered (removed from map).

#if defined AK_WII_FAMILY
	AkUInt32			m_PlayCount;
#endif
};
#endif
