#include "build.h"
#include "spawnTreeInitializerGuardArea.h"

#include "behTreeGuardAreaData.h"
#include "behTreeInstance.h"
#include "behTreeMachine.h"
#include "encounter.h"

#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/tagManager.h"
#include "../core/mathUtils.h"
#include "../engine/triggerAreaComponent.h"

IMPLEMENT_ENGINE_CLASS( ISpawnTreeInitializerGuardAreaBase )
IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerGuardArea )
IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerGuardAreaByHandle )

////////////////////////////////////////////////////////////////////
// ISpawnTreeInitializerGuardAreaBase
////////////////////////////////////////////////////////////////////
ISpawnTreeInitializer::EOutput ISpawnTreeInitializerGuardAreaBase::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	CBehTreeMachine* behTreeMachine = actor->GetBehTreeMachine();
	CBehTreeInstance* behTreeInstance = behTreeMachine ? behTreeMachine->GetBehTreeInstance() : nullptr;
	if ( !behTreeInstance )
	{
		return OUTPUT_SUCCESS;
	}


	CBehTreeGuardAreaData* data = CBehTreeGuardAreaData::Find( behTreeInstance );
	if ( !data )
	{
		return OUTPUT_SUCCESS;
	}

	CAreaComponent* guardArea = nullptr;
	CAreaComponent* pursuitArea = nullptr;

	Configure( guardArea, pursuitArea, instance );

	if ( m_pursuitRange >= 0.f )
	{
		data->SetupBaseState( guardArea, pursuitArea, m_pursuitRange );
	}
	else
	{
		data->SetupBaseState( guardArea, pursuitArea );
	}
	

	actor->SignalGameplayEvent( CNAME( AI_Change_Guard_Area ) );

	return OUTPUT_SUCCESS;
}

Bool ISpawnTreeInitializerGuardAreaBase::IsConflicting( const ISpawnTreeInitializer* initializer ) const
{
	return initializer->IsA< ISpawnTreeInitializerGuardAreaBase >();
}



////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerGuardArea
////////////////////////////////////////////////////////////////////
void CSpawnTreeInitializerGuardArea::Configure( CAreaComponent*& outGuardArea, CAreaComponent*& outPursuitArea, CSpawnTreeInstance* instance ) const
{
	struct Collector : public CTagManager::DefaultNodeIterator
	{
		Collector( CAreaComponent* area )
			: m_encounterArea( area )
			, m_bbox( area->GetBoundingBox() )
			, m_outArea( NULL )										{}

		void Process( CNode* node, Bool isInitialList )
		{
			CEntity* entity = Cast< CEntity >( node );
			if ( !entity )
			{
				return;
			}
			CAreaComponent* area = entity->FindComponent< CAreaComponent >();
			if ( !area || !area->GetBoundingBox().Touches( m_bbox ) )
			{
				return;
			}

			// Z boundings are tested - run 2d test
			if ( MathUtils::GeometryUtils::IsPolygonsIntersecting2D( m_encounterArea->GetWorldPoints(), area->GetWorldPoints() ) )
			{
				m_outArea = area;
			}
		}

		CAreaComponent*		m_encounterArea;
		Box					m_bbox;

		CAreaComponent*		m_outArea;
	};

	auto funFindArea =
		[ this, &instance ] ( CName tag ) -> CAreaComponent*
	{
		CTagManager* tagManager = GGame->GetActiveWorld()->GetTagManager();
		if ( m_findAreasInEncounter && instance )
		{
			CEncounter* encounter = instance->GetEncounter();
			CAreaComponent* area = encounter->GetTriggerArea();

			if ( area )
			{
				Collector collector( area );
				tagManager->IterateTaggedNodes( tag, collector );
				return collector.m_outArea;
			}
		}
		else
		{
			CEntity* guardAreaEntity = tagManager->GetTaggedEntity( tag );
			if( guardAreaEntity )
			{
				return guardAreaEntity->FindComponent< CAreaComponent >();
			}
		}
		return nullptr;
	};

	CAreaComponent* guardArea;
	CAreaComponent* pursuitArea;

	if ( instance )
	{
		guardArea = (*instance)[ i_guardArea ].Get();
		pursuitArea = (*instance)[ i_pursueArea ].Get();

		if ( !guardArea )
		{
			if ( !m_guardAreaTag.Empty() )
			{
				guardArea = funFindArea( m_guardAreaTag );
			}
			pursuitArea = nullptr;
			if ( !m_pursuitAreaTag.Empty() && guardArea )
			{
				CAreaComponent* area = funFindArea( m_pursuitAreaTag );
				if ( area )
				{
					pursuitArea = area;
				}
			}

			(*instance)[ i_guardArea ] = guardArea;
			(*instance)[ i_pursueArea ] = pursuitArea;
		}
	}
	else
	{
		guardArea = m_guardAreaTag ? funFindArea( m_guardAreaTag ) : nullptr;
		pursuitArea = (m_pursuitAreaTag && guardArea) ? funFindArea( m_pursuitAreaTag ) : nullptr;
	}

	outGuardArea = guardArea;
	outPursuitArea = pursuitArea;
}

String CSpawnTreeInitializerGuardArea::GetEditorFriendlyName() const
{
	static String STR( TXT("Guard Area by tag") );
	return STR;
}

void CSpawnTreeInitializerGuardArea::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_guardArea;
	compiler << i_pursueArea;
}

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerGuardAreaByHandle
////////////////////////////////////////////////////////////////////
void CSpawnTreeInitializerGuardAreaByHandle::Configure( CAreaComponent*& outGuardArea, CAreaComponent*& outPursuitArea, CSpawnTreeInstance* instance ) const
{
	auto funGetArea =
		[] ( EntityHandle& h ) -> CAreaComponent*
		{
			CEntity* e = h.Get();
			if ( e )
			{
				return e->FindComponent< CAreaComponent >();
			}
			return NULL;
		};

	CAreaComponent* guardArea = funGetArea( m_guardArea );
	CAreaComponent* pursuitArea = funGetArea( m_pursuitArea );

	outGuardArea = guardArea;
	outPursuitArea = pursuitArea;
}
String CSpawnTreeInitializerGuardAreaByHandle::GetEditorFriendlyName() const
{
	return TXT( "Guard Area" );
}
