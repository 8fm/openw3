#include "build.h"
#include "world.h"
#include "simpleBuoyancyComponent.h"
#include "../physics/physicsWrapper.h"
#include "..\engine\tickManager.h"
#include "..\engine\renderFrame.h"
#include "..\engine\renderVisibilityQuery.h"
#include "globalWater.h"


IMPLEMENT_ENGINE_CLASS( CSimpleBuoyancyComponent );

#define GETWATERLOD0 2500.f		// 50m squared and full detail movement
#define GETWATERLOD1 10000.f	// 100m squared and medium detailed movement
#define GETWATERLOD2 22500.f	// 150m squared and up and down movement
#define MAX_LOD_SAMPLE 1		// above that lod we will sample once

CSimpleBuoyancyComponent::CSimpleBuoyancyComponent()
	: m_waterOffset( 0.f )
#ifndef NO_EDITOR
	, m_shouldUpdate( true )
#endif
	, m_linearDamping( 0.f )
	, m_prevWaterLevelZ( 0.f )
	, m_prevWaterLevelF( 0.f )
	, m_prevWaterLevelB( 0.f )
	, m_prevWaterLevelL( 0.f )
	, m_prevWaterLevelR( 0.f )
	, m_pointFront( Vector(  0.f,  1.f, 0.f, 1.f ) )
	, m_pointBack ( Vector(  0.f, -1.f, 0.f, 1.f ) )
	, m_pointRight( Vector(  1.f,  0.f, 0.f, 1.f ) )
	, m_pointLeft ( Vector( -1.f,  0.f, 0.f, 1.f ) )
{
}

CSimpleBuoyancyComponent::~CSimpleBuoyancyComponent()
{
}


void CSimpleBuoyancyComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_SimpleBuoyancy );
	world->GetTickManager()->AddToGroup( this, TICK_PrePhysics );

	if ( world->IsWaterShaderEnabled() )
	{
		Float currWaterLevelZ = world->GetWaterLevel( GetLocalToWorld().GetTranslationRef(), 2 );
		m_prevWaterLevelZ = currWaterLevelZ;
		m_prevWaterLevelF = currWaterLevelZ;
		m_prevWaterLevelB = currWaterLevelZ;
		m_prevWaterLevelL = currWaterLevelZ;
		m_prevWaterLevelR = currWaterLevelZ;
	}
}

void CSimpleBuoyancyComponent::OnDetached( CWorld* world )
{
	world->GetTickManager()->RemoveFromGroup( this, TICK_PrePhysics );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_SimpleBuoyancy );
	TBaseClass::OnDetached( world );
}

