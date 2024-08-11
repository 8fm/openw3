#pragma once

#ifndef NO_RED_GUI 
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "redGuiScrollPanel.h"

namespace DebugWindows
{

class CDebugWindowMemoryObjects : public RedGui::CRedGuiWindow
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );

public:
	CDebugWindowMemoryObjects();
	virtual ~CDebugWindowMemoryObjects();

private:
	//--
	virtual void	OnWindowOpened( CRedGuiControl* control ) override;
	virtual void	OnWindowClosed(CRedGuiControl* control) override;

	//--
	void			CreateControls();
	void			RefreshClassTree();
	void			RefreshClassList();
	void			CaptureObjectsCount();

	//--
	void			NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
	void			NotifyOnClickedToggleAll( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
	void			NotifyOnClickedRefresh( RedGui::CRedGuiEventPackage& eventPackage );
	void			NotifyOnClickedSnapshot( RedGui::CRedGuiEventPackage& eventPackage );
	void			NotifyOnClickedForceGCPerFrame( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
	void			NotifyOnClickedRealtimeUpdate( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
	void			NotifyOnClickedRealtimeSort( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
	void			NotifyOnClickedDumpStats( RedGui::CRedGuiEventPackage& eventPackage );
	void			NotifyOnClickedForceGC( RedGui::CRedGuiEventPackage& eventPackage );
	void			NotifyDumpStatsFileOK( RedGui::CRedGuiEventPackage& eventPackage );

	void			NotifyEventSelectedSortType( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex );

	RedGui::CRedGuiTab*				m_tabs;
	RedGui::CRedGuiComboBox*		m_sortTypeCombobox;
	RedGui::CRedGuiCheckBox*		m_showAllCheckbox;
	RedGui::CRedGuiCheckBox*		m_forceGCPerFrameCheckbox;
	RedGui::CRedGuiCheckBox*		m_realtimeUpdateCheckbox;
	RedGui::CRedGuiCheckBox*		m_realtimeSortCheckbox;
	RedGui::CRedGuiTreeView*		m_classTreeView;
	RedGui::CRedGuiList*			m_classListView;
	RedGui::CRedGuiSaveFileDialog*	m_dumpSaveDialog;

	struct ClassNode;
	typedef TDynArray< ClassNode*, MC_RedGuiControls, MemoryPool_RedGui > TChildClassNodes;

	struct ClassEntry;
	typedef TDynArray< ClassEntry*, MC_RedGuiControls, MemoryPool_RedGui > TListClassEntries;

	enum ESortMode
	{
		eSortMode_Name,
		eSortMode_Count,
		eSortMode_Memory,
		eSortMode_Delta,
	};

	struct ClassNode
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );

	public:
		const CClass*					m_class;
		TChildClassNodes				m_children;
		ClassNode*						m_parent;
		Uint32							m_exclusiveCount;
		Uint32							m_exclusiveMemory;
		Uint32							m_totalCount;
		Uint32							m_totalMemory;
		Int32							m_capturedTotalCount;
		Int32							m_capturedTotalMemory;
		RedGui::CRedGuiTreeNode*		m_node;

		ClassNode( const CClass* classInfo );
		~ClassNode();

		RED_INLINE Int32 GetDelta(){ return m_totalCount - m_capturedTotalCount; }
		void UpdateCaption();
		Bool ConditionalRefresh( Bool force, Bool capture, ESortMode sortMode );
	};

	struct ClassEntry
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );

	public:
		const CClass*					m_class;
		Uint32							m_exclusiveCount;
		Uint32							m_exclusiveMemory;
		Int32							m_capturedExclusiveCount;
		Int32							m_capturedExclusiveMemory;
		RedGui::CRedGuiListItem*		m_item;

		ClassEntry( const CClass* classInfo );

		RED_INLINE Int32 GetDelta() const { return m_exclusiveCount - m_capturedExclusiveCount; }
		void UpdateCaption();
		Bool ConditionalRefresh( Bool force, Bool capture );
	};

	TChildClassNodes	m_rootNodes;
	TListClassEntries	m_listEntries;

	ESortMode			m_sortMode;
	Bool				m_showAll;
	Bool				m_forceGCEveryFrame;
	Bool				m_realtimeUpdate;
	Bool				m_realtimeSort;
};

} // DebugWindows

#endif
#endif