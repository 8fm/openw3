/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiButton.h"
#include "redGuiPanel.h"
#include "redGuiScrollPanel.h"
#include "redGuiLabel.h"
#include "redGuiGridLayout.h"
#include "redGuiSpin.h"
#include "redGuiCheckBox.h"
#include "debugWindowCallstackProfiler.h"
#include "../core/configVar.h"

#include "debugWindowProfilerSettings.h"

namespace DebugWindows
{

	CDebugWindowProfilerSettings::CDebugWindowProfilerSettings()
		: RedGui::CRedGuiWindow(200,200,300,200)
	{
		SetCaption(TXT("Profiler settings"));
		CreateControls();
	}


	CDebugWindowProfilerSettings::~CDebugWindowProfilerSettings()
	{
	}

	void CDebugWindowProfilerSettings::CreateControls()
	{
		this->EventWindowOpened.Bind( this, &CDebugWindowProfilerSettings::NotifyWindowOpened );

		RedGui::CRedGuiButton* applyButton = new RedGui::CRedGuiButton( 0, 0, 40, 20 );
		applyButton->SetText( TXT("Apply") );
		applyButton->SetDock( RedGui::DOCK_Bottom );
		applyButton->SetMargin( Box2( 5, 5, 5, 5 ) );
		applyButton->EventButtonClicked.Bind( this, &CDebugWindowProfilerSettings::NotifyApplyClicked );
		this->AddChild( applyButton );

		RedGui::CRedGuiPanel* levelPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 20 );
		levelPanel->SetBorderVisible( false );
		levelPanel->SetBackgroundColor( Color::CLEAR );
		levelPanel->SetDock( RedGui::DOCK_Top );
		levelPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
		this->AddChild( levelPanel );
		{
			RedGui::CRedGuiLabel* levelLabel = new RedGui::CRedGuiLabel( 0, 0, 40, 20 );
			levelLabel->SetText( TXT("Profiling Level:") );
			levelLabel->SetDock( RedGui::DOCK_Left );
			levelPanel->AddChild( levelLabel );

			m_levelSpinBox = new RedGui::CRedGuiSpin( 0, 0, 40, 20 );
			m_levelSpinBox->SetValue( Config::cvProfilingLevel.Get() );
			m_levelSpinBox->SetDock( RedGui::DOCK_Left );
			m_levelSpinBox->SetMinValue( -1 );
			levelPanel->AddChild( m_levelSpinBox );
		}

		RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
		layout->SetDock( RedGui::DOCK_Fill );
		layout->SetDimensions( 2, 1 );
		layout->SetMargin( Box2( 5, 5, 5, 5 ) );
		this->AddChild( layout );
		{
			RedGui::CRedGuiScrollPanel* leftPanel = new RedGui::CRedGuiScrollPanel( 0, 0, 100, 100 );
			leftPanel->SetBackgroundColor( Color(20, 20, 20, 255) );
			leftPanel->SetDock( RedGui::DOCK_Fill );
			leftPanel->SetMargin( Box2( 0, 0, 5, 0 ) );
			layout->AddChild( leftPanel );
			{
				// Initialize tool check boxes
				const CProfilerToolPool& toolFactory = UnifiedProfilerManager::GetInstance().GetToolPool();
				const CProfilerToolPool::ProfilerToolNames& toolNames = toolFactory.GetToolNames();

				m_profilersCheckBoxes.Resize( toolNames.Size() );

				const CProfilerToolPool::ProfilerToolArray& activeTools = UnifiedProfilerManager::GetInstance().GetActiveProfilerTools();

				for( Uint32 i=0; i<m_profilersCheckBoxes.Size(); ++i )
				{
					m_profilersCheckBoxes[i] = new RedGui::CRedGuiCheckBox( 0, 0, 20, 20 );
					m_profilersCheckBoxes[i]->SetText( toolNames[i] );
					m_profilersCheckBoxes[i]->SetChecked( false );
					m_profilersCheckBoxes[i]->SetDock( RedGui::DOCK_Top );
					m_profilersCheckBoxes[i]->SetMargin( Box2( 5, 2, 5, 2 ) );

					for( Uint32 j=0; j<activeTools.Size(); ++j )
					{
						if( String( activeTools[j]->GetName() ) == m_profilersCheckBoxes[i]->GetText() )
						{
							m_profilersCheckBoxes[i]->SetChecked( true );
						}
					}

					leftPanel->AddChild( m_profilersCheckBoxes[i] );
				}
			}

			RedGui::CRedGuiScrollPanel* rightPanel = new RedGui::CRedGuiScrollPanel( 0, 0, 100, 100 );
			rightPanel->SetBackgroundColor( Color(20, 20, 20, 255) );
			rightPanel->SetDock( RedGui::DOCK_Fill );
			rightPanel->SetMargin( Box2( 5, 0, 0, 0 ) );
			layout->AddChild( rightPanel );
			{
				// Initialize channel check boxes
				CEnum* profilerBlockChannel = SRTTI::GetInstance().FindEnum( CNAME( EProfilerBlockChannel ) );

				TDynArray<CName> enumOpt = profilerBlockChannel->GetOptions();
				Uint32 channelCount = enumOpt.Size();
				m_channelsCheckBoxes.Resize( channelCount );
				Uint32 activeChannels = Config::cvProfilingChannels.Get();

				CName allChannelName;
				profilerBlockChannel->FindName( PBC_ALL, allChannelName );

				for( Uint32 i=0; i<channelCount; ++i )
				{
					Uint32 channel = FLAG(i);
					m_channelsCheckBoxes[i] = new RedGui::CRedGuiCheckBox( 0, 0, 20, 20 );
					const String channelName = enumOpt[i].AsString();
					m_channelsCheckBoxes[i]->SetText( channelName );
					m_channelsCheckBoxes[i]->SetDock( RedGui::DOCK_Top );
					m_channelsCheckBoxes[i]->SetMargin( Box2( 5, 2, 5, 2 ) );

					if( enumOpt[i] == allChannelName )
					{
						m_channelsCheckBoxes[i]->EventCheckedChanged.Bind( this, &CDebugWindowProfilerSettings::NotifyChannelAllCheckChanged );
					}

					if( (channel & activeChannels) != 0 )
					{
						m_channelsCheckBoxes[i]->SetChecked( true );
					}
					else
					{
						m_channelsCheckBoxes[i]->SetChecked( false );
					}
					rightPanel->AddChild( m_channelsCheckBoxes[i] );
				}
			}
		}
	}

	Uint32 CDebugWindowProfilerSettings::GetActiveChannelsFromCheckBoxes()
	{
		Uint32 result = 0;
		CEnum* profilerBlockChannel = SRTTI::GetInstance().FindEnum( CNAME( EProfilerBlockChannel ) );
		Int32 tempVal = 0;

		for( Uint32 i=0; i<m_channelsCheckBoxes.Size(); ++i )
		{
			if( m_channelsCheckBoxes[i]->GetChecked() == true )
			{
				if( profilerBlockChannel->FindValue( CName( m_channelsCheckBoxes[i]->GetText() ), tempVal ) == true )
				{
					result |= static_cast<Uint32>( tempVal );
				}
			}
		}

		return result;
	}

	void CDebugWindowProfilerSettings::NotifyApplyClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		// Set tools
		CProfilerToolPool& toolFactory = UnifiedProfilerManager::GetInstance().GetToolPool();

		String profilerConfig = TXT("");
		Bool canPlaceTokens = false;

		CProfilerToolPool::ProfilerToolArray newTools;

		for( Uint32 i=0; i<m_profilersCheckBoxes.Size(); ++i )
		{
			if( m_profilersCheckBoxes[i]->GetChecked() == true )
			{
				if( canPlaceTokens == true )
				{
					profilerConfig += TXT("|");
				}

				profilerConfig += m_profilersCheckBoxes[i]->GetText();
				canPlaceTokens = true;

				newTools.PushBack( toolFactory.GetTool( i ) );
			}
		}

		if( profilerConfig.Empty() )
		{
			profilerConfig = TXT("none");
		}

		// Set level
		Int32 newLevel = m_levelSpinBox->GetValue();
		Config::cvProfilingLevel.Set( newLevel );
		UnifiedProfilerManager::GetInstance().SwitchTools( newTools );

		// Set channels
		Config::cvProfilingChannels.Set( GetActiveChannelsFromCheckBoxes() );

		// Save config
		Config::cvActiveProfilers.Set( profilerConfig );
		SConfig::GetInstance().Save();
	}

	void CDebugWindowProfilerSettings::NotifyChannelAllCheckChanged(RedGui::CRedGuiEventPackage& eventPackage, Bool value)
	{
		RedGui::CRedGuiCheckBox* checkBox = static_cast<RedGui::CRedGuiCheckBox*>( eventPackage.GetEventSender() );

		if( value == true )
		{
			for( Uint32 i=0; i<m_channelsCheckBoxes.Size(); ++i )
			{
				if( m_channelsCheckBoxes[i] != checkBox )
				{
					m_channelsCheckBoxes[i]->SetEnabled( false );
					m_channelsCheckBoxes[i]->SetText( m_channelsCheckBoxes[i]->GetText(), Color::GRAY );
				}
			}
		}
		else
		{
			for( Uint32 i=0; i<m_channelsCheckBoxes.Size(); ++i )
			{
				if( m_channelsCheckBoxes[i] != checkBox )
				{
					m_channelsCheckBoxes[i]->SetEnabled( true );
					m_channelsCheckBoxes[i]->SetText( m_channelsCheckBoxes[i]->GetText(), Color::WHITE );
				}
			}
		}
	}

	void CDebugWindowProfilerSettings::NotifyWindowOpened(RedGui::CRedGuiEventPackage& eventPackage)
	{
		// Initialize tool check boxes
		const CProfilerToolPool& toolFactory = UnifiedProfilerManager::GetInstance().GetToolPool();
		const CProfilerToolPool::ProfilerToolArray& activeTools = UnifiedProfilerManager::GetInstance().GetActiveProfilerTools();

		for( Uint32 i=0; i<m_profilersCheckBoxes.Size(); ++i )
		{
			m_profilersCheckBoxes[i]->SetChecked( false );

			for( Uint32 j=0; j<activeTools.Size(); ++j )
			{
				if( String( activeTools[j]->GetName() ) == m_profilersCheckBoxes[i]->GetText() )
				{
					m_profilersCheckBoxes[i]->SetChecked( true );
				}
			}
		}

		// Initialize channel check boxes
		CEnum* profilerBlockChannel = SRTTI::GetInstance().FindEnum( CNAME( EProfilerBlockChannel ) );
		TDynArray<CName> enumOpt = profilerBlockChannel->GetOptions();
		Uint32 channelCount = enumOpt.Size();
		Uint32 activeChannels = Config::cvProfilingChannels.Get();

		for( Uint32 i=0; i<channelCount; ++i )
		{
			Uint32 channel = FLAG(i);

			if( (channel & activeChannels) != 0 )
			{
				m_channelsCheckBoxes[i]->SetChecked( true );
			}
			else
			{
				m_channelsCheckBoxes[i]->SetChecked( false );
			}
		}
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
