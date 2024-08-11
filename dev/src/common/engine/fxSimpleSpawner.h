/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "fxSpawner.h"

/// Particle spawner on water surface
class CFXSimpleSpawner : public IFXSpawner
{
	DECLARE_ENGINE_CLASS( CFXSimpleSpawner, IFXSpawner, 0 );

public:
	CFXSimpleSpawner();

	//! Calculate spawn position and rotation, returns false if spawning is not possible
	virtual Bool Calculate( CEntity* parentEntity, Vector &position, EulerAngles &rotation, Uint32 pcNumber ) override;

	//! Called after node is spawned, good place for final updates
	virtual Bool PostSpawnUpdate( CEntity* parentEntity, CComponent* createdComponent, Uint32 pcNumber ) override;

	//! Should particle spawner spawn multiple particle components?
	virtual Uint32 AmountOfPC( CEntity* parentEntity, TDynArray< Uint32 > &indices ) override;

	//! Check if track item uses component with given name
	virtual Bool UsesComponent( const CName& componentName ) const override { return false; }

	//! Should position calculated be final?
	virtual Bool IsPositionFinal() const override { return false; }

private:
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;

	TDynArray< CName > m_slotNames;
	TDynArray< CName > m_boneNames;

	Vector      m_relativePos;
	EulerAngles m_relativeRot;

	TDynArray< const EntitySlot* > m_cachedSlots;
	CEntity* m_cachedEntity;
};

BEGIN_CLASS_RTTI( CFXSimpleSpawner )
	PARENT_CLASS( IFXSpawner )
	PROPERTY_EDIT( m_slotNames, TXT("List of slot names to spawn effect at") )
	PROPERTY_EDIT( m_boneNames, TXT("List of bone names to spawn effect at") )
	PROPERTY_EDIT( m_relativePos, TXT("Position relative to slot") )
	PROPERTY_EDIT( m_relativeRot, TXT("Rotation relative to slot") )
END_CLASS_RTTI()
