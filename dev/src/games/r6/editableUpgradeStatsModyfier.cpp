#include "build.h"

#include "editableUpgradeStatsModyfier.h"
#include "statsContainerComponent.h"
#include "../../common/core/gatheredResource.h"

IMPLEMENT_ENGINE_CLASS( CEditableUpgradeStatsModyfier );
IMPLEMENT_ENGINE_CLASS( SEditableUpgradeStatsModifierEntry );

CGatheredResource resStats( TXT("gameplay\\globals\\statistics.csv"), RGF_Startup );

void CEditableUpgradeStatsModyfier::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	valueProperties.m_array = resStats.LoadAndGet< C2dArray >();
	valueProperties.m_valueColumnName = TXT("Name");
}

void CEditableUpgradeStatsModyfier::ApplyChanges( CStatsContainerComponent* statsContainer, CEntity* ownerEnt )
{
	for( Uint32 i=0; i<m_statsModification.Size(); ++i )
	{
		SEditableUpgradeStatsModifierEntry& stat = m_statsModification[ i ];
		statsContainer->AddStatValue( stat.m_name, stat.m_value, stat.m_type, stat.m_addIfNotExists );
	}
}