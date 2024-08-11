/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "drawableComponent.h"
#include "environmentAreaParams.h"
#include "renderFrame.h"

enum ETransparencySortGroup : CEnum::TValueType;
enum EEnvAutoHideGroup : CEnum::TValueType;

struct SSimulationContextUpdate
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_ParticlesRendering );

	Vector	 						m_targetTranslation;		//!< Simulation target translation
	Float							m_globalEmissionScale;		//!< Global emission scale
	Float							m_effectAlpha;				//!< Alpha value override by effect
	Float							m_effectSize;				//!< Size value multiplier
	Float							m_timeMultiplier;			//!< Size value multiplier
	Vector3							m_windVector;				//!< Wind vector
	Vector3							m_windVectorOnly;			//!< Wind vector without
	class CPhysicsParticleWrapper*	m_wrapper;

	SSimulationContextUpdate()
		: m_wrapper( NULL )
		, m_timeMultiplier( 1.0f )
	{}
};


/// A particle system played on the level
class CParticleComponent : public CDrawableComponent
						 , public ILODable
{
	DECLARE_ENGINE_CLASS( CParticleComponent, CDrawableComponent, 0 );

	struct SEffectInfo
	{
		Float	m_alpha;
		Float	m_size;

		SEffectInfo() : m_alpha( 1.0f ), m_size( 1.0f ) {}
	};



protected:
	THandle< CParticleSystem >		m_particleSystem;			//!< Used particle system
	TSoftHandle< CParticleSystem >	m_particleSystemResource;	//!< Particle system resource to be loaded asynchronously
	ETransparencySortGroup			m_transparencySortGroup;	//!< Transparency sort group
	Float							m_globalEmissionScale;		//!< Global emission scale for this particle system
	SEffectInfo						m_effectInfo;				//!< Normalized time of the effect this particle system is played in
	EEnvAutoHideGroup				m_envAutoHideGroup;			//!< Environment auto hide group
	THandle< CNode >				m_simulationTarget;			//!< Optional target for the simulation
	Int32							m_targetBoneIndex;			//!< Optional target bone for the simulation

	Double							m_lastSimulationTime;		//!< Last simulation time
	class CPhysicsParticleWrapper*	m_wrapper;

	Bool							m_updateSimulationContext			: 1;	// Should update simulation context
	Bool							m_isRegisteredForComponentLODding	: 1;	// Is registered in component LOD system?

#ifndef NO_EDITOR
	Float							m_timeMultiplier;
	THandle< CBitmapTexture >		m_icon;						//!< Sprite icon to diplay
public:
	void							SetTimeMultiplier( Float multiplier ) { m_timeMultiplier = multiplier; m_updateSimulationContext = true; }
	
#endif
	// Get sprite icon
	virtual CBitmapTexture* GetSpriteIcon() const;
	
	// Get sprite rendering color
	virtual Color CalcSpriteColor() const;

public:
	//! Get the instanced particle system
	RED_INLINE CParticleSystem* GetParticleSystem() const { return m_particleSystem.Get(); }

	//! Get the global emission scale
	RED_INLINE Float GetGlobalEmissionScale() const { return m_globalEmissionScale; }

	//! Get transparency sort group
	RED_INLINE ETransparencySortGroup GetTransparencySortGroup() const { return m_transparencySortGroup; }

	//! Is using environment auto hide parameters
	RED_INLINE Bool IsUsingEnvAutoHideParams() const { return EAHG_None != m_envAutoHideGroup; }

	//! Get environment auto hide group
	RED_INLINE EEnvAutoHideGroup GetEnvAutoHideGroup() const { return m_envAutoHideGroup; }

	//! Get simulation time
	RED_INLINE Double GetLastSimulationTime() const { return m_lastSimulationTime; }

	//! Try to use given resource passed from editor during spawning (ex. mesh for StaticMesh template)
	virtual Bool TryUsingResource( CResource * resource );

	// Override the internal particle system
	virtual void SetResource( CResource* resource ) override;
	virtual void GetResource( TDynArray< const CResource* >& resources ) const override;

	virtual Float GetAutoHideDistance() const;
	virtual Float GetDefaultAutohideDistance() const { return 20.0f; }
	virtual Float GetMaxAutohideDistance() const { return 300.0f; }

public:
	CParticleComponent();
	~CParticleComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );
	virtual void OnDestroyed();
	virtual void OnSerialize( IFile& file );
	virtual void OnUpdateBounds();
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;
	virtual void OnTickPostPhysics( Float timeDelta ) override;
	virtual void OnTick( Float timeDelta ) override;

	// Should we update transform this node automatically when parent is update transformed /
	virtual bool UsesAutoUpdateTransform();

	virtual Bool ShouldPerformProxyAttachesOnDissolve() const override { return false; }

public:
	//! Change particle system
	void SetParticleSystem( CParticleSystem* system );

	//! Change particle system asynchronously
	void SetParticleSystemAsync( TSoftHandle< CParticleSystem > resource );

	//! Reset effect
	void Reset();

	//! Set particles global emission scale
	void SetGlobalEmissionScale( Float scale );

	//! Set effect time
	void SetEffectInfo( const SEffectInfo& info );

	//! Set target node
	void SetTarget( const CNode* target, const CName& bone = CName::NONE );

	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	// Set effect parameter value
	virtual Bool SetEffectParameterValue( CName paramName, const EffectParameterValue &value ) override;
	
	// Initialize rendering proxy
	virtual void OnInitializeProxy();

protected:
	// ILODable
	void UpdateLOD( ILODable::LOD newLOD, CLODableManager* manager ) override;
	ILODable::LOD ComputeLOD( CLODableManager* manager ) const override;

protected:
	//! Simulation step
	void SimulationUpdateStep();

	//! Internal get wind function
	Vector3 GetWindAtPos( Bool useforces = true );

	void SetSimulationContext( SSimulationContextUpdate& simulationContext );

	Bool CanAttachToRenderScene() const;
public:

#ifndef NO_DEBUG_PAGES
	void GetRenderSideBoundingBox( Box& box ) const;
#endif

#ifndef NO_RESOURCE_USAGE_INFO
	//! Resource usage reporting
	virtual void CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const override;
#endif

	virtual void RefreshRenderProxies();

	void CreatePhysicalWrapper();

};

BEGIN_CLASS_RTTI( CParticleComponent );
PARENT_CLASS( CDrawableComponent );
PROPERTY_EDIT( m_particleSystem, TXT("Particle System") );
PROPERTY_EDIT( m_transparencySortGroup, TXT("Transparency sort group") );
PROPERTY_EDIT( m_envAutoHideGroup, TXT("Environment auto hide group") );
END_CLASS_RTTI();
