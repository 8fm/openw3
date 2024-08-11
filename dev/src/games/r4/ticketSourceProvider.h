#pragma once

class CTicketSource;
class CTicketSourceConfiguration;

class ITicketSourceProvider
{
protected:
	THashMap< CName, CTicketSource* >	m_ticketSources;
public:
	~ITicketSourceProvider();
	CTicketSource* GetTicketSource( CName name, Bool lazyCreate = true );
	virtual CTicketSourceConfiguration* GetCustomConfiguration( CName name ) { return nullptr; };
	void ClearTickets();

	const THashMap< CName, CTicketSource* > GetTicketSourceList() const { return m_ticketSources; }
};