/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifdef  SOUND_DEBUG

#ifdef USE_WWISE
#include <AK/SoundEngine/Common/AkQueryParameters.h>
#endif	// USE_WWISE
#include "redGuiList.h"
#include "redGuiListItem.h"
#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "redGuiManager.h"
#include "redGuiSeparator.h"
#include "redGuiCheckBox.h"
#include "debugWindowSound.h"
#include "soundFileLoader.h"
#include "redGuiAdvancedSlider.h"
#include "soundSettings.h"
#include "redGuiTab.h"
#include "redGuiScrollPanel.h"
#include "soundSystem.h"

#include "soundEmitter.h"
#include "../core/clipboardBase.h"

#ifdef RED_PLATFORM_DURANGO
#include <apu.h>
#endif

namespace DebugWindows
{
	CDebugWindowSound::CDebugWindowSound() 
		: RedGui::CRedGuiWindow(100,100,600,550)
		, m_list(nullptr)
		, m_countAttachedEmitters(nullptr)
		, m_countActivegameObjects(nullptr)
		, m_laodedSoundBankSize(nullptr)
		, m_toolTip(nullptr)
	{
		SetCaption(TXT("Sound"));

		GRedGui::GetInstance().EventTick.Bind(this, &CDebugWindowSound::FrameTick);
		
		CreateControls();
	}

	CDebugWindowSound::~CDebugWindowSound()
	{
		/*intentionally empty*/
	}

