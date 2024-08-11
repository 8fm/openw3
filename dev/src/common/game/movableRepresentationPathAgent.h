#pragma once

#include "../engine/pathlibAgent.h"

#include "movementTargeter.h"

#ifndef NO_EDITOR_FRAGMENTS
#define PATHAGENT_DEBUG_Z_CORRECTION
#endif

////////////////////////////////////////////////////////////////////////////
// Its at the same time move representation AND navigation system agent.
// It has all functionalities of PathLib::CAgent - navqueries
////////////////////////////////////////////////////////////////////////////
class CPathAgent : public PathLib::CAgent, public IMovableRepresentation
{
protected:
	CMovingAgentComponent&	m_host;
	CPhysicsWorld*			m_physicsWorld;

	Bool					m_snapHeight;
	Bool					m_animatedMovement;
	Bool					m_forceHeightSnaping;
	Bool					m_forceInstantZCorrection;
#ifdef PATHAGENT_DEBUG_Z_CORRECTION
	Bool					m_didZCorrection;
#endif
	Bool					m_forceZCorrection;
	Bool					m_forceNavigationZCorrection;

	Float					m_representationZ;
	Vector3					m_lastPos;
	Float					m_lastPosRepresentationZ;
	Vector3					m_segmentStart;
	Vector3					m_segmentEnd;
	Float					m_segmentLen;

	void						DetermineHeightSnapping();
public:
	enum PathlibAgents : ClassId
	{
		CLASS_CPathAgent		= FLAG( 1 )
	};
	static const ClassId CLASS_ID = CLASS_CPathAgent;


	CPathAgent( CPathLibWorld* world, CPhysicsWorld* physicsWorld, CMovingAgentComponent& host, Float personalSpace );

	void						OnInit( const Vector& position, const EulerAngles& orientation ) override;
	void						OnActivate( const Vector& position, const EulerAngles& orientation ) override;
	void						OnDeactivate() override;
	void						OnMove( const Vector& deltaPosition, const EulerAngles& deltaOrientation ) override;
	void						OnSeparate( const Vector& deltaPosition ) override;
	Bool						Update2_PostSeparation( Float timeDelta, Bool continousMovement ) override;
	void						OnSetPlacement( Float timeDelta, const Vector& newPosition, const EulerAngles& newOrientation, const Bool correctZ ) override;
	Vector						GetRepresentationPosition( Bool smooth = false ) const override;
	EulerAngles					GetRepresentationOrientation() const override;
	CName						GetName() const override;
	Bool						IsAlwaysActive() const override;
	Bool						ForceMovementAIAction( IAITree* ai );

	void						SetAnimatedMovement( Bool enable );
	void						ForceHeightSnapping( Bool force );
	Bool						IsAnimatedMovement() const					{ return m_animatedMovement; }
	void						ForceInstantZCorrection()					{ m_forceInstantZCorrection = true; }
	Bool						CheckIfCenterIsOnNavData();

	void						GenerateDebugFragments( CRenderFrame* frame );
	Bool						DoZCorrection( Vector3& pos );

private:
	void						DoMove( const Vector& deltaPosition );
	String						GetDebugEntityName() const;
};

typedef TRefCountPointer< CPathAgent > CPathAgentPtr;
