#pragma once


#include "r6Component.h"
#include "itemPartDefinitionComponent.h"

enum ER6BodyType
{
	R6BT_ManAverage	,
	R6BT_ManBig		
};

BEGIN_ENUM_RTTI(ER6BodyType);
	ENUM_OPTION( R6BT_ManAverage	);
	ENUM_OPTION( R6BT_ManBig		);
END_ENUM_RTTI();

struct SCustomisationSlotDefinition
{
	DECLARE_RTTI_STRUCT( SCustomisationSlotDefinition );
	Int32										m_currentTemplate;
	CName										m_slotName;
	TDynArray< THandle< CEntityTemplate > >		m_templates;

	THandle< CEntityTemplate > FlipSlot();

};
BEGIN_CLASS_RTTI( SCustomisationSlotDefinition );
	PROPERTY_CUSTOM_EDIT( m_slotName	, TXT("Name of slot"), TXT("2daValueSelection") );
	PROPERTY_EDIT( m_templates, TXT("Templates") );
END_CLASS_RTTI();

struct SCustomisationCategoryDefinition
{
	DECLARE_RTTI_STRUCT( SCustomisationCategoryDefinition );

	CName										m_categoryName;	
	ER6BodyType									m_bodyType;
	TDynArray< SCustomisationSlotDefinition >	m_slots;

	THandle< CEntityTemplate > FlipSlotContent( CName slotName );
};
BEGIN_CLASS_RTTI( SCustomisationCategoryDefinition );
	PROPERTY_EDIT( m_categoryName	, TXT("Category name")	);
	PROPERTY_EDIT( m_bodyType		, TXT("Body type")		);
	PROPERTY_EDIT( m_slots			, TXT("Slots")			);
END_CLASS_RTTI();


class CUpgradesCustomisationComponent : public CR6Component, public I2dArrayPropertyOwner
{
	DECLARE_ENGINE_CLASS( CUpgradesCustomisationComponent, CR6Component, 0 );

private:
	SCustomisationCategoryDefinition*				m_currentCategory;

	CName											m_defaultCategory;
	TDynArray< SCustomisationCategoryDefinition >	m_categories;
	THandle< CItemPartDefinitionComponent >			m_thisItemPart;

private:
	void SpawnDefaultsSlotContent( SCustomisationCategoryDefinition& category ); 

public:
	void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties ) override;
	void OnAttachFinished( CWorld* world ) override;

	void FlipSlotContent( CName slotName );	
	void ChangeCategory( CName newCategoryName );	

//	void funcFillAvailableParts( CScriptStackFrame& stack, void* result );
	void funcFlipSlotContent( CScriptStackFrame& stack, void* result );
	void funcChangeCategory( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CUpgradesCustomisationComponent );
	PARENT_CLASS( CR6Component );
	PROPERTY_EDIT( m_defaultCategory, TXT("Default category") );
	PROPERTY_EDIT( m_categories, TXT("Categories") );
	NATIVE_FUNCTION( "I_FlipSlotContent", funcFlipSlotContent );
	NATIVE_FUNCTION( "I_ChangeCategory", funcChangeCategory );
END_CLASS_RTTI();