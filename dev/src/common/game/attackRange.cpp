/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "attackRange.h"

#include "../engine/physicsCharacterWrapper.h"
#include "../physics/physicsEngine.h"

#include "entityParams.h"
#include "findGameplayEntities.h"
#include "gameplayStorageAcceptors.h"
#include "movableRepresentationPhysicalCharacter.h"
#include "movingPhysicalAgentComponent.h"
#include "../engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CAIAttackRange );
IMPLEMENT_ENGINE_CLASS( CSphereAttackRange );
IMPLEMENT_ENGINE_CLASS( CCylinderAttackRange );
IMPLEMENT_ENGINE_CLASS( CConeAttackRange );
IMPLEMENT_ENGINE_CLASS( CBoxAttackRange );

//////////////////////////////////////////////////////////////////////////

CAIAttackRange::CAIAttackRange()		
	: m_rangeMax( 2.5f )
	, m_height( 4.0f )
	, m_angleOffset( 0.f )
	, m_position( Vector( 0.0f, 0.0f, 0.0f ) )
	, m_name( CName::NONE )
	, m_checkLineOfSight( false )
	, m_lineOfSightHeight( 0.0f )
	, m_useHeadOrientation( false )
{}

template < typename CustomAcceptor >
void CAIAttackRange::GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output, const CustomAcceptor& acceptor ) const
{
	CGameplayStorage::SSearchParams params;
	params.m_origin = parentEntity->GetWorldPositionRef();
	params.m_range = GetStorageMaxRange();
	if ( m_checkLineOfSight )
	{
		params.m_flags |= FLAG_TestLineOfSight;
		params.m_losPosition = GetLosTestPosition( parentEntity );
	}

	FindGameplayEntitiesInRange< CGameplayEntity, CustomAcceptor >( output, *parentEntity, params, acceptor );
}

void CAIAttackRange::GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output ) const
{
	GatherEntities( parentEntity, output, GameplayStorageAcceptors::DefaultAcceptor() );
}

template < typename CustomAcceptor >
Bool CAIAttackRange::Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime, const CustomAcceptor& acceptor ) const
{
	CGameplayStorage::SSearchParams params;
	params.m_origin = parentEntity->GetWorldPositionRef();
	params.m_range = GetStorageMaxRange();
	params.m_predictPositionInTime = predictPositionInTime;
	if ( m_checkLineOfSight )
	{
		params.m_flags |= FLAG_TestLineOfSight;
		params.m_losPosition = GetLosTestPosition( parentEntity );
	}

	FindGameplayEntities::SQueryFunctor< CGameplayEntity, CustomAcceptor > functor( nullptr, *parentEntity, params, acceptor );
	return functor.Accept( testedEntity );
}

Bool CAIAttackRange::Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime ) const
{
	return Test( parentEntity, testedEntity, predictPositionInTime, GameplayStorageAcceptors::DefaultAcceptor() );
}

Float CAIAttackRange::GetStorageMaxRange() const
{
	return m_rangeMax;
}

String CAIAttackRange::GetFriendlyName() const
{
	return String::Printf( TXT( "Range base [%s] - should be specialized!" ), m_name.AsString().AsChar() );
}

void CAIAttackRange::OnGenerateDebugFragments( const CEntity* parentActor, CRenderFrame* frame ) const
{
	// Empty default implementation
}

const CAIAttackRange* CAIAttackRange::Get( const CEntity* entity, CName attackRange )
{
	CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
	if ( !entityTemplate )
	{
		return nullptr;
	}

	struct Pred
	{
		Pred( CName attackRange )
			: m_attackRange( attackRange ), m_retval( NULL ) {}

		Bool operator()( CAttackRangeParam* attackRangeParam ) const
		{
			const CAIAttackRange* attackRange = attackRangeParam->GetAttackRange( m_attackRange );
			if ( attackRange != NULL )
			{
				m_retval = attackRange;
				return true;
			}
			return false;
		}
		CName							m_attackRange;
		mutable const CAIAttackRange*	m_retval;
	} pred( attackRange );

	entityTemplate->FindParameter< CAttackRangeParam >( true, pred, true );

	return pred.m_retval;
}

Vector CAIAttackRange::GetLosTestPosition( const CEntity* parent ) const
{
	const Float checkRatio = 0.7f;
	Float height = m_lineOfSightHeight;

	if ( parent->IsA< CActor >() )
	{
		CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( static_cast< const CActor* >( parent )->GetMovingAgentComponent() );
		if ( mpac != nullptr && mpac->GetPhysicalCharacter() != nullptr && mpac->GetPhysicalCharacter()->GetCharacterController() )
		{
#ifdef USE_PHYSX
			height = mpac->GetPhysicalCharacter()->GetCharacterController()->GetCurrentHeight() * checkRatio;
#endif
		}
	}

	return parent->GetWorldPosition() + Vector( 0.f, 0.f, height );
}

