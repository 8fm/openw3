#include "build.h"

#include "itemPartSlotDefinition.h"
#include "itemPartDefinitionComponent.h"

IMPLEMENT_ENGINE_CLASS( CItemPartSlotDefinition );

void CItemPartSlotDefinition::SetPluggedPart( CItemPartDefinitionComponent* pluggedPart )
{ 
	m_pluggedPart = THandle< CComponent >( pluggedPart ); 
}

CItemPartDefinitionComponent* CItemPartSlotDefinition::GetPluggenPart( )
{
	CComponent* cmp = m_pluggedPart.Get();
	if( cmp && cmp->IsA< CItemPartDefinitionComponent >() )
	{
		return static_cast< CItemPartDefinitionComponent* >( cmp );
	}
	return NULL;
}