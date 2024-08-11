/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "sceneGrid.h"
#include "component.h"

/// Component with bounding volume
class CBoundedComponent : public CComponent, public IHierarchicalGridElement
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CBoundedComponent, CComponent );

protected:
	Box		m_boundingBox;			//!< Component bounding box

public:
	//! Get bounding box
	RED_INLINE const Box& GetBoundingBox() const { return m_boundingBox; }

public:
	CBoundedComponent();

	// Internal transform update
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	// Update world space bounding box
	virtual void OnUpdateBounds() = 0;

#ifndef NO_DATA_VALIDATION
	// Check data, can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif // NO_DATA_VALIDATION

#ifndef NO_RESOURCE_USAGE_INFO
	//! Resource usage reporting
	virtual void CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const override;
#endif

private:
	void funcGetBoundingBox( CScriptStackFrame& stack, void* result );
};

BEGIN_ABSTRACT_CLASS_RTTI( CBoundedComponent );
	PARENT_CLASS( CComponent );
	PROPERTY( m_boundingBox );
	NATIVE_FUNCTION( "GetBoundingBox", funcGetBoundingBox );
END_CLASS_RTTI();
