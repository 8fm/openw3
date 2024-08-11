/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderParticleData.h"
#include "subframeInterpolation.h"
#include "../../common/engine/dynamicDecal.h"
#include "../engine/renderResource.h"
#include "../engine/particleEmitter.h"

class CRenderMesh;

enum ERenderParticleType : Int32 ;
enum EParticleVertexDrawerType : Int32;
enum EMeshParticleOrientationMode : Int32;

struct SCurveEvaluatorScalar;

#define		COPY_FIELD_AS_INT32(dst, src)		*(Int32*)& dst = *(Int32*)& src

// Context for overall simulation
struct SSimulationContext
{
	Matrix							m_componentTransform;		//!< Transformation of the particle component
	Vector							m_targetTranslation;		//!< Simulation target
	Vector							m_cameraPosition;			//!< Actual camera position
	Vector							m_velocity;					//!< Velocity of the particle component
	Float							m_timeDelta;				//!< Simulation time step
	Float							m_timeMultiplier;			//!< Time multiplier
	Float							m_globalEmissionScale;		//!< Global emission scale
	Float							m_effectAlpha;				//!< Alpha value override by effect
	Float							m_effectSize;				//!< Size value multiplier
	CSubframeInterpolator*			m_interpolator;				//!< Interpolator for subframe emission
	Vector3							m_windVector;				//!< Wind direction and strength (not normalized)
	Vector3							m_windVectorOnly;			//!< Wind direction and strength without external forces
	class CPhysicsParticleWrapper*	m_wrapper;

	Uint32							m_lod;						//!< Which LOD to use for simulation

	SSimulationContext()
		: m_interpolator( nullptr )
		, m_wrapper( nullptr )
		, m_timeMultiplier( 1.0f )
	{}

	~SSimulationContext()
	{
		if ( m_interpolator )
		{
			delete m_interpolator;
			m_interpolator = nullptr;
		}
	}
};

// Context for modifiers application step
struct SParticleUpdateContext
{
	Matrix							m_componentTransform;	//!< Owning component
	Float							m_emitterTime;			//!< Current emitter time
	Float							m_effectAlpha;			//!< Current effect alpha
	Float							m_effectSize;			//!< Current effect size
	Float							m_timeDelta;			//!< Time step
	Bool							m_localSimulation;		//!< Is simulation local at particle component
	Vector							m_targetTranslation;	//!< Target node for some particles
	Vector							m_cameraPosition;		//!< Actual camera position
	Vector3							m_windVector;			//!< Wind direction and strength (not normalized)
	Vector3							m_windVectorOnly;
	class CPhysicsParticleWrapper*  m_wrapper;

	Uint32							m_lod;

	//! Fill to defaults
	RED_INLINE SParticleUpdateContext()
		: m_emitterTime( 0.0f )
		, m_effectAlpha( 1.0f )
		, m_timeDelta( 0.0f )
		, m_wrapper( NULL )
	{};
};

// Context for initializers application step
struct ParticleInitializationContext
{
	const CSubframeInterpolator*				m_subframeInterpolator;	//!< Interpolator for subframe emitted particles
	Vector										m_baseVelocity;			//!< Base component velocity
	Matrix										m_localToWorld;			//!< Local to world matrix for emitter
	Float										m_normalizedSpawnTime;	//!< Normalized (0-1) time of the spawn
	Float										m_normalizedSpawnDelta;	//!< Normalized (0-1) spawn time step for each spawned particle
	Float										m_particleNormalizedSpawnTime;
	Float										m_frac;
	Vector										m_cameraPosition;		//!< Camera position
	Uint32										m_particleIndex;
	class CPhysicsParticleWrapper*				m_wrapper;

	//! Fill to defaults
	RED_INLINE ParticleInitializationContext()
		: m_subframeInterpolator( NULL )
		, m_normalizedSpawnTime( 0.0f )
		, m_normalizedSpawnDelta( 0.0f )
		, m_baseVelocity( Vector::ZEROS )
	{};
};

class CDecalSpawner;

