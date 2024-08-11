/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifndef NO_TELEMETRY

#include "redGuiManager.h"
#include "redGuiCheckBox.h"
#include "debugWindowTelemetryTags.h"
#include "game.h"
#include "../../common/core/configVar.h"
#include "../../common/core/configVarLegacyWrapper.h"
#include "../core/redTelemetryServicesManager.h"

#define TELEMETRY_SERVICE_NAME TXT("telemetry")

namespace RedGui
{
	class CRedGuiCheckBox;
}

namespace DebugWindows
{
	CDebugWindoTelemetryTags::CDebugWindoTelemetryTags() 
		: RedGui::CRedGuiWindow( 100, 100, 300, 400 )
	{
		SetEnabledCaptionButton( RedGui::CB_Maximize, false );
		SetResizable( false );

		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindoTelemetryTags::NotifyOnTick );

		SetCaption( TXT("Telemetry Tags") );
		CreateControls();
	}

	CDebugWindoTelemetryTags::~CDebugWindoTelemetryTags()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindoTelemetryTags::NotifyOnTick );
	}

	void CDebugWindoTelemetryTags::CreateControls()
	{
		String tags;

		SConfig::GetInstance().GetLegacy().ReadParam( TXT("telemetry_service_config"), TXT("Game"),	TXT( "PredefinedTags" ), tags ); //! load string with tags

		if( tags.GetLength() > 0 )
		{
			TDynArray<String> tagsArray = tags.Split( TXT(" ") );
			Uint32 arraySize = tagsArray.Size();
			for ( Uint32 tagIndex = 0; tagIndex < arraySize; ++tagIndex )
			{
				RedGui::CRedGuiCheckBox* newCheckbox = new RedGui::CRedGuiCheckBox(10, tagIndex*20 + 10, 0, 0);
				newCheckbox->SetText( tagsArray[tagIndex] );
				newCheckbox->SetMargin(Box2(3, 3, 0, 3));
				newCheckbox->EventCheckedChanged.Bind(this, &CDebugWindoTelemetryTags::NotifyCheckBoxValueChanged);
				newCheckbox->SetDock(RedGui::DOCK_Top);
				m_chackBoxTags.PushBack( newCheckbox );
				AddChild( newCheckbox );
			}
		}

	}

	void CDebugWindoTelemetryTags::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		if( GetVisible() == false )
		{
			return;
		}

		IRedTelemetryServiceInterface*  redTelemetryServiceInterface = SRedTelemetryServicesManager::GetInstance().GetService( TELEMETRY_SERVICE_NAME );
		if( redTelemetryServiceInterface != NULL )
		{
			Bool enableCheckBoxes = false;
			if ( redTelemetryServiceInterface->GetSessionId( Telemetry::BT_RED_TEL_API ).Empty() )
			{
				enableCheckBoxes = true;
			}

			TDynArray<RedGui::CRedGuiCheckBox*>::iterator end = m_chackBoxTags.End();
			for ( TDynArray<RedGui::CRedGuiCheckBox*>::iterator checkBoxIter = m_chackBoxTags.Begin(); checkBoxIter != end; ++checkBoxIter )
			{
				(*checkBoxIter)->SetEnabled( enableCheckBoxes ); //! if telemetry session is present all checkboxes should be disabled 
																 //! (adding or removing telemetry session tags is possible only before start telemetry session)
			}
			
		}
	}
	void CDebugWindoTelemetryTags::NotifyCheckBoxValueChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		RedGui::CRedGuiCheckBox* checkBox = static_cast<RedGui::CRedGuiCheckBox*>(sender);

		IRedTelemetryServiceInterface*  redTelemetryServiceInterface = SRedTelemetryServicesManager::GetInstance().GetService( TELEMETRY_SERVICE_NAME );
		if( redTelemetryServiceInterface != NULL )
		{
			if( value )
			{
				redTelemetryServiceInterface->AddSessionTag( checkBox->GetText() );
			}
			else
			{
				redTelemetryServiceInterface->RemoveSessionTag( checkBox->GetText() );
			}
			
		}
	}

}	// namespace DebugWindows

#endif	// NO_TELEMETRY
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
