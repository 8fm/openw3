/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "particleModule.h"
#include "environmentAreaParams.h"

class CDecalSpawner;
class IRenderResource;
class IMaterial;
enum EParticleType : Int32;

//////////////////////////////////////////////////////////////////////////
// Mask flags for initializer set
enum	EParticleInitializerFlags
{
	PIF_Alpha					= FLAG( 0 ),
	PIF_AlphaRandom				= FLAG( 1 ),
	PIF_Color					= FLAG( 2 ),
	PIF_ColorRandom				= FLAG( 3 ),

	PIF_LifeTime				= FLAG( 4 ),
	PIF_LifeTimeRandom			= FLAG( 5 ),

	PIF_Position				= FLAG( 6 ),
	PIF_PositionRandom			= FLAG( 7 ),

	PIF_Rotation				= FLAG( 8 ),
	PIF_RotationRandom			= FLAG( 9 ),

	PIF_Rotation3D				= FLAG( 10 ),
	PIF_Rotation3DRandom		= FLAG( 11 ),

	PIF_RotationRate			= FLAG( 12 ),
	PIF_RotationRateRandom		= FLAG( 13 ),

	PIF_RotationRate3D			= FLAG( 14 ),
	PIF_RotationRate3DRandom	= FLAG( 15 ),

	PIF_Size					= FLAG( 16 ),
	PIF_SizeRandom				= FLAG( 17 ),

	PIF_Size3D					= FLAG( 18 ),
	PIF_Size3DRandom			= FLAG( 19 ),

	PIF_SpawnBox				= FLAG( 20 ),
	PIF_SpawnCircle				= FLAG( 21 ),
	PIF_SpawnSphere				= FLAG( 22 ),

	PIF_Velocity				= FLAG( 23 ),
	PIF_VelocityRandom			= FLAG( 24 ),

	PIF_VelocityInherit			= FLAG( 25 ),
	PIF_VelocityInheritRandom	= FLAG( 26 ),

	PIF_VelocitySpread			= FLAG( 27 ),

	PIF_AnimationFrame			= FLAG( 28 ),
	PIF_AnimationFrameRandom	= FLAG( 29 ),

	PIF_CollisionSpawn			= FLAG( 30 ),
};

//////////////////////////////////////////////////////////////////////////
// Mask flags for modifier set
enum	EParticleModifierFlags
{
	PMF_VelocityOverLifeAbsolute= FLAG( 0 ),
	PMF_VelocityOverLifeModulate= FLAG( 1 ),

	PMF_AccelerationLocal		= FLAG( 2 ),
	PMF_AccelerationWorld		= FLAG( 3 ),

	PMF_AlphaOverLifeAbsolute	= FLAG( 4 ),
	PMF_AlphaOverLifeModulate	= FLAG( 5 ),

	PMF_ColorOverLifeAbsolute	= FLAG( 6 ),
	PMF_ColorOverLifeModulate	= FLAG( 7 ),

	// 2D particles only
	PMF_1ARotationOverLifeAbsolute	= FLAG( 8 ),
	PMF_1ARotationOverLifeModulate	= FLAG( 9 ),

	PMF_1ARotRateOverLifeAbsolute	= FLAG( 10 ),
	PMF_1ARotRateOverLifeModulate	= FLAG( 11 ),

	PMF_2DSizeOverLifeAbsolute		= FLAG( 12 ),
	PMF_2DSizeOverLifeModulate		= FLAG( 13 ),

	// 3D particle only
	PMF_3ARotationOverLifeAbsolute	= FLAG( 14 ),
	PMF_3ARotationOverLifeModulate	= FLAG( 15 ),

	PMF_3ARotRateOverLifeAbsolute	= FLAG( 16 ),
	PMF_3ARotRateOverLifeModulate	= FLAG( 17 ),

	PMF_3DSizeOverLifeAbsolute		= FLAG( 18 ),
	PMF_3DSizeOverLifeModulate		= FLAG( 19 ),

	// Less common modifiers
	PMF_AnimationFrameBySpeed		= FLAG( 20 ),
	PMF_AnimationFrameByLife		= FLAG( 21 ),

	PMF_Target						= FLAG( 22 ),
	PMF_TargetNode					= FLAG( 23 ),
	PMF_Turbulize					= FLAG( 24 ),

