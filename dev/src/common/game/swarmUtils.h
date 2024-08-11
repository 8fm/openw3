#pragma once

#include "../core/randomSystem.h"

//#define SWARM_ADVANCED_DEBUG


#define INVALID_POI_ID ((Boids::PointOfInterestId)0xffffffff)
#define SWARM_DEBUG_DISTANCE 25.0f

#define SWARM_FLOOD_FILL_SP_RANGE	2.0f

namespace Boids
{
	typedef CName		PointOfInterestType;
	typedef Uint32		PointOfInterestId;
	typedef Int32		BoidState;
}

class CBaseBoidNode;
typedef TDynArray< const CBaseBoidNode * >			CBaseBoidNode_Array;
class CPointOfInterestSpeciesConfig;
typedef TDynArray< CPointOfInterestSpeciesConfig *>	CPointOfInterestSpeciesConfig_Array;

typedef TDynArray< Uint32 > CUint_Array;

class CSwarmSoundConfig;
typedef TDynArray< const CSwarmSoundConfig * > CSwarmSoundConfig_CPointerArray;

enum 
{
	BOID_STATE_FLAG_INSIDE_CIRCLE_POI	=  1 << 0,
	BOID_STATE_FLAG_INSIDE_CONE_POI		=  1 << 1
};

enum ECritterState
{
	CRITTER_STATE_NOT_SPAWNED,
	CRITTER_STATE_COUNT,
};

enum EBoidState
{
	BOID_STATE_INVALID		= -2,
	BOID_STATE_NOT_SPAWNED	= -1,
	BOID_STATE_IDLE			= 0,
	BOID_STATE_MOVE,
	BOID_STATE_ATTACK,
	BOID_STATE_DEATH,
	BOID_STATE_RUN_PANIC,
	BOID_STATE_LAST			
};

namespace
{
	struct SRandomSeedModifier
	{
		static Uint32	s_baseSeed;	
		Uint32			m_seedModifier;
		SRandomSeedModifier()
			: m_seedModifier( ::CRandomSystem::GetRandomIntFromSeed( s_baseSeed ) )
		{}

		Uint32 ModifySeed( Uint32 seed ) const						{ return m_seedModifier ^ seed; }
	};
	Uint32 SRandomSeedModifier::s_baseSeed;	
};

#define CREATE_RANDOMIZED_VAR(  val, initialValue, allowedVariation )					\
	Float val;																			\
	{																					\
		static const ::SRandomSeedModifier s_randomSeed##val;							\
		val = initialValue;																\
		Uint32 localSeed = s_randomSeed##val.ModifySeed( m_randomSeed );				\
		val *= 1.f - allowedVariation													\
			+ 2.f * allowedVariation													\
			* CRandomSystem::GetRandomFloatFromSeed( localSeed );						\
	}

// Neither vector need to be normalised 
RED_INLINE Bool RemoveOppositionFromVect( Vector3 & inputVect, const Vector3 & opposVect )
{
	const Float dotProduct = opposVect.Dot( inputVect );
	if ( dotProduct < 0.0f )
	{
		inputVect -= opposVect * dotProduct;
		return true;
	}
	return false;
}

RED_INLINE Int32 Ceil( Float value )
{
	return (Int32)( value + 1.0f );
}


