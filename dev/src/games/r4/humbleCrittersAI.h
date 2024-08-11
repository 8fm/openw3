#pragma once

#include "r4BoidSpecies.h"
#include "r4SwarmUtils.h"
#include "../../common/game/baseCrittersAI.h"

class CHumbleCritterLairParams;
class CHumbleCritterAI : public CBaseCritterAI
{
	typedef CHumbleCrittersLairEntity Lair;
	typedef CHumbleCrittersAlgorithmData AlgorithmData;
public:
	typedef Float Time;

	
	CHumbleCritterAI();

	void							SetWorld( CHumbleCrittersAlgorithmData* algorithmData )		{ m_algorithmData = algorithmData; }
	Bool							SeeActor() const											{ return m_targetActor != INVALID_POI_ID; }

	void			Update( const SSwarmMemberStateData& currState, SSwarmMemberStateData& destState );
	void			Spawn( const Vector2& position );
	Bool			ApplyFire(Boids::PointOfInterestId poiID);
	void			ApplyAction(Boids::PointOfInterestId poiID);
	void			GoDespawn();
	void			CancelDespawn();
	const Float&	GetEffectorRandom()const{return m_effectorRandom;}


	void ApplyEffector( const CPoiJobData &staticPointData, const CPointOfInterestSpeciesConfig*const poiConfig );


protected:
	void		RandomizeWanderVector();
	void		ChangeState( EHumbleCritterState state );

	Bool		Turn( const CHumbleCritterLairParams *const params, Float& inoutYaw, Float desiredYaw, Float uberMultiplier = 1.f );
	Bool		TestLocation( const Vector2& dest ) const;
	Float		ComputeZ( const Vector2& pos ) const;

	void		Mark( const Vector2& position );
	void		UnMark();
	void		UpdateMark( Int32 fieldX, Int32 fieldY );

	CHumbleCrittersAlgorithmData*	m_algorithmData;
	Bool							m_stateChanged;
	Boids::PointOfInterestId		m_targetActor;
	Time							m_stateTimeout;
	Vector2							m_randomVector;
	Float							m_hunger;
	Bool							m_isAlive;
	Float							m_effectorRandom;

	// Use state specifics:
	Boids::PointOfInterestId		m_onActionPoiID;

	Bool								m_immobile;

	Vector2								m_velocity;
private:
	Int32								m_markedX;
	Int32								m_markedY;

	// Helper functions 
	Vector2					FindClosestSpawnPoint	( const Vector2& currentPosition, Float & closestSpawnPointSq )const;
	Vector2					ComputePoiGravity		( const CPoiJobData *const poiJobData, const CPointOfInterestSpeciesConfig *const pointOfInterestConfig, const SSwarmMemberStateData &currState, SSwarmMemberStateData &destState, Float randomisation )const;
	Vector2					ComputeGravityForCircle	( const CPointOfInterestSpeciesConfig *const poiSpeciesConfig, const Vector &boidPosition, const CPoiJobData &poiJobData, Float randomisation )const;
	Vector2					ComputeGravityForCone	( const CPointOfInterestSpeciesConfig *const poiSpeciesConfig, const Vector &boidPosition, const CPoiJobData &poiJobData, Float randomisation )const;
};