void CAIAttackRange::funcTest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CGameplayEntity >, sourceHandle, nullptr );
	GET_PARAMETER( THandle< CGameplayEntity >, targetHandle, nullptr );
	FINISH_PARAMETERS;

	Bool ret = false;

	CGameplayEntity* sourceEntity = sourceHandle.Get();
	CGameplayEntity* targetEntity = targetHandle.Get();
	
	if ( sourceEntity != nullptr && targetEntity != nullptr )
	{
		ret = Test( sourceEntity, targetEntity );
	}

	RETURN_BOOL( ret );
}
void CAIAttackRange::funcGatherEntities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CGameplayEntity >, sourceEntity, nullptr );
	GET_PARAMETER_REF( TDynArray< THandle< CGameplayEntity > >, output, TDynArray< THandle< CGameplayEntity > > () );
	FINISH_PARAMETERS;

	CGameplayEntity* source = sourceEntity.Get();
	if ( source != nullptr )
	{
		GatherEntities( source, output );
	}	
}

//////////////////////////////////////////////////////////////////////////

CSphereAttackRange::CSphereAttackRange()
{
}

GameplayStorageAcceptors::SphereAcceptor CSphereAttackRange::CreateAcceptor( const CGameplayEntity* parentEntity ) const
{
	Matrix localToWorld;
	GetAttackL2W( parentEntity, localToWorld );
	Vector position = localToWorld.TransformPoint( m_position );

	return GameplayStorageAcceptors::SphereAcceptor( position, m_rangeMax );
}

void CSphereAttackRange::GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output ) const
{
	CAIAttackRange::GatherEntities( parentEntity, output, CreateAcceptor( parentEntity ) );
}

Bool CSphereAttackRange::Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime ) const
{
	return CAIAttackRange::Test( parentEntity, testedEntity, predictPositionInTime, CreateAcceptor( parentEntity ) );
}

String CSphereAttackRange::GetFriendlyName() const
{
	return String::Printf( TXT( "Sphere range [%s]" ), m_name.AsString().AsChar() );
}

void CSphereAttackRange::OnGenerateDebugFragments( const CEntity* parentActor, CRenderFrame* frame ) const
{
	if( frame->GetFrameInfo().IsShowFlagOn( SHOW_AIRanges ) )
	{
		Matrix localToWorld;
		GetAttackL2W( parentActor, localToWorld );
		Vector position = localToWorld.TransformPoint( m_position );

		frame->AddDebugSphere( position, m_rangeMax, Matrix::IDENTITY, Color::MAGENTA, true );
	}
}

//////////////////////////////////////////////////////////////////////////

CCylinderAttackRange::CCylinderAttackRange()
{
}

GameplayStorageAcceptors::CylinderAcceptor CCylinderAttackRange::CreateAcceptor( const CGameplayEntity* parentEntity ) const
{
	Matrix localToWorld;
	GetAttackL2W( parentEntity, localToWorld );
	Vector position = localToWorld.TransformPoint( m_position );

	return GameplayStorageAcceptors::CylinderAcceptor( position, m_rangeMax, m_height );
}

void CCylinderAttackRange::GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output ) const
{
	CAIAttackRange::GatherEntities( parentEntity, output, CreateAcceptor( parentEntity ) );
}

Bool CCylinderAttackRange::Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime ) const
{
	return CAIAttackRange::Test( parentEntity, testedEntity, predictPositionInTime, CreateAcceptor( parentEntity ) );
}

String CCylinderAttackRange::GetFriendlyName() const
{
	return String::Printf( TXT( "Cylinder range [%s]" ), m_name.AsString().AsChar() );
}

void CCylinderAttackRange::OnGenerateDebugFragments( const CEntity* parentActor, CRenderFrame* frame ) const
{
	if( frame->GetFrameInfo().IsShowFlagOn( SHOW_AIRanges ) )
	{
		Matrix localToWorld;
		GetAttackL2W( parentActor, localToWorld );
		Vector position = localToWorld.TransformPoint( m_position );

		frame->AddDebugCapsule( FixedCapsule( position, m_rangeMax, m_height ), Matrix::IDENTITY, Color::MAGENTA, true );
	}
}

//////////////////////////////////////////////////////////////////////////

CConeAttackRange::CConeAttackRange()
	: m_rangeAngle( 100.0f )
{
}

GameplayStorageAcceptors::ConeAcceptor CConeAttackRange::CreateAcceptor( const CGameplayEntity* parentEntity ) const
{
	Matrix localToWorld;
	GetAttackL2W( parentEntity, localToWorld );
	Vector position = localToWorld.TransformPoint( m_position );
	Float direction = localToWorld.GetYaw() + m_angleOffset;

	return GameplayStorageAcceptors::ConeAcceptor( position, direction, m_rangeAngle, m_rangeMax, m_height );
}

