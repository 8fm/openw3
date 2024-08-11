/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiWindow.h"

namespace DebugWindows
{
	class CDebugWindowSound : public RedGui::CRedGuiWindow
	{
		enum ETabType
		{
			TT_Main,
			TT_SoundBanks,
			TT_Banks,
			TT_Count
		};

	public:
		CDebugWindowSound();
		~CDebugWindowSound();

	protected:
		void FrameTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta);
		void NotifyOnCheckedChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyOnSliderChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value );
		void CreateSlider( RedGui::CRedGuiControl* parent, Float min, Float max, Float step, const String& name, const String& userDataType, RedGui::RedGuiAny userData );

		void CreateControls();

		// main tab
		void CreateMainTab();
		void UpdateMainTab();

		// Banks
		void CreateSoundBanksTab();
		void UpdateSoundBanksTab();

		// quest Banks
		void CreateBanksTab();
		void UpdateBanksTab();

		// On select events
		void NotifyOnSelectedGUID( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void NotifyOnSelectedBank( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void NotifyOnSelectedSoundBank( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );

	private:
		RedGui::CRedGuiTab*			m_tabs;

		RedGui::CRedGuiList*		m_list;
		RedGui::CRedGuiLabel*		m_countAttachedEmitters;
		RedGui::CRedGuiLabel*		m_countActivegameObjects;

		// real soundbanks
		RedGui::CRedGuiLabel*		m_laodedSoundBankSize;
		RedGui::CRedGuiLabel*		m_laodedSpecyficSoundBankSize;
		RedGui::CRedGuiPanel*		m_toolTip;
		RedGui::CRedGuiCheckBox*	m_listnerFromHeadOrCamera;

		// quest banks
		RedGui::CRedGuiList*			m_banksDescriptionList;
		RedGui::CRedGuiList*			m_guidsForSelectedList;

#ifdef RED_PLATFORM_DURANGO
		RedGui::CRedGuiLabel*	m_cachedBytesFree;
		RedGui::CRedGuiLabel*	m_cachedBytesAllocated;
		RedGui::CRedGuiLabel*	m_cachedBytesLost;
		RedGui::CRedGuiLabel*	m_cachedMaximumBlockSizeAvailable;
		RedGui::CRedGuiLabel*	m_cachedAllocationCount;
		RedGui::CRedGuiLabel*	m_noncachedBytesFree;
		RedGui::CRedGuiLabel*	m_noncachedBytesAllocated;
		RedGui::CRedGuiLabel*	m_noncachedBytesLost;
		RedGui::CRedGuiLabel*	m_noncachedMaximumBlockSizeAvailable;
		RedGui::CRedGuiLabel*	m_noncachedAllocationCount;
#endif
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
