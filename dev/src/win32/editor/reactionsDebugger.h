/**8
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/game/newNpc.h"

void StartReactionsDebugger( CNewNPC* npc );

class IReactionDebugListener
{
	virtual void UpdateNPCData() = 0;
};

class CEdReactionsDebugger : public wxSmartLayoutPanel, public IEdEventListener, public IReactionsDebugListener
{
	DECLARE_EVENT_TABLE()

protected:
	wxListCtrl* m_managerInstancesCtrl;
	wxListCtrl* m_npcDelayedCtrl;
	wxListCtrl* m_npcAffectingCtrl;
	wxStaticText* m_npcNameStaticText;
	wxStaticText* m_npcRainTimeStaticText;

	CEdTimer*			m_timer;
	THandle< CNewNPC >	m_npc;

	static const Int32 COLUMN_INSTANCES_NAME		= 0;
	static const Int32 COLUMN_INSTANCES_TARGET	= 1;
	static const Int32 COLUMN_INSTANCES_NODE		= 2;
	static const Int32 COLUMN_INSTANCES_POSITION	= 3;
	static const Int32 COLUMN_INSTANCES_TTL		= 4;

	static const Int32 COLUMN_REACTIONS_NAME		= 0;
	static const Int32 COLUMN_REACTIONS_NODE		= 1;
	static const Int32 COLUMN_REACTIONS_POSITION	= 2;
	static const Int32 COLUMN_REACTIONS_INDEX		= 3;

public:
	CEdReactionsDebugger( wxWindow* parent, CNewNPC* npc );
	~CEdReactionsDebugger();

	// IEdEventListener
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data ) {}

	void OnTimer( wxCommandEvent &event );

	virtual void UpdateDelayed();
	virtual void UpdateAffecting();	

private:
	void FillManagerData();	
	void FillManagerItem( const CInterestPointInstance* instance, CNewNPC* target, Uint32 id );
	void SetupReactionList( wxListCtrl* listCtrl );
	void ClearReactionData( wxListCtrl* listCtrl );
	void FillReactionData( CNewNPC* npc, wxListCtrl* listCtrl, const TDynArray< CNewNPC::InterestPointReactionData >& dataArray );
	void SetNPCName();
	static wxString VectorToString( const Vector& vec );
};