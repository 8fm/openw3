#pragma once



#include "baseUpgradeEventHandlerParam.h"

class CItemPartDefinitionComponent;

struct SUpgradeEventHandlerParam
{
	DECLARE_RTTI_STRUCT( SUpgradeEventHandlerParam );

	CName												m_eventName;
	CName												m_childSlotName;
	CItemPartDefinitionComponent*						m_owner;
	THandle< CUpgradeEventHandlerParam >				m_params;
	CUpgradeEventHandlerParam*							m_dynamicParams;

};
BEGIN_CLASS_RTTI( SUpgradeEventHandlerParam )	
	PROPERTY_NAME( m_eventName		, TXT("i_eventName")	);
	PROPERTY_NAME( m_childSlotName	, TXT("i_childSlotName"));
	PROPERTY_NAME( m_owner			, TXT("i_owner")		);
	PROPERTY_NAME( m_dynamicParams	, TXT("i_dynamicParams"));
END_CLASS_RTTI();

class CUpgradeEventHandler : public CObject
{
	DECLARE_ENGINE_CLASS( CUpgradeEventHandler, CObject, 0 );
public:	
	virtual void HandleEvent( SUpgradeEventHandlerParam& params ){};
};
BEGIN_CLASS_RTTI( CUpgradeEventHandler )
	PARENT_CLASS( CObject );	
END_CLASS_RTTI();

class CScriptedUgpradeEventHandler : public CUpgradeEventHandler
{
	DECLARE_ENGINE_CLASS( CScriptedUgpradeEventHandler, CUpgradeEventHandler, 0 );
	
public:	
	void HandleEvent( SUpgradeEventHandlerParam& params ) override;
};

BEGIN_CLASS_RTTI( CScriptedUgpradeEventHandler )
	PARENT_CLASS( CUpgradeEventHandler );	
END_CLASS_RTTI();