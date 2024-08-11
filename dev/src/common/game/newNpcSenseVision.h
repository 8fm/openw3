/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiProfile.h"
#include "newNpcSensesManager.h"
#include "newNpcNoticedObject.h"

class CNewNPCSenseVision : public CNewNPCSense
{
	struct SDelayedQuery
	{
		THandle< CActor >	m_actor;
		VisibilityQueryId	m_queryId;

		SDelayedQuery()
			: m_actor( nullptr )
		{}

		SDelayedQuery( CActor* actor, const VisibilityQueryId& queryId )
			: m_actor( actor )
			, m_queryId( queryId )
		{}
	};

	enum ERangeTestResult
	{
		RTR_None,
		RTR_Passed,
		RTR_Failed,
		RTR_Delayed,
	};

	struct RangeTestResultData
	{
		explicit RangeTestResultData( ERangeTestResult result,
									  NewNPCNoticedObject::TFlags flags = NewNPCNoticedObject::TFlags( 0 ),
									  const VisibilityQueryId queryId = VisibilityQueryId() )
			: m_result( result )
			, m_flags( flags )
			, m_delayedQueryId( queryId )
		{
		}

		ERangeTestResult				m_result;
		NewNPCNoticedObject::TFlags		m_flags;
		VisibilityQueryId				m_delayedQueryId;
	};

	struct SSenseParamsPrecals
	{
		CAISenseParams*	m_params;
		Float			m_rangeMinSqr;
		Float			m_rangeMaxSqr;
		Float			m_cosHalfAngle;

		void Set( CAISenseParams* params )
		{
			m_params = params;
			m_rangeMinSqr = params->m_rangeMin * params->m_rangeMin;
			m_rangeMaxSqr = params->m_rangeMax * params->m_rangeMax;
			m_cosHalfAngle = MCos( DEG2RAD( params->m_rangeAngle * 0.5f ) );
		}
	};

	struct SRangeTestData
	{
		const SSenseParamsPrecals*	m_senseParamsPrecalcs;
		Vector						m_actorPos;
		Vector						m_senseNearestPos;
		Float						m_distSqr;

		SRangeTestData()
			: m_senseParamsPrecalcs( nullptr )
			, m_actorPos( Vector::ZERO_3D_POINT )
			, m_senseNearestPos( Vector::ZERO_3D_POINT )
			, m_distSqr( 0.0f )
		{}
	};


protected:

	Int32							m_boneIndex;			//!< Bone index

	typedef TList< SDelayedQuery > TDelayedQueries;
	mutable TDelayedQueries			m_delayedQueries;		//!< Delayed "line of sight" test queries
	CAISenseParams*					m_absoluteParams;		//!< Cached absolute sense params
	CAISenseParams*					m_visionParams;			//!< Cached vision sense params

	// per-frame cached data
	Vector							m_sensePosition;
	Vector							m_senseDirection;
	SSenseParamsPrecals				m_absoluteParamsPrecalcs;
	SSenseParamsPrecals				m_visionParamsPrecalcs;
	Bool							m_detectOnlyHostiles;

#ifndef RED_FINAL_BUILD
	TList< SRaycastDebugData >		m_raycastsDebugData;	//!< Line of sight tests debug data
	static const Uint32 MAX_DEBUG_DATA_SIZE = 3;
#endif

public:
	CNewNPCSenseVision( CNewNPC* npc )
		: CNewNPCSense( npc )
		, m_boneIndex( -1 )
	{
	}

	virtual ~CNewNPCSenseVision();

	//! Initialize
	virtual void Initialize();

	//! Set template sense params pointer
	void SetSenseParams( EAISenseType senseType, CAISenseParams* senseParams ) override;

	//! Get template sense params pointer
	CAISenseParams* GetSenseParams( EAISenseType senseType ) const override;

	//! Get max range
	Float GetRangeMax() const override;

	//! Get max range for specified sense type
	Float GetRangeMax( EAISenseType senseType ) const override;

	//! Check if given sense type is valid
	Bool IsValid( EAISenseType senseType ) const override;

	//! Get sense info
	String GetInfo() const override;

	//! Initialize sense update
	Bool BeginUpdate( TNoticedObjects& noticedObjects ) override;

	//! Finalize sense update
	NewNPCSenseUpdateFlags EndUpdate( TNoticedObjects& noticedObjects, NewNPCSenseUpdateFlags updated, Float timeDelta ) override;

	//! Update player
	NewNPCSenseUpdateFlags UpdatePlayer( TNoticedObjects& noticedObjects ) override;

	//! Update NPCs
	NewNPCSenseUpdateFlags UpdateNPC( CNewNPC* npc, TNoticedObjects& noticedObjects ) override;
	
protected:

	//! Cache bone index
	void CacheBoneIndex();

	//! Player range test passed
	NewNPCSenseUpdateFlags OnPlayerRangeTestPassed( TNoticedObjects& noticedObjects, NewNPCNoticedObject::TFlags flags );

	//! NPC range test passed
	NewNPCSenseUpdateFlags OnNPCRangeTestPassed( TNoticedObjects& noticedObjects, CActor* actor, NewNPCNoticedObject::TFlags flags );

	//! Player noticed by sense
	Bool OnPlayerNoticed( CPlayer * player );

public:

	//! On actor noticed 
	NewNPCSenseUpdateFlags OnActorNoticed( CActor* actor, TNoticedObjects& noticedObjects, NewNPCNoticedObject::TFlags flags, Int32& objectIndex );

protected:

	//! Range test
	RangeTestResultData RangeTest( CActor* actor ) const;

	//! Internal range test for SRangeTestData
	Bool RangeTest( SRangeTestData& testData ) const;

	//! Specific actor test
	Bool ActorTest( CActor* actor );

	//! Process delayed line of sight test queries
	NewNPCSenseUpdateFlags ProcessDelayedQueries( TNoticedObjects& noticedObjects );

	//! Cache per-frame sense params
	void PerFramePrecalcs();

public:

	//! Generate debug fragments
	void GenerateDebugFragments( CRenderFrame* frame ) override;

protected:

	//! Draw debug data for specified sense params
	void DrawDebug(  CRenderFrame* frame, CAISenseParams* senseParams, EAISenseType senseType );
};
