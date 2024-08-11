/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "r4BehTreeInstance.h"
#include "ticketSystem.h"

RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC( 4355 )												// 'this' used in member initializer list

class ITicketSourceProvider;

enum EBehTreeTicketSourceProviderType
{
	BTTSP_Combat,
	BTTSP_Global,

	BTTSP_Size
};

BEGIN_ENUM_RTTI( EBehTreeTicketSourceProviderType )
	ENUM_OPTION( BTTSP_Combat );
	ENUM_OPTION( BTTSP_Global );
END_ENUM_RTTI()

//////////////////////////////////////////////////////////////////////////
// Ticket nodes AI storage data 
class CBehTreeTicketData : public CTicket::IListener
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehTreeTicketData );

protected:
	CR4BehTreeInstance*					m_owner;
	CTicket								m_ticket;
	CName								m_ticketName;
	Float								m_lastTicketAquisition;
	Bool								m_request;
	EBehTreeTicketSourceProviderType	m_sourceProvider;

	// CAIStorageItem::CNamedInitializer interface
	void			OnTicketLost() override;

	CTicketSource*	UpdateTicketSource() const;

	// tickets source providers
	ITicketSourceProvider* CombatTicketsSourceProvider() const;
	ITicketSourceProvider* GlobalTicketsSourceProvider() const;
	RED_INLINE ITicketSourceProvider* GetCurrentProvider() const			{ return m_sourceProvider == BTTSP_Combat ? CombatTicketsSourceProvider() : GlobalTicketsSourceProvider(); }

public:
	class CInitializer : public CAIStorageItem::CNamedInitializer
	{
		typedef CAIStorageItem::CNamedInitializer Super;
	protected:
		CBehTreeInstance*					m_owner;
		EBehTreeTicketSourceProviderType	m_sourceProvider;
	public:
		// NOTICE: We use same ticket name for ai storage UNIQUE name and ticket system UNIQUE name. Ticket names should follow some naming standard.
		CInitializer( CBehTreeInstance* owner, CName ticketName, EBehTreeTicketSourceProviderType sourceProvider );

		void InitializeItem( CAIStorageItem& item ) const override;
		IRTTIType* GetItemType() const override;
	};

	CBehTreeTicketData();
	~CBehTreeTicketData();

	// we wrap up all ticket interface so sometimes we can do something special / store some outcome
	Bool			CanAquireTicket( Float importance, Uint16 ticketsCount );
	Bool			Aquire( Float importance, Uint16 ticketsCount, Bool force );

	void			ManagerPersistantRequest();
	void			ManagerRequest( Float validityTime );
	void			ManagerClearRequest();
	void			FreeTicket();
	Bool			HasPendingRequest() const								{ return m_request; }
	void			ClearRequest()											{ m_request = false; }
	Bool			HasTicket() const										{ return m_ticket.HasTicket(); }
	void			LockTicket( Bool b )									{ m_ticket.Lock( b ); }
	CName			GetTicketName() const									{ return m_ticketName; }

	CTicket&		GetTicket()												{ return m_ticket; }
	const CTicket&	GetTicket() const										{ return m_ticket; }

	void			SetImportance( Float importance )						{ m_ticket.SetImportance( importance ); }
	Bool			UpdateImportance( Float importance )					{ return m_ticket.UpdateImportance( importance ); }
	Float			GetTimeSinceMyLastAquisition() const;
	Float			GetTimeSinceAnyoneLastAquisition() const;
	Float			GetLastAquisitionTime() const							{ return m_lastTicketAquisition; }
	void			NotifyOfTicketLost();
};

BEGIN_CLASS_RTTI( CBehTreeTicketData )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
// Storage pointer
class CBehTreeTicketDataPtr : public TAIStoragePtr< CBehTreeTicketData >
{
	typedef TAIStoragePtr< CBehTreeTicketData > Super;
public:
	CBehTreeTicketDataPtr( CBehTreeInstance* owner, CName ticketName, EBehTreeTicketSourceProviderType sourceProvider )
		: Super( CBehTreeTicketData::CInitializer( owner, ticketName, sourceProvider ), owner ) {}

	CBehTreeTicketDataPtr()
		: Super()															{}
	CBehTreeTicketDataPtr( const CBehTreeTicketDataPtr& p )
		: Super( p )														{}
	CBehTreeTicketDataPtr( CBehTreeTicketDataPtr&& p )
		: Super( Move( p ) )												{}
};

RED_WARNING_POP()