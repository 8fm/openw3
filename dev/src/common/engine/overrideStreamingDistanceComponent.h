/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "component.h"

class COverrideStreamingDistanceComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( COverrideStreamingDistanceComponent, CComponent, 0 );

	Uint32 m_streamingDistanceMinOverride;

public:
	COverrideStreamingDistanceComponent() : m_streamingDistanceMinOverride( 0 ) {}

	//! Returns the minimum streaming distance for this component
	virtual Uint32 GetMinimumStreamingDistance() const override { return m_streamingDistanceMinOverride; }
};

BEGIN_CLASS_RTTI( COverrideStreamingDistanceComponent )
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT( m_streamingDistanceMinOverride, TXT( "Minimal streaming distance." ) );
END_CLASS_RTTI();
