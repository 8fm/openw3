/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "interactionArea.h"
#include "movingPhysicalAgentComponent.h"
#include "../engine/renderFrame.h"
#include "container.h"

IMPLEMENT_ENGINE_CLASS( CInteractionAreaComponent );

RED_DEFINE_STATIC_NAME( CanStartTalk );
RED_DEFINE_STATIC_NAME( IsUsingVehicle );
RED_DEFINE_STATIC_NAME( OnInteractionActivationTest );

//////////////////////////////////////////////////////////////////////////

const Float CInteractionAreaComponent::s_activatorHeightTolerance = 1.0f;
const Float CInteractionAreaComponent::s_lineOfSightTestInterval = 0.5f;
const Float CInteractionAreaComponent::s_forceLineOfSightTestInterval = 1.0f;

//////////////////////////////////////////////////////////////////////////

SActivatorData::SActivatorData()
	: m_activator( nullptr )
	, m_flags( 0 )
{
}

SActivatorData::SActivatorData( CEntity* entity )
	: m_activator( entity )
	, m_flags( 0 )
{
	if ( entity != nullptr )
	{
		SetFlag( AF_IsPlayer, entity->IsA< CPlayer >() );
	}
}

Bool SActivatorData::IsPlayer() const
{
	return GetFlag( AF_IsPlayer );
}

Bool SActivatorData::IsInCombat() const
{
	if ( !IsInitialized( AF_IsInCombat ) )
	{
		CActor* actor = ( m_activator != nullptr ) ? Cast< CActor >( m_activator ) : nullptr;
		SetFlag( AF_IsInCombat, ( actor != nullptr ) ? actor->IsInCombat() : false );
	}
	return GetFlag( AF_IsInCombat );
}

Bool SActivatorData::CanStartTalk() const
{
	if ( !IsInitialized( AF_CanStartTalk ) )
	{
		Bool ret = false;
		if ( m_activator != nullptr )
		{
			CallFunctionRet( m_activator, CNAME( CanStartTalk ), ret );
		}
		SetFlag( AF_CanStartTalk, ret );
	}
	return GetFlag( AF_CanStartTalk );
}

Bool SActivatorData::IsUsingVehicle() const
{
	if ( !IsInitialized( AF_IsUsingVehicle ) )
	{
		Bool ret = false;
		if ( m_activator != nullptr )
		{
			CallFunctionRet( m_activator, CNAME( IsUsingVehicle ), ret );
		}
		SetFlag( AF_IsUsingVehicle, ret );
	}
	return GetFlag( AF_IsUsingVehicle );
}

Bool SActivatorData::IsSwimming() const
{
	if ( !IsInitialized( AF_IsSwimming ) )
	{
		Bool ret = false;
		if ( m_activator != nullptr && m_activator->IsA< CPlayer >() )
		{
			CMovingAgentComponent* mac = static_cast< CPlayer* >( m_activator )->GetMovingAgentComponent();
			if ( mac != nullptr )
			{
				CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( mac );
				if ( mpac != nullptr )
				{
					ret = mpac->IsSwimming();
				}
			}
		}
		SetFlag( AF_IsSwimming, ret );
	}
	return GetFlag( AF_IsSwimming );
}

Vector SActivatorData::GetCenter() const
{
	if ( m_activator != nullptr )
	{
		// "hardcoded" center
		return m_activator->GetWorldPosition() + Vector::EZ;
	}
	return Vector::ZERO_3D_POINT;
}

Vector SActivatorData::GetLineOfSightPosition() const
{
	if ( m_activator != nullptr && m_activator->IsA< CActor >() )
	{
		CActor* actor = static_cast< CActor* >( m_activator );
		Int32 boneIndex = actor->GetHeadBone();
		if ( boneIndex >= 0 )
		{
			CAnimatedComponent* ac = actor->GetRootAnimatedComponent();
			if ( ac != nullptr )
			{
				return ac->GetBoneMatrixWorldSpace( actor->GetHeadBone() ).GetTranslation();
			}
		}
	}
	return GetCenter();
}

Bool SActivatorData::IsInitialized( EActivatorFlags flag ) const
{
	return ( m_flags & ( 1 << ( static_cast< Uint32 >( flag ) + static_cast< Uint32 >( AF_Total ) ) ) ) != 0;
}

Bool SActivatorData::GetFlag( EActivatorFlags flag ) const
{
	return ( m_flags & ( 1 << static_cast< Uint32 >( flag ) ) ) != 0;
}

void SActivatorData::SetFlag( EActivatorFlags flag, Bool f ) const
{
	// marking as initialized
	m_flags |= ( 1 << ( static_cast< Uint32 >( flag ) + static_cast< Uint32 >( AF_Total ) ) );
	// setting value
	if ( f )
	{
		m_flags |= ( 1 << static_cast< Uint32 >( flag ) );
	}
	else
	{
		m_flags &= ~( 1 << static_cast< Uint32 >( flag ) );
	}
}

