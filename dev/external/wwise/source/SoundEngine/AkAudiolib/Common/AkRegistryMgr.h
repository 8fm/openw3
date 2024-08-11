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
// AkRegistryMgr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _REGISTRY_MGR_H_
#define _REGISTRY_MGR_H_

#include "AkHashList.h"
#include <AK/Tools/Common/AkLock.h>
#include "AkRegisteredObj.h"
#include <AK/SoundEngine/Common/AkQueryParameters.h>

class CAkParameterNodeBase;
class CAkPBI;
class AkSoundPositionRef;

inline bool CheckObjAndPlayingID(
								 const CAkRegisteredObj* in_pObjSearchedFor, 
								 const CAkRegisteredObj* in_pObjActual, 
								 AkPlayingID in_PlayingIDSearched, 
								 AkPlayingID in_PlayingIDActual)
{
	if( in_pObjSearchedFor == NULL || in_pObjSearchedFor == in_pObjActual )
	{
		if( in_PlayingIDSearched == AK_INVALID_PLAYING_ID || in_PlayingIDSearched == in_PlayingIDActual )
		{
			return true;
		}
	}
	return false;
}

//CAkRegistryMgr Class
//Unique container containing the registered objects info
class CAkRegistryMgr
{
	friend class CAkRegisteredObj;

public:
	CAkRegistryMgr();
	~CAkRegistryMgr();

	AKRESULT	Init();
	void		Term();

	// Register a game object
	CAkRegisteredObj* RegisterObject(
		AkGameObjectID in_GameObjectID,	//GameObject (Ptr) to register
		AkUInt32 uListenerMask = 0xFF,
		void * in_pMonitorData = NULL
		);

	// Unregister the specified game object
	void UnregisterObject(
		AkGameObjectID in_GameObjectID	//Game Object to unregister
		);

#ifndef AK_OPTIMIZED
	// Set position relative to a listener. 
	// Currently, position of game object 0 (Transport) relative to listener 0 (default) is assumed,
	// hence the absence of corresponding input arguments. 
	void SetRelativePosition( 
		const AkSoundPosition & in_rPosition
		);
#endif

	// Set the current position of a game object
	AKRESULT SetPosition( 
		AkGameObjectID in_GameObjectID, 
		const AkSoundPosition* in_aPositions,
		AkUInt16 in_uNumPositions,
		AK::SoundEngine::MultiPositionType in_eMultiPositionType
		);

	AKRESULT SetActiveListeners(
		AkGameObjectID in_GameObjectID,	///< Game object.
		AkUInt32 in_uListenerMask		///< Bitmask representing active listeners. LSB = Listener 0, set to 1 means active.
		);

#if defined AK_WII_FAMILY
	AKRESULT SetActiveControllers(
		AkGameObjectID in_GameObjectID,			///< Game object.
		AkUInt32 in_uActiveControllerMask		///< Bitmask representing active controllers. LSB = Controller 0, set to 1 means active.
		);
#endif

	AKRESULT GetPosition( 
		AkGameObjectID in_GameObjectID, 
		AkSoundPositionRef & out_Position
		);

	AKRESULT SetGameObjectAuxSendValues( 
		AkGameObjectID		in_GameObjectID,
		AkAuxSendValue*	in_aEnvironmentValues,
		AkUInt32			in_uNumEnvValues
		);

	AKRESULT SetGameObjectOutputBusVolume( 
		AkGameObjectID		in_GameObj,
		AkReal32			in_fControlValue
		);

	AKRESULT SetGameObjectScalingFactor(
		AkGameObjectID		in_GameObj,
		AkReal32			in_fControlValue
		);


	AKRESULT SetObjectObstructionAndOcclusion(  
		AkGameObjectID in_GameObjectID,     ///< Game object ID.
		AkUInt32 in_uListener,             ///< Listener ID.
		AkReal32 in_fObstructionLevel,		///< ObstructionLevel : [0.0f..1.0f]
		AkReal32 in_fOcclusionLevel			///< OcclusionLevel   : [0.0f..1.0f]
		);

	AkSwitchHistItem GetSwitchHistItem( 
		CAkRegisteredObj *	in_pGameObj,
		AkUniqueID          in_SwitchContID
		);

	AKRESULT SetSwitchHistItem( 
		CAkRegisteredObj *	in_pGameObj,
		AkUniqueID          in_SwitchContID,
		const AkSwitchHistItem &  in_SwitchHistItem
		);

	AKRESULT ClearSwitchHist( 
		AkUniqueID          in_SwitchContID,
		CAkRegisteredObj *	in_pGameObj = NULL
		);

    AKSOUNDENGINE_API CAkRegisteredObj * GetObjAndAddref( AkGameObjectID in_GameObjectID );

	CAkRegisteredObj* ActivatePBI(
		CAkPBI * in_pPBI,			// PBI to set as active
		AkGameObjectID in_GameObjectID	//Game object associated to the PBI
		);

	// Signify to the registry that the specified AudioNode is containing specific information
	void SetNodeIDAsModified(
		CAkParameterNodeBase* in_pNode		//Audionode Modified
		);

	void NotifyListenerPosChanged(
		AkUInt32 in_uListenerMask	// Bitmask of listeners whose position changed.
		);

	// Unregister all the objects registered in the registry.
	// It also removes all the specific memory allocated for specific parameters.
	void UnregisterAll();

	void PostEnvironmentStats();
	void PostObsOccStats();
	void PostListenerStats();
#ifdef AK_MOTION
	void PostfeedbackGameObjStats();
#endif // AK_MOTION
#if defined AK_WII_FAMILY
	void PostControllerStats();
#endif

	//inline, for profiling purpose only, no lock required since this information is not time critical
	AkUInt32 NumRegisteredObject(){ return m_mapRegisteredObj.Length(); }

	AKRESULT GetActiveGameObjects( 
		AK::SoundEngine::Query::AkGameObjectsList& io_GameObjectList	// returned list of active game objects.
		);

	bool IsGameObjectActive( 
		AkGameObjectID in_GameObjectId
		);

	typedef AkHashList< AkGameObjectID, CAkRegisteredObj*, AK_LARGE_HASH_SIZE > AkMapRegisteredObj;

	const AkListNode*		GetModifiedElementList(){ return &m_listModifiedNodes; }
	AkMapRegisteredObj&		GetRegisteredObjectList(){ return m_mapRegisteredObj; }
	
private:
	AkListNode m_listModifiedNodes;
	
	AkMapRegisteredObj m_mapRegisteredObj; //map of all actually registered objects

#ifndef AK_OPTIMIZED
public:
	AkSoundPosition m_relativePositionTransport;
#endif
};

extern AKSOUNDENGINE_API CAkRegistryMgr* g_pRegistryMgr;

#endif
