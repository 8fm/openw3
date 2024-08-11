#include "build.h"
#include "r4interiorAreaComponent.h"

#include "../../common/engine/pathlibConst.h"
#include "../../common/engine/pathlibCookerData.h"
#include "../../common/engine/pathlibSpecialZoneMap.h"

#include "../../common/game/newNpc.h"

#include "commonMapManager.h"
#include "r4Player.h"

IMPLEMENT_ENGINE_CLASS( CR4InteriorAreaComponent );

void CR4InteriorAreaComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// required for wetness params not being bumped up
	// if we are in an interior
	CEnvironmentManager* envMgr = world->GetEnvironmentManager();
	if( envMgr ) m_weatherManager = envMgr->GetWeatherManager();

	if ( GGame->IsActive() )
	{
		CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
		if ( commonMapManager )
		{
			commonMapManager->OnAttachedInterior( this );
		}
	}
}

void CR4InteriorAreaComponent::OnDetached( CWorld* world )
{
	if ( GGame->IsActive() )
	{
		CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
		ASSERT( commonMapManager );
		if ( commonMapManager )
		{
			commonMapManager->OnDetachedInterior( this );
		}
	}

	m_weatherManager = nullptr;

	TBaseClass::OnDetached( world );
}

RED_DEFINE_STATIC_NAME( OnInteriorStateChanged );
RED_DEFINE_STATIC_NAME( OnPlayerEntered );
void CR4InteriorAreaComponent::EnteredArea( CComponent* component )
{
	TBaseClass::EnteredArea( component ) ;

	if ( component && GGame->IsActive() )
	{
		if ( component->GetParent() == GGame->GetPlayerEntity() )
		{
			CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
			ASSERT( commonMapManager );
			if ( commonMapManager )
			{
				commonMapManager->OnPlayerEnteredInterior( this );
			}

			CR4Player* player = Cast< CR4Player >( GCommonGame->GetPlayer() );
			if ( player )
			{
				if( !player->IsInInterior() )
				{
					player->CallEvent( CNAME( OnInteriorStateChanged ), true );
				}
				CallEvent( CNAME( OnPlayerEntered), true );

				player->SetInInterior( 1 );
			}
			
			if( m_weatherManager ) m_weatherManager->SetPlayerIsInInterior( true );
		}
		else 
		{
			CNewNPC* npc = Cast< CNewNPC >( component->GetParent() );
			if ( npc )
			{
				npc->SetInInterior( 1 );
			}
		}

		CActor* actor = Cast< CActor >( component->GetParent() );
		if( actor )
		{
			m_actorsInside.PushBack( actor );
		}
	}
}

void CR4InteriorAreaComponent::ExitedArea( CComponent* component )
{
	TBaseClass::ExitedArea( component ) ;

	if ( component && GGame->IsActive() )
	{
		if ( component->GetParent() == GGame->GetPlayerEntity() )
		{
			CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
			ASSERT( commonMapManager );
			if ( commonMapManager )
			{
				commonMapManager->OnPlayerExitedInterior( this );
			}

			CR4Player* player = Cast< CR4Player >( GCommonGame->GetPlayer() );
			if ( player )
			{
				player->SetInInterior( -1 );

				if( !player->IsInInterior() )
				{
					player->CallEvent( CNAME( OnInteriorStateChanged ), false );
				}
				CallEvent( CNAME( OnPlayerEntered), false );
			}

			if( m_weatherManager ) m_weatherManager->SetPlayerIsInInterior( false );
		}
		else 
		{
			CNewNPC* npc = Cast< CNewNPC >( component->GetParent() );
			if ( npc )
			{
				npc->SetInInterior( -1 );
			}
		}

		CActor* actor = Cast< CActor >( component->GetParent() );
		if( actor )
		{
			for( Int32 i = 0; i<m_actorsInside.SizeInt(); ++i )
			{
				if( m_actorsInside[ i ].Get() == actor )
				{
					m_actorsInside.RemoveAtFast( i );
					break;
				}
			}
		}
	}
}

#ifndef NO_EDITOR
void CR4InteriorAreaComponent::OnNavigationCook( CWorld* world, CNavigationCookingContext* context )
{
	if ( false == context->ShouldIgnorePathlib() )
	{
		context->GetPathlibCookerData()->GetSpecialZones()->Collect( this, 0, PathLib::NF_INTERIOR );
	}
}
#endif

////////////////////            CInteriorsRegistry         //////////////////////////////////////////////
void CInteriorsRegistry::Add( CR4InteriorAreaComponent* interior )
{
	CInteriorsRegistryMemberData member;
	member.m_interior = interior;
	CQuadTreeStorage::Add( member );
}

void CInteriorsRegistry::Remove( CR4InteriorAreaComponent* interior )
{
	CInteriorsRegistryMemberData member;
	member.m_interior = interior;
	CQuadTreeStorage::Remove( member );
}

Int32 CInteriorsRegistry::FindInteriorContainingPoint( const Vector& point, CR4InteriorAreaComponent** foundInteriors, Int32 maxElements )
{
	struct Functor
	{
		enum { SORT_OUTPUT = false };
		
		Vector						m_point;				
		Int32						m_maxElements;
		Int32						m_nextFreeElement;
		CR4InteriorAreaComponent**	m_foundInteriors;

		Functor( const Vector& point, CR4InteriorAreaComponent** foundInteriors, Int32 maxElements )
			: m_point( point )
			, m_maxElements( maxElements )
			, m_nextFreeElement( 0 )
			, m_foundInteriors( foundInteriors )
		{
			ASSERT( m_maxElements > 0 );
		}

		RED_FORCE_INLINE Bool operator()( const CInteriorsRegistryMemberData& memberData )
		{	
			CR4InteriorAreaComponent* interior = memberData.Get();
			if( interior->TestPointOverlap( m_point ) )
			{
				ASSERT( m_nextFreeElement < m_maxElements );
				m_foundInteriors[ m_nextFreeElement++ ] = interior;
				return m_nextFreeElement < m_maxElements;
			}
			return true;
		}
	};

	Functor func( point, foundInteriors, maxElements );

	Box bbox( Vector::ZEROS, 0.1f );	

	TQuery( point, func, bbox, true, NULL, 0 );

	return func.m_nextFreeElement;

}

////////////////////            ~CInteriorsRegistry         //////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CR4InteriorAreaEntity );