//////////////////////////////////////////////////////////////////////////

CInteractionAreaComponent::CInteractionAreaComponent()
	: m_isEnabled( true )
	, m_rangeMin( 0.0f )
	, m_rangeMax( 3.0f )
	, m_rangeAngle( 360 )
	, m_height( 2.0f )
	, m_isPlayerOnly( true )
	, m_manualTestingOnly( false )
	, m_checkLineOfSight( true )
	, m_alwaysVisibleRange( 0.5f )
	, m_lineOfSightOffset( Vector::ZEROS )
	, m_performScriptedTest( false )
	, m_scriptedTestFunction( nullptr )
	, m_lastLineOfSightResult( false )
	, m_lastLineOfSightTest( EngineTime::ZERO )
{
}

void CInteractionAreaComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CInteractionAreaComponent_OnAttached );

	// Register to editor fragment list
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Logic );

	RecalculateGeometryData();
}

void CInteractionAreaComponent::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );

	// Unregister from editor fragment list
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Logic );
}

//////////////////////////////////////////////////////////////////////////
// Activation testing
//////////////////////////////////////////////////////////////////////////

Bool CInteractionAreaComponent::ActivationFastTest( const SActivatorData& activatorData ) const
{
#ifdef PROFILE_INTERACTION
	PC_SCOPE_PIX( ActivationFastTest );
#endif

	// Disabled interactions cannot be activated
	if ( !m_isEnabled )
	{
		return false;
	}

	// All computations here are done in "2.5 dimensional" space:
	// XY plane for distance and Z axis for height

	const Vector activatorWorldPos = activatorData.GetCenter();
	const Vector posDelta = activatorWorldPos - GetWorldPositionRef();
	// 0. Long distance test
	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( Test_0_LongDistance );
#endif

		// Fast "long-distance" filter
		if ( Abs( posDelta.X ) > m_rangeMax || Abs( posDelta.Y ) > m_rangeMax )
		{
			return false;
		}
	}

	// 1. Elevation test
	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( Test_1_Elevation );
#endif

		// local space height delta
		if ( posDelta.Z < -s_activatorHeightTolerance || posDelta.Z > ( m_height + s_activatorHeightTolerance ) )
		{
			return false;
		}
	}

	Float distanceToActivatorSqrLS = 0.0f;
	// 2. Distance test
	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( Test_2_Distance );
#endif

		distanceToActivatorSqrLS = posDelta.SquareMag2();		
		if ( distanceToActivatorSqrLS < m_rangeMinSquare || distanceToActivatorSqrLS > m_rangeMaxSquare )
		{
			return false;
		}
	}

	// 3. FOV angle test
	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( Test_3_FOV );
#endif

		if ( m_rangeAngle < 360 )
		{
			// to obtain cosine of angle we compute the dot product of area's Y axis and (normalized) posDelta
			Float dot = Vector::Dot2( m_localToWorld.GetAxisY().Normalized2(), posDelta ) / MSqrt( distanceToActivatorSqrLS );
			// it should be greater than precomputed cosine of half range angle
			if ( dot < m_cosHalfRangeAngle )
			{
				return false;
			}
		}
	}

	// Interaction area can be activated by given activator position
	return true;
}

Bool CInteractionAreaComponent::ActivationSlowTest( const SActivatorData& activatorData ) const
{
#ifdef PROFILE_INTERACTION
	PC_SCOPE_PIX( ActivationSlowTest );
#endif

	if ( m_performScriptedTest )
	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( PerformScriptedTest );
#endif
		if ( m_scriptedTestFunction == nullptr )
		{
			CacheScriptedFunctions();
			// in case when there's no suitable method in entity's class we won't preform this test anymore
			if ( m_scriptedTestFunction == nullptr )
			{
				m_performScriptedTest = false;
			}
		}
		if ( m_scriptedTestFunction != nullptr )
		{
			const CEntity* activator = activatorData.GetEntity();
			Bool result = false;

			void* stack = RED_ALLOCA( m_scriptedTestFunction->GetStackSize() );

			TStackParam< String >				param0( stack, GetName(),					    m_scriptedTestFunction->GetParameter( 0 )->GetDataOffset() );
			TStackParam< THandle< CEntity > >	param1( stack, THandle< CEntity >( activator ), m_scriptedTestFunction->GetParameter( 1 )->GetDataOffset() );

			m_scriptedTestFunction->Call( GetEntity(), stack, &result );
			if ( !result )
			{
				return false;
			}
		}
	}

	if ( m_checkLineOfSight )
	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( LineOfSightTest );