	void CDebugWindowSound::FrameTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
		if(GetEnabled() == true && GetVisible() == true)
		{
			switch( m_tabs->GetActiveTabIndex() )
			{
			case TT_Main:
				UpdateMainTab();
				break;
			case TT_SoundBanks:
				UpdateSoundBanksTab();
				break;
			case TT_Banks:
				UpdateBanksTab();
				break;
			}
		}
	}

	void CDebugWindowSound::CreateSlider( RedGui::CRedGuiControl* parent, Float min, Float max, Float step, const String& name, const String& userDataType, RedGui::RedGuiAny userData )
	{
		RedGui::CRedGuiPanel* panel = new RedGui::CRedGuiPanel( 0, 0, 100, 40 );
		panel->SetDock( RedGui::DOCK_Top );
		panel->SetMargin( Box2( 5, 5, 5, 5 ) );
		panel->SetBorderVisible( false );
		parent->AddChild( panel );
		{
			RedGui::CRedGuiAdvancedSlider* slider = new RedGui::CRedGuiAdvancedSlider( 0, 0, 300, 20 );
			slider->SetDock( RedGui::DOCK_Left );
			slider->SetMinValue( min );
			slider->SetMaxValue( max );
			slider->SetStepValue( step );
			slider->SetValue( userDataType == TXT("Int") ? *(Int32*)(userData) : *(Float*)(userData) );
			slider->SetUserString( TXT("Type"), userDataType );
			slider->SetUserData( userData );
			slider->EventScroll.Bind( this, &CDebugWindowSound::NotifyOnSliderChanged );
			panel->AddChild( slider );

			RedGui::CRedGuiLabel* label = new RedGui::CRedGuiLabel( 0, 0, 200, 20 );
			label->SetText( name );
			label->SetDock( RedGui::DOCK_Left );
			label->SetMargin( Box2( 30, 5, 0, 0 ) );
			panel->AddChild( label );
		}
	}
	void CDebugWindowSound::NotifyOnCheckedChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		Bool* userValue = sender->GetUserData< Bool >();
		if( userValue )
		{
			*userValue = value;
		}
	}

	void CDebugWindowSound::NotifyOnSliderChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		Float* userValue = sender->GetUserData< Float >();
		if( userValue )
		{
			*userValue = value;
		}
	}

	void CDebugWindowSound::CreateControls()
	{
		m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
		m_tabs->SetMargin( Box2( 5, 5, 5 ,5 ) );
		m_tabs->SetDock( RedGui::DOCK_Fill );
		AddChild( m_tabs );

		// create tabs
		CreateMainTab();
		CreateSoundBanksTab();
		CreateBanksTab();

		m_tabs->SetActiveTab( TT_Main );
	}

	void CDebugWindowSound::CreateMainTab()
	{
		m_tabs->AddTab( TXT("Main") );

		RedGui::CRedGuiScrollPanel* componentsTab = m_tabs->GetTabAt( TT_Main );

		// create info panel with informatinos labels
		{
			RedGui::CRedGuiPanel* soundInfoPanel = new RedGui::CRedGuiPanel(0,0, 250, 100);
			soundInfoPanel->SetMargin(Box2(5, 5, 5, 5));
			soundInfoPanel->SetPadding(Box2(5, 5, 5, 5));
			soundInfoPanel->SetBackgroundColor(Color(20, 20, 20, 255));
			soundInfoPanel->SetDock(RedGui::DOCK_Top);
			componentsTab->AddChild(soundInfoPanel);

			// create info panel about textures
			RedGui::CRedGuiPanel* soundInfoPanelLeft = new RedGui::CRedGuiPanel(0,0, 250, 80);
			soundInfoPanelLeft->SetMargin(Box2(5, 5, 5, 5));
			soundInfoPanelLeft->SetPadding(Box2(5, 5, 5, 5));
			soundInfoPanelLeft->SetBackgroundColor(Color(20, 20, 20, 255));
			soundInfoPanelLeft->SetDock(RedGui::DOCK_Left);
			soundInfoPanel->AddChild(soundInfoPanelLeft);

			RedGui::CRedGuiPanel* soundInfoPanelRight = new RedGui::CRedGuiPanel(0,0, 250, 80);
			soundInfoPanelRight->SetMargin(Box2(5, 5, 5, 5));
			soundInfoPanelRight->SetPadding(Box2(5, 5, 5, 5));
			soundInfoPanelRight->SetBackgroundColor(Color(20, 20, 20, 255));
			soundInfoPanelRight->SetDock(RedGui::DOCK_Right);
			soundInfoPanel->AddChild(soundInfoPanelRight);
			
			{
				RedGui::CRedGuiCheckBox* checkBox = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				checkBox->SetMargin( Box2( 5, 5, 5, 5 ) );
				checkBox->SetDock( RedGui::DOCK_Top );
				checkBox->SetText( TXT( "voice Event In Cutscenes Fixed" ) );
				checkBox->SetUserString( TXT("Type"), TXT("Bool") );
				checkBox->SetChecked( SSoundSettings::m_voiceEventInCutscenesFixed );
				checkBox->SetUserData( &SSoundSettings::m_voiceEventInCutscenesFixed );
				checkBox->EventCheckedChanged.Bind( this, &CDebugWindowSound::NotifyOnCheckedChanged );
				soundInfoPanelRight->AddChild( checkBox );
			}

			{
				RedGui::CRedGuiCheckBox* checkBox = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				checkBox->SetMargin( Box2( 5, 5, 5, 5 ) );
				checkBox->SetDock( RedGui::DOCK_Top );
				checkBox->SetText( TXT( "Contacts Logging" ) );
				checkBox->SetUserString( TXT("Type"), TXT("Bool") );
				checkBox->SetChecked( SSoundSettings::m_contactsLogging );
				checkBox->SetUserData( &SSoundSettings::m_contactsLogging );
				checkBox->EventCheckedChanged.Bind( this, &CDebugWindowSound::NotifyOnCheckedChanged );
				soundInfoPanelRight->AddChild( checkBox );
			}

			{
				RedGui::CRedGuiCheckBox* checkBox = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				checkBox->SetMargin( Box2( 5, 5, 5, 5 ) );
				checkBox->SetDock( RedGui::DOCK_Top );
				checkBox->SetText( TXT( "Contacts Sounds Enabled" ) );
				checkBox->SetUserString( TXT("Type"), TXT("Bool") );
				checkBox->SetChecked( SSoundSettings::m_contactSoundsEnabled );
				checkBox->SetUserData( &SSoundSettings::m_contactSoundsEnabled );
				checkBox->EventCheckedChanged.Bind( this, &CDebugWindowSound::NotifyOnCheckedChanged );
				soundInfoPanelRight->AddChild( checkBox );
			}

			CreateSlider( componentsTab, 0.f, 1.f, 0.05f, TXT("listner On Player From Camera"), TXT("Float"), &SSoundSettings::m_listnerOnPlayerFromCamera );
			CreateSlider( componentsTab, 0.0f, 100.0f, 1.0f, TXT("Contact Sounds Distance From Cammera"), TXT("Float"), &SSoundSettings::m_contactSoundsDistanceFromCamera );
			CreateSlider( componentsTab, 0.0f, 1.0f, 0.01f, TXT("Contact Sounds Min Velocity Filter"), TXT("Float"), &SSoundSettings::m_contactSoundsMinVelocityLimit );
			CreateSlider( componentsTab, 0.0f, 10000.0f, 10.0f, TXT("Contact Sounds Max Velocity Clamp"), TXT("Float"), &SSoundSettings::m_contactSoundsMaxVelocityClamp );
			CreateSlider( componentsTab, 0.0f, 1.0f, 0.01f, TXT("Contact Sounds Per Time Interval Limit"), TXT("Float"), &SSoundSettings::m_contactSoundPerTimeIntervalLimit );
			CreateSlider( componentsTab, 0.f, 100.f, 0.01f, TXT("occlusion distance limiter"), TXT("Float"), &SSoundSettings::m_occlusionDistanceLimiter );

			m_countAttachedEmitters = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_countAttachedEmitters->SetMargin(Box2(5, 5, 0, 0));
			m_countAttachedEmitters->SetText(TXT("Sound emitters attached: "));
			soundInfoPanelLeft->AddChild(m_countAttachedEmitters);
			m_countAttachedEmitters->SetDock(RedGui::DOCK_Top);
	
			m_countActivegameObjects = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_countActivegameObjects->SetMargin(Box2(5, 5, 0, 0));
			m_countActivegameObjects->SetText(TXT("Active game objects: "));
			soundInfoPanelLeft->AddChild(m_countActivegameObjects);
			m_countActivegameObjects->SetDock(RedGui::DOCK_Top);
	
			m_laodedSoundBankSize = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_laodedSoundBankSize->SetMargin(Box2(5, 5, 0, 0));
			m_laodedSoundBankSize->SetText(TXT("Loaded sound banks size: "));
			soundInfoPanelLeft->AddChild(m_laodedSoundBankSize);
			m_laodedSoundBankSize->SetDock(RedGui::DOCK_Top);

			m_laodedSpecyficSoundBankSize = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_laodedSpecyficSoundBankSize->SetMargin(Box2(5, 5, 0, 0));
			m_laodedSpecyficSoundBankSize->SetText(TXT("Loaded quests sound banks size: "));
			m_laodedSpecyficSoundBankSize->SetForegroundColor( Color::YELLOW );
			soundInfoPanelLeft->AddChild(m_laodedSpecyficSoundBankSize);
			m_laodedSpecyficSoundBankSize->SetDock(RedGui::DOCK_Top);

#ifdef RED_PLATFORM_DURANGO
			RedGui::CRedGuiPanel* cachedPanel = new RedGui::CRedGuiPanel(0,0, 250, 80);
			cachedPanel->SetMargin(Box2(5, 5, 5, 5));
			cachedPanel->SetPadding(Box2(5, 5, 5, 5));
			cachedPanel->SetBackgroundColor(Color(20, 20, 20, 255));
			cachedPanel->SetDock(RedGui::DOCK_Top);
			componentsTab->AddChild( cachedPanel );

			m_cachedBytesFree = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_cachedBytesFree->SetMargin(Box2(5, 5, 0, 0));
			m_cachedBytesFree->SetText(TXT("Cached Bytes Free: "));
			cachedPanel->AddChild(m_cachedBytesFree);
			m_cachedBytesFree->SetDock(RedGui::DOCK_Top);

			m_cachedBytesAllocated = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_cachedBytesAllocated->SetMargin(Box2(5, 5, 0, 0));
			m_cachedBytesAllocated->SetText(TXT("Cached Bytes Allocated: "));
			cachedPanel->AddChild(m_cachedBytesAllocated);
			m_cachedBytesAllocated->SetDock(RedGui::DOCK_Top);

			m_cachedBytesLost = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_cachedBytesLost->SetMargin(Box2(5, 5, 0, 0));
			m_cachedBytesLost->SetText(TXT("Cached Bytes Lost: "));
			cachedPanel->AddChild(m_cachedBytesLost);
			m_cachedBytesLost->SetDock(RedGui::DOCK_Top);

			m_cachedMaximumBlockSizeAvailable = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_cachedMaximumBlockSizeAvailable->SetMargin(Box2(5, 5, 0, 0));
			m_cachedMaximumBlockSizeAvailable->SetText(TXT("Cached Maximum Block Size Available: "));
			cachedPanel->AddChild(m_cachedMaximumBlockSizeAvailable);
			m_cachedMaximumBlockSizeAvailable->SetDock(RedGui::DOCK_Top);

			m_cachedAllocationCount = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_cachedAllocationCount->SetMargin(Box2(5, 5, 0, 0));
			m_cachedAllocationCount->SetText(TXT("Noncached Allocation Lost: "));
			cachedPanel->AddChild(m_cachedAllocationCount);
			m_cachedAllocationCount->SetDock(RedGui::DOCK_Top);

			RedGui::CRedGuiPanel* nonCachedPanel = new RedGui::CRedGuiPanel(0,0, 250, 80);
			nonCachedPanel->SetMargin(Box2(5, 5, 5, 5));
			nonCachedPanel->SetPadding(Box2(5, 5, 5, 5));
			nonCachedPanel->SetBackgroundColor(Color(20, 20, 20, 255));
			nonCachedPanel->SetDock(RedGui::DOCK_Top);
			componentsTab->AddChild( nonCachedPanel );

			m_noncachedBytesFree = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_noncachedBytesFree->SetMargin(Box2(5, 5, 0, 0));
			m_noncachedBytesFree->SetText(TXT("Noncached Bytes Allocated: "));
			nonCachedPanel->AddChild(m_noncachedBytesFree);
			m_noncachedBytesFree->SetDock(RedGui::DOCK_Top);

			m_noncachedBytesAllocated = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_noncachedBytesAllocated->SetMargin(Box2(5, 5, 0, 0));
			m_noncachedBytesAllocated->SetText(TXT("Noncached Bytes Allocated: "));
			nonCachedPanel->AddChild(m_noncachedBytesAllocated);
			m_noncachedBytesAllocated->SetDock(RedGui::DOCK_Top);

			m_noncachedBytesLost = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_noncachedBytesLost->SetMargin(Box2(5, 5, 0, 0));
			m_noncachedBytesLost->SetText(TXT("Noncached Bytes Lost: "));
			nonCachedPanel->AddChild(m_noncachedBytesLost);
			m_noncachedBytesLost->SetDock(RedGui::DOCK_Top);

			m_noncachedMaximumBlockSizeAvailable = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_noncachedMaximumBlockSizeAvailable->SetMargin(Box2(5, 5, 0, 0));
			m_noncachedMaximumBlockSizeAvailable->SetText(TXT("Noncached Maximum Block Size Available: "));
			nonCachedPanel->AddChild(m_noncachedMaximumBlockSizeAvailable);
			m_noncachedMaximumBlockSizeAvailable->SetDock(RedGui::DOCK_Top);

			m_noncachedAllocationCount = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
			m_noncachedAllocationCount->SetMargin(Box2(5, 5, 0, 0));
			m_noncachedAllocationCount->SetText(TXT("Noncached Allocation Lost: "));
			nonCachedPanel->AddChild(m_noncachedAllocationCount);
			m_noncachedAllocationCount->SetDock(RedGui::DOCK_Top);
#endif
		}
	}

	void CDebugWindowSound::UpdateMainTab()
	{

	}

	void CDebugWindowSound::CreateSoundBanksTab()
	{
		m_tabs->AddTab( TXT("Real SoundBanks") );

		RedGui::CRedGuiScrollPanel* componentsTab = m_tabs->GetTabAt( TT_SoundBanks );

		// create list
		m_list = new RedGui::CRedGuiList(10,90,380,400);
		m_list->SetMargin(Box2(5, 5, 5, 5));
		componentsTab->AddChild(m_list);
		m_list->SetDock(RedGui::DOCK_Fill);
		m_list->AppendColumn( TXT("Sound objects"), 100 );
		m_list->SetSelectionMode( RedGui::SM_None );
		m_list->SetNeedToolTip(true);
		m_list->SetSorting( true );
		m_list->EventSelectedItem.Bind( this, &CDebugWindowSound::NotifyOnSelectedSoundBank );

		// create tooltip
		m_toolTip = new RedGui::CRedGuiPanel(0, 0, 200, 50);
		m_toolTip->SetBackgroundColor(Color(53, 53, 53));
		m_toolTip->AttachToLayer(TXT("Pointers"));
		m_toolTip->SetVisible(false);
		m_toolTip->SetPadding(Box2(5, 5, 5, 5));
		m_toolTip->SetAutoSize(true);
		m_list->SetToolTip(m_toolTip);

		RedGui::CRedGuiLabel* greenInfo = new RedGui::CRedGuiLabel(10, 10, 10, 15);
		greenInfo->SetText(TXT("Reference count is greater than zero"), Color::GREEN);
		(*m_toolTip).AddChild( greenInfo );

		RedGui::CRedGuiLabel* redInfo = new RedGui::CRedGuiLabel(10, 25, 10, 15);
		redInfo->SetText(TXT("Reference count is is equal to zero"), Color::DARK_RED);
		(*m_toolTip).AddChild( redInfo );

	}

	void CDebugWindowSound::UpdateSoundBanksTab()
	{
#ifdef USE_WWISE
		AK::SoundEngine::Query::AkGameObjectsList list;
		AK::SoundEngine::Query::GetActiveGameObjects( list );
		m_countActivegameObjects->SetText(String::Printf(TXT("Active game objects: %d"), list.Length()));
		list.Term();
#endif	// USE_WWISE
		Uint32 soundBanksCount = CSoundBank::GetSoundBanksCount();
		Uint32 sumSize = 0;
		Uint32 sumSizeSpecyfic = 0;
		if(m_list->GetItemCount() != soundBanksCount)
		{
			m_list->RemoveAllItems();
			for(Uint32 i = 0; i != soundBanksCount; ++i)
			{
				const CSoundBank& soundBank = CSoundBank::GetSoundBank(i);

				Uint32 ref = soundBank.GetRefCount();
				Uint32 size = soundBank.GetSize();
				m_list->AddItem( String::Printf( TXT( " %s %i %i" ), soundBank.GetFileName().AsChar(), size, ref ), (ref == 0) ? Color::DARK_RED : Color::GREEN );

				if( ref > 0 )
				{
					sumSize += size;
				}
			}
		}
		else 
		{
			TSet< String > soundBanksNames;
			for(Uint32 i = 0; i != soundBanksCount; ++i)
			{
				const CSoundBank& soundBank = CSoundBank::GetSoundBank(i);

				Uint32 ref = soundBank.GetRefCount();
				Uint32 size = soundBank.GetSize();
				String text = String::Printf( TXT( "%s %i %i" ), soundBank.GetFileName().AsChar(), size, ref );
				String temp = m_list->GetItem( i )->GetText();
				if( m_list->GetItem( i )->GetText() != text )
				{
					m_list->GetItem( i )->SetText( text );
				}
				else
				{
					int a = 0;
				}
				Color color = (ref == 0) ? Color::DARK_RED : Color::GREEN;
				size_t pos = 0;
				if( text.FindSubstring( TXT( "br_" ), pos ) ||
					text.FindSubstring( TXT( "mh_" ), pos ) ||
					text.FindSubstring( TXT( "mq_" ), pos ) ||
					text.FindSubstring( TXT( "qu_" ), pos ) ||
					text.FindSubstring( TXT( "sq_" ), pos ) )
				{
					if( pos == 0 )
					{
						color = (ref == 0) ? Color::DARK_RED : Color::YELLOW;
						if( ref > 0 )
						{
							sumSizeSpecyfic += size;
						}
					}
				}

				m_list->GetItem( i )->SetTextColor( color );
				if( ref > 0 )
				{
					sumSize += size;
				}
			}
		}
		m_list->SortItems( 0 );
		m_laodedSoundBankSize->SetText(String::Printf(TXT("Loaded sound banks size:: %d MB"), ( sumSize )));
		m_laodedSpecyficSoundBankSize->SetText(String::Printf(TXT("Loaded QUEST sound banks size: %d MB"), ( sumSizeSpecyfic )));
#ifdef RED_PLATFORM_DURANGO
		ApuHeapState state;
		HRESULT result = ApuHeapGetState( &state, APU_ALLOC_CACHED );
		m_cachedBytesFree->SetText(String::Printf(TXT("Cached Bytes Free: %d"), state.bytesFree));
		m_cachedBytesAllocated->SetText(String::Printf(TXT("Cached Bytes Allocated: %d"), state.bytesAllocated));
		m_cachedBytesLost->SetText(String::Printf(TXT("Cached Bytes Lost: %d"), state.bytesLost));
		m_cachedMaximumBlockSizeAvailable->SetText(String::Printf(TXT("Cached Maximum Block Size Available: %d"), state.maximumBlockSizeAvailable));
		m_cachedAllocationCount->SetText(String::Printf(TXT("Cached Allocation Count: %d"), state.allocationCount));
		result = ApuHeapGetState( &state, APU_ALLOC_NONCACHED );
		m_noncachedBytesFree->SetText(String::Printf(TXT("Noncached Bytes Free: %d"), state.bytesFree));
		m_noncachedBytesAllocated->SetText(String::Printf(TXT("Noncached Bytes Allocated: %d"), state.bytesAllocated));
		m_noncachedBytesLost->SetText(String::Printf(TXT("Noncached Bytes Lost: %d"), state.bytesLost));
		m_noncachedMaximumBlockSizeAvailable->SetText(String::Printf(TXT("Noncached Maximum Block Size Available: %d"), state.maximumBlockSizeAvailable));
		m_noncachedAllocationCount->SetText(String::Printf(TXT("Noncached Allocation Count: %d"), state.allocationCount));
#endif
	}

	void CDebugWindowSound::CreateBanksTab()
	{
		m_tabs->AddTab( TXT("Quest SoundBanks") );

		RedGui::CRedGuiScrollPanel* componentsTab = m_tabs->GetTabAt( TT_Banks );
		if( componentsTab != nullptr )
		{
			m_banksDescriptionList = new RedGui::CRedGuiList( 0, 0, 400, 100 );
			m_banksDescriptionList->SetDock( RedGui::DOCK_Left );
			m_banksDescriptionList->SetSorting( true );
			m_banksDescriptionList->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_banksDescriptionList->EventSelectedItem.Bind( this, &CDebugWindowSound::NotifyOnSelectedBank );

			m_banksDescriptionList->AppendColumn( TXT("Bank name"), 250, RedGui::SA_String );
			m_banksDescriptionList->AppendColumn( TXT("Bank refcount"), 150, RedGui::SA_Integer );

			componentsTab->AddChild( m_banksDescriptionList );

			m_guidsForSelectedList = new RedGui::CRedGuiList( 0, 0, 450, 100 );
			m_guidsForSelectedList->SetDock( RedGui::DOCK_Left );
			m_guidsForSelectedList->SetSorting( true );
			m_guidsForSelectedList->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_guidsForSelectedList->EventSelectedItem.Bind( this, &CDebugWindowSound::NotifyOnSelectedGUID );

			m_guidsForSelectedList->AppendColumn( TXT("Quest phases guids"), 450, RedGui::SA_String );

			componentsTab->AddChild( m_guidsForSelectedList );
		}
	}

	void CDebugWindowSound::UpdateBanksTab()
	{
		THashMap< String, SSoundBankQuests > stringBanks = GSoundSystem->GetQuestBanks();

		Int32 selectInd =  m_banksDescriptionList->GetSelection();
		String selectedName = m_banksDescriptionList->GetItemText( selectInd );

		m_guidsForSelectedList->RemoveAllItems();

		// Update current entries
		Uint32 itemCount = m_banksDescriptionList->GetItemCount();
		for(Uint32 i = 0; i < itemCount; )
		{
			const String& bankName = m_banksDescriptionList->GetItem( i )->GetText(0);
			if ( SSoundBankQuests* sSBQ = stringBanks.FindPtr( bankName ) )
			{
				m_banksDescriptionList->SetItemText( ToString( sSBQ->m_refCount ), i, 1 );

				if( selectedName == bankName )
				{
					Uint32 size = sSBQ->m_blockPhases.Size();
					for( Uint32 i = 0; i < size; ++i )
					{
						String name = sSBQ->m_blockPhases[i];
						m_guidsForSelectedList->AddItem( name );
					}
					m_banksDescriptionList->SetSelection(bankName, true);
				}
				++i;
			}
			else
			{
				m_banksDescriptionList->RemoveItem( i );
				itemCount--;
			}
		}

		// Add new ones
		for( THashMap< String, SSoundBankQuests >::iterator iter = stringBanks.Begin(), end = stringBanks.End(); iter != end; ++iter )
		{
			String newItem = iter->m_first;
			SSoundBankQuests count = iter->m_second;
			Uint32 index = m_banksDescriptionList->Find( newItem );	

			if( index == -1 )
			{
				index = m_banksDescriptionList->AddItem( newItem );
				m_banksDescriptionList->SetItemText( ToString( count.m_refCount ), index, 1 );
			}
		
		}
	}

	void CDebugWindowSound::NotifyOnSelectedGUID( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

		const String leak = m_guidsForSelectedList->GetItemText( value );
		GClipboard->Copy( leak );
	}

	void CDebugWindowSound::NotifyOnSelectedBank( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

		const String leak = m_banksDescriptionList->GetItemText( value );
		GClipboard->Copy( leak );
	}

	void CDebugWindowSound::NotifyOnSelectedSoundBank( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

		const String leak = m_list->GetItemText( value );
		GClipboard->Copy( leak );
	}


}	// namespace DebugWindows

#endif	// SOUND_DEBUG
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