	PMF_EffectAlpha					= FLAG( 25 ),
	PMF_Collision					= FLAG( 26 ),
	PMF_AlphaByDistance				= FLAG( 27 ),

	PMF_Invalid						= FLAG( 28 )
};

//////////////////////////////////////////////////////////////////////////
// Light Vector Evaluators

struct SCurveEvaluatorVector2
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Particles );

	TDynArray< Vector2 >		m_samples;

	void SetData( const TDynArray< Vector >& initData )
	{
		for ( Uint32 i=0; i<initData.Size(); ++i )
		{
			m_samples.PushBack( initData[i] );
		}
	}

	friend IFile& operator<<( IFile &file, SCurveEvaluatorVector2 &data )
	{
		file << data.m_samples;
		return file;
	}
};

struct SCurveEvaluatorVector3
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Particles );

	TDynArray< Vector3 >		m_samples;

	void SetData( const TDynArray< Vector >& initData )
	{
		for ( Uint32 i=0; i<initData.Size(); ++i )
		{
			m_samples.PushBack( initData[i] );
		}
	}

	friend IFile& operator<<( IFile &file, SCurveEvaluatorVector3 &data )
	{
		file << data.m_samples;
		return file;
	}
};

struct SCurveEvaluatorVector4
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Particles );

	TDynArray< Vector >			m_samples;

	friend IFile& operator<<( IFile &file, SCurveEvaluatorVector4 &data )
	{
		file << data.m_samples;
		return file;
	}
};

//////////////////////////////////////////////////////////////////////////
// Light Scalar Evaluators

struct SCurveEvaluatorScalar
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Particles );

	TDynArray< Float >		m_samples;

	friend IFile& operator<<( IFile &file, SCurveEvaluatorScalar &data )
	{
		file << data.m_samples;
		return file;
	}
};

//////////////////////////////////////////////////////////////////////////
// Evaluation
//////////////////////////////////////////////////////////////////////////

namespace LightEval
{
	RED_INLINE void GetLowerIndex( Float t, Uint32& index, Uint32 num, Float& frac )
	{
		Float fnum = (Float)num;
		const Float indexWithFrac = t * ( fnum - 1.0f );
		const Float fIndex = MFloor( indexWithFrac );
		frac = indexWithFrac - fIndex;
		index = (Uint32)::Clamp< Float >( fIndex, 0, fnum - 2.0f );
	}

	RED_INLINE Float EvaluateScalar( const SCurveEvaluatorScalar* parameter, Float normalizedLife )
	{
		Uint32 index;
		Float frac;
		GetLowerIndex( normalizedLife, index, parameter->m_samples.Size(), frac );
		const Float result =  parameter->m_samples[ index ] * ( 1.0f - frac ) + parameter->m_samples[ index + 1 ] * frac;
		return result;
	}

	RED_INLINE Vector EvaluateVector( const SCurveEvaluatorVector4* parameter, Float normalizedLife )
	{
		Uint32 index;
		Float frac;
		GetLowerIndex( normalizedLife, index, parameter->m_samples.Size(), frac );
		const Vector result =  parameter->m_samples[ index ] * ( 1.0f - frac ) + parameter->m_samples[ index + 1 ] * frac;
		return result;
	}

	RED_INLINE Vector3 EvaluateVector( const SCurveEvaluatorVector3* parameter, Float normalizedLife )
	{
		Uint32 index;
		Float frac;
		GetLowerIndex( normalizedLife, index, parameter->m_samples.Size(), frac );
		const Vector3 result =  parameter->m_samples[ index ] * ( 1.0f - frac ) + parameter->m_samples[ index + 1 ] * frac;
		return result;
	}

	RED_INLINE Vector2 EvaluateVector( const SCurveEvaluatorVector2* parameter, Float normalizedLife )
	{
		Uint32 index;
		Float frac;
		GetLowerIndex( normalizedLife, index, parameter->m_samples.Size(), frac );
		const Vector2 result =  parameter->m_samples[ index ] * ( 1.0f - frac ) + parameter->m_samples[ index + 1 ] * frac;
		return result;
	}
}