void CSimpleBuoyancyComponent::OnTickPrePhysics( Float timeDelta )
{
	TBaseClass::OnTickPrePhysics( timeDelta );

	CWorld* world = GetWorld();

#ifndef NO_EDITOR
	if ( !m_shouldUpdate || world->GetPreviewWorldFlag() ) return;
#endif
	// early out
	CEntity* e = GetEntity();
	if( e != nullptr && !e->WasVisibleLastFrame() ) return;

	if( world != nullptr )
	{
		const Matrix& l2w = m_localToWorld;
		Vector pos = l2w.GetTranslation();

		const Vector& camPos = world->GetCameraPosition();
		Float distSqr = camPos.DistanceSquaredTo( l2w.GetTranslationRef() );

		// water approximation
		Uint32 waterLod = MAX_LOD_SAMPLE+1;
		if( distSqr < GETWATERLOD0 ) 
		{
			waterLod = 0;
		}
		else if ( distSqr < GETWATERLOD1 ) 
		{
			waterLod = 1;
		}

		Float currWaterLevelZ = world->GetWaterLevel( pos, Min< Uint32 >( waterLod, MAX_LOD_SAMPLE ) );
		RED_ASSERT( IsFinite( currWaterLevelZ ) );
		pos.Z = GetCurrentSmoothValue( m_prevWaterLevelZ, currWaterLevelZ, timeDelta );

		// get water level in WS
		Vector f = l2w.TransformPoint( m_pointFront );
		Vector b = l2w.TransformPoint( m_pointBack );
		Vector l = l2w.TransformPoint( m_pointLeft );
		Vector r = l2w.TransformPoint( m_pointRight );

		Float waterLevelFront = currWaterLevelZ;
		Float waterLevelBack  = currWaterLevelZ;
		Float waterLevelLeft  = currWaterLevelZ;
		Float waterLevelRight = currWaterLevelZ;

		// this way we will have up and down movement from far distance. and no sampling 4 times.
		if( waterLod <= MAX_LOD_SAMPLE )
		{
			waterLevelFront = world->GetWaterLevel( f, waterLod );
			waterLevelBack  = world->GetWaterLevel( b, waterLod );
			waterLevelLeft  = world->GetWaterLevel( l, waterLod );
			waterLevelRight = world->GetWaterLevel( r, waterLod );
		}

		// calculate world coord of points
		f.Z = GetCurrentSmoothValue( m_prevWaterLevelF, waterLevelFront, timeDelta );
		b.Z = GetCurrentSmoothValue( m_prevWaterLevelB, waterLevelBack,  timeDelta );
		r.Z = GetCurrentSmoothValue( m_prevWaterLevelR, waterLevelRight, timeDelta );
		l.Z = GetCurrentSmoothValue( m_prevWaterLevelL, waterLevelLeft,  timeDelta );

		// cache current as prev
		m_prevWaterLevelZ = pos.Z;
		m_prevWaterLevelF = f.Z;
		m_prevWaterLevelB = b.Z;
		m_prevWaterLevelR = r.Z;
		m_prevWaterLevelL = l.Z;

		// calculate new transform
		Vector frontDir = f - b;
		Vector rightDir = r - l;

		frontDir.Normalize3();
		rightDir.Normalize3();
		// calculate normal
		Vector upDir = Vector::Cross( rightDir, frontDir );
		upDir.Normalize3();
		
		// if user would like to move up/down refresh it
		pos.Z += GetWaterOffset();

		// new transform
		Matrix m( rightDir, frontDir, upDir, pos );

		RED_ASSERT( m.IsOk(), TXT("Buoyancy matrix construction failed") );
		const Vector& newPos = m.GetTranslationRef();
		const EulerAngles newRot = m.ToEulerAngles();

		// apply to entity
		e->SetRawPlacement( &newPos, &newRot, nullptr );
	}
}

void CSimpleBuoyancyComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	if( flag == SHOW_SimpleBuoyancy )
	{
		const Matrix& ltw = m_localToWorld;
		Box c = Box( Vector::ZERO_3D_POINT, 0.25f );
		frame->AddDebugSolidBox( c, ltw, Color::WHITE );

		frame->AddDebugSphere( ltw.TransformPoint( m_pointFront ), 0.5f, Matrix::IDENTITY, Color::RED, true, true );
		frame->AddDebugSphere( ltw.TransformPoint( m_pointBack ), 0.5f, Matrix::IDENTITY, Color::RED, true, true );
		frame->AddDebugSphere( ltw.TransformPoint( m_pointLeft ), 0.5f, Matrix::IDENTITY, Color::RED, true, true );
		frame->AddDebugSphere( ltw.TransformPoint( m_pointRight ), 0.5f, Matrix::IDENTITY, Color::RED, true, true );
	}
}

void CSimpleBuoyancyComponent::OnSelectionChanged()
{
#ifndef NO_EDITOR
	if ( GetFlags() & NF_Selected )
	{
		m_shouldUpdate = false;
	}
	else
	{
		ForceUpdateTransformNodeAndCommitChanges();
		m_shouldUpdate = true;
	}
#endif // !NO_EDITOR
}

Float CSimpleBuoyancyComponent::GetCurrentSmoothValue(Float prevVal, Float waterLvl, Float dt)
{
	const Float dampFactor = 1.f-m_linearDamping;
	return ( ( waterLvl - prevVal ) * dt * dampFactor ) + prevVal;
}

void CSimpleBuoyancyComponent::OnPropertyPostChange(IProperty* property)
{
	TBaseClass::OnPropertyPostChange( property );
	if ( property->GetName() == TXT("linearDamping") )
	{
		m_linearDamping = Clamp<Float>( m_linearDamping, 0.f, 1.f );
	}
}
