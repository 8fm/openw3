/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorEditorPanel.h"

class CBehaviorGraphAnimationBaseSlotNode;

class CEdBehaviorGraphSlotTester	: public CEdBehaviorEditorSimplePanel
									, public ISlotAnimationListener
{
	DECLARE_EVENT_TABLE()

	CBehaviorGraphAnimationBaseSlotNode* m_slot;

	TDynArray< CName > m_slotAnimations;
	TDynArray< CName > m_localSlotAnimation;

	wxChoice*		m_slotChoice;
	wxTreeCtrl*		m_animTree;
	wxListBox*		m_animList;

	wxStaticText*	m_dispDuration;
	wxStaticText*	m_dispProgress;
	wxStaticText*	m_dispTime;
	wxStaticText*	m_dispBlendIn;
	wxStaticText*	m_dispBlendOut;
	wxCheckBox*		m_dispBlendInCheck;
	wxCheckBox*		m_dispBlendOutCheck;

	Float			m_paintTimerSign;
	Float			m_paintTimer;
	static const Float PAINT_TIMER_DURATION;

public:
	CEdBehaviorGraphSlotTester( CEdBehaviorEditor* editor );

	virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, EStatus status );
	virtual String GetListenerName() const { return TXT("CEdBehaviorGraphSlotTester"); }

public:

	virtual wxString	GetPanelName() const	{ return wxT("SlotTester"); }
	virtual wxString	GetPanelCaption() const { return wxT("Slot tester"); }
	virtual wxString	GetInfo() const			{ return wxT("Slot tester"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnPanelClose();
	virtual void OnReset();
	virtual void OnInstanceReload();
	virtual void OnTick( Float dt );
	virtual void OnNodesSelect( const TDynArray< CGraphBlock* >& nodes );
	virtual void OnNodesDeselect();
	virtual void OnPrintNodes( CEdGraphEditor* graphCanvas );

	static wxString GetProgressStr( Float progress );
	static wxString GetTimeStr( Float time );

protected:
	void OnSlotChoice( wxCommandEvent& event );
	void OnPlay( wxCommandEvent& event );
	void OnStop( wxCommandEvent& event );
	void OnLoop( wxCommandEvent& event );
	void OnAdd( wxCommandEvent& event );
	void OnRemove( wxCommandEvent& event );

protected:
	Bool IsAutoSelect() const;
	Bool IsLooped()		const;
	Float GetBlendIn()	const;
	Float GetBlendOut() const;
	Bool IsAnimLooped() const;
	CName GetSlotName() const;

	Bool PlayNextAnim();
	void StopAnim();
	void FinishAnim();

	void FillSlotChoice();
	void FillAnimTree();
	void UpdateAnimList( Int32 sel = -1 );
	void UpdateSlotDisp();

	void SetFreezeUserElem( Bool flag );
};
