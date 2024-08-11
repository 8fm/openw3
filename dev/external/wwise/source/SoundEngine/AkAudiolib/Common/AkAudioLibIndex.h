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
// AkAudioLibIndex.h
//
// Class containing the maps allowing to make the link between 
// string to IDs and between IDs to pointers.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIOLIB_INDEX_H_
#define _AUDIOLIB_INDEX_H_

#include "AkHashList.h"
#include "AkKeyArray.h"
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkIndexable.h"
#include "AkAttenuationMgr.h"

class CAkParameterNodeBase;
class CAkState;
class CAkEvent;
class CAkFxCustom;
class CAkFxShareSet;
class CAkAction;
class CAkLayer;
class CAkAttenuation;
class CAkDynamicSequence;
class CAkDialogueEvent;

template <class U_PTR> class CAkIndexItem
{
	friend class CAkStateMgr;
	friend class CAkAudioLibIndex;
	friend class CAkActionActive;
	friend class CAkBankMgr;
	friend class AkMonitor;
    friend class CAkAudioMgr;

public:

	//Set an ID corresponding to a pointer
	void SetIDToPtr( U_PTR in_Ptr )
	{
		AkAutoLock<CAkLock> IndexLock( m_IndexLock );
		AKASSERT( in_Ptr );
		m_mapIDToPtr.Set( in_Ptr );
	}

	//Remove an ID from the index
	void RemoveID( AkUniqueID in_ID )
	{
		AkAutoLock<CAkLock> IndexLock( m_IndexLock );
		m_mapIDToPtr.Unset( in_ID );
	}

	U_PTR GetPtrAndAddRef( AkUniqueID in_ID ) 
    { 
		AkAutoLock<CAkLock> IndexLock( m_IndexLock ); 
		CAkIndexable * pIndexable = m_mapIDToPtr.Exists( in_ID ); 
		if( pIndexable ) 
		{ 
			pIndexable->AddRefUnsafe(); 
			return static_cast<U_PTR>( pIndexable );
		} 
		else 
		{ 
			return NULL; 
		} 
    } 


	void Init()
	{
		m_mapIDToPtr.Init();
	}

	void Term()
	{
		//If this assert pops, that mean that a Main ref-counted element of the audiolib was not properly released
		AKASSERT( m_mapIDToPtr.Length() == 0 );
		m_mapIDToPtr.Term();
	}

	CAkLock& GetLock() { return m_IndexLock; }

	typedef AkHashListBare<AkUniqueID, CAkIndexable, AK_LARGE_HASH_SIZE> AkMapIDToPtr;

//private:

	CAkLock			m_IndexLock;
	AkMapIDToPtr	m_mapIDToPtr;
};

class CAkIndexSiblingItem
{

public:

	void Init() {}
	void Term();

	//Set an ID corresponding to a pointer
	AKRESULT SetIDToPtr( AkStateGroupID in_StateGroupID, CAkState* in_Ptr );

	//Remove an ID from the index
	void RemoveID( AkStateGroupID in_StateGroupID, AkUniqueID in_ID );

	CAkState* GetPtrAndAddRef( AkStateGroupID in_StateGroupID, AkUniqueID in_ID );

	CAkLock& GetLock() { return m_IndexLock; }

private:
	
	CAkIndexItem<CAkState*>* GetStateGroup( AkStateGroupID in_StateGroupID );
	CAkIndexItem<CAkState*>* GetOrCreateStateGroup( AkStateGroupID in_StateGroupID );

	typedef CAkKeyArray< AkStateGroupID, CAkIndexItem<CAkState*>*, 4 > AkMapSibling;

	CAkLock			m_IndexLock;
	AkMapSibling	m_ArrayStateGroups;
};

// Class containing the maps allowing to make the link between 
// string to IDs and between IDs to pointers.
//
// Author:  alessard
class  CAkAudioLibIndex
{
	friend class CAkStateMgr;
	friend class CAkURenderer;

public:
	void Init();
	void Term();

	void ReleaseTempObjects();
	void ReleaseDynamicSequences();

#ifndef AK_OPTIMIZED
	AKRESULT ResetRndSeqCntrPlaylists();
	void ClearMonitoringSoloMute();
#endif

private:
	CAkIndexItem<CAkParameterNodeBase*> m_idxAudioNode;	// AudioNodes index
	CAkIndexItem<CAkParameterNodeBase*> m_idxBusses;	// AudioNodes index

public:

	CAkParameterNodeBase* GetNodePtrAndAddRef( AkUniqueID in_ID, AkNodeType in_NodeType );
	CAkParameterNodeBase* GetNodePtrAndAddRef( WwiseObjectIDext& in_rIDext );
	CAkLock& GetNodeLock( AkNodeType in_NodeType );
	CAkIndexItem<CAkParameterNodeBase*>& GetNodeIndex( AkNodeType in_NodeType );

	CAkIndexItem<CAkState*> m_idxCustomStates;	// Custom States index

	CAkIndexItem<CAkEvent*> m_idxEvents;		// Events index
	CAkIndexItem<CAkAction*> m_idxActions;		// Actions index

	CAkIndexItem<CAkLayer*> m_idxLayers;		// Layers index

	CAkIndexItem<CAkAttenuation*> m_idxAttenuations;// Actions index

	CAkIndexItem<CAkDynamicSequence*> m_idxDynamicSequences; // Dynamic Sequence index
	CAkIndexItem<CAkDialogueEvent*> m_idxDialogueEvents; // Dynamic Sequence index

	CAkIndexItem<CAkFxShareSet*> m_idxFxShareSets; // Fx ShareSets
	CAkIndexItem<CAkFxCustom*> m_idxFxCustom; // Fx Custom
};

extern CAkAudioLibIndex* g_pIndex;

#endif
