/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "boundedComponent.h"
#include "../core/scriptStackFrame.h"
#include "../core/dataError.h"
#include "../core/resourceUsage.h"
#include "entity.h"
#include "layer.h"
#include "utils.h"

IMPLEMENT_ENGINE_CLASS( CBoundedComponent )

CBoundedComponent::CBoundedComponent()
	: m_boundingBox( Vector::ZERO_3D_POINT, 0.1f )
{
}

void CBoundedComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	// Pass to base class
	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	// Recalculate bounding box
	OnUpdateBounds();
}

void CBoundedComponent::OnUpdateBounds()
{
	m_boundingBox = Box( GetWorldPosition(), 0.1f );
}

#ifndef NO_DATA_VALIDATION
void CBoundedComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );

	if ( IsAttached() && GetEntity()->WasVisibleLastFrame() && !isInTemplate )
	{
		TDynArray< const CResource* > resources;
		GetResource( resources );

		if ( resources.Size() == 0 || resources[0] == nullptr )
		{
			resources.Clear();
			resources.PushBack( CResourceObtainer::GetResource( this ) );
		}

		// Make sure bounding box is not NULL
		const Vector size = m_boundingBox.CalcSize();
		if ( Vector::Near3( size, Vector::ZEROS ) )
		{
			DATA_HALT( DES_Minor, resources[0], TXT("World"), TXT("Component '%ls' in entity has empty bounding box."), GetName().AsChar() );
		}

		// Bounding box is not calculated or inverted
		if ( size.X < 0.0f || size.Y < 0.0f || size.Z < 0.0f )
		{
			DATA_HALT( DES_Major, resources[0], TXT("World"), TXT("Component '%ls' in entity has empty bounding box."), GetName().AsChar() );
		}
	}
}
#endif // NO_DATA_VALIDATION

#ifndef NO_RESOURCE_USAGE_INFO
void CBoundedComponent::CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const
{
	TBaseClass::CollectResourceUsage( collector, isStremable );

	collector.ReportBoundingBox( GetBoundingBox() );
}
#endif

void CBoundedComponent::funcGetBoundingBox( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRUCT( Box, GetBoundingBox() );
}