struct SParticleUpdaterData
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Particles );

	// Initializers
	SCurveEvaluatorScalar			m_alphaInit;
	SCurveEvaluatorVector3			m_colorInit;
	SCurveEvaluatorScalar			m_lifeTimeInit;

	SCurveEvaluatorVector3			m_positionInit;
	Float							m_positionOffset;

	SCurveEvaluatorScalar			m_rotationInit;

	SCurveEvaluatorVector3			m_rotation3DInit;

	SCurveEvaluatorScalar			m_rotationRateInit;

	SCurveEvaluatorVector3			m_rotationRate3DInit;

	SCurveEvaluatorVector2			m_sizeInit;
	SCurveEvaluatorVector3			m_size3DInit;
	Bool							m_sizeSpillFirstAxis;

	SCurveEvaluatorVector3			m_spawnBoxExtents;
	SCurveEvaluatorScalar			m_spawnCircleOuterRadius;
	SCurveEvaluatorScalar			m_spawnCircleInnerRadius;
	Bool							m_spawn_BoxCircle_WorldSpace;
	Bool							m_spawn_CircleSphere_SurfaceOnly;
	EulerAngles						m_spawnCircleRotation;
	Matrix							m_spawnCircleSpawnToLocal;

	Bool							m_spawnSpherePositiveX;
	Bool							m_spawnSphereNegativeX;
	Bool							m_spawnSpherePositiveY;
	Bool							m_spawnSphereNegativeY;
	Bool							m_spawnSpherePositiveZ;
	Bool							m_spawnSphereNegativeZ;
	Bool							m_spawnSphereVelocity;

	SCurveEvaluatorVector3			m_velocityInit;
	Bool							m_velocityInitWorldSpace;

	SCurveEvaluatorScalar			m_velocityInheritInit;
	SCurveEvaluatorScalar			m_velocitySpreadInit;

	Bool							m_conserveVelocitySpreadMomentum;

	SCurveEvaluatorScalar			m_animFrameInit;

	Float							m_rainStreaksRadius;
	Float							m_rainStreaksSpeed;
	Float							m_rainDropsArea;

	// Modifiers
	SCurveEvaluatorVector3			m_velocityEval;

	SCurveEvaluatorVector3			m_accelerationDirEval;
	SCurveEvaluatorScalar			m_accelerationScaleEval;

	SCurveEvaluatorScalar			m_RotEval;
	SCurveEvaluatorScalar			m_RotRateEval;

	SCurveEvaluatorVector3			m_rotEval3D;
	SCurveEvaluatorVector3			m_rotRateEval3D;

	SCurveEvaluatorVector3			m_colorEval;
	SCurveEvaluatorScalar			m_alphaEval;

	SCurveEvaluatorVector2			m_sizeEval;
	SCurveEvaluatorVector3			m_sizeEval3D;

	SCurveEvaluatorScalar			m_animFrameEval;

	SCurveEvaluatorVector3			m_turbulenceScale;
	SCurveEvaluatorScalar			m_turbulenceTimeLifeLimit;

	Float							m_turbulenceNoiseInterval;
	Float							m_turbulenceDuration;

	SCurveEvaluatorScalar			m_targetForceScale;
	SCurveEvaluatorScalar			m_targetKillRadius;
	Float							m_targetMaxForce;
	SCurveEvaluatorVector3			m_targetPosition;

	Float							m_collisionSpawnProbability;
	Int32							m_collisionSpawnParentModuleIndex;

	Uint64							m_collisionMask;
	Float							m_collisionDynamicFriction;
	Float							m_collisionStaticFriction;
	Float							m_collisionRestition;
	Float							m_collisionVelocityDamp;
	Bool							m_collisionDisableGravity;
	Bool							m_collisionUseGpuSimulationIfAvaible;
	Float							m_collisionRadius;
	Bool							m_collisionKillWhenCollide;

	Int32							m_collisionEmitterIndex;

	Float							m_farDistance;
	Float							m_nearDistance;

	SParticleUpdaterData()
		: m_collisionEmitterIndex( -1 )
		, m_collisionSpawnParentModuleIndex( -1 )
	{
	}

	friend IFile& operator<<( IFile &file, SParticleUpdaterData &data )
	{
		file << data.m_alphaInit;
		file << data.m_colorInit;
		file << data.m_lifeTimeInit;

		file << data.m_positionInit;
		file << data.m_positionOffset;

		file << data.m_rotationInit;

		file << data.m_rotation3DInit;

		file << data.m_rotationRateInit;

		file << data.m_rotationRate3DInit;

		file << data.m_sizeInit;
		file << data.m_size3DInit;
		file << data.m_sizeSpillFirstAxis;

		file << data.m_spawnBoxExtents;
		file << data.m_spawnCircleOuterRadius;
		file << data.m_spawnCircleInnerRadius;
		file << data.m_spawn_BoxCircle_WorldSpace;
		file << data.m_spawn_CircleSphere_SurfaceOnly;
		file << data.m_spawnCircleRotation;
		file << data.m_spawnCircleSpawnToLocal;

		file << data.m_velocityInit;
		file << data.m_velocityInitWorldSpace;

		file << data.m_velocityInheritInit;
		file << data.m_velocitySpreadInit;

		file << data.m_conserveVelocitySpreadMomentum;

		file << data.m_animFrameInit;

		file << data.m_rainStreaksRadius;
		file << data.m_rainStreaksSpeed;
		file << data.m_rainDropsArea;

		// Modifiers
		file << data.m_velocityEval;

		file << data.m_accelerationDirEval;
		file << data.m_accelerationScaleEval;

		file << data.m_RotEval;
		file << data.m_RotRateEval;

		file << data.m_rotEval3D;
		file << data.m_rotRateEval3D;

		file << data.m_colorEval;
		file << data.m_alphaEval;

		file << data.m_sizeEval;
		file << data.m_sizeEval3D;

		file << data.m_animFrameEval;

		file << data.m_turbulenceScale;
		file << data.m_turbulenceTimeLifeLimit;

		file << data.m_turbulenceNoiseInterval;
		file << data.m_turbulenceDuration;

		file << data.m_targetForceScale;
		file << data.m_targetKillRadius;
		file << data.m_targetMaxForce;
		file << data.m_targetPosition;

		file << data.m_spawnSpherePositiveX;
		file << data.m_spawnSphereNegativeX;
		file << data.m_spawnSpherePositiveY;
		file << data.m_spawnSphereNegativeY;
		file << data.m_spawnSpherePositiveZ;
		file << data.m_spawnSphereNegativeZ;
		file << data.m_spawnSphereVelocity;

		file << data.m_collisionMask;
		file << data.m_collisionDynamicFriction;
		file << data.m_collisionStaticFriction;
		file << data.m_collisionRestition;
		file << data.m_collisionVelocityDamp;
		file << data.m_collisionDisableGravity;
		file << data.m_collisionUseGpuSimulationIfAvaible;
		file << data.m_collisionRadius;
		file << data.m_collisionKillWhenCollide;
		file << data.m_collisionEmitterIndex;

		file << data.m_collisionSpawnProbability;
		file << data.m_collisionSpawnParentModuleIndex;

		file << data.m_farDistance;
		file << data.m_nearDistance;

		return file;
	}
};

