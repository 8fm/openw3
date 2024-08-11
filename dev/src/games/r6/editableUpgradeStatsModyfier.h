#pragma once


#include "baseUpgradeStatsModyfier.h"
#include "statsContainerComponent.h"

class CStatsContainerComponent;

struct SEditableUpgradeStatsModifierEntry
{
	DECLARE_RTTI_STRUCT( SEditableUpgradeStatsModifierEntry );	

	CName		m_name;
	EStatsType	m_type;
	Float		m_value;
	Bool		m_addIfNotExists;	
};

BEGIN_CLASS_RTTI( SEditableUpgradeStatsModifierEntry )
	//PROPERTY_EDIT( m_name			, TXT("") );	
	PROPERTY_CUSTOM_EDIT( m_name	, TXT("Stat name"), TXT("StatisticsNamesEditor") );
	PROPERTY_EDIT( m_type	, TXT("Type of stat" )	);	
	PROPERTY_EDIT( m_value			, TXT("") );
	PROPERTY_EDIT( m_addIfNotExists	, TXT("") );
END_CLASS_RTTI();

class CEditableUpgradeStatsModyfier : public CBaseUpgradeStatsModyfier, public I2dArrayPropertyOwner
{
	DECLARE_ENGINE_CLASS( CEditableUpgradeStatsModyfier, CBaseUpgradeStatsModyfier, 0 );
private:
	TDynArray< SEditableUpgradeStatsModifierEntry > m_statsModification;

public:
	void ApplyChanges( CStatsContainerComponent* statsContainer, CEntity* ownerEnt ) override;
	void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties ) override;
};

BEGIN_CLASS_RTTI( CEditableUpgradeStatsModyfier )	
	PARENT_CLASS( CBaseUpgradeStatsModyfier );
	PROPERTY_EDIT( m_statsModification	, TXT("") );
END_CLASS_RTTI();