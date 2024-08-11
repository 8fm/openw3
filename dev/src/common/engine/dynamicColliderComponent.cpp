
#include "build.h"
#include "dynamicCollisionCollector.h"
#include "dynamicColliderComponent.h"
#include "globalWater.h"
#include "renderFrame.h"
#include "world.h"

IMPLEMENT_ENGINE_CLASS( CDynamicColliderComponent );

CDynamicColliderComponent::CDynamicColliderComponent()
	: m_useInWaterDisplacement( false )
	, m_useInWaterNormal( true )
	, m_useInGrassDisplacement( false )
	, m_useHideFactor( false )
{
}

void CDynamicColliderComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_DynamicComponent );

	CDynamicCollisionCollector* dynamicCollisions = world->GetDynamicCollisionsCollector();
	if ( dynamicCollisions )
	{
		dynamicCollisions->Add( this );
	}
}

void CDynamicColliderComponent::OnDetached( CWorld* world )
{
	CDynamicCollisionCollector* dynamicCollisions = world->GetDynamicCollisionsCollector();
	if ( dynamicCollisions )
	{
		dynamicCollisions->Remove( this );
	}

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_DynamicComponent );

	TBaseClass::OnDetached( world );
}

void CDynamicColliderComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	if ( flag == SHOW_DynamicComponent )
	{
		const Matrix& l2w = GetLocalToWorld();
		Matrix out = Matrix::IDENTITY;
		Matrix gtm = Matrix::Mul( l2w, out );

		const Vector& row0 = gtm.GetRow(0);
		const Vector& row1 = gtm.GetRow(1);
		const Vector& row2 = gtm.GetRow(2);
		Matrix rot( row0, row1, row2, Vector::ZERO_3D_POINT );

		if ( m_useInGrassDisplacement )
		{
			frame->AddDebugSphere( gtm.GetTranslation(), 1.01f, rot, Color::DARK_GREEN );
		}
		if ( m_useInWaterDisplacement )
		{
			frame->AddDebugSphere( gtm.GetTranslation(), 1.f, rot, Color::DARK_BLUE );
		}
		if ( m_useInWaterNormal )
		{
			frame->AddDebugSphere( gtm.GetTranslation(), 0.99f, rot, Color::LIGHT_BLUE );
		}
	}
}