/// Particle burst
struct ParticleBurst
{
	DECLARE_RTTI_STRUCT( ParticleBurst );

	Float		m_burstTime;			//!< When to fire the burst
	Uint32		m_spawnCount;			//!< How many particles to spawn at this burst
	Float		m_spawnTimeRange;		//!< Spawned particles are assumed to be spawned during this period of time
	Float		m_repeatTime;			//!< Time interval between bursts

	RED_INLINE ParticleBurst()
		: m_burstTime( 0.0f )
		, m_spawnCount( 1 )
		, m_spawnTimeRange( 0.0f )
		, m_repeatTime( 0.0f )
	{};
};

BEGIN_CLASS_RTTI( ParticleBurst );
PROPERTY_EDIT( m_burstTime, TXT("When to fire the burst") )
PROPERTY_EDIT( m_spawnCount, TXT("How many particles to spawn") )
PROPERTY_EDIT( m_spawnTimeRange, TXT("Spawned particles are assumed to be spawned during this period of time") )
PROPERTY_EDIT( m_repeatTime, TXT("Time span between bursts, optional") )
END_CLASS_RTTI();

/// Emitter Duration
struct EmitterDurationSettings
{
	DECLARE_RTTI_STRUCT( EmitterDurationSettings );

	Float		m_emitterDuration;			//!< Duration for emitter
	Float		m_emitterDurationLow;		//!< Used with Duration range, lowest emitter duration time 
	Bool		m_useEmitterDurationRange;	//!< Randomized emitter duration

	RED_INLINE EmitterDurationSettings()
		: m_emitterDuration( 1.0f )
		, m_emitterDurationLow( 0.0f )
		, m_useEmitterDurationRange( false )
	{};
};

