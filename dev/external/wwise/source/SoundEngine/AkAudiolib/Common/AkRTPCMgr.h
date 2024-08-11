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
// AkRTPCMgr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _RTPC_MGR_H_
#define _RTPC_MGR_H_

#include "AkCommon.h"
#include "AkRTPC.h"
#include "AkHashList.h"
#include <AK/SoundEngine/Common/IAkRTPCSubscriber.h>
#include "AkConversionTable.h"
#include "AudiolibDefs.h"
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkArray.h>
#include "AkList2.h"
#include "AkKeyArray.h"
#include "AkPoolSizes.h"
#include "AkMultiKeyList.h"
#include "ITransitionable.h"
#include "PrivateStructures.h"
#include <AK/Tools/Common/AkListBareLight.h>

class CAkRegisteredObj;
class CAkSwitchAware;

typedef AkArray<CAkRegisteredObj *, CAkRegisteredObj *, ArrayPoolDefault, (DEFAULT_POOL_BLOCK_SIZE / sizeof(CAkRegisteredObj *))> GameObjExceptArray;

//Struct definitions

struct AkRTPCSubscriptionKey
{
	void*				pSubscriber;	// Cast to appropriate interface/class depending on eType
	AkRTPC_ParameterID	ParamID;

	bool operator==( const AkRTPCSubscriptionKey & in_other ) const
	{
		return pSubscriber == in_other.pSubscriber && 
			   ParamID == in_other.ParamID;
	}
};
	
struct AkSwitchKey
{
	AkSwitchGroupID m_SwitchGroup;
	CAkRegisteredObj * m_pGameObj;
	bool operator ==(AkSwitchKey& in_Op)
	{
		return ( (m_SwitchGroup == in_Op.m_SwitchGroup) && (m_pGameObj == in_Op.m_pGameObj) );
	}
};

inline AkUIntPtr AkHash( AkRTPCSubscriptionKey in_key ) { return (AkUIntPtr) in_key.pSubscriber + (AkUIntPtr) in_key.ParamID; }
inline AkUIntPtr AkHash( AkSwitchKey in_key ) { return (AkUIntPtr) in_key.m_SwitchGroup + (AkUIntPtr) in_key.m_pGameObj; }

class CAkLayer;

// CAkRTPCMgr Class
// Unique per instance of Audiolib, the RTPC manager owns the real 
// time parameters and notified those who wants to be
class CAkRTPCMgr
{	
public:
	enum SubscriberType
	{
		SubscriberType_IAkRTPCSubscriber	= 0,
		SubscriberType_CAkBus				= 1,
		SubscriberType_CAkParameterNodeBase	= 2,
		SubscriberType_CAkLayer				= 3,
		SubscriberType_PBI					= 4
	};

friend class AkMonitor;

private:

	// A single RTPC Curve
	struct RTPCCurve
	{
		AkUniqueID RTPCCurveID;
		AkRtpcID RTPC_ID;
		CAkConversionTable<AkRTPCGraphPoint, AkReal32> ConversionTable;

		bool operator == ( const RTPCCurve& in_other ) const
		{
			return in_other.RTPCCurveID == RTPCCurveID;
		}
	};

	// Multiple RTPC Curves
	typedef AkArray<RTPCCurve, const RTPCCurve&, ArrayPoolDefault> RTPCCurveArray;

	// Each subscription, for a given Subscriber/Property/Game Object, now has
	// a list of Curves
	struct AkRTPCSubscription
	{
		// For AkMapRTPCSubscribers
		AkRTPCSubscriptionKey key; 
		AkRTPCSubscription * pNextItem;

		CAkRegisteredObj *	TargetGameObject;
		SubscriberType		eType;
		RTPCCurveArray		Curves;

		~AkRTPCSubscription()
		{
			AKASSERT( Curves.IsEmpty() );
			Curves.Term();
		}

	private:
		bool operator == ( const AkRTPCSubscription& in_other );
	};

	struct AkSwitchSubscription
	{
		CAkSwitchAware*		pSwitch;
		AkSwitchGroupID		switchGroup;
	};

	typedef CAkList2<CAkSwitchAware*, CAkSwitchAware*, AkAllocAndKeep> AkListRTPCSwitchSubscribers;

	struct AkRTPCSwitchAssociation
	{
		AkSwitchGroupID											switchGroup;
		AkRtpcID												RTPC_ID;
		CAkConversionTable<AkRTPCGraphPointInteger, AkUInt32>	ConversionTable;
		AkListRTPCSwitchSubscribers listRTPCSwitchSubscribers;
	};

	// Helpers
	AKRESULT UpdateRTPCSubscriberInfo( void* in_pSubscriber );

	void UpdateSubscription( AkRTPCSubscription& in_rSubscription );


public:
	CAkRTPCMgr();
	~CAkRTPCMgr();

	AKRESULT Init();

	AKRESULT Term();

	///////////////////////////////////////////////////////////////////////////
	// Main SET func
	///////////////////////////////////////////////////////////////////////////

	AKRESULT AddSwitchRTPC(
		AkSwitchGroupID				in_switchGroup,
		AkRtpcID					in_RTPC_ID,
		AkRTPCGraphPointInteger*	in_pArrayConversion,//NULL if none
		AkUInt32						in_ulConversionArraySize//0 if none
		);

	void RemoveSwitchRTPC(
		AkSwitchGroupID				in_switchGroup
		);

	AKRESULT AKSOUNDENGINE_API SetRTPCInternal( 
		AkRtpcID in_RTPCid, 
		AkReal32 in_Value, 
		CAkRegisteredObj * in_pGameObj,
		TransParams & in_transParams,
		AkValueMeaning in_eValueMeaning	= AkValueMeaning_Independent	// AkValueMeaning_Independent (absolute) or AkValueMeaning_Offset (relative)
		);

	AKRESULT SetSwitchInternal( 
		AkSwitchGroupID in_SwitchGroup,
		AkSwitchStateID in_SwitchState,
		CAkRegisteredObj * in_pGameObj = NULL 
		);	

///////////////////////////////////////////////////////////////////////////
	// Main GET func
///////////////////////////////////////////////////////////////////////////

	AkSwitchStateID GetSwitch( 
		AkSwitchGroupID in_SwitchGroup, 
		CAkRegisteredObj * in_pGameObj = NULL 
		);

	AkReal32 GetRTPCConvertedValue(
		void*					in_pSubscriber,
		AkUInt32				in_ParamID,
		CAkRegisteredObj *		in_GameObj
		);

	AkReal32 GetRTPCConvertedValue( // IMPORTANT: must already hold the RTPCLock when calling this method
		void *					in_pToken,
		CAkRegisteredObj *		in_GameObj,
		AkRtpcID				in_RTPCid
		);

	bool GetRTPCValue(
		AkRtpcID in_RTPC_ID,
		CAkRegisteredObj* in_GameObj,
		AkReal32& out_value,
		bool& out_bGameObjectSpecificValue
		);

///////////////////////////////////////////////////////////////////////////
	// Subscription Functions
///////////////////////////////////////////////////////////////////////////

	AKRESULT SubscribeRTPC(
		void*						in_pSubscriber,
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,		//# of the param that must be notified on change
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,//NULL if none
		AkUInt32					in_ulConversionArraySize,//0 if none
		CAkRegisteredObj *			in_TargetGameObject,
		SubscriberType				in_eType
		);

	AKRESULT SubscribeSwitch(
		CAkSwitchAware*		in_pSwitch,
		AkSwitchGroupID		in_switchGroup,
		bool				in_bForceNoRTPC = false
		);

	AKRESULT RegisterLayer( CAkLayer* in_pLayer, AkRtpcID in_rtpcID );
	void UnregisterLayer( CAkLayer* in_pLayer, AkRtpcID in_rtpcID );

	void UnSubscribeRTPC( void* in_pSubscriber, AkUInt32 in_ParamID, AkUniqueID in_RTPCCurveID, bool* out_bMoreCurvesRemaining = NULL );
	void UnSubscribeRTPC( void* in_pSubscriber, AkUInt32 in_ParamID );
	void UnSubscribeRTPC( void* in_pSubscriber );

	void UnSubscribeSwitch( CAkSwitchAware* in_pSwitch );

	void UnregisterGameObject( CAkRegisteredObj * in_GameObj );

//	void Reset( CAkRegisteredObj * in_GameObj = NULL );

	void ResetSwitches( CAkRegisteredObj * in_GameObj = NULL );

	void ResetRTPC( CAkRegisteredObj * in_GameObj = NULL );

	void SetDefaultParamValue(AkRtpcID in_RTPCid, AkReal32 in_fValue);
	void ResetRTPCValue(AkRtpcID in_idParam, CAkRegisteredObj * in_GameObj, TransParams & in_transParams);

	AkReal32 GetDefaultValue( AkRtpcID in_RTPCid, bool* out_pbWasFound = NULL );

private:
	struct AkRTPCValue
	{
		CAkRegisteredObj * pGameObj;
		AkReal32 fValue;

		bool operator==( const AkRTPCValue& in_other ) const
		{
			return in_other.pGameObj == pGameObj;
		}
	};

	/// Next item name policy.
	struct AkRTPCValueGetKey
	{
		/// Default policy.
		static AkForceInline CAkRegisteredObj *& Get( AkRTPCValue & in_item ) 
		{
			return in_item.pGameObj;
		}
	};

	struct AkSubsPtrGetKey
	{
		/// Default policy.
		static AkForceInline AkRTPCSubscription *& Get( AkRTPCSubscription *& in_item ) 
		{
			return in_item;
		}
	};

	typedef AkSortedKeyArray<CAkRegisteredObj *, AkRTPCValue, ArrayPoolDefault, AkRTPCValueGetKey, 8> AkRTPCValues;
	typedef AkSortedKeyArray<AkRTPCSubscription *, AkRTPCSubscription *, ArrayPoolDefault, AkSubsPtrGetKey, 2> AkRTPCSubscriptions;

	class AkRTPCEntry;
	class CAkRTPCTransition : public ITransitionable
	{
	public:
		CAkRTPCTransition( AkRTPCEntry * in_pOwner, CAkRegisteredObj * in_pGameObject );
		~CAkRTPCTransition();

		AKRESULT Start( 
			AkReal32 in_fStartValue,
			AkReal32 in_fTargetValue,
			TransParams & in_transParams,
			bool in_bRemoveEntryWhenDone
			);

		void Update( 
			AkReal32 in_fNewTargetValue,
			TransParams & in_transParams,
			bool in_bRemoveEntryWhenDone 
			);

		inline CAkRegisteredObj * GameObject() const { return m_pGameObject; }
		AkReal32 GetTargetValue();

		CAkRTPCTransition * pNextLightItem;	// List bare light sibling

	protected:
		virtual void TransUpdateValue(AkIntPtr in_eTargetType, AkReal32 in_unionValue, bool in_bIsTerminated);
		
	private:
		CAkTransition *		m_pTransition;
		AkRTPCEntry *		m_pOwner;
		CAkRegisteredObj *	m_pGameObject;
		bool				m_bRemoveEntryWhenDone;
	};

	typedef AkListBareLight<CAkRTPCTransition> AkRTPCTransitions;

	class AkRTPCEntry
	{
		friend class CAkRTPCTransition;
	public:
		AkRTPCEntry( AkRtpcID in_rtpcID )
			: key( in_rtpcID )
			, fDefaultValue( 0.f ) {}
		~AkRTPCEntry();

		AKRESULT SetRTPC(
			AkRTPCValue * in_pValueEntry, // Pass in the value entry of the game object map. MUST be == ValueExists( in_GameObj ). NULL if there is none.
			AkReal32 in_Value, 
			CAkRegisteredObj * in_GameObj,
			TransParams & in_transParams,
			bool in_bUnsetWhenDone		// Remove game object's specific RTPC Value once value is set.
			);

		// Remove RTPC value for specified game object.
		// Important: If in_GameObj is NULL, ALL values are removed.
		// IMPORTANT: Subscribers are not notified.
		void RemoveGameObject( CAkRegisteredObj * in_pGameObj );

		// Warning: This requires a linear search.
		inline AkRTPCValue * ValueExists( CAkRegisteredObj * in_pGameObj ) { return values.Exists( in_pGameObj ); }

		// Returns the default value if current value does not exist.
		// However, the global value is used instead if available.
		AkReal32 GetCurrentValue( AkRTPCValue * in_pValueEntry );

		// Returns the current target value if there is a transition, or the current value otherwise.
		AkReal32 GetCurrentTargetValue( AkRTPCValue * in_pValueEntry );

	private:
		// Helpers.
		//
		// Notifies the subscribers and manages the map of values.
		AKRESULT ApplyRTPCValue( 
			AkRTPCValue * in_pValueEntry, 
			AkReal32 in_NewValue, 
			CAkRegisteredObj * in_GameObj,
			bool in_bUnsetValue		// If true, the game object's specific RTPC entry is removed instead of being stored.
			);

		// Notifies all subscribers of this RTPC entry, and calls RTPCMgr::NotifyClientRTPCChange()
		// to notify all other global objects of this game parameter change.
		void NotifyRTPCChange( 
			AkRTPCValue * in_pValueEntry, 
			AkReal32 in_NewValue, 
			CAkRegisteredObj * in_GameObj );

		void GetRTPCExceptions( 
			GameObjExceptArray& out_ExceptArrayObj 
			);

		// Creates or modifies a game parameter transition. 
		// Returns false if creation fails, or if no transition is required whatsoever (transition is 
		// cleaned up inside). In such a case, the caller should apply the RTPC value immediately. 
		// Otherwise, RTPC updates are handled by the transition.
		bool CreateOrModifyTransition(
			CAkRegisteredObj * in_pGameObj, 
			AkReal32 in_fStartValue,
			AkReal32 in_fTargetValue,
			TransParams & in_transParams,
			bool in_bRemoveEntryWhenDone
			);

		// Returns valid iterator if found. Otherwise, iterator equals transitions.End(). 
		void FindTransition( CAkRegisteredObj * in_pGameObj, AkRTPCTransitions::IteratorEx & out_iter );

		// Destroys transition pointed by valid iterator. Returns the iterator to the next transition.
		inline AkRTPCTransitions::IteratorEx DestroyTransition( AkRTPCTransitions::IteratorEx & in_iter )
		{
			AKASSERT( in_iter != transitions.End() );
			CAkRTPCTransition * pTrRTPC = (*in_iter);
			transitions.Erase( in_iter );
			AkDelete( g_DefaultPoolId, pTrRTPC );
			return in_iter;
		}

	public:

		// For AkMapRTPCEntries
		AkRtpcID key;
		AkRTPCEntry * pNextItem;

		AkReal32 fDefaultValue;
	private:
		AkRTPCValues values;
		AkRTPCTransitions transitions;
	public:
		AkRTPCSubscriptions subscriptions; // WG-15880: list of subscriptions for NotifyRTPCChange performance
	};

	void NotifyClientRTPCChange(
		AkRtpcID in_idRTPC,
		AkRTPCValue * in_pValueEntry,
		AkReal32 in_NewValue, 
		CAkRegisteredObj * in_GameObj );

	AkRTPCEntry * GetRTPCEntry( AkRtpcID in_RTPCid );
	void RemoveReferencesToSubscription( AkRTPCSubscription * in_pSubscription );

	typedef AkHashListBare< AkRtpcID, AkRTPCEntry, AK_LARGE_HASH_SIZE > AkMapRTPCEntries;
	AkMapRTPCEntries m_RTPCEntries;

	typedef AkHashList<AkSwitchKey, AkSwitchStateID, AK_LARGE_HASH_SIZE> AkMapSwitchEntries;
	AkMapSwitchEntries m_SwitchEntries;

	typedef AkHashListBare< AkRTPCSubscriptionKey, AkRTPCSubscription, AK_LARGE_HASH_SIZE > AkMapRTPCSubscribers;
	AkMapRTPCSubscribers m_RTPCSubscribers;

	typedef CAkList2<AkSwitchSubscription, const AkSwitchSubscription&, AkAllocAndKeep> AkListSwitchSubscribers;
	AkListSwitchSubscribers m_listSwitchSubscribers;

	typedef CAkList2<AkRTPCSwitchAssociation, const AkRTPCSwitchAssociation&, AkAllocAndKeep> AkListRTPCSwitch;
	AkListRTPCSwitch m_listRTPCSwitch;
};

extern AKSOUNDENGINE_API CAkRTPCMgr* g_pRTPCMgr;

#endif //_RTPC_MGR_H_
