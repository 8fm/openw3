/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "binaryStorage.h"

class CRoadsManager : public IGameSystem
{
	DECLARE_ENGINE_CLASS( CRoadsManager, IGameSystem, 0 );

public:
	CRoadsManager();

public:
	struct SRoadParameters
	{
		SRoadParameters();

		CStripeComponent*		m_roadComponent;
		SMultiCurvePosition		m_desiredPointIndex;
		Vector					m_desiredPointPosition;
		Vector					m_desiredPointTangent;
		Vector					m_directionToDesiredPoint;
		Vector					m_rightLanePosition;
		Float					m_desiredDirectionDotProduct;
		Float					m_distanceToDesiredPoint;
		Float					m_angleToDesiredPointTangent;
		Float					m_roadScore;
	};

public:
	virtual void OnGameEnd( const CGameInfo& gameInfo ) override;
	virtual void OnWorldEnd( const CGameInfo& gameInfo ) override;
	virtual void Tick( Float timeDelta ) override;

#ifndef NO_EDITOR
	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) override;
#endif

public:
	//! Register the stripe component (usually on its attaching)
	void RegisterRoad( CStripeComponent* road );

	//! Unregister the stripe component (most likely on its detaching)
	void UnregisterRoad( CStripeComponent* road );

	//! Search for the closest road with the best direction and return the heading to the right side of the best point of this road
	Bool FindClosestRoad( const Vector& riderPosition, Float speed, Float maxAngle, Float maxDistance, Vector& riderDirection );
	void Reset();
	void EvaluateCandidate( CStripeComponent& element,
							SRoadParameters& candidate,
							const Vector& referencePosition,
							const Vector& referenceDirection,
							const Vector& lookAheadPosition,
							Float distanceCoeff,
							Float angleCoeff,
							Float turnAmountFee,
							Float currentRoadReward ) const;

private:
	Bool SelectBestDirectionToFollow( Float maxAngle, Float maxDistance, Vector& outDirection );

private:
	typedef CQuadTreeStorage< CStripeComponent, TPointerWrapper< CStripeComponent > > TRoads;

	struct SRoadDebugData
	{
		SRoadDebugData()				{}

		TDynArray< SRoadParameters >	m_collectedParameters;
		Box								m_searchBox;
		Vector							m_referencePosition;
		Vector							m_referenceDirection;
		Vector							m_lookAheadPosition;
	};

private:
	//! Spatial storage of the registered roads
	TRoads						m_roads;

	//! Current data => for debug purposes
	SRoadDebugData			m_currentRoadData;
	Float					m_coolDownTimeout;
	Bool					m_wasOnTheRoad;

	//! All the roads within desired search box and their computed parameters
	TDynArray< SRoadParameters > m_currentCandidates;
	CStripeComponent*		m_lastSelectedRoad;

	ASSIGN_GAME_SYSTEM_ID( GS_RoadsManager );
};

BEGIN_CLASS_RTTI( CRoadsManager );
	PARENT_CLASS( IGameSystem );
END_CLASS_RTTI();