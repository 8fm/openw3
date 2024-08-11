/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/questsSystem.h"
#include "../../common/game/questThread.h"
#include "questEdTool.h"
#include "questDebugInfo.h"

class wxTreeCtrl;

///////////////////////////////////////////////////////////////////////////////

enum EQuestsDebuggerIcons
{
	QDG_ICON_BLOCK,
	QDG_ICON_THREAD,
	QDG_ICON_QUEST,
	QDG_ICON_PAUSED,
	QDG_ICON_RUNNING,

	QDG_MAX
};


struct SThreadData : public wxTreeItemData
{
	CQuestThread* data;
	SThreadData( CQuestThread* _data ) : data( _data ) {}
	virtual ~SThreadData() {}
};

struct SQuestData : public SThreadData
{
	CQuest& quest;
	SQuestData( CQuestThread* _data, CQuest& _quest ) 
		: SThreadData( _data ), quest( _quest ) 
	{}
};

struct SBlockData : public wxTreeItemData
{
	const CQuestGraphBlock* data;
	SBlockData( const CQuestGraphBlock* _data ) : data( _data ) {}
};

///////////////////////////////////////////////////////////////////////////////

class CQuestsDebugger : public wxPanel,
	public IQuestEdTool,
	public IQuestSystemListener,
	public IQuestDebugInfo,
	public IEdEventListener
{
	DECLARE_EVENT_TABLE()

private:
	wxBitmap											m_icons[ QDG_MAX ];		//!< Debug icons
	wxImageList										m_debugImages;				//!< Debug icons image list
	wxTreeCtrl*										m_activeThreadsTree;
	wxTreeItemId									m_rootItem;
	CEdQuestEditor*								m_host;
	CQuestsSystem*									m_system;

	CQuestGraph*									m_observedGraph;
	CQuestThread*									m_observedQuestThread;
	TDynArray< const CQuestGraphBlock* >	m_breakpoints;

	Bool									m_gameEnding;
	TDynArray< const CQuestGraphBlock* >			m_blocksToRemove;
	TDynArray< CQuestThread* >						m_threadsToRemove;

public:
	CQuestsDebugger();
	~CQuestsDebugger();
	virtual void OnAttach( CEdQuestEditor& host, wxWindow* parent );
	virtual void OnDetach();
	virtual void OnCreateBlockContextMenu( TDynArray< SToolMenu >& subMenus, const CQuestGraphBlock* atBlock  );
	virtual void OnGraphSet( CQuestGraph& graph );
	virtual wxPanel* GetPanel() { return this; }
	virtual String GetToolName() const { return TXT( "Callstack" ); }

	void ToggleBreakpoint( const CQuestGraphBlock* block );
	void ContinueFrom( const CQuestGraphBlock* block, const CQuestGraphSocket* socket );
	
	CQuestThread* GetObservedQuestThread() { return m_observedQuestThread; }

protected:
	void OnClose( wxCloseEvent& event );
	void ShowContextMenu( wxTreeEvent& event );
	void ShowInEditor( wxTreeEvent& event );
	void OnStart( wxCommandEvent& event );
	void OnStop( wxCommandEvent& event );
	void OnBlockCommand( wxCommandEvent& event );

	// ------------------------------------------------------------------------
	// IQuestSystemListener impl
	// ------------------------------------------------------------------------
	void OnQuestStarted( CQuestThread* thread, CQuest& quest );
	void OnQuestStopped( CQuestThread* thread );
	void OnSystemPaused( bool paused );

	// ------------------------------------------------------------------------
	// IQuestThreadListener impl
	// ------------------------------------------------------------------------
	void OnThreadPaused( CQuestThread* thread, bool paused );
	void OnAddThread( CQuestThread* parentThread, CQuestThread* thread );
	void OnRemoveThread( CQuestThread* parentThread, CQuestThread* thread );
	void OnAddBlock( CQuestThread* thread, const CQuestGraphBlock* block );
	void OnRemoveBlock( CQuestThread* thread, const CQuestGraphBlock* block );
	void OnBlockInputActivated( CQuestThread* thread, const CQuestGraphBlock* block );

	// ------------------------------------------------------------------------
	// IQuestDebugInfo impl
	// ------------------------------------------------------------------------
	Bool IsBreakpointToggled( CQuestGraphBlock* block ) const;
	Bool IsBlockActive( CQuestGraphBlock* block ) const;
	Bool IsBlockVisited( CQuestGraphBlock* block ) const;
	Bool IsGraphInActive( CQuestGraph* graph ) const;

private:
	template< typename T >
	wxTreeItemId FindNode( wxTreeItemId root, const T& itemData )
	{
		wxTreeItemIdValue searchCookie;
		wxTreeItemId child, parent;

		TDynArray< wxTreeItemId > itemsStack;
		itemsStack.PushBack( root );

		while( itemsStack.Empty() == false )
		{
			parent = itemsStack.PopBack();

			for ( child = m_activeThreadsTree->GetFirstChild( parent, searchCookie );
				child.IsOk();
				child = m_activeThreadsTree->GetNextChild( parent, searchCookie ) )
			{
				wxTreeItemData* data = m_activeThreadsTree->GetItemData( child );
				T* tData = dynamic_cast< T* >( data );

				if ( tData && ( tData->data == itemData.data ) )
				{
					return child;
				}
				else
				{
					itemsStack.PushBack( child );
				}
			}
		}

		return wxTreeItemId();
	}

	void ShowSystemContextMenu();
	void ShowQuestContextMenu( wxTreeItemId& itemClicked );
	void ShowBlockContextMenu( wxTreeItemId& itemClicked );
	void ExecuteBreakpoints( const CQuestGraphBlock* block );

	wxString CreateBlockName( CQuestThread* thread, const CQuestGraphBlock* block ) const;
	void RefreshVisualiser();

	void FocusEditorOnItem( wxTreeItemId item );

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

};
