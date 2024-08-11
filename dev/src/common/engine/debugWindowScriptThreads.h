/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"

namespace DebugWindows
{
	class CDebugWindowScriptThreads : public RedGui::CRedGuiWindow
	{
		enum EScriptThreadSortType
		{
			STST_Id,
			STST_Owner,
			STST_Function,
			STST_Ticks,
			STST_Time,
		};

		struct SThreadInfo
		{
			Uint32		m_id;
			Bool		m_isLost;
			Bool		m_isDead;
			Uint32		m_lastTotalTicks;
			Float		m_lastTotalTime;
			Float		m_highlightTime;
			Color		m_highlightColor;
			String		m_function;
			String		m_owner;

			SThreadInfo( const CScriptThread& thread );
			void Update( const CScriptThread& thread );
		};

		struct SScriptThreadsSorter
		{
			template < typename T >
			RED_INLINE static Bool GetCompareValue( const T& a, const T& b )
			{
				if ( a < b ) 
				{
					return true;
				}
				return false;
			}

			RED_INLINE static Bool Less( const SThreadInfo* threadInfo1, const SThreadInfo* threadInfo2 )
			{
				switch ( s_sortType )
				{
				case STST_Id: 
					return GetCompareValue( threadInfo1->m_id, threadInfo2->m_id );
				case STST_Owner: 
					return !GetCompareValue( threadInfo1->m_owner, threadInfo2->m_owner );
				case STST_Function: 
					return !GetCompareValue( threadInfo1->m_function, threadInfo2->m_function );
				case STST_Ticks: 
					return GetCompareValue( threadInfo1->m_lastTotalTicks, threadInfo2->m_lastTotalTicks );
				case STST_Time: 
					return GetCompareValue( threadInfo1->m_lastTotalTime, threadInfo2->m_lastTotalTime );
				}

				return GetCompareValue( threadInfo1->m_id, threadInfo2->m_id );
			}

			static EScriptThreadSortType	s_sortType;
		};

	public:
		CDebugWindowScriptThreads();
		~CDebugWindowScriptThreads();

	private:
		void NotifyEventTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime);
		void NotifyEventSelectedIndexChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex);

		void UpdateGuiInformation();

		virtual void OnPendingDestruction() override;

		RedGui::CRedGuiComboBox*	m_sortType;
		RedGui::CRedGuiList*		m_threadsList;

		TSortedArray< SThreadInfo*, SScriptThreadsSorter >	m_threads;
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
