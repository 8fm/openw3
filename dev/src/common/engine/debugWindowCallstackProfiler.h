/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"
#include "redGuiTreeNode.h"

namespace DebugWindows
{
	class CDebugWindowCallstackProfiler : public RedGui::CRedGuiWindow
	{
		struct SInternalCounterInfo
		{
			SInternalCounterInfo( CPerfCounter* counter, SInternalCounterInfo* parent = nullptr );
			~SInternalCounterInfo();

			void Tick();

			CProfilerStatBox		m_statBox;
			SInternalCounterInfo*	m_parent;
			SInternalCounterInfo*	m_child;
			SInternalCounterInfo*	m_sibling;
		};

		struct SInternalCouterListInfo
		{
			SInternalCouterListInfo( SInternalCounterInfo* counter, const String& name );
			~SInternalCouterListInfo();

			void Reset();
			void Update();

			String m_name;
			double m_time;
			double self_time;
			double m_hitCount;
			TDynArray< SInternalCounterInfo* > m_counters;

			RedGui::CRedGuiListItem*	m_connectedListItem;
		};

		enum ETabType
		{
			TT_TreeView,
			TT_FlatView,

			TT_Count
		};

	public:
		CDebugWindowCallstackProfiler();
		~CDebugWindowCallstackProfiler();

		virtual void SetVisible(Bool value);

		void SelectCounter( const String& counterName );

	private:
		void CreateControls();
		void CreateSearchControls();
		void CreateTreeViewControls();
		void CreateFlatViewControls();

		void NotifyEventTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta );

		// search panel
		void NotifyEventButtonClickedSearch( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventButtonClickedNextResult( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventButtonClickedPreviousResult( RedGui::CRedGuiEventPackage& eventPackage );

		// tree view panel
		void FillTreeView();
		void RecursiveFillTreeView( SInternalCounterInfo* counterInfo, RedGui::CRedGuiTreeNode* parentNode );
		RedGui::CRedGuiTreeNode* GetDCBProfiler( SInternalCounterInfo* statNode, RedGui::CRedGuiTreeNode* checkParent );
		void UpdateTreeView();

		// flat list view		
		void FillListView();
		void RecursiveFillListView( SInternalCounterInfo* counterInfo );
		void UpdateListView();

		// recreate counters logic
		void RebuildCountersViews();
		void CollectCounters();

		void UpdateSearchTextLabel();

		String StripPath(SInternalCounterInfo* statNode);

	private:
		RedGui::CRedGuiTab*						m_tabs;

		// tree view
		RedGui::CRedGuiTreeView*			m_functionsTree;

		// flat view
		RedGui::CRedGuiList*					m_functionsList;

		// search line
		RedGui::CRedGuiTextBox*				m_searchLine;
		RedGui::CRedGuiButton*				m_previousButton;
		RedGui::CRedGuiButton*				m_nextButton;
		RedGui::CRedGuiButton*				m_searchButton;
		RedGui::CRedGuiLabel*				m_searchInfo;

		// logic
		Int32								m_currentSearchResultIndex;
		RedGui::TreeNodeCollection			m_searchResults;
		size_t								m_counterCount;
		TDynArray< SInternalCounterInfo* >		m_countersInfo;
		TDynArray< SInternalCouterListInfo >	m_listInfo;
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
