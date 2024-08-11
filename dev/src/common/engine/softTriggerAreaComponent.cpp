/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "softTriggerAreaComponent.h"
#include "renderFragment.h"
#include "game.h"
#include "world.h"
#include "renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CSoftTriggerAreaComponent );

CSoftTriggerAreaComponent::CSoftTriggerAreaComponent()
	: m_compiledBevelDebugMesh( NULL )
	, m_outerTriggerObject( NULL )
	, m_outerIncludedChannels( 0 )
	, m_outerExcludedChannels( 0 )
{
}

void CSoftTriggerAreaComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// attach
	RecreateOuterShape();
}

void CSoftTriggerAreaComponent::RecreateOuterShape()
{
	// release the debug mesh
	SAFE_RELEASE( m_compiledBevelDebugMesh );

	// remove the current object
	if ( NULL != m_outerTriggerObject )
	{
		m_outerTriggerObject->Remove();
		m_outerTriggerObject->Release();
		m_outerTriggerObject = NULL;
	}

	// if we have a valid world we are attached to create the new beveled shape
	const Float bevelRadius = CalcBevelRadius();
	const Float bevelRadiusVertical = CalcVerticalBevelRadius();
	if ( IsAttached() && (bevelRadius > 0.0f || bevelRadiusVertical > 0.0f) )
	{
		// create the trigger object description
		CTriggerObjectInfo initInfo;
		initInfo.m_callback = this;
		initInfo.m_bevelRadius = Max< Float >( 0.1f, bevelRadius); // there's a lower limit for normal beveling if it's enabled
		initInfo.m_bevelRadiusVertical = Max< Float >( 0.0f, bevelRadiusVertical );
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
		initInfo.m_debugName = GetFriendlyName() + TXT("_Outside");
#endif
		initInfo.m_includeChannels = m_outerIncludedChannels;
		initInfo.m_excludeChannels = m_outerExcludedChannels;
		initInfo.m_shape = const_cast< CAreaShape* >( &GetCompiledShape() );
		initInfo.m_localToWorld = GetLocalToWorld();
		initInfo.m_component = this;

		// insert the trigger object for the outer area into the trigger system
		CWorld* attachedWorld = GetWorld();
		m_outerTriggerObject = attachedWorld->GetTriggerManager()->CreateTrigger( initInfo );
	}
}

void CSoftTriggerAreaComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// detach outer trigger
	if ( NULL != m_outerTriggerObject )
	{
		m_outerTriggerObject->Remove();
		m_outerTriggerObject->Release();
		m_outerTriggerObject = NULL;
	}

	// release the debug mesh
	SAFE_RELEASE( m_compiledBevelDebugMesh );
}

void CSoftTriggerAreaComponent::OnAreaShapeChanged()
{
	TBaseClass::OnAreaShapeChanged();

	RecreateOuterShape();
}

void CSoftTriggerAreaComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CSoftTriggerAreaComponent );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	// force the debug mesh to be recompiled every time we move
	if ( !GGame->IsActive() )
	{
		SAFE_RELEASE( m_compiledBevelDebugMesh );
	}

	// move the outer shape
	if ( NULL != m_outerTriggerObject )
	{
		m_outerTriggerObject->SetPosition( GetLocalToWorld() );
	}
}

void CSoftTriggerAreaComponent::OnPropertyPostChange( CProperty* prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	// vertical beveling changed
	if ( prop->GetName() == TXT("verticalGrowFactor") )
	{
		if ( !GGame->IsActive() )
		{
			SAFE_RELEASE( m_compiledBevelDebugMesh );
			RecreateOuterShape();
		}
	}
}

void CSoftTriggerAreaComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	const Float bevelRadius = CalcBevelRadius();
	if ( flag == SHOW_AreaShapes && bevelRadius > 0.0f )
	{
		// recompile
		if ( NULL == m_compiledBevelDebugMesh )
		{
			Matrix bevelMatrix = GetLocalToWorld();
			bevelMatrix.SetTranslation( 0,0,0 );

			const Float bevelRadiusVertical = Max< Float >( 0.0f, CalcVerticalBevelRadius() );
			m_compiledBevelDebugMesh = GetCompiledShape().CompileDebugMesh( bevelRadius, bevelRadiusVertical, bevelMatrix );
		}

		// draw
		if ( NULL != m_compiledBevelDebugMesh )
		{
			Matrix renderMatrix;
			renderMatrix.SetIdentity();
			renderMatrix.SetTranslation( GetWorldPosition() );

			// Hit proxy
			if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
			{
#ifndef NO_COMPONENT_GRAPH
				new ( frame ) CRenderFragmentDebugMesh( frame, renderMatrix, m_compiledBevelDebugMesh, GetHitProxyID() ); // draw as hitproxy mesh
#endif
			}
			else
			{
				new ( frame ) CRenderFragmentDebugMesh( frame, renderMatrix, m_compiledBevelDebugMesh, true ); // draw as transparent mesh
			}
		}
	}
}

Float CSoftTriggerAreaComponent::CalcBevelRadius() const
{
	return 0.0f;
}

Float CSoftTriggerAreaComponent::CalcVerticalBevelRadius() const
{
	return 0.0f;
}

const Float CSoftTriggerAreaComponent::CalcPenetrationFraction( const Vector& point, Vector* outClosestPoint ) const
{
	Float result = 0.0f;

	const Float bevelRadius = CalcBevelRadius();
	const Float bevelRadiusVertical = CalcVerticalBevelRadius();
	if ( bevelRadius > 0.0f || bevelRadiusVertical > 0.0f )
	{
		// estimate maximum search distance
		const Float maxSearchDistance = Max< Float >( bevelRadius, bevelRadiusVertical );

		// calcualte closest point on the inner shape
		Vector closestPoint;
		Float closestDistance;
		if ( FindClosestPoint( point, maxSearchDistance, closestPoint, closestDistance ) )
		{
			if( outClosestPoint ) *outClosestPoint = closestPoint;
			result = Max<Float>( 0.0f, 1.0f - (closestDistance / maxSearchDistance) );
		}
	}
	else if ( TestPointOverlap( point ) )
	{
		if( outClosestPoint ) *outClosestPoint = point;
		result = 1.0f;  // inside
	}

	return m_invertPenetrationFraction ? ( 1.0f - result ) : result;
}

void CSoftTriggerAreaComponent::OnActivatorEntered( const class ITriggerObject* object, const class ITriggerActivator* activator )
{
	if ( object == m_outerTriggerObject )
	{
		CComponent* component = activator ? activator->GetComponent() : NULL;
		EnteredOuterArea( component, activator );
		return;
	}
	
	TBaseClass::OnActivatorEntered( object, activator );
}

void CSoftTriggerAreaComponent::OnActivatorExited( const class ITriggerObject* object, const class ITriggerActivator* activator )
{
	if ( object == m_outerTriggerObject )
	{
		CComponent* component = activator ? activator->GetComponent() : NULL;
		ExitedOuterArea( component, activator );
		return;
	}

	TBaseClass::OnActivatorExited( object, activator );
}

void CSoftTriggerAreaComponent::EnteredOuterArea( CComponent* component, const class ITriggerActivator* activator )
{
	// called when something entered out outer (beveled) area
	// NULL is passed for camera
}

void CSoftTriggerAreaComponent::ExitedOuterArea( CComponent* component, const class ITriggerActivator* activator )
{
	// called when something exited out outer (beveled) area
	// NULL is passed for camera
}