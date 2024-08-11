/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "reactionsDebugger.h"
#include "../../common/game/reactionsManager.h"
#include "../../common/game/interestPointComponent.h"

void StartReactionsDebugger( CNewNPC* npc )
{	
	CEdReactionsDebugger* editor = new CEdReactionsDebugger( NULL, npc );		
}

// Event table
BEGIN_EVENT_TABLE( CEdReactionsDebugger, wxSmartLayoutPanel )
END_EVENT_TABLE()

CEdReactionsDebugger::CEdReactionsDebugger( wxWindow* parent, CNewNPC* npc )
: wxSmartLayoutPanel( parent, TXT("ReactionsDebugger"), false )
{
	m_npc = npc;

	m_managerInstancesCtrl = XRCCTRL( *this, "managerInstancesList", wxListCtrl );
	m_npcDelayedCtrl = XRCCTRL( *this, "npcDelayedList", wxListCtrl );
	m_npcAffectingCtrl = XRCCTRL( *this, "npcAffectingList", wxListCtrl );
	m_npcNameStaticText = XRCCTRL( *this, "npcNameStaticText", wxStaticText );
	m_npcRainTimeStaticText = XRCCTRL( *this, "npcRainTimeStaticText", wxStaticText );

	m_managerInstancesCtrl->InsertColumn( COLUMN_INSTANCES_NAME, TXT("Name") );
	m_managerInstancesCtrl->InsertColumn( COLUMN_INSTANCES_TARGET, TXT("Target") );
	m_managerInstancesCtrl->InsertColumn( COLUMN_INSTANCES_NODE, TXT("Node") );
	m_managerInstancesCtrl->InsertColumn( COLUMN_INSTANCES_POSITION, TXT("Position") );
	m_managerInstancesCtrl->InsertColumn( COLUMN_INSTANCES_TTL, TXT("TTL") );

	m_managerInstancesCtrl->SetColumnWidth( COLUMN_INSTANCES_NAME, 150 );
	m_managerInstancesCtrl->SetColumnWidth( COLUMN_INSTANCES_TARGET, 100 );
	m_managerInstancesCtrl->SetColumnWidth( COLUMN_INSTANCES_NODE, 100 );
	m_managerInstancesCtrl->SetColumnWidth( COLUMN_INSTANCES_POSITION, 150 );
	m_managerInstancesCtrl->SetColumnWidth( COLUMN_INSTANCES_TTL, 100 );

	SetupReactionList( m_npcDelayedCtrl );
	SetupReactionList( m_npcAffectingCtrl );

	Layout();
	Show();
	Raise();

	m_timer = new CEdTimer();
	m_timer->Connect( wxEVT_TIMER, wxCommandEventHandler( CEdReactionsDebugger::OnTimer ), NULL, this );
	m_timer->Start( 200, false );

	FillManagerData();

	if( npc )
	{
		npc->SetReactionsDebugListener( this );
		SetNPCName();
	}
}

CEdReactionsDebugger::~CEdReactionsDebugger()
{
	delete m_timer;
	m_timer = NULL;

	if( m_npc.Get() )
	{
		m_npc.Get()->SetReactionsDebugListener( NULL );
	}
}

void CEdReactionsDebugger::SetupReactionList( wxListCtrl* listCtrl )
{
	listCtrl->InsertColumn( COLUMN_REACTIONS_NAME, TXT("Name") );
	listCtrl->InsertColumn( COLUMN_REACTIONS_NODE, TXT("Node") );
	listCtrl->InsertColumn( COLUMN_REACTIONS_POSITION, TXT("Position") );
	listCtrl->InsertColumn( COLUMN_REACTIONS_INDEX, TXT("Index") );

	listCtrl->SetColumnWidth( COLUMN_REACTIONS_NAME, 150 );
	listCtrl->SetColumnWidth( COLUMN_REACTIONS_NODE, 100 );
	listCtrl->SetColumnWidth( COLUMN_REACTIONS_POSITION, 150 );
	listCtrl->SetColumnWidth( COLUMN_REACTIONS_INDEX, 100 );
}

void CEdReactionsDebugger::OnTimer( wxCommandEvent &event )
{
	FillManagerData();
	SetNPCName();
}

void CEdReactionsDebugger::SetNPCName()
{
	CNewNPC* npc = m_npc.Get();
	if( npc )
	{
		m_npcNameStaticText->SetLabel( wxString::Format( wxT( "NPC: %s" ), npc->GetName().AsChar() ) );
	}
	else
	{
		m_npcNameStaticText->SetLabel( wxT( "NPC: None" ) );
	}
}

void CEdReactionsDebugger::UpdateDelayed()
{
	CNewNPC* npc = m_npc.Get();
	if( npc )
	{
		FillReactionData( npc, m_npcDelayedCtrl, npc->GetDelayedInterestPoints() );
	}
	else
	{
		ClearReactionData( m_npcDelayedCtrl );
	}
}

void CEdReactionsDebugger::UpdateAffecting()
{
	CNewNPC* npc = m_npc.Get();
	if( npc )
	{
		FillReactionData( npc, m_npcAffectingCtrl, npc->GetAffectingInterestPoints() );
	}
	else
	{
		ClearReactionData( m_npcAffectingCtrl );
	}
}

