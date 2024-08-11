#include "build.h"

#include "upgradesCustomisationComponent.h"
#include "../../common/core/gatheredResource.h"

IMPLEMENT_RTTI_ENUM( ER6BodyType );
IMPLEMENT_ENGINE_CLASS( SCustomisationSlotDefinition );
IMPLEMENT_ENGINE_CLASS( SCustomisationCategoryDefinition );
IMPLEMENT_ENGINE_CLASS( CUpgradesCustomisationComponent );

RED_DEFINE_STATIC_NAME( bodyTypeEnum );

CGatheredResource resCustomizationSlots( TXT("gameplay\\globals\\customisationslots.csv"), RGF_Startup );

void CUpgradesCustomisationComponent::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	valueProperties.m_array = resCustomizationSlots.LoadAndGet< C2dArray >();
	valueProperties.m_valueColumnName = TXT("Name");
}

THandle< CEntityTemplate > SCustomisationSlotDefinition::FlipSlot()
{
	if( m_templates.Size()==0 )
		return NULL;

	++m_currentTemplate;
	if( m_currentTemplate<0 || m_currentTemplate>= (Int32)m_templates.Size() )
		m_currentTemplate = 0;

	return m_templates[ m_currentTemplate ];
}

THandle< CEntityTemplate > SCustomisationCategoryDefinition::FlipSlotContent( CName slotName )
{
	for( Uint32 i=0; i<m_slots.Size(); ++i )
	{
		if( m_slots[i].m_slotName == slotName )
		{
			return m_slots[i].FlipSlot();
		}
	}
	return NULL;
}

void CUpgradesCustomisationComponent::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );

	m_thisItemPart = GetEntity()->FindComponent< CItemPartDefinitionComponent >();
	
	m_currentCategory = NULL;
	if( m_defaultCategory )
	{	
		ChangeCategory( m_defaultCategory );
	}	
}

void CUpgradesCustomisationComponent::FlipSlotContent( CName slotName )
{
	if( !m_currentCategory || !m_thisItemPart.Get() )
		return;
	
	const THandle< CEntityTemplate > templateToPlug = m_currentCategory->FlipSlotContent( slotName );
	if( !templateToPlug )
		return;
	
	m_thisItemPart.Get()->PlugEntityTemplateToSlotInit( templateToPlug, slotName, true );
}

void CUpgradesCustomisationComponent::ChangeCategory( CName newCategoryName )
{	
	for( Uint32 i=0; i<m_categories.Size(); ++i )
	{
		if( m_categories[i].m_categoryName == newCategoryName )
		{
			GetEntity()->SetBehaviorVariable( CNAME( bodyTypeEnum ), ( Float )m_categories[i].m_bodyType );
			//if( m_currentCategory == &(m_categories[i]) )
			SpawnDefaultsSlotContent( m_categories[i] );
			m_currentCategory = &(m_categories[i]);
			break;
		}
	}
}


void CUpgradesCustomisationComponent::SpawnDefaultsSlotContent( SCustomisationCategoryDefinition& category )
{
	if( !m_thisItemPart.Get() )
		return;

	for( Uint32 i=0; i<category.m_slots.Size(); ++i )
	{
		const THandle< CEntityTemplate > newEntityToPlug = category.m_slots[i].FlipSlot();
		if( newEntityToPlug )
		{
			m_thisItemPart.Get()->PlugEntityTemplateToSlotInit( newEntityToPlug, category.m_slots[i].m_slotName, false );
		}		
	}
}

void CUpgradesCustomisationComponent::funcFlipSlotContent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slotToFlip, CName::NONE );
	FINISH_PARAMETERS;

	FlipSlotContent( slotToFlip );
}

void CUpgradesCustomisationComponent::funcChangeCategory( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, newCategory, CName::NONE );
	FINISH_PARAMETERS;

	ChangeCategory( newCategory );
}



