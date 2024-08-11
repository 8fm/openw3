#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "debugWindowTextureStreaming.h"
#include "../engine/redGuiList.h"
#include "../engine/redGuiListItem.h"
#include "../engine/redGuiManager.h"
#include "../engine/redGuiTab.h"
#include "../engine/redGuiScrollPanel.h"
#include "../engine/redGuiButton.h"
#include "../engine/redGuiSaveFileDialog.h"
#include "../engine/redGuiPanel.h"
#include "../engine/redGuiLabel.h"
#include "../engine/redGuiProgressBar.h"
#include "../core/depot.h"
#include "renderTextureStreaming.h"
#include "../engine/renderSettings.h"
#include "../engine/redGuiComboBox.h"
#include "../engine/redGuiCheckBox.h"
#include "../engine/redGuiSpin.h"

namespace DebugWindows
{

enum EFilterType
{
	EFilterGroups,
	EFilterMips
};

	CDebugWindowTextureStreaming::CDebugWindowTextureStreaming()
		: RedGui::CRedGuiWindow( 200, 200, 1000, 600 )
		, m_tabs( nullptr )
		, m_filteredGroupName( TXT("All") )
		, m_filteredMip( -2 )
	{
		SetCaption( TXT("Texture streaming") );
		CreateControls();

		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowTextureStreaming::NotifyOnTick );
	}

	CDebugWindowTextureStreaming::~CDebugWindowTextureStreaming()
	{
		/* intentionally empty */
	}

	RedGui::CRedGuiPanel* CDebugWindowTextureStreaming::CreateInfoPanel()
	{
		RedGui::CRedGuiPanel* btnPanel = new RedGui::CRedGuiPanel( 0, 0, 240, 30 );
		btnPanel->SetMargin( Box2( 15, 5, 10, 10 ) );
		btnPanel->SetDock( RedGui::DOCK_Top );

		RedGui::CRedGuiLabel* label = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
		label->SetDock( RedGui::DOCK_Left );
		label->SetMargin( Box2( 5, 8, 5, 5 ) );
		label->SetText( TXT("Total data loaded:") );
		btnPanel->AddChild( label );

		m_totalDataLoaded = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
		m_totalDataLoaded->SetDock( RedGui::DOCK_Left );
		m_totalDataLoaded->SetMargin( Box2( 5, 8, 5, 5 ) );
		btnPanel->AddChild( m_totalDataLoaded );

		RedGui::CRedGuiButton* resetBtn = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
		resetBtn->SetText( TXT("Reset") );
		resetBtn->SetMargin( Box2( 5, 5, 5, 5 ) );
		resetBtn->SetDock( RedGui::DOCK_Left );
		resetBtn->EventButtonClicked.Bind( this, &CDebugWindowTextureStreaming::NotifyOnResetTotalDataLoaded );
		btnPanel->AddChild( resetBtn );

		RedGui::CRedGuiPanel* progressPanel = new RedGui::CRedGuiPanel( 0, 0, 360, 30 );
		progressPanel->SetDock( RedGui::DOCK_Right );
		progressPanel->SetBorderVisible( false );
		btnPanel->AddChild( progressPanel );

		m_streamingTime = new RedGui::CRedGuiLabel( 0, 0, 120, 30 );
		m_streamingTime->SetDock( RedGui::DOCK_Left );
		m_streamingTime->SetMargin( Box2( 5, 8, 5, 5 ) );
		progressPanel->AddChild( m_streamingTime );

		RedGui::CRedGuiLabel* inflightLabel = new RedGui::CRedGuiLabel( 0, 0, 120, 20 );
		inflightLabel->SetText( TXT("In-flight:") );
		inflightLabel->SetDock( RedGui::DOCK_Left );
		inflightLabel->SetMargin( Box2( 5, 8, 5, 5 ) );
		progressPanel->AddChild( inflightLabel );

		m_inflight = new RedGui::CRedGuiProgressBar( 0, 0, 150, 20 );
		m_inflight->SetShowProgressInformation( false );
		m_inflight->SetMargin( Box2( 5, 5, 5, 5 ) );
		m_inflight->SetDock( RedGui::DOCK_Right );
		progressPanel->AddChild( m_inflight );

		return btnPanel;
	}

	RedGui::CRedGuiPanel* CDebugWindowTextureStreaming::CreateOptionsPanel()
	{
		//filters
		RedGui::CRedGuiPanel* optionsPanel = new RedGui::CRedGuiPanel( 0, 0, 240, 80 );
		optionsPanel->SetMargin( Box2( 15, 5, 10, 10 ) );
		optionsPanel->SetDock( RedGui::DOCK_Top );
		
		optionsPanel->AddChild( CreateUnloadButtonsPanel() );
		optionsPanel->AddChild( CreateComboBoxTypeFiltersPanel() );
		optionsPanel->AddChild( CreateNumberFilterPanel( TXT("Distance:"), &m_distLessCBox, &m_distMoreCBox, &m_distLessSpin, &m_distMoreSpin ) );
		optionsPanel->AddChild( CreateNumberFilterPanel( TXT("Size:"), &m_sizeLessCBox, &m_sizeMoreCBox, &m_sizeLessSpin, &m_sizeMoreSpin ) );
		
		return optionsPanel;
	}

	RedGui::CRedGuiPanel* CDebugWindowTextureStreaming::CreateUnloadButtonsPanel()
	{
		// unload buttons
		RedGui::CRedGuiPanel* btnPanel2 = new RedGui::CRedGuiPanel( 0, 0, 160, 60 );
		btnPanel2->SetMargin( Box2( 5, 5, 5, 5 ) );
		btnPanel2->SetBorderVisible( false );
		btnPanel2->SetDock( RedGui::DOCK_Left );

		RedGui::CRedGuiButton* unloadBtn = new RedGui::CRedGuiButton( 0, 0, 150, 20 );
		unloadBtn->SetText( TXT("Unload Unused") );
		unloadBtn->SetMargin( Box2( 5, 5, 5, 5 ) );
		unloadBtn->SetDock( RedGui::DOCK_Top );
		unloadBtn->EventButtonClicked.Bind( this, &CDebugWindowTextureStreaming::NotifyOnUnload );
		btnPanel2->AddChild( unloadBtn );

		RedGui::CRedGuiButton* unloadAllBtn = new RedGui::CRedGuiButton( 0, 0, 150, 20 );
		unloadAllBtn->SetText( TXT("Unload All") );
		unloadAllBtn->SetMargin( Box2( 5, 5, 5, 5 ) );
		unloadAllBtn->SetDock( RedGui::DOCK_Top );
		unloadAllBtn->EventButtonClicked.Bind( this, &CDebugWindowTextureStreaming::NotifyOnUnloadAll );
		btnPanel2->AddChild( unloadAllBtn );

		return btnPanel2;
	}

	RedGui::CRedGuiPanel* CDebugWindowTextureStreaming::CreateComboBoxTypeFiltersPanel()
	{
		RedGui::CRedGuiPanel* cboxes = new RedGui::CRedGuiPanel( 0, 0, 240, 60 );
		cboxes->SetMargin( Box2( 5, 5, 5, 5 ) );
		cboxes->SetBorderVisible( false );
		cboxes->SetDock( RedGui::DOCK_Left );

		{
			//texture group
			TDynArray< CName > groups;
			SRenderSettingsManager::GetInstance().GetTextureGroups().GetKeys( groups );

			TDynArray< String > groupsOptions;
			for ( Uint32 i = 0; i < groups.Size(); ++i )
			{
				groupsOptions.PushBack( groups[i].AsString() );
			}
			Sort( groupsOptions.Begin(), groupsOptions.End() );
			cboxes->AddChild( CreateComboBoxPanel( TXT("Filter by group:"), groupsOptions, EFilterGroups ) );
		}

		{
			// mips
			TDynArray< String > options;
			for ( Int32 i = -1; i < 7; ++i )
			{
				options.PushBack( ToString( i ) );
			}
			cboxes->AddChild( CreateComboBoxPanel( TXT("Filter by mip:"), options, EFilterMips ) );
		}

		return cboxes;
	}

	RedGui::CRedGuiPanel* CDebugWindowTextureStreaming::CreateNumberFilterPanel( const String& labelText, RedGui::CRedGuiCheckBox** lessThanCBox, RedGui::CRedGuiCheckBox** biggerThanCBox, 
																					RedGui::CRedGuiSpin** lessThanSpin, RedGui::CRedGuiSpin** biggerThanSpin )
	{
		// distance
		RedGui::CRedGuiPanel* mainPanel = new RedGui::CRedGuiPanel( 0, 0, 240, 60 );
		mainPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
		mainPanel->SetBorderVisible( false );
		mainPanel->SetDock( RedGui::DOCK_Left );

		RedGui::CRedGuiLabel* label = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
		label->SetDock( RedGui::DOCK_Left );
		label->SetMargin( Box2( 20, 4, 0, 0 ) );
		label->SetText( labelText );
		mainPanel->AddChild( label );

		{
			RedGui::CRedGuiPanel* internalPanel = new RedGui::CRedGuiPanel( 0, 0, 240, 50 );
			internalPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
			internalPanel->SetBorderVisible( false );
			internalPanel->SetDock( RedGui::DOCK_Left );
			mainPanel->AddChild( internalPanel );

			internalPanel->AddChild( CreateToggledSpinBoxPanel( TXT("smaller than:"), lessThanCBox, lessThanSpin, 100 ) );
			internalPanel->AddChild( CreateToggledSpinBoxPanel( TXT("greater than:"), biggerThanCBox, biggerThanSpin, 0 ) );
		}
		return mainPanel;
	}

	RedGui::CRedGuiPanel* CDebugWindowTextureStreaming::CreateComboBoxPanel( const String& labelText, const TDynArray< String >& options, Uint32 filterType )
	{
		RedGui::CRedGuiPanel* panel = new RedGui::CRedGuiPanel( 0, 0, 260, 20 );
		panel->SetMargin( Box2( 5, 5, 5, 5 ) );
		panel->SetBorderVisible( false );
		panel->SetDock( RedGui::DOCK_Top );

		RedGui::CRedGuiLabel* label = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
		label->SetDock( RedGui::DOCK_Left );
		label->SetText( labelText );
		panel->AddChild( label );

		RedGui::CRedGuiComboBox* comboBox = new RedGui::CRedGuiComboBox( 0, 0, 150, 20, 300 );
		comboBox->AddItem( TXT("All") );
		for ( Uint32 i = 0; i < options.Size(); ++i )
		{
			comboBox->AddItem( options[i] );
		}
		comboBox->SetSelectedIndex( 0 );
		comboBox->SetDock( RedGui::DOCK_Right );

		if ( filterType == EFilterGroups )
		{
			comboBox->EventSelectedIndexChanged.Bind( this, &CDebugWindowTextureStreaming::NotifySelectedGroupChanged );
		}
		else if ( filterType == EFilterMips )
		{
			comboBox->EventSelectedIndexChanged.Bind( this, &CDebugWindowTextureStreaming::NotifySelectedMipChanged );
		}
		panel->AddChild( comboBox );

		return panel;
	}

	RedGui::CRedGuiPanel* CDebugWindowTextureStreaming::CreateToggledSpinBoxPanel( const String& labelText, RedGui::CRedGuiCheckBox** cBox, RedGui::CRedGuiSpin** spin, Uint32 defaultValue )
	{
		RedGui::CRedGuiPanel* panel = new RedGui::CRedGuiPanel( 0, 0, 240, 20 );
		panel->SetMargin( Box2( 5, 5, 5, 5 ) );
		panel->SetBorderVisible( false );
		panel->SetDock( RedGui::DOCK_Top );

		*cBox = new RedGui::CRedGuiCheckBox( 0, 0, 20, 20 );
		(*cBox)->SetDock( RedGui::DOCK_Left );
		(*cBox)->SetMargin( Box2( 5, 0, 0, 0 ) );
		(*cBox)->EventCheckedChanged.Bind( this, &CDebugWindowTextureStreaming::NotifyFilterCBoxClicked );
		(*cBox)->SetChecked( false );
		panel->AddChild( *cBox );

		RedGui::CRedGuiLabel* lessThanLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20 );
		lessThanLabel->SetDock( RedGui::DOCK_Left );
		lessThanLabel->SetText( labelText );
		panel->AddChild( lessThanLabel );

		*spin = new RedGui::CRedGuiSpin( 0, 0, 100, 20 );
		(*spin)->SetDock( RedGui::DOCK_Left );
		(*spin)->SetMinValue( 0 );
		(*spin)->SetMaxValue( 1000000 );
		(*spin)->SetValue( defaultValue );
		(*spin)->SetEnabled( false );
		panel->AddChild( *spin );

		return panel;
	}

	void CDebugWindowTextureStreaming::CreateControls()
	{
		m_dumpSaveDialog = new RedGui::CRedGuiSaveFileDialog();
		m_dumpSaveDialog->AddFilter( TXT("Text file"), TXT("txt") );
		m_dumpSaveDialog->EventFileOK.Bind( this, &CDebugWindowTextureStreaming::NotifyDumpNonStreamedListFileOK );

		RedGui::CRedGuiButton* dumpsStats = new RedGui::CRedGuiButton( 0, 0, 50, 20 );
		dumpsStats->SetText( TXT("Dump currently selected tab") );
		dumpsStats->SetMargin( Box2( 15, 5, 10, 0 ) );
		dumpsStats->SetDock( RedGui::DOCK_Top );
		dumpsStats->EventButtonClicked.Bind( this, &CDebugWindowTextureStreaming::NotifyOnDump );
		AddChild( dumpsStats );

		AddChild( CreateInfoPanel() );
		AddChild( CreateOptionsPanel() );

		m_tabs = new RedGui::CRedGuiTab( 0, 30, 100, 100 );
		m_tabs->SetMargin(Box2(5, 5, 5, 5));
		m_tabs->SetDock( RedGui::DOCK_Fill );
		AddChild( m_tabs );

		AddTabWithList( m_lists[0], TXT( "Non-streamed" ) );
		AddTabWithList( m_lists[1], TXT( "Currently streaming" ) );
		AddTabWithList( m_lists[2], TXT( "Out Of Budget" ) );
		AddTabWithList( m_lists[3], TXT( "Streamed in" ) );
		AddTabWithList( m_lists[4], TXT( "Stream-Locked" ) );

		{
			Uint32 index = m_tabs->AddTab( TXT("Streaming Details") );
			RedGui::CRedGuiScrollPanel* nextTab = m_tabs->GetTabAt( index );

			{
				RedGui::CRedGuiScrollPanel* topPanel = new RedGui::CRedGuiScrollPanel( 0, 0, 100, 40 );
				topPanel->SetDock( RedGui::DOCK_Top );

				m_statsInitialSize = new RedGui::CRedGuiLabel( 0, 0, 200, 20 );
				m_statsInitialSize->SetDock( RedGui::DOCK_Top );
				topPanel->AddChild( m_statsInitialSize );

				m_statsFinalSize = new RedGui::CRedGuiLabel( 0, 0, 200, 20 );
				m_statsFinalSize->SetDock( RedGui::DOCK_Top );
				topPanel->AddChild( m_statsFinalSize );

				nextTab->AddChild( topPanel );
			}


			RedGui::CRedGuiList* list = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			list->SetMargin( Box2( 5, 5, 5, 5 ) );
			list->SetDock( RedGui::DOCK_Fill );

			list->AppendColumn( TXT("#"), 30, RedGui::SA_Integer );
			list->AppendColumn( TXT("textureName"), 500 );
			list->AppendColumn( TXT("order"), 70, RedGui::SA_Integer );
			list->AppendColumn( TXT("lock"), 30 );
			list->AppendColumn( TXT("orig"), 30, RedGui::SA_Integer );
			list->AppendColumn( TXT("requ"), 30, RedGui::SA_Integer );
			list->AppendColumn( TXT("base"), 30, RedGui::SA_Integer );
			list->AppendColumn( TXT("curr"), 30, RedGui::SA_Integer );
			list->AppendColumn( TXT("pend"), 30, RedGui::SA_Integer );
			list->SetSorting( true );
			list->SetTextAlign( RedGui::IA_MiddleLeft );
			nextTab->AddChild( list );

			m_lists[5] = list;
		}

		// on "Stream-Locked" tab, "Lock" column becomes "Streamed".
		m_lists[4]->SetColumnLabel( TXT("Streamed"), 1 );

		m_tabs->SetActiveTab( 0 );
	}

	void CDebugWindowTextureStreaming::AddTabWithList( RedGui::CRedGuiList*& list, const String& tabName )
	{
		Uint32 index = m_tabs->AddTab( tabName );
		RedGui::CRedGuiScrollPanel* nextTab = m_tabs->GetTabAt( index );
		list = new RedGui::CRedGuiList( 0, 0, 100, 100 );
		list->SetMargin( Box2( 5, 5, 5, 5 ) );
		list->SetDock( RedGui::DOCK_Fill );
		list->AppendColumn( TXT("Distance"), 100, RedGui::SA_Real );
		list->AppendColumn( TXT("Lock"), 30 );
		list->AppendColumn( TXT("Mip"), 30, RedGui::SA_Integer );
		list->AppendColumn( TXT("Path"), 500 );
		list->AppendColumn( TXT("Group"), 200 );
		list->AppendColumn( TXT("Size (Mb)"), 200, RedGui::SA_Real );
		list->SetSorting( true );
		list->SetTextAlign( RedGui::IA_MiddleLeft );
		nextTab->AddChild( list );
	}

	void CDebugWindowTextureStreaming::OnWindowOpened( CRedGuiControl* control )
	{
	}

	void CDebugWindowTextureStreaming::OnWindowClosed( CRedGuiControl* control )
	{
	}

	void CDebugWindowTextureStreaming::NotifyOnDump(RedGui::CRedGuiEventPackage& eventPackage)
	{
		RED_UNUSED( eventPackage );
		
		Uint32 activeTab = m_tabs->GetActiveTabIndex();

		// TODO : Not implemented for Streaming Stats tab
		if ( activeTab == 5 )
		{
			return;
		}

		switch (activeTab)
		{
		case 0:
			m_dumpSaveDialog->SetDefaultFileName( TXT("non_streamed_textures.txt") );
			break;
		case 1:
			m_dumpSaveDialog->SetDefaultFileName( TXT("currently_streaming_textures.txt") );
			break;
		case 2:
			m_dumpSaveDialog->SetDefaultFileName( TXT("out_of_budget_textures.txt") );
			break;
		case 3:
			m_dumpSaveDialog->SetDefaultFileName( TXT("streamed_in_textures.txt") );
			break;
		case 4:
			m_dumpSaveDialog->SetDefaultFileName( TXT("streamlocked_textures.txt") );
			break;
		default:
			break;
		}

		m_dumpSaveDialog->SetVisible( true );
	}

	void CDebugWindowTextureStreaming::NotifyDumpNonStreamedListFileOK(RedGui::CRedGuiEventPackage& eventPackage)
	{
		RED_UNUSED( eventPackage );

		String path = String::EMPTY;
		GDepot->GetAbsolutePath( path );
		path += m_dumpSaveDialog->GetFileName();

		Float totalUnstreamedSize = 0;

		// gather information
		String text = String::EMPTY;
		text += TXT("========================================================================================================================|\n");
		text += TXT("|                                                                            Depot path |  Texture group  |  Memory MB  |\n");
		text += TXT("========================================================================================================================|\n");
		for ( TRenderResourceIterator< CRenderTextureBase > it; it; ++it )
		{
			CRenderTextureBase* tex = *it;
			Float size = tex->GetUsedVideoMemory() / (1024.f*1024.f);
			Uint32 activeTab = m_tabs->GetActiveTabIndex();
			Bool textureOnActiveTab = false;
			if ( activeTab == 0 && !tex->HasStreamingSource() )
			{
				size = tex->GetUsedVideoMemory() / (1024.f*1024.f);
				textureOnActiveTab = true;
			}
			else if ( activeTab == 1 && tex->HasStreamingSource() && tex->HasStreamingPending() )
			{
				size = tex->GetApproxSize( tex->GetMaxStreamingMipIndex() ) / (1024.f*1024.f);
				textureOnActiveTab = true;
			}
			else if ( activeTab == 2 && tex->HasStreamingSource() && (!tex->HasStreamingPending() && !tex->HasHiResLoaded()) && tex->GetLastBindDistance() < GetRenderer()->GetTextureStreamingManager()->GetMaxStreamingDistance( tex->GetTextureCategory() ) )
			{
				size = tex->GetApproxSize( tex->GetMaxStreamingMipIndex() ) / (1024.f*1024.f);
				textureOnActiveTab = true;
			}
			else if ( activeTab == 3 && tex->HasStreamingSource() && tex->HasHiResLoaded() )
			{
				size = tex->GetUsedVideoMemory() / (1024.f*1024.f);
				textureOnActiveTab = true;
			}
			else if ( activeTab == 4 && tex->HasStreamingLock() )
			{
				size = tex->GetUsedVideoMemory() / (1024.f*1024.f);
				textureOnActiveTab = true;
			}

			if ( textureOnActiveTab )
			{
				totalUnstreamedSize += size;

				text += tex->GetDepotPath().AsChar();
				text += TXT("|");
				text += tex->GetTextureGroupName().AsChar();
				text += TXT("|");
				text += String::Printf( TXT("%.2f"), size ).AsChar();
				text += TXT("|\n");
			}
		}
		text += TXT("==================================================================================================================================|\n");
		text += TXT("|TOTAL:                                                                                              |  ");
		text += String::Printf( TXT("%.2f"), totalUnstreamedSize ).AsChar();
		text += TXT("  |\n");
		text += TXT("==================================================================================================================================|\n");

		text += TXT("\n\n\n\n");

		// write text to file
		GFileManager->SaveStringToFile(path, text);
	}

	void CDebugWindowTextureStreaming::GenerateList( Uint32 tabIndex, RedGui::CRedGuiList* list )
	{
		if ( tabIndex == 5 )
		{
			GenerateStreamingDetailsList( list );
			return;
		}

		Uint32 listItemCount = list->GetItemCount();
		if ( listItemCount == 0 )
		{
			list->AddItem( new RedGui::CRedGuiListItem( String::Printf( TXT("-") ), nullptr, Color::CYAN, Color::GRAY ) );
			list->SetItemText( String::Printf( TXT("-") ), 0, 1 );
			list->SetItemText( String::Printf( TXT("-") ), 0, 2 );
			list->SetItemText( String::Printf( TXT("All out of budget") ), 0, 3 );
			list->SetItemText( String::Printf( TXT("-") ), 0, 4 );
			list->SetItemText( String::Printf( TXT("-") ), 0, 5 );
			listItemCount = list->GetItemCount();
		}

		Float accumulatedSize = 0;
		THashMap< String, TextureInfo > contents;
		GatherCurrentTabContents( tabIndex, contents, accumulatedSize );		
		UpdateList( list, contents );

		list->SetItemText( String::Printf( TXT("%.2f"), accumulatedSize ), 0, 5 );
	}

	void CDebugWindowTextureStreaming::GenerateStreamingDetailsList( RedGui::CRedGuiList* list )
	{
		Uint32 listItemCount = list->GetItemCount();

		STextureStreamingStats stats;
		GetRenderer()->GetTextureStreamingManager()->GetLastUpdateStats( stats );
		THashMap< String, TPair< Uint32, STextureStreamingTexStat > > textures;

		for ( Uint32 i = 0; i < stats.m_textures.Size(); ++i )
		{
			const auto& s = stats.m_textures[i];
			textures.Insert( s.m_textureName, TPair< Uint32, STextureStreamingTexStat >( i, s ) );
		}

		for ( Uint32 i = 1; i < listItemCount; )
		{
			String depotPath = list->GetItemText( i, 1 );
			if ( TPair< Uint32, STextureStreamingTexStat >* ti = textures.FindPtr( depotPath ) )
			{
				list->SetItemText( String::Printf( TXT("%d"), ti->m_first ), i, 0 );
				list->SetItemText( String::Printf( TXT("%u"), ti->m_second.m_streamingOrder ), i, 2 );
				list->SetItemText( ti->m_second.m_hasLock ? TXT("X") : TXT(" "), i, 3 );
				list->SetItemText( String::Printf( TXT("%u"), ti->m_second.m_originalRequestedMip ), i, 4 );
				list->SetItemText( String::Printf( TXT("%u"), ti->m_second.m_requestedMip), i, 5 );
				list->SetItemText( String::Printf( TXT("%u"), TXT("%u"), ti->m_second.m_baseMip ), i, 6 );
				list->SetItemText( String::Printf( TXT("%u"), ti->m_second.m_currentMip ), i, 7 );
				list->SetItemText( String::Printf( TXT("%d"), ti->m_second.m_pendingMip ), i, 8 );
				i++;
			}
			else
			{
				list->RemoveItem( i );
				listItemCount--;
			}
			textures.Erase( depotPath );
		}

		for ( THashMap< String, TPair< Uint32, STextureStreamingTexStat > >::const_iterator it = textures.Begin(); it != textures.End(); ++it )
		{
			TPair< Uint32, STextureStreamingTexStat > ti = it->m_second;

			RedGui::CRedGuiListItem* newItem = new RedGui::CRedGuiListItem( String::Printf( TXT("%d"), ti.m_first ), 0 );
			newItem->SetText( it->m_first, 1 );
			newItem->SetText( String::Printf( TXT("%u"), ti.m_second.m_streamingOrder ), 2 );
			newItem->SetText( ti.m_second.m_hasLock ? TXT("X") : TXT(" "), 3 );
			newItem->SetText( String::Printf( TXT("%u"), ti.m_second.m_originalRequestedMip ), 4 );
			newItem->SetText( String::Printf( TXT("%u"), ti.m_second.m_requestedMip), 5 );
			newItem->SetText( String::Printf( TXT("%u"), TXT("%u"), ti.m_second.m_baseMip ), 6 );
			newItem->SetText( String::Printf( TXT("%u"), ti.m_second.m_currentMip ), 7 );
			newItem->SetText( String::Printf( TXT("%d"), ti.m_second.m_pendingMip ), 8 );

			list->AddItem( newItem );
		}

		m_statsInitialSize->SetText( String::Printf( TXT("Initial Request: %.2f mb (Budget: %d)"), stats.m_initialRequestedSize / (1024.f*1024.f), Config::cvTextureMemoryBudget.Get() ) );
		m_statsFinalSize->SetText( String::Printf( TXT("Final Request: %.2f mb"), stats.m_finalRequestedSize / (1024.f*1024.f) ) );
	}

	Bool CDebugWindowTextureStreaming::ShouldBeFilteredOut( const CRenderTextureBase* tex, Float size )
	{
		RED_ASSERT( tex != nullptr, TXT("Null texture passed to private function! Check your logic!") );
		if ( tex == nullptr )
		{
			return true;
		}

		if ( m_filteredGroupName != TXT("All") && tex->GetTextureGroupName().AsString() != m_filteredGroupName )
		{
			return true;
		}

		if ( m_filteredMip > -2 && tex->GetStreamingMipIndex() != m_filteredMip )
		{
			return true;
		}

		if ( m_distLessSpin->GetEnabled() && m_distLessSpin->GetValue() <= tex->GetLastBindDistance() )
		{
			return true;
		}

		if ( m_distMoreSpin->GetEnabled() && m_distMoreSpin->GetValue() >= tex->GetLastBindDistance() )
		{
			return true;
		}

		if ( m_sizeLessSpin->GetEnabled() && m_sizeLessSpin->GetValue() <= size )
		{
			return true;
		}

		if ( m_sizeMoreSpin->GetEnabled() && m_sizeMoreSpin->GetValue() >= size )
		{
			return true;
		}

		return false;
	}

	void CDebugWindowTextureStreaming::GatherCurrentTabContents( Uint32 tabIndex, THashMap< String, TextureInfo >& contents, Float& accumulatedSize )
	{
		for ( TRenderResourceIterator< CRenderTextureBase > it; it; ++it )
		{
			CRenderTextureBase* tex = *it;
			Float size = 0;
			const Float distance = tex->GetLastBindDistance();
			const Float maxDistance = GetRenderer()->GetTextureStreamingManager()->GetMaxStreamingDistance( tex->GetTextureCategory() );			

			Bool textureOnActiveTab = false;
			if ( tabIndex == 0 && !tex->HasStreamingSource() )
			{
				size = tex->GetUsedVideoMemory() / (1024.f*1024.f);
				textureOnActiveTab = true;
			}
			else if ( tabIndex == 1 && tex->HasStreamingSource() && tex->HasStreamingPending() )
			{
				size = tex->GetApproxSize( tex->GetMaxStreamingMipIndex() ) / (1024.f*1024.f);
				textureOnActiveTab = true;
			}
			else if ( tabIndex == 2 && tex->HasStreamingSource() && (!tex->HasStreamingPending() && !tex->HasHiResLoaded()) && ( distance < maxDistance || tex->HasStreamingLock() ) )
			{
				size = tex->GetApproxSize( tex->GetMaxStreamingMipIndex() ) / (1024.f*1024.f);
				textureOnActiveTab = true;
			}
			else if ( tabIndex == 3 && tex->HasStreamingSource() && tex->HasHiResLoaded() )
			{
				size = tex->GetUsedVideoMemory() / (1024.f*1024.f);
				textureOnActiveTab = true;
			}
			else if ( tabIndex == 4 && tex->HasStreamingLock() )
			{
				size = tex->GetUsedVideoMemory() / (1024.f*1024.f);
				textureOnActiveTab = true;
			}

			if ( ShouldBeFilteredOut( tex, size ) )
			{
				continue;
			}

			if ( textureOnActiveTab )
			{
				Bool lock = tabIndex == 4 ? ( tex->HasStreamingPending() || tex->HasHiResLoaded() ) : tex->HasStreamingLock();
				contents.Insert( tex->GetDepotPath(), 
					TextureInfo( tex->GetDepotPath(), tex->GetTextureGroupName().AsString(), size, distance, lock, tex->GetStreamingMipIndex() ) );

				accumulatedSize += size;
			}
		}
	}

	void CDebugWindowTextureStreaming::UpdateList( RedGui::CRedGuiList* list, THashMap< String, TextureInfo >& contents )
	{
		Uint32 listSize = list->GetItemCount();
		for ( Uint32 i = 1; i < listSize; )
		{
			String depotPath = list->GetItemText( i, 3 );
			if ( TextureInfo* ti = contents.FindPtr( depotPath ) )
			{
				list->SetItemText( String::Printf( TXT("%.2f"), ti->m_distance ), i, 0 );
				list->SetItemText( ti->m_lock ? TXT("X") : TXT(" "), i, 1 );
				list->SetItemText( String::Printf( TXT("%i"), ti->m_mip ), i, 2 );
				list->SetItemText( String::Printf( TXT("%.2f"), ti->m_size ), i, 5 );
				i++;
			}
			else
			{
				list->RemoveItem( i );
				listSize--;
			}
			contents.Erase( depotPath );
		}

		for ( THashMap< String, TextureInfo >::const_iterator it = contents.Begin(); it != contents.End(); ++it )
		{
			RedGui::CRedGuiListItem* newItem = new RedGui::CRedGuiListItem( String::Printf( TXT("%.2f"), it->m_second.m_distance ) );
			newItem->SetText( it->m_second.m_lock ? TXT("X") : TXT(" "), 1 );
			newItem->SetText( String::Printf( TXT("%i"), it->m_second.m_mip ), 2 );
			newItem->SetText( it->m_first, 3 );
			newItem->SetText( it->m_second.m_groupName, 4 );
			newItem->SetText( String::Printf( TXT("%.2f"), it->m_second.m_size ), 5 );

			list->AddItem( newItem );
		}
	}

	void CDebugWindowTextureStreaming::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
		if( GetVisible() == false )
		{
			return;
		}

		Uint32 activeTab = m_tabs->GetActiveTabIndex();
		GenerateList( activeTab, m_lists[activeTab] );

		const GpuApi::TextureStats* stats = GpuApi::GetTextureStats();
		m_inflight->SetProgressRange( ( Float )Config::cvTextureInFlightBudget.Get() );
		m_inflight->SetProgressPosition( ( Float )stats->m_streamableTextureMemoryInFlight / ( 1024.0f * 1024.0f ) );

		Float updateTimeMS = GetRenderer()->GetTextureStreamingManager()->GetStreamingTaskTime() * 1000.0f;
		m_streamingTime->SetText( String::Printf( TXT("Update Time: %.2fms"), updateTimeMS ) );

		Float totalLoadedMB = GetRenderer()->GetTextureStreamingManager()->GetTotalDataLoaded() / ( 1024.0f * 1024.0f );
		m_totalDataLoaded->SetText( String::Printf( TXT("%.2f MB"), totalLoadedMB ) );
	}

	void CDebugWindowTextureStreaming::NotifyOnUnload( RedGui::CRedGuiEventPackage& eventPackage )
	{
		GetRenderer()->Flush();
		GetRenderer()->GetTextureStreamingManager()->CancelTextureStreaming( true );
	}

	void CDebugWindowTextureStreaming::NotifyOnUnloadAll( RedGui::CRedGuiEventPackage& eventPackage )
	{
		GetRenderer()->Flush();
		GetRenderer()->GetTextureStreamingManager()->CancelTextureStreaming( false );
	}

	void CDebugWindowTextureStreaming::NotifySelectedGroupChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		if ( RedGui::CRedGuiComboBox* sender = static_cast< RedGui::CRedGuiComboBox* >( eventPackage.GetEventSender() ) )
		{
			m_filteredGroupName = sender->GetSelectedItemName();
		}
	}
	
	void CDebugWindowTextureStreaming::NotifySelectedMipChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		if ( RedGui::CRedGuiComboBox* sender = static_cast< RedGui::CRedGuiComboBox* >( eventPackage.GetEventSender() ) )
		{
			if( sender->GetSelectedIndex() == 0 )
			{
				m_filteredMip = -2;
			}
			else
			{
				FromString( sender->GetSelectedItemName(), m_filteredMip );
			}
		}
	}

	void CDebugWindowTextureStreaming::NotifyFilterCBoxClicked( RedGui::CRedGuiEventPackage& package, Bool value )
	{
		RedGui::CRedGuiCheckBox* sender = static_cast< RedGui::CRedGuiCheckBox* >( package.GetEventSender() );
		RedGui::CRedGuiControl* ctrlToEnable = nullptr;

		if ( sender == m_distLessCBox )
		{
			ctrlToEnable = m_distLessSpin;
		}
		else if ( sender == m_distMoreCBox )
		{
			ctrlToEnable = m_distMoreSpin;
		}
		else if ( sender == m_sizeLessCBox )
		{
			ctrlToEnable = m_sizeLessSpin;
		}
		else if ( sender == m_sizeMoreCBox )
		{
			ctrlToEnable = m_sizeMoreSpin;
		}

		if ( ctrlToEnable )
		{
			ctrlToEnable->SetEnabled( value );
		}
	}

	void CDebugWindowTextureStreaming::NotifyOnResetTotalDataLoaded( RedGui::CRedGuiEventPackage& eventPackage )
	{
		GetRenderer()->GetTextureStreamingManager()->ResetTotalDataLoaded();
	}

}

#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI