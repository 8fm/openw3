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
// ActivityChunk.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTIVITYCHUNK_H_
#define _ACTIVITYCHUNK_H_

#include "AkPBI.h"
#include <AK/Tools/Common/AkListBareLight.h>

typedef AkArray<CAkParameterNodeBase*, CAkParameterNodeBase*, ArrayPoolDefault> AkChildArray;

enum AKVoiceLimiterType
{
	AKVoiceLimiter_Global,
	AKVoiceLimiter_AM,
	AKVoiceLimiter_Bus
};

class CAkLimiter
{
public:
	CAkLimiter( AkUInt16 in_u16LimiterMax, bool in_bDoesKillNewest, bool in_bAllowUseVirtualBehavior )
		:m_u16LimiterMax(in_u16LimiterMax)
		,m_bDoesKillNewest( in_bDoesKillNewest )
		,m_bAllowUseVirtualBehavior( in_bAllowUseVirtualBehavior )
	{}

	void Term();

	typedef AkPriorityStruct AkLimiterKey;

	struct AkSortedPBIGetKey
	{
		/// Default policy.
		static AkForceInline AkLimiterKey& Get( CAkPBI*& in_item ) 
		{
			return in_item->GetPriorityKey();
		}
	};

	typedef AkSortedKeyArray<AkLimiterKey, CAkPBI*, ArrayPoolDefault, AkSortedPBIGetKey, 8 > AkSortedPBIPriorityList;

	AKRESULT Add( CAkPBI* in_pPBI, AKVoiceLimiterType in_Type );
	void Remove( CAkPBI* in_pPBI, AKVoiceLimiterType in_Type );
	void Update( AkReal32 in_NewPriority, CAkPBI* in_pPBI );

	void UpdateFlags();

	void UpdateMax( AkUInt16 in_u16LimiterMax )
	{
		m_u16LimiterMax = in_u16LimiterMax;
	}

	void SetUseVirtualBehavior( bool in_bAllowUseVirtualBehavior )
	{
		m_bAllowUseVirtualBehavior = in_bAllowUseVirtualBehavior;
	}

	AkUInt16 GetMaxInstances()	{ return 	m_u16LimiterMax; }
	bool 	DoesKillNewest()	{ return	m_bDoesKillNewest; }

	void SwapOrdering();

	inline const AkSortedPBIPriorityList& GetPBIList() { return m_sortedPBIList; }

#if defined(_DEBUG)
	bool LookForCtx( CAkPBI* in_pCtx );
#endif

private:
	AkSortedPBIPriorityList m_sortedPBIList;

	AkUInt16	m_u16LimiterMax;
	bool		m_bDoesKillNewest;
	bool		m_bAllowUseVirtualBehavior;

public:
	CAkLimiter *pNextLightItem;	//For AkListBareLight
};

class StructMaxInst
{
public:
	//Constructor
	StructMaxInst()
		:m_pLimiter( NULL )
		,u16Current( 0 )
		,u16CurrentVirtual( 0 )
	{
	}

	StructMaxInst( AkUInt16 in_u16Current, AkUInt16 in_u16Max, bool in_bDoesKillNewest, bool in_bAllowUseVirtualBehavior )
		:u16Current( in_u16Current )
		,u16CurrentVirtual( 0 )
	{
		// If it fails there will be no limitor, we can't help it, memory will be the new limit.
		m_pLimiter = AkNew( g_DefaultPoolId, CAkLimiter( in_u16Max, in_bDoesKillNewest, in_bAllowUseVirtualBehavior ) );
	}

	void DisableLimiter()
	{
		if( m_pLimiter )
		{
			m_pLimiter->Term();
			AkFree( g_DefaultPoolId, m_pLimiter); 
			m_pLimiter = NULL;
		}
	}

	void Increment(){++u16Current;} 
	void Decrement()
	{
		AKASSERT( u16Current != 0 );
		--u16Current;
	}

	void IncrementVirtual(){++u16CurrentVirtual;}
	void DecrementVirtual()
	{
		AKASSERT( u16CurrentVirtual != 0 );
		--u16CurrentVirtual;
	}

	void SetMax( AkUInt16 in_u16Max )
	{ 
		if( m_pLimiter )
			m_pLimiter->UpdateMax( in_u16Max );
	}

	bool IsMaxNumInstancesActivated(){ return GetMax() != 0; }

	AkUInt16 GetCurrent(){ return u16Current; }
	AkUInt16 GetCurrentVirtual(){ return u16CurrentVirtual; }
	AkUInt16 GetMax()
	{ 
		if( m_pLimiter )
			return m_pLimiter->GetMaxInstances(); 
		return 0;
	}

	void SwapOrdering()
	{
		if( m_pLimiter )
			m_pLimiter->SwapOrdering();
	}

	void SetUseVirtualBehavior( bool in_bUseVirtualBehavior )
	{
		if( m_pLimiter )
			m_pLimiter->SetUseVirtualBehavior( in_bUseVirtualBehavior );
	}

	CAkLimiter* m_pLimiter;// One limiting list per game object.

private:

	AkUInt16 u16Current;
	AkUInt16 u16CurrentVirtual;
};


typedef CAkKeyArray<CAkRegisteredObj *, StructMaxInst> AkPerObjPlayCount;

// The following class contains multiple information that is not required when the node is not playing.
// ( saving significant memory amount on each AudioNode in % )
class AkActivityChunk
{
public:
	AkActivityChunk( AkUInt16 in_u16MaxNumInstance, bool in_bIsGlobalLimit, bool in_bDoesKillNewest,  bool in_bAllowUseVirtualBehavior )
		:m_Limiter( in_u16MaxNumInstance, in_bDoesKillNewest, in_bAllowUseVirtualBehavior )
		,m_PlayCount(0)
		,m_uActivityCount(0)
		,m_iPlayCountValid(0)
		,m_iVirtualCountValid(0)
		,m_bIsGlobalLimit( in_bIsGlobalLimit )
	{
	}

	~AkActivityChunk()
	{
		m_Limiter.Term();
		m_listPBI.Term();
		AKASSERT( m_ListPlayCountPerObj.Length() == 0 );//If not true, we would have to term m_ListPlayCountPerObj members.
		m_ListPlayCountPerObj.Term();
		m_ActiveChildren.Term();
	}

	AkUInt16 GetPlayCount(){ return m_PlayCount; }
	AkUInt16 GetActivityCount(){ return m_uActivityCount; }

	AkUInt16 GetPlayCountValid(){ return m_iPlayCountValid; }
	AkUInt16 GetVirtualCountValid(){ return m_iVirtualCountValid; }

	void IncrementPlayCount(){++m_PlayCount;}
	void DecrementPlayCount()
	{
		AKASSERT( m_PlayCount );
		--m_PlayCount;
	}

	void IncrementActivityCount(){++m_uActivityCount;}
	void DecrementActivityCount()
	{
		AKASSERT( m_uActivityCount );
		--m_uActivityCount;
	}

	void IncrementPlayCountValid(){++m_iPlayCountValid;}
	void DecrementPlayCountValid()
	{
		AKASSERT( m_iPlayCountValid );
		--m_iPlayCountValid;
	}

	void IncrementVirtualCountValid(){++m_iVirtualCountValid;}
	void DecrementVirtualCountValid()
	{
		AKASSERT( m_iVirtualCountValid );
		--m_iVirtualCountValid;
	}

	bool ChunkIsUseless()
	{
		return !m_PlayCount 
			&& !m_uActivityCount 
			&& !m_iPlayCountValid
			&& !m_iVirtualCountValid
			&& (m_listPBI.IsEmpty()) 
			&& (m_ListPlayCountPerObj.IsEmpty() ) ;
	}

	bool SetFastActive( CAkParameterNodeBase* in_pChild )
	{
		return m_ActiveChildren.AddLast( in_pChild ) != NULL;
	}
	void UnsetFastActive( CAkParameterNodeBase* in_pChild )
	{
		m_ActiveChildren.RemoveSwap( in_pChild );
	}

	AkPerObjPlayCount m_ListPlayCountPerObj;

	typedef AkListBareLight<CAkPBI> AkListLightCtxs;
	AkListLightCtxs m_listPBI;

	void UpdateMaxNumInstanceGlobal( AkUInt16 in_u16LastMaxNumInstanceForRTPC )
	{ 
		m_Limiter.UpdateMax( in_u16LastMaxNumInstanceForRTPC );
	}

	bool IsGlobalLimit(){ return m_bIsGlobalLimit; }

	CAkLimiter m_Limiter;

	AkChildArray& GetActiveChildren(){return m_ActiveChildren;}

private:

	AkChildArray m_ActiveChildren;

	AkUInt16 m_PlayCount;			// Total
	AkUInt16 m_uActivityCount;		// Total

	AkUInt16 m_iPlayCountValid;		// Only used for computed max
	AkUInt16 m_iVirtualCountValid;	// Only used for computed max

	AkUInt8 m_bIsGlobalLimit : 1;
};

#endif //_ACTIVITYCHUNK_H_