#endif
		if ( !LineOfSightTest( activatorData.GetLineOfSightPosition() ) )
		{
			return false;
		}
	}

	return true;
}

Bool CInteractionAreaComponent::ActivationTest( CEntity* activator, const SActivatorData& activatorData ) const
{
	RED_ASSERT( activator != nullptr );

	if ( !ActivationFastTest( activatorData ) )
	{
		return false;
	}

	if ( !ActivationSlowTest( activatorData ) )
	{
		return false;
	}

	return true;
}

Bool CInteractionAreaComponent::LineOfSightTest( const Vector& activatorPos ) const
{
	// Always pass visibility test if activator is close to us
	Vector diff = activatorPos - GetWorldPositionRef();
	if ( diff.SquareMag2() < m_alwaysVisibleRangeSqr )
	{
		return true;
	}

	EngineTime currentTime = GGame->GetEngineTime();
	if ( !m_lastLineOfSightTest.IsValid() ||
		 static_cast< Float >( currentTime - m_lastLineOfSightTest ) > s_forceLineOfSightTestInterval ||
		 GCommonGame->GetNpcSensesManager() == nullptr )
	{
		static TDynArray< const CEntity* > ignoreEntities( 1 );
		ignoreEntities[ 0 ] = GetEntity();
		m_lastLineOfSightResult = GGame->GetActiveWorld()->TestLineOfSight( activatorPos, GetLineOfSightTestPosition( activatorPos ), &ignoreEntities );
		m_lastLineOfSightTest = GGame->GetEngineTime();
	}
	else
	{
		if ( static_cast< Float >( currentTime - m_lastLineOfSightTest ) > s_lineOfSightTestInterval )
		{
			// we pass current entity as caster, so that in can be skipped in raycast results
			m_lineOfSightQuery = GCommonGame->GetNpcSensesManager()->SubmitQuery( GetEntity(), activatorPos, GetLineOfSightTestPosition( activatorPos ) );
			m_lastLineOfSightTest = currentTime;
		}
		else if ( m_lineOfSightQuery.IsValid() )
		{
			CNewNpcSensesManager::EVisibilityQueryState queryState = GCommonGame->GetNpcSensesManager()->GetQueryState( m_lineOfSightQuery );
			// wrong job id or timed out
			if ( queryState == CNewNpcSensesManager::QS_NotFound )
			{
				// hold last result
				m_lineOfSightQuery = VisibilityQueryId::INVALID;
			}
			else if ( queryState == CNewNpcSensesManager::QS_False )
			{
				m_lastLineOfSightResult = false;
				m_lineOfSightQuery = VisibilityQueryId::INVALID;
			}
			else if ( queryState == CNewNpcSensesManager::QS_True )
			{
				m_lastLineOfSightResult = true;
				m_lineOfSightQuery = VisibilityQueryId::INVALID;
			}
			// else do nothing -> just wait a little more
		}
	}

	return m_lastLineOfSightResult;
}

void CInteractionAreaComponent::SetEnabled( Bool enabled )
{
	if ( m_isEnabled != enabled )
	{
		// nasty hack, interaction component should not override save flag for w3 container.
		W3Container* container = Cast< W3Container >( GetEntity() );
		if( container )
		{
			SetShouldSave( container->CheckShouldSave() );
		}
		else
		{
			SetShouldSave( true );
		}
	}

	// Toggle interaction state
	m_isEnabled = enabled;
}

Vector CInteractionAreaComponent::GetLineOfSightTestPosition( const Vector& activatorPosition /* = Vector::ZEROS */ ) const
{
	Vector position = GetWorldPositionRef() + m_lineOfSightOffset;
	position.Z += m_height * 0.5f;
	if ( activatorPosition != Vector::ZEROS )
	{
		// since interaction is always visible within the "always visible" range we will check
		// the visibility from the point moved "alwaysVisible" distance towards observer (activator)
		Vector diff = activatorPosition - GetWorldPositionRef();
		diff.Normalize2();
		position.X += diff.X * m_alwaysVisibleRange;
		position.Y += diff.Y * m_alwaysVisibleRange;
	}
	return position;
}

void CInteractionAreaComponent::OnScriptReloaded()
{
	TBaseClass::OnScriptReloaded();
	if ( m_scriptedTestFunction != nullptr )
	{
		CacheScriptedFunctions();
	}
}