class CRenderDecalSpawner
{
protected:
	IRenderResource*				m_materialDiffuseTexture;
	IRenderResource*				m_materialNormalTexture;
	Color							m_diffuseRandomColor0;
	Color							m_diffuseRandomColor1;
	Color							m_materialSpecularColor;
	Float							m_materialSpecularScale;
	Float							m_materialSpecularBase;
	Float							m_materialSpecularity;

	Float							m_farZ;
	Float							m_nearZ;
	Float							m_depthFadePower;
	Float							m_normalFadeBias;
	Float							m_normalFadeScale;
	Bool							m_doubleSided;
	Float							m_chance;
	Float							m_spawnFrequency;

	ERenderDynamicDecalProjection	m_projectionMode;

	Float							m_decalFadeTime;
	Float							m_decalFadeInTime;
	Bool							m_projectOnlyOnStatic;
	Float							m_startScale;
	Float							m_scaleTime;
	Bool							m_useVerticalProjection;
	SCurveEvaluatorScalar*			m_size;
	SCurveEvaluatorScalar*			m_decalLifetime;
	EDynamicDecalSpawnPriority		m_spawnPriority;
	Float							m_autoHideDistance;
	ERenderDynamicDecalAtlas		m_subUVType;

	Float							m_lastSpawnTime;

public:
	CRenderDecalSpawner( const CDecalSpawner* decalSpawner );
	~CRenderDecalSpawner();

	void	SpawnDecalForParticle( const Vector3* position, const Vector3* velocity, const Vector* cameraPosition ) const;

	Bool	 AllowSpawnParticle();
	void	 MarkSpawnTime();

};

class CRenderParticleEmitter : public IRenderResource
{
	friend class CRenderPartricleBatcher;
	friend class CSimulateParticlesTask;

	struct LOD
	{
		TDynArray< ParticleBurst >	m_burstList;				//!< List of emission bursts
		SCurveEvaluatorScalar*		m_birthRate;				//!< Rate of spawn
		EmitterDurationSettings		m_emitterDurationSettings;	//!< Duration when spawning particles
		EmitterDelaySettings		m_emitterDelaySettings;		//!< Delay before spawning particles

		Bool						m_sortBackToFront;			//!< Sort particles every frame

		Bool						m_isEnabled;				//!< Is the emitter enabled at this LOD?

		LOD();
		~LOD();
	};

protected:
	// Particle simulation stuff moved from CParticleEmitter
	SParticleUpdaterData			m_updaterData;				//!< Updater data
	Uint32							m_maxParticles;				//!< Maximum particles 
	Int32							m_emitterLoops;				//!< Number of loops
	EEnvColorGroup					m_envColorGroup;			//!< Environment color group
	EParticleVertexDrawerType		m_vertexDrawerType;			//!< Vertex drawer type
	ERenderParticleType				m_particleType;				//!< Type of the simulation-level particle representation
	CRenderDecalSpawner*			m_decalSpawner;				//!< Decal spawner on particle death
	CRenderDecalSpawner*			m_collisionDecalSpawner;	//!< Decal spawner on particle death
	CRenderDecalSpawner*			m_motionDecalSpawner;		//!< Decal spawner on particle death
	TDynArray< CRenderMesh* >		m_renderMeshes;				//!< Mesh resources
	Float							m_windInfluence;			//!< Normalized wind influence. Wind doesn't move it if 0.0f.	
	Bool							m_useOnlyWindInfluence;		//!< If set to true, particles will ignore forces other than wind

	TDynArray< LOD >				m_lods;

	union UDrawerData
	{
		struct
		{
			Float	m_stretchPerVelocity;
			Float	m_oneOverBlendVelocityRange;
		} motionBlur;
		struct
		{
			Float m_texturesPerUnit;
			Bool  m_dynamicTexCoords;
			Float m_minSegmentsPer360Degrees;
		} trail;
		struct
		{
			Float				m_texturesPerUnit;
			Uint32				m_numSegments;
			Float				m_spreadRange[3];
		} beam;
		struct
		{
			EMeshParticleOrientationMode	m_orientationMode;	//<! Orientation mode for meshes
			Uint8							m_lightChannels;	//!< Light channels
		} mesh;
	} m_drawerData;