void CEdReactionsDebugger::FillManagerItem( const CInterestPointInstance* instance, CNewNPC* target, Uint32 id )
{
	// Name
	{
		wxListItem item;
		item.SetId( id );
		item.SetColumn( COLUMN_INSTANCES_NAME );

		if( instance->GetParentPoint() )
			item.SetText( instance->GetParentPoint()->GetFieldName().AsString().AsChar() );
		else
			item.SetText( wxT("-") );

		m_managerInstancesCtrl->InsertItem( item );
	}

	// Target
	{
		wxListItem item;
		item.SetId( id );
		item.SetColumn( COLUMN_INSTANCES_TARGET );
		if( target )
			item.SetText( target->GetName().AsChar() );
		else
			item.SetText( wxT("Broadcast") );
		Bool res = m_managerInstancesCtrl->SetItem( item );
		ASSERT( res );
	}

	// Node
	{
		wxListItem item;
		item.SetId( id );
		item.SetColumn( COLUMN_INSTANCES_NODE );					
		const CNode* node = instance->GetNode().Get();
		if( node )
			item.SetText( node->GetName().AsChar() );
		else
			item.SetText( wxT("NULL") );
		Bool res = m_managerInstancesCtrl->SetItem( item );
		ASSERT( res );
	}

	// Position
	{
		wxListItem item;
		item.SetId( id );
		item.SetColumn( COLUMN_INSTANCES_POSITION );					
		item.SetText( VectorToString( instance->GetWorldPosition() ) );
		Bool res = m_managerInstancesCtrl->SetItem( item );
		ASSERT( res );
	}

	// TTL
	{
		wxListItem item;
		item.SetId( id );
		item.SetColumn( COLUMN_INSTANCES_TTL );					
		item.SetText( wxString::Format( wxT("%.2f"), instance->GetTimeToLive() ) );				
		Bool res = m_managerInstancesCtrl->SetItem( item );
		ASSERT( res );
	}	
}

void CEdReactionsDebugger::FillManagerData()
{
	CReactionsManager* mgr = GCommonGame->GetReactionsManager();
	if( !mgr )
	{
		m_managerInstancesCtrl->Freeze();
		m_managerInstancesCtrl->DeleteAllItems();
		m_managerInstancesCtrl->Thaw();
		return;
	}

	const TDynArray< CInterestPointInstance* >& instances = mgr->GetInstances();	
	const TDynArray< CReactionsManager::MappedInstance >& mappedInstances = mgr->GetMappedInstances();

	m_managerInstancesCtrl->Freeze();
	m_managerInstancesCtrl->DeleteAllItems();

	Uint32 id = 0;

	for( Uint32 i=0; i<instances.Size(); i++ )
	{
		const CInterestPointInstance* instance = instances[i];

		if( !instance )
			continue;

		FillManagerItem( instance, NULL, id );

		id++;
	}

	for( Uint32 i=0; i<mappedInstances.Size(); i++ )
	{
		const CInterestPointInstance* instance = mappedInstances[i].m_instance;

		if( !instance )
			continue;

		FillManagerItem( instance, mappedInstances[i].m_npc, id );

		id++;
	}

	m_managerInstancesCtrl->Thaw();
}

void CEdReactionsDebugger::ClearReactionData( wxListCtrl* listCtrl )
{
	listCtrl->DeleteAllItems();
	listCtrl->Disable();	
}

void CEdReactionsDebugger::FillReactionData( CNewNPC* npc, wxListCtrl* listCtrl, const TDynArray< CNewNPC::InterestPointReactionData >& dataArray )
{
	ASSERT( npc );

	listCtrl->Freeze();
	listCtrl->DeleteAllItems();

	Uint32 id = 0;

	for( Uint32 i=0; i<dataArray.Size(); i++ )
	{
		CInterestPointInstance* instance = dataArray[i].m_interestPoint.Get();

		if( !instance )
			continue;

		// Name
		{
			wxListItem item;
			item.SetId( id );
			item.SetColumn( COLUMN_REACTIONS_NAME );

			if( instance->GetParentPoint() )
				item.SetText( instance->GetParentPoint()->GetFieldName().AsString().AsChar() );
			else
				item.SetText( wxT("-") );

			listCtrl->InsertItem( item );
		}

		// Node
		{
			wxListItem item;
			item.SetId( id );
			item.SetColumn( COLUMN_REACTIONS_NODE );					
			const CNode* node = instance->GetNode().Get();
			if( node )
				item.SetText( node->GetName().AsChar() );
			else
				item.SetText( wxT("NULL") );
			Bool res = listCtrl->SetItem( item );
			ASSERT( res );
		}

		// Position
		{
			wxListItem item;
			item.SetId( id );
			item.SetColumn( COLUMN_REACTIONS_POSITION );					
			item.SetText( VectorToString( instance->GetWorldPosition() ) );
			Bool res = listCtrl->SetItem( item );
			ASSERT( res );
		}

		// Index
		{
			wxListItem item;
			item.SetId( id );
			item.SetColumn( COLUMN_REACTIONS_INDEX );				
			item.SetText( ToString( dataArray[i].m_reactionIndex ).AsChar() );
			Bool res = listCtrl->SetItem( item );
			ASSERT( res );
		}	

		id++;
	}

	listCtrl->Thaw();
}

wxString CEdReactionsDebugger::VectorToString( const Vector& vec )
{
	return wxString::Format( wxT("%.2f %.2f %.2f"), vec.X, vec.Y, vec.Z );
}
