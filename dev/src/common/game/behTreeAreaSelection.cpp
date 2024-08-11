/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeAreaSelection.h"

#include "../engine/triggerAreaComponent.h"

#include "aiSpawnTreeParameters.h"
#include "behTreeInstance.h"


IMPLEMENT_RTTI_ENUM( EAIAreaSelectionMode )

IMPLEMENT_ENGINE_CLASS( CBehTreeValAreaSelectionMode )
IMPLEMENT_ENGINE_CLASS( SBehTreeAreaSelection )



void SBehTreeAreaSelection::InitInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, SBehTreeSelectedAreaInstance& outInstance ) const
{
	switch ( m_selectionMode.GetVal( context ) )
	{
	case EAIASM_Encounter:
		{
			CEncounter* encounter = CEncounterParameters::GetEncounter( context );
			if ( encounter )
			{
				outInstance.m_area = encounter->GetTriggerArea();
			}
		}
		break;

	case EAIASM_GuardArea:
		outInstance.m_guardAreaPtr = CBehTreeGuardAreaDataPtr( owner );
		break;

	case EAIASM_ByTag:
		{
			CName tag = m_optionalAreaTag.GetVal( context );
			if ( !tag.Empty() )
			{
				CTagManager* tagManager = GGame->GetActiveWorld()->GetTagManager();
				CEntity* entity = tagManager->GetTaggedEntity( tag );
				if ( entity )
				{
					CAreaComponent* areaComponent = entity->FindComponent< CAreaComponent >();
					if ( areaComponent )
					{
						outInstance.m_area = areaComponent;
					}
				}
			}
		}
		break;

	case EAIASM_ByTagInEncounter:
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

			CName tag = m_optionalAreaTag.GetVal( context );
			if ( !tag.Empty() )
			{
				
				CEncounter* encounter = CEncounterParameters::GetEncounter( context );
				CAreaComponent* encounterArea = encounter ? encounter->GetTriggerArea() : nullptr;
				if ( encounterArea )
				{
					Collector collector( encounterArea );

					CTagManager* tagManager = GGame->GetActiveWorld()->GetTagManager();
					CEntity* entity = tagManager->GetTaggedEntity( tag );
					if ( entity )
					{
						CAreaComponent* areaComponent = entity->FindComponent< CAreaComponent >();
						if ( areaComponent )
						{
							outInstance.m_area = areaComponent;
						}
					}
				}
			}
		}
		break;

	default:
		break;

	}
}

CAreaComponent* SBehTreeSelectedAreaInstance::GetArea()
{
	CAreaComponent* area = m_area.Get();
	if ( area )
	{
		return area;
	}
	if ( m_guardAreaPtr )
	{
		return m_guardAreaPtr->GetGuardArea();
	}
	return nullptr;
}
