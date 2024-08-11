/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/behTreeNodeCustomSteering.h"

#include "combatDataComponent.h"

class CBehTreeNodeStrafingInstance;
class CBehTreeStrafingAlgorithmDefinition;
class CBehTreeStrafingAlgorithmInstance;


class CBehTreeNodeStrafingDefinition : public CBehTreeNodeCustomSteeringDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeStrafingDefinition, CBehTreeNodeCustomSteeringDefinition, CBehTreeNodeStrafingInstance, Strafing );
protected:
	CBehTreeValFloat		m_updateFrequency;

	CBehTreeValFloat		m_steeringSpeed;
	CBehTreeValFloat		m_steeringImportance;

	CBehTreeValFloat		m_accelerationRate;

	CBehTreeValFloat		m_strafingWeight;
	CBehTreeValFloat		m_keepDistanceWeight;
	CBehTreeValFloat		m_randomStrafeWeight;

	CBehTreeValFloat		m_randomizationFrequency;

	CBehTreeValFloat		m_minRange;
	CBehTreeValFloat		m_maxRange;

	CBehTreeValFloat		m_desiredSeparationAngle;
	CBehTreeValBool			m_gravityToSeparationAngle;

	CBehTreeValBool			m_lockOrientation;

	CBehTreeValInt			m_strafingRing;
	CBehTreeStrafingAlgorithmDefinition*		m_customAlgorithm;
public:
	CBehTreeNodeStrafingDefinition()
		: m_updateFrequency( 0.15f )
		, m_steeringSpeed( 1.f )
		, m_steeringImportance( 0.25f )
		, m_accelerationRate( 2.f )
		, m_strafingWeight( 1.f )
		, m_keepDistanceWeight( 1.f )
		, m_randomStrafeWeight( 0.25f )
		, m_randomizationFrequency( 3.f )
		, m_minRange( 3.f )
		, m_maxRange( 6.f )
		, m_desiredSeparationAngle( 180.f )
		, m_gravityToSeparationAngle( false )
		, m_lockOrientation( true )
		, m_strafingRing( 1 )
		, m_customAlgorithm( NULL )											{}

	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeStrafingDefinition );
	PARENT_CLASS( CBehTreeNodeCustomSteeringDefinition );
	PROPERTY_EDIT( m_updateFrequency, TXT("Frequency of recalculating side strafing, in seconds") );
	PROPERTY_EDIT( m_steeringSpeed, TXT("Reference (max) speed passed to steering") );
	PROPERTY_EDIT( m_steeringImportance, TXT("Heading importance multiplier passed to steering") );
	PROPERTY_EDIT( m_accelerationRate, TXT("Max output change per second") );
	PROPERTY_EDIT( m_strafingWeight, TXT("Weight of 'strafe to position' algorithm") );
	PROPERTY_EDIT( m_keepDistanceWeight, TXT("Weight of 'keep distance' algorithm") );
	PROPERTY_EDIT( m_randomStrafeWeight, TXT("Weight of 'randomization' algorithm") );
	PROPERTY_EDIT( m_randomizationFrequency, TXT("Frequency in seconds of changing randomization component and desired range from target.") );
	PROPERTY_EDIT( m_minRange, TXT("Minimum strafing distance"));
	PROPERTY_EDIT( m_maxRange, TXT("Maximum strafing distance"));
	PROPERTY_EDIT( m_desiredSeparationAngle, TXT("Desired angular distance to closest ally"));
	PROPERTY_EDIT( m_gravityToSeparationAngle, TXT("If this flag is on, if npc is above desiredSeparationAngle from ally, he will steer back to him."));
	PROPERTY_EDIT( m_lockOrientation, TXT("Lock orientation to target by default.") );
	PROPERTY_EDIT( m_strafingRing, TXT("Strafing ring id. NPCs are aware of guys strafing on smaller or equal strafing ring id."));
	PROPERTY_INLINED( m_customAlgorithm, TXT("Custom algorithm definition") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeStrafingInstance : public CBehTreeNodeCustomSteeringInstance
{
	friend class CBehTreeStrafingAlgorithmInstance;
	typedef CBehTreeNodeCustomSteeringInstance Super;
protected:
	CCombatDataPtr						m_targetData;

	Float								m_updateFrequency;

	Float								m_steeringSpeed;
	Float								m_steeringImportance;

	Float								m_accelerationRate;

	Float								m_strafingWeight;
	Float								m_keepDistanceWeight;
	Float								m_randomStrafeWeight;

	Float								m_randomizationFrequency;

	Float								m_minRange;
	Float								m_rangeSpan;
	Float								m_currDesiredDistance;
	Float								m_desiredRangeTimeout;

	Float								m_desiredSeparationAngle;

	Vector2								m_currentWorldHeading;
	Float								m_updateTimeout;
	Vector2								m_randomizer;
	Float								m_randomizerTimeout;

	Float								m_desiredAngleDistance;
	Float								m_currentTargetDistance;
	Float								m_customSpeed;

	Int16								m_strafingRing;

	Bool								m_useCustomSpeed;
	Bool								m_lockOrientationByDefault;
	Bool								m_gravityToSeparationAngle;
	Bool								m_lockOrientation;
	CBehTreeStrafingAlgorithmInstance*	m_customAlgorithm;

public:

	typedef CBehTreeNodeStrafingDefinition Definition;

	CBehTreeNodeStrafingInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	Bool Activate() override;
	void Deactivate() override;
	void Update() override;
	Bool OnEvent( CBehTreeEvent& e ) override;
	////////////////////////////////////////////////////////////////////
	void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta ) override;
	Bool IsFinished() const override;
};