	Bool							m_useSubFrameEmission:1;	//!< Use sub-frame particle emission
	Bool							m_keepSimulationLocal:1;	//!< Make whole simulation in emitter space
	Uint8							m_internalPriority;			//!< Sets rendering priority over other emitters

#ifndef NO_EDITOR
	Int32	m_uniqueId;											//<! Same as engine side emitter. Enables easy recognition of corresponding engine and render side representations.

public:
	Int32 GetUniqueId() const { return m_uniqueId; }
	void SetUniqueId( Int32 uniqueId ) { ASSERT( m_uniqueId == -1 ); m_uniqueId = uniqueId; }
#endif

public:
	CRenderParticleEmitter();
	~CRenderParticleEmitter();

	// Compile from particle emitter resource
	static IRenderResource* Create( const CParticleEmitter* particleEmitter );

	//////////////////////////////IRenderResource/////////////////////////////
	// Get resource category
	virtual CName GetCategory() const { return CNAME( RenderParticleEmitter ); }

	// Calculate video memory used by resource
	virtual Uint32 GetUsedVideoMemory() const { return 0; }	// Drey TODO
	//////////////////////////////////////////////////////////////////////////

public:
	//! Get type of particle
	virtual ERenderParticleType GetParticleType() const { return m_particleType; }

	//! Get vertex type
	virtual EParticleVertexDrawerType GetVertexType() const { return m_vertexDrawerType; }

	//! Get max particles
	Uint32 GetMaxParticles() const { return m_maxParticles; }

	RED_INLINE Uint32 GetNumInitializers() const { return m_numInitializers; }
	const TDynArray< SSeedKeyValue >& GetSeeds() const { return m_seedsInitializers; };

	//! Get priority of rendering order
	RED_INLINE Uint8 GetInternalPriority( ) const { return m_internalPriority; }

	//! Get emitter duration
	Float GetEmitterDuration( Uint32 lod ) const { return m_lods[lod].m_emitterDurationSettings.m_emitterDuration; }
	Float GetEmitterDurationLow( Uint32 lod ) const { return m_lods[lod].m_emitterDurationSettings.m_emitterDurationLow; }
	Float GetEmitterDelay( Uint32 lod ) const { return m_lods[lod].m_emitterDelaySettings.m_emitterDelay; }
	Float GetEmitterDelayLow( Uint32 lod ) const { return m_lods[lod].m_emitterDelaySettings.m_emitterDelayLow; }

#ifndef NO_EDITOR
	Bool CheckBufferCompatibility( CRenderParticleEmitter* other );
#endif

	SParticleUpdaterData& GetUpdaterData() { return m_updaterData; }

protected:
	void CompileModifiersList();
	void CompileInitializersList();
	
protected:

	enum
	{
		PARTICLE_KILLED				= FLAG(0),		//<! Particle killed by some murder (modifier is the suspect)
		PARTICLE_COLLIDED			= FLAG(1),		//<! Particle is colliding with something
		PARTICLE_DEATH				= FLAG(2),			//<! Particle died in natural way (by life time)
		PARTICLE_MOVED				= FLAG(3) 
	};

	template< typename PARTICLE_TYPE >
	void PerformActions( char* particle , Uint32 actionsPerformed, const SParticleUpdateContext& updateContext ) const;

