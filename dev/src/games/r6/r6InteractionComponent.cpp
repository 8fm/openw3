#include "build.h"

#include "r6InteractionComponent.h"
#include "r6BehaviorSlotComponent.h"
#include "../../common/game/questCondition.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/utils.h"


IMPLEMENT_ENGINE_CLASS( CR6InteractionComponent );

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
THashMap< CEntity*, TDynArray< CR6InteractionComponent* > >	CR6InteractionComponent::sm_worldInteractions;

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CR6InteractionComponent::CR6InteractionComponent() 
	: m_range( 4.0f )
	, m_radius( 0.6f )
	, m_positionTolerance( 0.2f )
	, m_rotationTolerance( 1.2f )
	, m_useExactLocation( true )
	, m_interactorComponent( nullptr )
{
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CR6InteractionComponent::AreAllQuestConditionsFullfilled() const
{
	for ( Uint32 i = 0; i < m_questConditions.Size(); ++i )
	{
		IQuestCondition* condition = m_questConditions[ i ];
		if ( nullptr == condition )
		{
			continue;
		}

		if ( false == condition->IsFulfilled() )
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InteractionComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// tw> TODO: refactor, remove sm_worldInteractions, move functionality to a proper game system

	// add array for this entity
	CEntity* entity = GetEntity();
	if ( false == sm_worldInteractions.KeyExist( entity ) )
	{
		sm_worldInteractions.Insert( entity, TDynArray< CR6InteractionComponent* >() );
	}

	// Add self to static array
	THashMap< CEntity*, TDynArray< CR6InteractionComponent* > >::iterator it = sm_worldInteractions.Find( entity );
	if ( it != sm_worldInteractions.End() )
	{
		( *it ).m_second.PushBackUnique( this );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CR6InteractionComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// tw> TODO: refactor, remove sm_worldInteractions, move functionality to a proper game system

	// remove interaction from lookup map
	CEntity* entity = GetEntity();
	if ( sm_worldInteractions.KeyExist( entity ) )
	{
		THashMap< CEntity*, TDynArray< CR6InteractionComponent* > >::iterator it = sm_worldInteractions.Find( entity );
		if ( it != sm_worldInteractions.End() )
		{
			// remove interaction from array
			( *it ).m_second.Remove( this );
			
			// remove entity if it hasn't got anymore interactions?
			if ( ( *it ).m_second.Empty() )
			{
				sm_worldInteractions.Erase( entity );
			}
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Bool CR6InteractionComponent::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName == CNAME( m_interactionRange ) && readValue.GetType() == GetTypeName< Float > () )
	{
		Float val( 0.f );
		if ( readValue.AsType< Float >( val ) )
		{
			m_range = val;
			return true;
		}
	}

	if ( propertyName == CNAME( m_interactionRadius ) && readValue.GetType() == GetTypeName< Float > () )
	{
		Float val( 0.f );
		if ( readValue.AsType< Float >( val ) )  
		{
			m_radius = val;
			return true;
		}
	}

	return TBaseClass::OnPropertyMissing( propertyName, readValue );	
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
Vector CR6InteractionComponent::GetInteractLocationForNode( const CNode* node ) const
{
	if ( m_useExactLocation || m_radius == 0.f || nullptr == node )
	{
		return GetWorldPosition();
	}

	const Vector toInteractor = ( node->GetWorldPosition() - GetWorldPosition() ).Normalized3();
	return toInteractor * m_radius + GetWorldPosition();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
EulerAngles CR6InteractionComponent::GetInteractRotationForNode( const CNode* node ) const
{
	if ( m_useExactLocation || m_radius == 0.f || nullptr == node )
	{
		return GetWorldRotation();
	}

	const Vector v = ( GetWorldPosition() - GetInteractLocationForNode( node ) ).Normalized3();
	return v.ToEulerAngles();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CR6InteractionComponent* CR6InteractionComponent::CheckNodeForInteraction( CNode* node )
{
	if ( nullptr == node )
	{
		return nullptr;
	}

	CR6InteractionComponent* interaction = Cast< CR6InteractionComponent > ( node ); 
	if ( nullptr == interaction )
	{
		CEntity* entity = Cast< CEntity > ( node ); 
		if ( nullptr == entity )
		{
			// not an interaction
			return nullptr;
		}

		return *( ComponentIterator< CR6InteractionComponent > ( entity ) );
	}

	return interaction;
}


















Bool CR6InteractionComponent::IsUsableFor( CComponent* interactor ) const
{
	R6_ASSERT( interactor, TXT( "Interactor must exist!" ) );
	if( !interactor )
	{
		return false;
	}

	return CanInteract( interactor );
}















Bool CR6InteractionComponent::Use( CComponent* interactor )
{
	R6_ASSERT( interactor );
	R6_ASSERT( nullptr == m_interactorComponent, TXT("Using interaction TWICE!") );
	m_interactorComponent = interactor;
	m_timeAccum = 0.f;

	CEntity* entity = interactor->GetEntity();
	R6_ASSERT( entity, TXT( "Interaction component is trying to be used but has no parent entity." ) );
	if( !entity )
	{
		return false;
	}


	auto cit = ComponentIterator< CR6BehaviorSlotComponent >( entity );
	if( !cit || !( *cit ) )
	{
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT( "Interaction" ), TXT( "CR6InteractionComponents requires CR6BehaviorSlotComponent to work properly." ) );
		return false;
	}

	CR6BehaviorSlotComponent* behSlotComponent = *cit;
	R6_ASSERT( behSlotComponent );

	behSlotComponent->SetBehaviorSlot( BS_INTERACTION, CNAME( interactions ) );

	{ // @todo MS: remove this
		CActor* hack = Cast< CActor > ( interactor->GetEntity() );
		if ( nullptr == hack )
		{
			return false;
		}

		CallFunction< THandle< CActor > > ( this, CNAME( InitInteraction ), hack );
	}

	if( !OnStartInteraction( interactor ) )
	{
		return false;
	}

	if ( CR_EventFailed == CallEvent( CNAME( OnStartInteraction ) ) )
	{	
		return false;
	}

	return true;
}


















void CR6InteractionComponent::FinishUsing( CComponent* interactor )
{
	R6_ASSERT( interactor );
	if ( m_interactorComponent )
	{
		R6_ASSERT( m_interactorComponent == interactor );

		{ // @todo MS: remove this
			CallFunction( this, CNAME( OnStopInteraction ) );
		}

		OnStopInteraction( interactor );
	}
}










void CR6InteractionComponent::Abort( CComponent* interactor )
{
	R6_ASSERT( interactor );
	if ( m_interactorComponent )
	{
		R6_ASSERT( m_interactorComponent == interactor );

		{ // @todo MS: remove this
			CallFunction( this, CNAME( OnAbortInteraction ) );
		}

		OnAbortInteraction( interactor );

		CleanUp();
	}
}

















Bool CR6InteractionComponent::Update( Float timeDelta )
{
	R6_ASSERT( m_interactorComponent, TXT("Updating interaction which is not used.") );
	m_timeAccum += timeDelta;

	{ // @todo MS: remove this
		Bool result( false );
		if( CallFunctionRet< Bool, Float, Float > ( this, CNAME( OnInteractionUpdate ), timeDelta, m_timeAccum, result ) )
		{
			if ( false == result )
			{
				// it just finished itself, another hack to do the script-side cleanup
				Abort( m_interactorComponent );
				return false;
			}
		}
	}

	return OnInteractionUpdate( timeDelta );
}















void CR6InteractionComponent::CleanUp()
{
	m_interactorComponent = nullptr;	
}















Bool CR6InteractionComponent::CanInteract( CComponent* interactor ) const
{
	RED_UNUSED( interactor );

	// @todo MS: After removing THandle<CR6InventoryItemComponent>	m_equippedItem; from R6Player
	// Check if interactor doesn't have any equipped item!!!
	return true;
}














Bool CR6InteractionComponent::OnStartInteraction( CComponent* interactor )
{
	RED_UNUSED( interactor );
	return true;
}












void CR6InteractionComponent::OnStopInteraction( CComponent* interactor )
{
	RED_UNUSED( interactor );
}













void CR6InteractionComponent::OnAbortInteraction( CComponent* interactor )
{
	RED_UNUSED( interactor );
}







Bool CR6InteractionComponent::OnInteractionUpdate( Float timeDelta )
{
	RED_UNUSED( timeDelta );
	return true;
}












void CR6InteractionComponent::funcAreAllQuestConditionsFullfilled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( AreAllQuestConditionsFullfilled() );
}












void CR6InteractionComponent::funcGetInteractLocation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, node, nullptr );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetInteractLocationForNode( node.Get() ) );
}













void CR6InteractionComponent::funcGetInteractRotation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, node, nullptr );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EulerAngles, GetInteractRotationForNode( node.Get() ) );

}
















void CR6InteractionComponent::funcGetInteractRadius( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( m_radius );
}