BEGIN_CLASS_RTTI( EmitterDurationSettings );
PROPERTY_EDIT( m_emitterDuration, TXT("Duration for emitter") )
PROPERTY_EDIT( m_emitterDurationLow, TXT("Used with Duration range, lowest emitter duration time ") )
PROPERTY_EDIT( m_useEmitterDurationRange, TXT("Randomized emitter duration") )
END_CLASS_RTTI();

/// Emitter Delay
struct EmitterDelaySettings
{
	DECLARE_RTTI_STRUCT( EmitterDelaySettings );

	Float		m_emitterDelay;			//!< Delay before the emitter starts
	Float		m_emitterDelayLow;		//!< Used with Delay Range to have a random delay
	Bool		m_useEmitterDelayRange;	//!< Randomized delay of particles
	Bool		m_useEmitterDelayOnce;	//!< Delay the particles spawn just once

	RED_INLINE EmitterDelaySettings()
		: m_emitterDelay( 0.0f )
		, m_emitterDelayLow( 0.0f )
		, m_useEmitterDelayRange( false )
		, m_useEmitterDelayOnce( false )
	{};
};

BEGIN_CLASS_RTTI( EmitterDelaySettings );
PROPERTY_EDIT( m_emitterDelay, TXT("Delay before the emitter starts") )
PROPERTY_EDIT( m_emitterDelayLow, TXT("Used with Delay Range to have a random delay") )
PROPERTY_EDIT( m_useEmitterDelayRange, TXT("Randomized delay of particles") )
PROPERTY_EDIT( m_useEmitterDelayOnce, TXT("Delay the particles spawn just on") )
END_CLASS_RTTI();


struct SParticleEmitterLODLevel
{
	DECLARE_RTTI_STRUCT( SParticleEmitterLODLevel );

	TDynArray< ParticleBurst >	m_burstList;				//!< List of emission bursts
	IEvaluatorFloat*			m_birthRate;				//!< Rate of spawn
	EmitterDurationSettings		m_emitterDurationSettings;	//!< Duration when spawning particles
	EmitterDelaySettings		m_emitterDelaySettings;		//!< Delay before spawning particles

	Bool						m_sortBackToFront;			//!< Sort particles every frame

	Bool						m_isEnabled;				//!< Is the emitter enabled at this LOD?

	SParticleEmitterLODLevel();
	
	SParticleEmitterLODLevel( const SParticleEmitterLODLevel& other );
	SParticleEmitterLODLevel& operator =( const SParticleEmitterLODLevel& other );

	SParticleEmitterLODLevel( SParticleEmitterLODLevel&& other );
	SParticleEmitterLODLevel& operator =( SParticleEmitterLODLevel&& other );
};

BEGIN_CLASS_RTTI( SParticleEmitterLODLevel );
PROPERTY_EDIT( m_emitterDurationSettings, TXT("Duration Settings") );
PROPERTY_EDIT( m_emitterDelaySettings, TXT("Delay Settings") );
PROPERTY_EDIT( m_burstList, TXT("List of particle emitter bursts") );
PROPERTY_INLINED( m_birthRate, TXT("Particle birth rate") );
PROPERTY_EDIT( m_sortBackToFront, TXT("Sort particles every frame (USE ONLY WHEN ABSOLUTELY NECESSARY)") );
PROPERTY( m_isEnabled );
END_CLASS_RTTI();

struct SSeedKeyValue
{
	DECLARE_RTTI_STRUCT( SSeedKeyValue );

	Uint32 m_key;
	Uint32 m_val;

	SSeedKeyValue( ){}
	SSeedKeyValue( Uint32 key, Uint32 val )
		: m_key ( key )
		, m_val ( val )
	{}
};

BEGIN_CLASS_RTTI( SSeedKeyValue );
PROPERTY( m_key );
PROPERTY( m_val );
END_CLASS_RTTI();

/// A particle emitter
class CParticleEmitter : public IParticleModule
{
	DECLARE_ENGINE_CLASS( CParticleEmitter, IParticleModule, 0 );
	
protected:
	TDynArray< SParticleEmitterLODLevel >	m_lods;

