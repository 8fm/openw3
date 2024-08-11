/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxSpawner.h"

/// Derive spawn position from entity component
class CFXSpawnerComponent : public IFXSpawner
{
	DECLARE_ENGINE_CLASS( CFXSpawnerComponent, IFXSpawner, 0 );

protected:
	Vector			m_relativePosition;			//!< Relative position to apply on spawn
	EulerAngles		m_relativeRotation;			//!< Relative rotation to apply on spawn
	Float			m_percentage;				//!< Percentage of rigid bodies in which particle emitters are spawned
	CName			m_componentName;			//!< Can be component or slot name
	CName			m_parentSlotName;			//!< Slot in the parent object
	Bool			m_copyRotation;				//!< Copy rotation from the component
	Bool			m_freePositionAxisX;		//!< Free position X
	Bool			m_freePositionAxisY;		//!< Free position Y
	Bool			m_freePositionAxisZ;		//!< Free position Z
	Bool			m_freeRotation;				//!< Do not lock rotation to the parent slot
	Bool			m_attach;					//!< Attach to target

public:
	CFXSpawnerComponent();

	//! Calculate spawn position and rotation, returns false if spawning is not possible
	virtual Bool Calculate( CEntity* parentEntity, Vector &position, EulerAngles &rotation, Uint32 pcNumber );

	//! Follow movement component
	virtual Bool PostSpawnUpdate( CEntity* parentEntity, CComponent* createdComponent, Uint32 pcNumber );

	//! Should particle spawner spawn multiple particle components?
	virtual Uint32 AmountOfPC( CEntity* parentEntity, TDynArray<Uint32> &ind );

	//! Check if track item uses component with given name
	virtual Bool UsesComponent( const CName& componentName ) const;

	//! Should position calculated be final?
	virtual Bool IsPositionFinal() const { return false; }

	//! Exteact the name of a component
	RED_INLINE const CName& GetParentComponentName() const { return m_componentName; } 

	//! Exteact the entity template 
	const CEntityTemplate* GetEntityTemplate() const;
};

BEGIN_CLASS_RTTI( CFXSpawnerComponent )
	PARENT_CLASS( IFXSpawner )
	PROPERTY_CUSTOM_EDIT( m_componentName, TXT("Name of the entity component to spawn at"), TXT("EntityComponentAndSlotsList") );
	PROPERTY_EDIT( m_copyRotation, TXT("Copy rotation from component") );
	PROPERTY_EDIT( m_attach, TXT("Attach to component") );
	PROPERTY_EDIT( m_relativeRotation, TXT("Attach with this relative rotation") );
	PROPERTY_EDIT( m_relativePosition, TXT("Attach with this relative position") );
	PROPERTY_EDIT( m_parentSlotName, TXT("Name of the parent slot") );
	PROPERTY_EDIT( m_freePositionAxisX, TXT("Free position axis X") );
	PROPERTY_EDIT( m_freePositionAxisY, TXT("Free position axis Y") );
	PROPERTY_EDIT( m_freePositionAxisZ, TXT("Free position axis Z") );
	PROPERTY_EDIT( m_freeRotation, TXT("Do not lock rotation to the parent slot") );
	PROPERTY_EDIT_RANGE( m_percentage, TXT("Percentage of rigid bodies in which particles are spawn"), 0.01f, 1.0f );
END_CLASS_RTTI()
