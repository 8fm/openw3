#pragma once

#include "../../common/game/swarmSound.h"
#include "r4SwarmUtils.h"
#include "humbleCrittersAI.h"
#include "humbleCritterAlgorithmData.h"
/*

class CHumbleCrittersSound : public CBoidSound
{
protected:
	ESwarmSounds					m_soundId;
	Float									m_countScale;

public:
	CHumbleCrittersSound( const StringAnsi& eventStart, const StringAnsi& eventStop, const StringAnsi& countParameter, ESwarmSounds soundId, Float countScale = 1.f )
		: CBoidSound( eventStart, eventStop, countParameter )
		, m_soundId( soundId )
		, m_countScale( countScale )							{}

	void Update( const Vector& cameraPos, IBoidLairEntity& lair ) override;
};


struct CCrittersSoundAcceptorDying
{
	RED_INLINE Bool InitialAccept( CHumbleCrittersAlgorithmData* crittersData ) const
	{
		return crittersData->GetActivityCount( R4Boids::STATE_DIE_BURN_SHAKE ) > 0 ;
	}
	RED_INLINE Bool AcceptCritter( const CHumbleCritterAI& critter ) const
	{
		return critter.GetState() == R4Boids::STATE_DIE_BURN_SHAKE;
	}
};

struct CCrittersSoundAcceptorMoving
{
	RED_INLINE Bool InitialAccept( CHumbleCrittersAlgorithmData* crittersData ) const
	{
		return crittersData->GetActivityCount( R4Boids::STATE_IDLE ) > 0 || crittersData->GetActivityCount( R4Boids::STATE_WANDER ) > 0;
	}
	RED_INLINE Bool AcceptCritter( const CHumbleCritterAI& critter ) const
	{
		return critter.GetState() == R4Boids::STATE_IDLE || critter.GetState() == R4Boids::STATE_WANDER;
	}
};

struct CCrittersSoundAcceptorMovingIdle : public CCrittersSoundAcceptorMoving
{
	RED_INLINE Bool AcceptCritter( const CHumbleCritterAI& critter ) const
	{
		return Super::AcceptCritter( critter ) && !critter.SeeActor();
	}
private:
	typedef CCrittersSoundAcceptorMoving Super;
};
struct CCrittersSoundAcceptorMovingAttack : public CCrittersSoundAcceptorMoving
{
	RED_INLINE Bool AcceptCritter( const CHumbleCritterAI& critter ) const
	{
		return Super::AcceptCritter( critter ) && critter.SeeActor();
	}
private:
	typedef CCrittersSoundAcceptorMoving Super;
};*/


