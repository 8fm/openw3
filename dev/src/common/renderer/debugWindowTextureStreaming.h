/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "..\engine\redGuiWindow.h"
#include "..\engine\redGuiDelegate2.h"
#include "..\engine\redGuiEventPackage.h"
#include "renderTextureBase.h"

namespace DebugWindows
{
	class CDebugWindowTextureStreaming : public RedGui::CRedGuiWindow
	{
		struct TextureInfo
		{
			String	m_path;
			String	m_groupName;
			Float	m_size;
			Float	m_distance;
			Bool	m_lock;
			Int8	m_mip;

			TextureInfo( String path, String groupName, Float size, Float distance, Bool lock, Int8 mip )
				: m_path( path )
				, m_groupName( groupName )
				, m_size( size )
				, m_distance( distance )
				, m_lock( lock )
				, m_mip( mip ) {}
		};

	public:
		CDebugWindowTextureStreaming();
		~CDebugWindowTextureStreaming();

	private:
		virtual void OnWindowOpened( CRedGuiControl* control );
		virtual void OnWindowClosed( CRedGuiControl* control );

		void NotifyOnDump(RedGui::CRedGuiEventPackage& eventPackage);
		void NotifyDumpNonStreamedListFileOK(RedGui::CRedGuiEventPackage& eventPackage);

		void CreateControls();
		RedGui::CRedGuiPanel* CreateInfoPanel();
		RedGui::CRedGuiPanel* CreateOptionsPanel();
		RedGui::CRedGuiPanel* CreateUnloadButtonsPanel();
		RedGui::CRedGuiPanel* CreateComboBoxTypeFiltersPanel();
		RedGui::CRedGuiPanel* CreateNumberFilterPanel( const String& label, RedGui::CRedGuiCheckBox** lessThanCBox, RedGui::CRedGuiCheckBox** biggerThanCBox, 
			RedGui::CRedGuiSpin** lessThanSpin, RedGui::CRedGuiSpin** biggerThanSpin );

		RedGui::CRedGuiPanel* CreateComboBoxPanel( const String& labelText, const TDynArray< String >& options, Uint32 filterType );
		RedGui::CRedGuiPanel* CreateToggledSpinBoxPanel( const String& labelText, RedGui::CRedGuiCheckBox** cBox, RedGui::CRedGuiSpin** spin, Uint32 defaultValue );

		void AddTabWithList( RedGui::CRedGuiList*& list, const String& tabName );
		void GenerateList( Uint32 tabIndex, RedGui::CRedGuiList* list );
		void GenerateStreamingDetailsList( RedGui::CRedGuiList* list );
		Bool ShouldBeFilteredOut( const CRenderTextureBase* tex, Float size );
		void GatherCurrentTabContents( Uint32 tabIndex, THashMap< String, TextureInfo >& contents, Float& accumulatedSize );
		void UpdateList( RedGui::CRedGuiList* list, THashMap< String, TextureInfo >& contents );

		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta );

		void NotifyOnUnload( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnUnloadAll( RedGui::CRedGuiEventPackage& eventPackage );
		
		void NotifySelectedGroupChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void NotifySelectedMipChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void NotifyFilterCBoxClicked( RedGui::CRedGuiEventPackage& package, Bool value );

		void NotifyOnResetTotalDataLoaded( RedGui::CRedGuiEventPackage& eventPackage );

		String GpuApiFormatToStr( GpuApi::eTextureFormat format );
		String GpuApiTexTypeToStr( GpuApi::eTextureType type );

	private:
		RedGui::CRedGuiTab*		m_tabs;
		RedGui::CRedGuiList*	m_lists[6];
		RedGui::CRedGuiSaveFileDialog*	m_dumpSaveDialog;

		RedGui::CRedGuiSpin* m_distLessSpin;
		RedGui::CRedGuiSpin* m_distMoreSpin;
		RedGui::CRedGuiCheckBox* m_distLessCBox;
		RedGui::CRedGuiCheckBox* m_distMoreCBox;

		RedGui::CRedGuiSpin* m_sizeLessSpin;
		RedGui::CRedGuiSpin* m_sizeMoreSpin;
		RedGui::CRedGuiCheckBox* m_sizeLessCBox;
		RedGui::CRedGuiCheckBox* m_sizeMoreCBox;

		RedGui::CRedGuiProgressBar*	m_inflight;
		RedGui::CRedGuiLabel*	m_streamingTime;

		RedGui::CRedGuiLabel*	m_statsInitialSize;
		RedGui::CRedGuiLabel*	m_statsFinalSize;

		RedGui::CRedGuiLabel*	m_totalDataLoaded;

		String m_filteredGroupName;
		Int32 m_filteredMip;
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