	THandle< IMaterial >			m_material;					//!< Material used to draw particles
	Uint32							m_maxParticles;				//!< Maximum particles
	TDynArray< IParticleModule* >	m_modules;					//!< Particles
	Int32							m_emitterLoops;				//!< Number of loops
	IParticleDrawer*				m_particleDrawer;			//!< Particle fragment generator
	CDecalSpawner*					m_decalSpawner;		//!< Decal spawner for death of particle
	CDecalSpawner*					m_collisionDecalSpawner;	//!< Decal spawner for collision detection
	CDecalSpawner*					m_motionDecalSpawner;		//!< Decal spawner for motion change
	Bool							m_useSubFrameEmission;		//!< Use sub-frame particle emission
	Bool							m_keepSimulationLocal;		//!< Make whole simulation in emitter space
	Float							m_windInfluence;			//!< Normalized wind influence value
	Bool							m_useOnlyWindInfluence;		//!< If set to true, particles will ignore forces other than wind
	EEnvColorGroup					m_envColorGroup;			//!< Environment color group

	Int32							m_positionX;				//!< Position in the particle editor
	Int32							m_positionY;				//!< Position in the particle editor
	Uint8							m_internalPriority;			//!< Sets rendering priority over other emitters in particle system
	
	// Only for the cooked resource
	SParticleUpdaterData*			m_updaterData;				//!< Updater data
	Uint32							m_numInitializers;
	Uint32							m_initializerSetMask;
	Uint32							m_numModifiers;
	Uint32							m_modifierSetMask;

	//!< This should be HashMap in fact, but we don't support hash maps in RTTI.
	//!< This will be removed in CP77 as emitter definition there is refactored with this in mind.
	TDynArray< SSeedKeyValue >		m_seeds;					

	IRenderResource*				m_renderResource;			//!< Render side emitter resource

#ifndef NO_EDITOR
	Int32 m_uniqueId;											//!< ID for render side emitter identification

public:
	void SetUniqueId( Int32 uniqueId ) { ASSERT( m_uniqueId == -1 ); m_uniqueId = uniqueId; }
	Int32 GetUniqueId() const { return m_uniqueId; }
#endif

public:
	//! Get emitter material
	RED_INLINE IMaterial* GetMaterial() const { return m_material.Get(); }

	//! Set emitter material
	RED_INLINE void SetMaterial( IMaterial* material ) { m_material = material; }

	//! Get the maximum particles allowed by this emitter
	RED_INLINE Uint32 GetMaxParticles() const { return m_maxParticles; }

	//! Get the list of modules in this emitter
	RED_INLINE const TDynArray< IParticleModule* >& GetModules() const { return m_modules; }

	//! Get emitter's position in graph editor
	RED_INLINE void GetPosition(Int32 &x, Int32 &y) const { x = m_positionX; y = m_positionY; }
	
	//! Get priority of rendering order
	RED_INLINE Uint8 GetInternalPriority( ) const { return m_internalPriority; }

	//! Emitter setup getters
	RED_INLINE Uint32 GetLODCount() const { return m_lods.Size(); }
	RED_INLINE const SParticleEmitterLODLevel& GetLOD( Uint32 lod ) const { return m_lods[lod]; }

#ifndef NO_EDITOR
	RED_INLINE SParticleEmitterLODLevel& GetLOD( Uint32 lod ) { return m_lods[lod]; }
#endif

	// Ideally these should be !NO_EDITOR, but we need them to load an old particle resource.
	void AddLOD();
	void RemoveLOD( Uint32 level );
	void SetLODCount( Uint32 lodCount );

	RED_INLINE Int32  GetEmitterLoops() const { return m_emitterLoops; }
	RED_INLINE Bool	IsUseSubFrameEmission() const { return m_useSubFrameEmission; }
	RED_INLINE Bool	IsKeepSimulationLocal() const { return m_keepSimulationLocal; }
	RED_INLINE EEnvColorGroup GetEnvColorGroup() const { return m_envColorGroup; }
	RED_INLINE Float	GetWindInfluence() const { return m_windInfluence; }
	RED_INLINE Bool		GetUseOnlyWindInfluence() const { return m_useOnlyWindInfluence; }

	//! Returns looped flag for this emitter - convenience
	RED_INLINE Bool IsLooped() const { return GetEmitterLoops() < 1 ; }

	//! Change emitter's position in editor graph
	RED_INLINE void SetPosition( Int32 x, Int32 y) { m_positionX = x; m_positionY = y; }

