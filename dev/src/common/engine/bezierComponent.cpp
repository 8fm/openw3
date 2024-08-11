
#include "build.h"
#include "bezierComponent.h"
#include "skeletonUtils.h"
#include "worldShowFlags.h"
#include "world.h"
#include "node.h"
#include "animationController.h"
#include "skeleton.h"

IMPLEMENT_ENGINE_CLASS( CBezierComponent );

CBezierComponent::CBezierComponent()
	: m_curve( NULL )
{
}

CBezierComponent::~CBezierComponent()
{
	if ( m_curve )
	{
		delete m_curve;
		m_curve = NULL;
	}
}

void CBezierComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// Curve will be visible when show flag Behavior 
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Behavior );
}

void CBezierComponent::OnDetached( CWorld *world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Behavior );
	
	TBaseClass::OnDetached( world );
}

void CBezierComponent::OnSpawned( const SComponentSpawnInfo& spawnInfo )
{
	// Pass to base class
	TBaseClass::OnSpawned( spawnInfo );

	if ( !m_curve )
	{
		m_curve = new QuadraticBezierNamespace::Curve2();
		if( m_curve->m_Segments == NULL )
		{
			QuadraticBezierNamespace::float2 data[14];
			data[0]  = QuadraticBezierNamespace::float2(1,1) ;
			data[1]  = QuadraticBezierNamespace::float2(2,1) ;
			data[2]  = QuadraticBezierNamespace::float2(2,2) ;
			data[3]  = QuadraticBezierNamespace::float2(3,2) ;
			data[4]  = QuadraticBezierNamespace::float2(3,1) ;
			data[5]  = QuadraticBezierNamespace::float2(4,1) ;
			data[6]  = QuadraticBezierNamespace::float2(4,0) ;
			data[7]  = QuadraticBezierNamespace::float2(3,0) ;
			data[8]  = QuadraticBezierNamespace::float2(3,-1);
			data[9]  = QuadraticBezierNamespace::float2(2,-1);
			data[10] = QuadraticBezierNamespace::float2(2,0) ;
			data[11] = QuadraticBezierNamespace::float2(1,0) ;
			data[12] = QuadraticBezierNamespace::float2(1,1) ;
			data[13] = QuadraticBezierNamespace::float2(2,1) ;
			m_curve->Update((QuadraticBezierNamespace::float2*)&data,14);
		}
	}
	else
	{
		ASSERT( !m_curve );
	}
}

void CBezierComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );
	if ( flags == SHOW_Behavior && m_curve)
	{
		QuadraticBezierNamespace::EngineBezierTools::Draw2(m_curve,frame,10);
	}
}

void CBezierComponent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( !file.IsGarbageCollector() )
	{
		QuadraticBezierNamespace::EngineBezierTools::Serialize2( m_curve, file );
	}
}



////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBezierMovableComponent );

CBezierMovableComponent::CBezierMovableComponent()
	: m_curve( NULL )
	, m_speed( 1.f )
	, FinalMatrix()
{

}

CBezierMovableComponent::~CBezierMovableComponent()
{
	if ( m_curve )
	{
		delete m_curve;
		m_curve = NULL;
	}
}

void CBezierMovableComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// Curve will be visible when show flag Behavior 
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Behavior );
}

void CBezierMovableComponent::OnDetached( CWorld *world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Behavior );

	TBaseClass::OnDetached( world );
}

void CBezierMovableComponent::OnSpawned( const SComponentSpawnInfo& spawnInfo )
{
	// Pass to base class
	TBaseClass::OnSpawned( spawnInfo );

	if ( !m_curve )
	{
		m_curve = new QuadraticBezierNamespace::Curve2();
		if( m_curve->m_Segments == NULL )
		{
			QuadraticBezierNamespace::float2 data[14];
			data[0]  = QuadraticBezierNamespace::float2(1,1) ;
			data[1]  = QuadraticBezierNamespace::float2(2,1) ;
			data[2]  = QuadraticBezierNamespace::float2(2,2) ;
			data[3]  = QuadraticBezierNamespace::float2(3,2) ;
			data[4]  = QuadraticBezierNamespace::float2(3,1) ;
			data[5]  = QuadraticBezierNamespace::float2(4,1) ;
			data[6]  = QuadraticBezierNamespace::float2(4,0) ;
			data[7]  = QuadraticBezierNamespace::float2(3,0) ;
			data[8]  = QuadraticBezierNamespace::float2(3,-1);
			data[9]  = QuadraticBezierNamespace::float2(2,-1);
			data[10] = QuadraticBezierNamespace::float2(2,0) ;
			data[11] = QuadraticBezierNamespace::float2(1,0) ;
			data[12] = QuadraticBezierNamespace::float2(1,1) ;
			data[13] = QuadraticBezierNamespace::float2(2,1) ;
			m_curve->Update((QuadraticBezierNamespace::float2*)&data,14);
		}
	}
	else
	{
		ASSERT( !m_curve );
	}
}

void CBezierMovableComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );
	if ( flags == SHOW_Behavior && m_curve)
	{
		QuadraticBezierNamespace::EngineBezierTools::Draw2(m_curve,frame,10);
	}
}

void CBezierMovableComponent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( !file.IsGarbageCollector() )
	{
		QuadraticBezierNamespace::EngineBezierTools::Serialize2( m_curve, file );
	}
}

void CBezierMovableComponent::UpdateAsync( Float timeDelta )
{
	Matrix pathMatrix;
	if ( m_curve )
	{
		Float leng = m_curve->CalculateLength();
		m_positionOnCurve += m_speed * timeDelta;
		if ( m_positionOnCurve > leng )
		{
			m_positionOnCurve -= leng;
		}

		QuadraticBezierNamespace::float2 pos = m_curve->PointOnLength( m_positionOnCurve );
		QuadraticBezierNamespace::float2 dir = m_curve->VelocityOnLength( m_positionOnCurve );

		Vector newPos = GetWorldPosition();
		newPos.X = pos.x;
		newPos.Y = pos.y;

		Vector newRot;
		newRot.X = dir.x;
		newRot.Y = dir.y;
		newRot.Z = 0.0f;
		//newRot.Z = -rot*(180.f/M_PI);

		pathMatrix.BuildFromDirectionVector(newRot);
		pathMatrix.SetTranslation(newPos);
	}
	FinalMatrix = Matrix::Mul(m_localToWorld, pathMatrix);

	PC_SCOPE_PIX( SkeletalAnimCompUpdate );

	ASSERT( m_isOk );
	ASSERT( m_skeletonModelSpace.Size() > 0 );

	// Update
	m_controller->Update( timeDelta );

	// Sample
	SampleAnimations();

	// Calc WS
	SkeletonBonesUtils::MulMatrices( m_skeletonModelSpace.TypedData(), m_skeletonWorldSpace.TypedData(), m_skeletonModelSpace.Size(), &FinalMatrix );

	// Update skinning
	UpdateAttachedSkinningComponentsTransforms();
}

void CBezierMovableComponent::CalcBox( Box& box ) const
{
	if ( m_controller )
	{
		m_controller->CalcBox( box );
		box = FinalMatrix.TransformBox( box );
	}
	else
	{
		box = FinalMatrix.TransformBox( Box( Vector( -0.5f, -0.5f, 0.f ), Vector( 0.5f, 0.5f, 2.f ) ) );
	}
}
