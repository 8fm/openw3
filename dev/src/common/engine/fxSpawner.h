/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Particle spawner
class IFXSpawner : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IFXSpawner, CObject );

public:
	IFXSpawner();

	//! Calculate spawn position and rotation, returns false if spawning is not possible
	virtual Bool Calculate( CEntity* parentEntity, Vector &position, EulerAngles &rotation, Uint32 pcNumber ) = 0;

	//! Called after node is spawned, good place for final updates
	virtual Bool PostSpawnUpdate( CEntity* parentEntity, CComponent* createdComponent, Uint32 pcNumber ) = 0;

	//! Should particle spawner spawn multiple particle components?
	virtual Uint32 AmountOfPC( CEntity* parentEntity, TDynArray<Uint32> &indices ) = 0;

	//! Check if track item uses component with given name
	virtual Bool UsesComponent( const CName& componentName ) const = 0;

	//! Should position calculated be final?
	virtual Bool IsPositionFinal() const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( IFXSpawner );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();