	//////////////////////////////////////////////////////////////////////////
	// Templated updater related stuff
	//! Get modifier set representation in approximated form
	void GenerateApproximatedUpdaterData( SParticleUpdaterData& updaterData ) const;

	//! Setup simulation masks based on modules list
	void SetupSimulationFlags( Uint32& modifierSetMask, Uint32& initializerSetMask, Uint32& numModifiers, Uint32& numInitializers, TDynArray< SSeedKeyValue >& seeds ) const;
		  
	//! Get proper particle type for this emitter
	EParticleType	GetParticleType() const;

	//! Get drawer data
	IParticleDrawer*	GetParticleDrawer() const { return m_particleDrawer; }

	//! Get decal spawners
	CDecalSpawner*	GetDeathDecalSpawner() const { return m_decalSpawner; }
	CDecalSpawner*	GetCollisionSpawner() const { return m_collisionDecalSpawner; }
	CDecalSpawner*	GetMotionDecalSpawner() const { return m_motionDecalSpawner; }

	//////////////////////////////////////////////////////////////////////////

public:
	CParticleEmitter();
	~CParticleEmitter();

	//! Property changed in editor
	virtual void OnPropertyPostChange( IProperty* property );

	// Called after object is loaded
	virtual void OnPostLoad();

	// Object serialization interface
	virtual void OnSerialize( IFile& file );

#ifndef NO_RESOURCE_COOKING
	// Cooking
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif

public:

	//! Reset emitter instanced
	void ResetInstances();

public:
	//! Add module to particle emitter
	IParticleModule* AddModule( CClass* moduleClass, const String& moduleName = String::EMPTY );

	//! Remove module from particle emitter
	void RemoveModule( IParticleModule* module );

	//! Move given module up/down
	Bool MoveModule( IParticleModule* module, Int32 delta );

	//! Collect meshes from mesh drawers
	Bool CollectMeshes( TDynArray< CMesh* >& meshes ) const;

	void CreateRenderResource();
	IRenderResource* GetRenderResource() const;
	void ReleaseRenderResource();

	// Old property was missing
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	SParticleUpdaterData* GetUpdaterData() const { return m_updaterData; }

private:
	void InsertSeedInOrder( Uint32 flag, Uint32 seed, TDynArray< SSeedKeyValue >& seeds ) const;

#ifndef NO_EDITOR
public:
	// Get render resouce only if created
	IRenderResource* QueryRenderResource() const { return m_renderResource; }
#endif
};

BEGIN_CLASS_RTTI( CParticleEmitter );
	PARENT_CLASS( IParticleModule );
	PROPERTY_NOT_COOKED( m_modules );
	PROPERTY( m_positionX );
	PROPERTY( m_positionY );
	PROPERTY_EDIT( m_material, TXT("Material") );
	PROPERTY_EDIT_RANGE( m_maxParticles, TXT("Max number of particles"), 1, 1000 );
	PROPERTY_EDIT_RANGE( m_emitterLoops, TXT("Number of loops, -1 for endless loop"), 0, 100000 ); // FIXME: < 1 for endless loop
	PROPERTY_INLINED( m_particleDrawer, TXT("Particle positioning and orientation mode") );
	PROPERTY_INLINED( m_decalSpawner, TXT("Decal generator - on particle dies (by life time)") );
	PROPERTY_INLINED( m_collisionDecalSpawner, TXT("Decal generator - on particle collision") );
	PROPERTY_INLINED( m_motionDecalSpawner, TXT("Decal generator - on particle moved") );
	PROPERTY_EDIT( m_useSubFrameEmission, TXT("Use sub-frame particle emission for this emitter") );
	PROPERTY_EDIT( m_keepSimulationLocal, TXT("Perform whole simulation in emitter space") );
	PROPERTY_EDIT( m_envColorGroup, TXT("Environment color group") );
	PROPERTY_EDIT( m_windInfluence, TXT("How much wind influences particles") );
	PROPERTY_EDIT( m_useOnlyWindInfluence, TXT("If set to true, particles will ignore forces other than wind") );
	PROPERTY( m_modifierSetMask );
	PROPERTY( m_numModifiers );
	PROPERTY( m_initializerSetMask );
	PROPERTY( m_numInitializers );
	PROPERTY( m_seeds );
	PROPERTY_EDIT( m_internalPriority, TXT("Internal priority") );
	PROPERTY( m_lods );
END_CLASS_RTTI();


