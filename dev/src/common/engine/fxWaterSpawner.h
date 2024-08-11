/*
 * Copyright © 201 CD Projekt Red. All Rights Reserved.
 */

#pragma once

/// Particle spawner on water surface
class CFXWaterSpawner : public IFXSpawner
{
	DECLARE_ENGINE_CLASS( CFXWaterSpawner, IFXSpawner, 0 );

public:
	CFXWaterSpawner();

	//! Calculate spawn position and rotation, returns false if spawning is not possible
	virtual Bool Calculate( CEntity* parentEntity, Vector &position, EulerAngles &rotation, Uint32 pcNumber );

	//! Called after node is spawned, good place for final updates
	virtual Bool PostSpawnUpdate( CEntity* parentEntity, CComponent* createdComponent, Uint32 pcNumber ) { return true; }

	//! Should particle spawner spawn multiple particle components?
	virtual Uint32 AmountOfPC( CEntity* parentEntity, TDynArray<Uint32> &indices );

	//! Check if track item uses component with given name
	virtual Bool UsesComponent( const CName& componentName ) const { return false; }

	//! Should position calculated be final?
	virtual Bool IsPositionFinal() const { return true; }

private:
	//! Linar cast down for looking water
	Bool TraceWater( const Vector& position, Vector& target );
};

BEGIN_CLASS_RTTI( CFXWaterSpawner )
	PARENT_CLASS( IFXSpawner )
END_CLASS_RTTI()
