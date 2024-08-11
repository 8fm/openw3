/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "../core/resource.h"
#include "drawableComponent.h"

class CParticleEmitter;
class CParticleComponent;

struct SParticleSystemLODLevel
{
	DECLARE_RTTI_STRUCT( SParticleSystemLODLevel );

	Float						m_distance;					//!< Distance at which this LOD level starts to show up

	SParticleSystemLODLevel()
		: m_distance( 0.0f )
	{}
};

BEGIN_CLASS_RTTI( SParticleSystemLODLevel );
PROPERTY_EDIT( m_distance, TXT("Distance at which this LOD shows up") );
END_CLASS_RTTI();

/// Particle system - group of emitters
class CParticleSystem : public CResource					 
{
	DECLARE_ENGINE_RESOURCE_CLASS( CParticleSystem, CResource, "w2p", "Particle System" );

protected:
	TDynArray< SParticleSystemLODLevel >	m_lods;
	TDynArray< CParticleEmitter* >			m_emitters;					//!< Emitters in the system

	Float									m_autoHideDistance;			//!< Distance behind which particle system won't be rendered
	Float									m_autoHideRange;			//!< Range over which particle system will be smoothly hidden

public:
	Color									m_previewBackgroundColor;	//!< Clear color of the preview panel
	Bool									m_previewShowGrid;			//!< Show the grid in the preview panel
	Bool									m_visibleThroughWalls;		//!< Should this particle system be visible thorugh the walls
	Float									m_prewarmingTime;			//!< Prewarming the emitter

	ERenderingPlane							m_renderingPlane;			//!< Tells if emitter should be rendered in the background (clouds etc).

#ifndef NO_EDITOR
	Int32 m_nextUniqueId;	//!< Assigning uniqueID to all emitters, for further recognition with render side emitter representation
	void AssignUniqueId( CParticleEmitter* emitter );
#endif

public:
	//! Get list of particle system emitters
	RED_INLINE const TDynArray< CParticleEmitter* >& GetEmitters() const { return m_emitters; }

	RED_INLINE Float GetPrewarmingTime() const { return m_prewarmingTime; }

public:
	CParticleSystem();

	// Object serialization interface
	virtual void OnSerialize( IFile& file );

	// Called after object is loaded
	virtual void OnPostLoad();

#ifndef NO_RESOURCE_COOKING

	virtual void OnCook( class ICookerFramework& cooker ) override;

#endif
public:
	//! Add emitter to particle system
	CParticleEmitter* AddEmitter( CClass* emitterClass, const String& emitterName = String::EMPTY );
	Bool AddEmitter( CParticleEmitter* emitter );

	//! Clone emitter
	CParticleEmitter* CloneEmitter( CParticleEmitter* emitterToClone, const String& emitterName = String::EMPTY );

	//! Remove emitter from particle system
	void RemoveEmitter( CParticleEmitter* emitter );

	//! Reset all particle components using this system
	void ResetInstances();

	void RecompileEmitters();


	RED_INLINE Uint32 GetLODCount() const { return m_lods.Size(); }
	RED_INLINE const SParticleSystemLODLevel& GetLOD( Uint32 level ) const { return m_lods[level]; }

#ifndef NO_EDITOR
	RED_INLINE SParticleSystemLODLevel& GetLOD( Uint32 level ) { return m_lods[level]; }
#endif

	// Ideally, these would be !NO_EDITOR, but we need them (well, we need AddLOD) for loading old particle resources.
	void AddLOD();
	void RemoveLOD( Uint32 level );

	//! Get auto hide distance
	RED_INLINE Float GetAutoHideDistance() const { return m_autoHideDistance; }

	//! Get auto hide range
	RED_INLINE Float GetAutoHideRange() const { return m_autoHideRange; }
	
	//! Get rendering plane
	RED_INLINE ERenderingPlane GetRenderingPlane() const { return m_renderingPlane; }

	//! Get visibility through walls property
	RED_INLINE Bool IsVisibleThroughWalls() const { return m_visibleThroughWalls; }
};

BEGIN_CLASS_RTTI( CParticleSystem );
	PARENT_CLASS( CResource );
	PROPERTY_EDIT( m_previewBackgroundColor, TXT("Clear color of the preview panel") );
	PROPERTY_EDIT( m_previewShowGrid, TXT("Show the grid in the preview panel") );
	PROPERTY_EDIT( m_visibleThroughWalls, TXT("Should it be visible throguh the walls") );
	PROPERTY_EDIT( m_prewarmingTime, TXT("Prewarming the emitter") );
	PROPERTY( m_emitters );
	PROPERTY( m_lods );
	PROPERTY_EDIT( m_autoHideDistance, TXT("Distance behind which particle system won't be rendered") );
	PROPERTY_EDIT( m_autoHideRange, TXT("Range over which particle system will be smoothly hidden") );
	PROPERTY_EDIT( m_renderingPlane, TXT( "Rendering plane" ) );
END_CLASS_RTTI();
