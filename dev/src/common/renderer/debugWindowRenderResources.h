/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "..\engine\redGuiWindow.h"

namespace DebugWindows
{
	class CDebugWindowdRenderResources : public RedGui::CRedGuiWindow
	{
		struct SRenderTypeInfo
		{
			CName	m_category;
			Uint32	m_count;
			Uint32	m_totalMemSize;

			SRenderTypeInfo( CName category )
				: m_category( category )
				, m_count( 0 )
				, m_totalMemSize( 0 )
			{ /* intentionally empty */ }
		};

	public:
		CDebugWindowdRenderResources();
		~CDebugWindowdRenderResources();

	private:
		// general
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta );

		void CreateControls();

	private:
		RedGui::CRedGuiList*			m_renderResourceList;
		RedGui::CRedGuiList*			m_renderTexturesList;
		RedGui::CRedGuiList*			m_renderTextureArraysList;
		
		TDynArray< SRenderTypeInfo >	m_renderCategoryTypes;

	};

}

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