void CInteractionAreaComponent::CacheScriptedFunctions() const
{
	m_scriptedTestFunction = nullptr;
	IScriptable* context = GetEntity();
	m_scriptedTestFunction = GetEntity()->FindFunction( context, CNAME( OnInteractionActivationTest ), false );
	if ( m_scriptedTestFunction != nullptr )
	{
		RED_ASSERT( m_scriptedTestFunction->GetNumParameters() == 2 );
		RED_ASSERT( m_scriptedTestFunction->GetParameter( 0 ) != nullptr );
		RED_ASSERT( m_scriptedTestFunction->GetParameter( 0 )->GetType()->GetName() == TTypeName< String >::GetTypeName() );
		RED_ASSERT( m_scriptedTestFunction->GetParameter( 1 ) != nullptr );
		RED_ASSERT( m_scriptedTestFunction->GetParameter( 1 )->GetType()->GetName() == TTypeName< THandle< CEntity > >::GetTypeName() );
		RED_ASSERT( m_scriptedTestFunction->GetReturnValue() != nullptr );
		RED_ASSERT( m_scriptedTestFunction->GetReturnValue()->GetType()->GetName() == TTypeName< Bool >::GetTypeName() );
	}
}

void CInteractionAreaComponent::RecalculateGeometryData()
{
	// squared ranges
	m_rangeMinSquare = m_rangeMin * m_rangeMin;
	m_rangeMaxSquare = m_rangeMax * m_rangeMax;
	m_alwaysVisibleRangeSqr = m_alwaysVisibleRange * m_alwaysVisibleRange;
	// cosine of the half angle (in gradients) transformed to radians:
	// ( m_rangeAngle * 0.5f ) * M_PI / 180.0f = m_rangeAngle * M_PI / 360.0f
	m_cosHalfRangeAngle = MCos( m_rangeAngle * M_PI / 360.0f );
}

//////////////////////////////////////////////////////////////////////////
// Generate fragments
//////////////////////////////////////////////////////////////////////////

Color CInteractionAreaComponent::GetColor() const
{
	return Color(100,255,100);
}

void CInteractionAreaComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	Vector observerPosition = Vector::ZEROS;

	if ( GCommonGame != nullptr && GCommonGame->GetPlayer() != nullptr )
	{
		observerPosition = GCommonGame->GetPlayer()->GetWorldPositionRef();
		const Vector3 diff = observerPosition - GetWorldPositionRef();
		// don't draw too distant interaction while playing game
		if ( MAbs( diff.X ) > 30 || MAbs( diff.Y ) > 30 || MAbs( diff.Z ) > 10 )
		{
			return;
		}
	}
	// if there's no player, we will use camera as observer
	else if ( GGame != nullptr && GGame->IsFreeCameraEnabled() )
	{
		GGame->GetFreeCameraWorldPosition( &observerPosition, nullptr, nullptr );
	}
	else if ( GGame != nullptr && GGame->GetActiveWorld() != nullptr )
	{
		observerPosition = GGame->GetActiveWorld()->GetCameraPosition();
	}

	if ( flags == SHOW_Logic )
	{
		Matrix m;
		m.SetIdentity();
		m.SetRotZ33( DEG2RAD( GetWorldYaw() ) );
		m.SetTranslation( GetWorldPosition() );
		// Draw min range
		if ( m_rangeMin > 0.0f )
		{
			Color color = Color(255,100,100);

			// Dim
			if ( !IsEnabled() )
			{
				color.Mul3( 0.1f );
			}
			frame->AddDebugAngledRange( m, m_height, m_rangeMin, Float( m_rangeAngle ), color, false );
		}

		// Draw max range
		if ( m_rangeMax > 0.0f )
		{
			Color color = GetColor();

			// Dim
			if ( !IsEnabled() )
			{
				color.Mul3( 0.1f );
			}
			frame->AddDebugAngledRange( m, m_height, m_rangeMax, Float( m_rangeAngle ), color, true );
		}

		if ( m_checkLineOfSight )
		{
			// Get activator (ie. player) position
			const Color color = m_lastLineOfSightResult ? GetColor() : Color( 100, 255, 100 );
			frame->AddDebugSphere( GetLineOfSightTestPosition( observerPosition ), 0.25f, Matrix::IDENTITY, color );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Script
//////////////////////////////////////////////////////////////////////////

void CInteractionAreaComponent::funcGetRangeMin( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( m_rangeMin );
}

void CInteractionAreaComponent::funcGetRangeMax( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( m_rangeMax );
}

void CInteractionAreaComponent::funcSetRanges( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, rangeMin, 0.0f );
	GET_PARAMETER( Float, rangeMax, 3.0f );
	GET_PARAMETER( Float, height, 2.0f );
	FINISH_PARAMETERS;

	m_rangeMin = rangeMin;
	m_rangeMax = rangeMax;
	m_height = height;
	RecalculateGeometryData();
}

void CInteractionAreaComponent::funcSetRangeAngle( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, rangeAngle, 0 );
	FINISH_PARAMETERS;

	m_rangeAngle = rangeAngle;
	RecalculateGeometryData();
}

void CInteractionAreaComponent::funcSetCheckLineOfSight( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;
	
	m_checkLineOfSight = flag;

	RETURN_VOID();
};