	typedef void (*ModifierFunc)( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	ModifierFunc*	m_modifierFunctions;
	Uint32			m_numModifiers;
	Uint32			m_modifierSetMask;

	typedef void (*InitializerFunc)( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	InitializerFunc*	 m_initializerFunctions;
	Uint32				 m_numInitializers;
	Uint32				 m_initializerSetMask;

	TDynArray< SSeedKeyValue >  m_seedsInitializers;

public:
	//! Simulate particle emitter ( using instanced data ). Returns false if emitter has finished.
	virtual Bool Simulate( IParticleData* data, const SSimulationContext& simulationContext, Bool onScreen ) const;

	template< typename PARTICLE_TYPE >
	Bool PerformSimulation( CParticleData< PARTICLE_TYPE >* data, const SSimulationContext& simulationContext, Bool onScreen ) const;

	template< typename PARTICLE_TYPE, typename PARTICLE_VERTEX_TYPE >
	Bool UpdateParticles( CParticleBuffer< PARTICLE_TYPE >& buffer, const SParticleUpdateContext& updateContext, Bool onScreen ) const;

	// Spawn N particles during the given time range
	template< typename PARTICLE_TYPE >
	void SpawnParticles( CParticleData< PARTICLE_TYPE >* data, const SSimulationContext& simulationContext, Uint32 count, Float startTime, Float spawnTimeRange ) const;

	template< typename PARTICLE_TYPE, typename PARTICLE_VERTEX_TYPE >
	RED_INLINE void GenerateVertexData( const PARTICLE_TYPE* RESTRICT particle ) const;

protected:
	//////////////////////////////////////////////////////////////////////////
	// Static initializer methods
	//////////////////////////////////////////////////////////////////////////
	static void InitAlpha( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitAlphaRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitColor( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitColorRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitLifeTime( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitLifeTimeRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitPosition( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitPositionRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitRotation( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitRotationRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitRotation3D( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitRotation3DRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitRotationRate( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitRotationRateRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitRotationRate3D( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitRotationRate3DRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitSize( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitSizeRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitSize3D( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitSize3DRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitSpawnBox( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitSpawnCircle( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitSpawnSphere( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitVelocity( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitVelocityRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitVelocityInherit( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitVelocityInheritRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitVelocitySpread( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitVelocitySpreadRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitAnimationFrame( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );
	static void InitAnimationFrameRandom( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	static void InitCollisionSpawn( char* particle, const SParticleUpdaterData& updateData, const ParticleInitializationContext& updateContext, Generator< FastRand >& generator );

	//////////////////////////////////////////////////////////////////////////
	// Static update methods
	//////////////////////////////////////////////////////////////////////////

	static void VelocityAbsolute( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void VelocityModulate( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );

	static void AccelerationLocal( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void AccelerationWorld( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );

	static void ColorAbsolute( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void ColorModulate( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );

	static void AlphaAbsolute( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void AlphaModulate( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );

	//////////////////////////////////////////////////////////////////////////
	static void RotationAbsolute_1A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void RotationModulate_1A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );

	static void RotRateAbsolute_1A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void RotRateModulate_1A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );

	static void SizeAbsolute_2D( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void SizeModulate_2D( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	//////////////////////////////////////////////////////////////////////////
	static void RotationAbsolute_3A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void RotationModulate_3A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );

	static void RotRateAbsolute_3A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void RotRateModulate_3A( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );

	static void SizeAbsolute_3D( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void SizeModulate_3D( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	//////////////////////////////////////////////////////////////////////////

	static void AnimationFrame_Speed( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void AnimationFrame_Life( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );

	static void Turbulize( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void TargetNode( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void TargetPosition( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );

	static void EffectAlpha( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void Collision( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex , Uint32& actionPerformed );
	static void AlphaDistance( char* particle, const SParticleUpdaterData& updateData, const SParticleUpdateContext& updateContext, Float normalizedLife, Uint32 particleIndex, Uint32& actionPerformed );
};


template< typename PARTICLE_TYPE, typename PARTICLE_VERTEX_TYPE >
void CRenderParticleEmitter::GenerateVertexData( const PARTICLE_TYPE* particle ) const
{
	//HALT( TXT("Undefined CRenderParticleEmitter::GenerateVertexData called") );
}

template<> void CRenderParticleEmitter::GenerateVertexData< SimpleParticle, ParticleSimpleVertex >( const SimpleParticle* RESTRICT particle ) const;
template<> void CRenderParticleEmitter::GenerateVertexData< SimpleParticle, ParticleMotionVertex >( const SimpleParticle* RESTRICT particle ) const;
template<> void CRenderParticleEmitter::GenerateVertexData< MeshParticle, void >( const MeshParticle* RESTRICT particle ) const;