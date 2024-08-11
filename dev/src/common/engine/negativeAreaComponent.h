/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "areaComponent.h"

/// specialized class used to cut holes in trigger areas (and any other areas to)
class CNegativeAreaComponent : public CAreaComponent
{
	DECLARE_ENGINE_CLASS( CNegativeAreaComponent, CAreaComponent, 0 );

public:
	CNegativeAreaComponent();

	// Get contour rendering color
	virtual Color CalcLineColor() const;

	// Transformation
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

#ifndef NO_EDITOR
	// End vertex edit mode
	virtual void OnEditorEndVertexEdit();

	// Insert editor vertex
	virtual Bool OnEditorVertexInsert( Int32 edge, const Vector& wishedPosition, Vector& allowedPosition, Int32& outInsertPos );

	// Editor vertex deleted
	virtual Bool OnEditorVertexDestroy( Int32 vertexIndex );

	// Editor vertex was moved
	virtual void OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition );
#endif

	// Spawning
	virtual void OnSpawned( const SComponentSpawnInfo& spawnInfo );

#ifndef NO_DATA_VALIDATION
	// Check data, can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif	// NO_DATA_VALIDATION

private:
	//! Invalidate all normal area components that touch this negative area
	void InvalidateTouchingAreas();
};

BEGIN_CLASS_RTTI( CNegativeAreaComponent );
	PARENT_CLASS( CAreaComponent );
END_CLASS_RTTI();