void CConeAttackRange::GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output ) const
{
	CAIAttackRange::GatherEntities( parentEntity, output, CreateAcceptor( parentEntity ) );
}

Bool CConeAttackRange::Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime ) const
{
	return CAIAttackRange::Test( parentEntity, testedEntity, predictPositionInTime, CreateAcceptor( parentEntity ) );
}

String CConeAttackRange::GetFriendlyName() const
{
	return String::Printf( TXT( "Cone range [%s]" ), m_name.AsString().AsChar() );
}

void CConeAttackRange::OnGenerateDebugFragments( const CEntity* parentActor, CRenderFrame* frame ) const
{
	if( frame->GetFrameInfo().IsShowFlagOn( SHOW_AIRanges ) )
	{
		Matrix localToWorld;
		GetAttackL2W( parentActor, localToWorld );
		Matrix rangePlacement( Matrix::IDENTITY );
		rangePlacement.SetTranslation( Vector( m_position.X, m_position.Y, m_position.Z - m_height * 0.5f ) );
		rangePlacement.SetRotZ33( DEG2RAD( m_angleOffset ) );
		rangePlacement = Matrix::Mul( localToWorld, rangePlacement );

		frame->AddDebugAngledRange( rangePlacement, m_height, m_rangeMax, m_rangeAngle, Color::MAGENTA, true );
	}
}

//////////////////////////////////////////////////////////////////////////

CBoxAttackRange::CBoxAttackRange()
	: m_rangeWidth( 1.0f )
{	
}

GameplayStorageAcceptors::BoxAcceptor CBoxAttackRange::CreateAcceptor( const CGameplayEntity* parentEntity ) const
{
	Matrix localToWorld;
	GetAttackL2W( parentEntity, localToWorld );

	Vector position = localToWorld.GetTranslation();
	Box boxLS;
	CreateLocalBox( boxLS );
	Float rotation = localToWorld.GetYaw() + m_angleOffset;

	return GameplayStorageAcceptors::BoxAcceptor( position, boxLS, rotation );
}

void CBoxAttackRange::GatherEntities( const CGameplayEntity* parentEntity, TDynArray< THandle< CGameplayEntity > > & output ) const
{
	CAIAttackRange::GatherEntities( parentEntity, output, CreateAcceptor( parentEntity ) );
}

Bool CBoxAttackRange::Test( const CGameplayEntity* parentEntity, const CGameplayEntity* testedEntity, Float predictPositionInTime ) const
{
	return CAIAttackRange::Test( parentEntity, testedEntity, predictPositionInTime, CreateAcceptor( parentEntity ) );
}

Float CBoxAttackRange::GetStorageMaxRange() const
{
	Box box;
	CreateLocalBox( box );
	// We take the box corner that is the furthest from the center
	// and multiple it by Sqrt(2) - cause this is the furthest possible axis aligned point (when box is rotated by 45 degrees).
	const Float SQRT_2 = 1.4142135f;
	return Max( Max( MAbs( box.Min.X ), MAbs( box.Min.Y ) ), Max( MAbs( box.Max.X ), MAbs( box.Max.Y ) ) ) * SQRT_2;
}

void CBoxAttackRange::CreateLocalBox( Box& box ) const
{
	box = Box( m_position - Vector( m_rangeWidth * 0.5f , 0.0f, m_height * 0.5f ),
			   m_position + Vector( m_rangeWidth * 0.5f , m_rangeMax, m_height * 0.5f ) );
}

String CBoxAttackRange::GetFriendlyName() const
{
	return String::Printf( TXT( "Box range [%s]" ), m_name.AsString().AsChar() );
}

void CBoxAttackRange::OnGenerateDebugFragments( const CEntity* parentActor, CRenderFrame* frame ) const
{
	if( frame->GetFrameInfo().IsShowFlagOn( SHOW_AIRanges ) )
	{
		Matrix localToWorld;
		GetAttackL2W( parentActor, localToWorld );

		Matrix rangePlacement( Matrix::IDENTITY );
		rangePlacement.SetRotZ33( DEG2RAD( m_angleOffset ) );		
		rangePlacement = Matrix::Mul( localToWorld, rangePlacement );

		Box rangeBox(	
			m_position - Vector( m_rangeWidth*0.5f , 0.f,	m_height*0.5f ),
			m_position + Vector( m_rangeWidth*0.5f , m_rangeMax,  m_height*0.5f ) );

		frame->AddDebugBox( rangeBox, rangePlacement, Color::MAGENTA );
	}
}
