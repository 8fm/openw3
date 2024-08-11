#pragma once

#include "../../common/game/swarmLairEntity.h"
#include "../../common/game/swarmUpdateJob.h"

class CHumbleCrittersAlgorithmData;
class CHumbleCritterLairParams;

////////////////////////////////////////////////////////////////
/// Boid class for all creatures that move on the ground 
class CHumbleCrittersLairEntity : public CSwarmLairEntity
{
	DECLARE_ENGINE_CLASS( CHumbleCrittersLairEntity, CSwarmLairEntity, 0 );

	friend class CHumbleCrittersAlgorithmData;
public:



	CHumbleCrittersLairEntity();

	// CSwarmLairEntity virtual functions
	void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;
	void OnAttachFinished( CWorld* world ) override;

	void NoticeFireInCone( const Vector& position, const Vector2& coneDir, Float coneHalfAngle, Float coneRange );

	volatile CHumbleCrittersAlgorithmData* GetAlgorithmData() const volatile;
protected:
	CSwarmUpdateJob*		NewUpdateJob() override;
	CSwarmAlgorithmData*	NewAlgorithmData() override;

	Uint32					m_breakCounter;


	////////////////////////////////////////////////////////////
	// Here more custom, and less readable shit starts.
public:
	struct SFireInConeInfo
	{
		SFireInConeInfo()
			: m_isPending( false )								{}
		Bool				m_isPending;
		Bool				m_isPointOfInterestActive;
		Vector3				m_origin;
		Vector2				m_dir;
		Float				m_halfAngle;
		Float				m_range;
	};

	const SFireInConeInfo& GetFireInConeInfo() const			{ return m_fireInConeInfo; }
	void DisposePendingFireInConeInfo()							{ m_fireInConeInfo.m_isPending = false; }
	
protected:
	SFireInConeInfo			m_fireInConeInfo;
};

BEGIN_CLASS_RTTI( CHumbleCrittersLairEntity )
	PARENT_CLASS( CSwarmLairEntity )
	PROPERTY_EDIT( m_breakCounter, TXT("When there are less critters, they are 'defeated' and use 'actorsBreakGravity'.") )
END_CLASS_RTTI()


class CHumbleCritterLairParams : public CSwarmLairParams
{
public:
	Float					m_maxVelocity;
	Float					m_turnRatio;
	Float					m_individualRandomization;

	Float					m_actorsRangeDesired;
	Float					m_actorsRangeMax;
	Float					m_actorsGravity;

	Float					m_fireRangeMin;
	Float					m_fireRangeMax;
	Float					m_fireRepultion;

	Float					m_mutualUndesiredDensity;
	Float					m_mutualRepultion;

	Float					m_hungerRatio;
	Float					m_eatingTime;
	Float					m_hungerGravity;

	Float					m_wanderTime;
	Float					m_wanderGravity;

	Float					m_wallsDistance;
	Float					m_wallsRepulsion;
	
	Float					m_panicSpeedBoost;
	Float					m_panicActorsGravity;
	Float					m_actorRangeMultWhenPanic;

	Float					m_burnResistance;
	Float					m_postPanicDeathChance;
	Float					m_attackRange;
	Bool					m_hasAttackBehavior;

	Bool					m_testLocation_hack;

	Bool					m_walkSideway;					// Desired Yaw +/- 90

	enum ETypes
	{
		TYPE_HUMBLE_CRITTER_LAIR_PARAMS	= TYPE_SWARM_LAIR_PARAMS | FLAG( 2 ),
	};
	enum 
	{ 
		E_TYPE = TYPE_HUMBLE_CRITTER_LAIR_PARAMS, 
	};
	CHumbleCritterLairParams(Bool isValid);
	virtual Bool ParseXmlAttribute(const SCustomNodeAttribute & att);

	static const CHumbleCritterLairParams sm_defaultParams;
private:
	Bool VirtualCopyTo(CBoidLairParams* const params)const override;
};

