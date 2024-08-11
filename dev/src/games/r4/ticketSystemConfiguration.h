#pragma once

class CTicketSource;

class CTicketSourceConfiguration
{
	DECLARE_RTTI_SIMPLE_CLASS( CTicketSourceConfiguration );
public:
	CName						m_name;
	Uint16						m_ticketsPoolSize;
	Float						m_minimalImportance;

	CTicketSourceConfiguration()
		: m_ticketsPoolSize( 100 )
		, m_minimalImportance( 100.f )										{}

	void						SetupTicketSource( CTicketSource& outSource );
};

BEGIN_CLASS_RTTI( CTicketSourceConfiguration )
	PROPERTY_EDIT( m_name, TXT("Ticket source name") )
	PROPERTY_EDIT( m_ticketsPoolSize, TXT("Size of tickets pool" ) )
	PROPERTY_EDIT( m_minimalImportance, TXT("Minimal importance needed to get ticket" ) )
END_CLASS_RTTI()

class CTicketingConfiguration
{
protected:
	TDynArray< CTicketSourceConfiguration >				m_tickets;

public:
	CTicketSourceConfiguration* GetConfiguration( CName name );

	void PostEditionSort();
};


class CTicketConfigurationParam : public CGameplayEntityParam, public CTicketingConfiguration
{
	DECLARE_ENGINE_CLASS( CTicketConfigurationParam, CGameplayEntityParam, 0 );

public:
	CTicketConfigurationParam()												{}

	void OnPreSave() override;
};

BEGIN_CLASS_RTTI( CTicketConfigurationParam )
	PARENT_CLASS( CGameplayEntityParam )
	PROPERTY_EDIT( m_tickets, TXT("Tickets configuration") )
END_CLASS_RTTI()

class CTicketsDefaultConfiguration : public CObject, public CTicketingConfiguration
{
	DECLARE_ENGINE_CLASS( CTicketsDefaultConfiguration, CObject, 0 );

public:
	CTicketsDefaultConfiguration()											{}

	void Initialize();

private:
	void funcSetupTicketSource( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CTicketsDefaultConfiguration )
	PARENT_CLASS( CObject )
	PROPERTY_EDIT( m_tickets, TXT("Tickets configuration") )
	NATIVE_FUNCTION( "SetupTicketSource", funcSetupTicketSource )
END_CLASS_RTTI()