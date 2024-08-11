/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "r4Popup.h"
#include "..\..\games\r4\r4GuiManager.h"
#include "..\..\common\engine\guiGlobals.h"
#include "..\..\common\engine\viewport.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CR4Popup );

RED_DEFINE_STATIC_NAME( OnClosingPopup )

const Int32 CR4Popup::m_popupTimeScalePriority = 0x7FFFFFFF;

Bool CR4Popup::MakeModal( Bool make )
{
	CR4GuiManager* guiManager = Cast< CR4GuiManager >( m_guiManagerHandle.Get() );
	if ( ! guiManager )
	{
		GUI_ERROR( TXT("CR4Popup::MakeModal - no CGuiManager") );
		return false;
	}

	return guiManager->MakeModal( this, make );
}

void CR4Popup::EnableInput( Bool enable )
{
	CFlashMovie* movie = GetFlashMovie();
	if ( movie )
	{
		movie->SetInputSourceFlags( enable? CFlashMovie::ISF_All: CFlashMovie::ISF_None );
	}
}

void CR4Popup::ApplyParams( Bool onStart )
{
	const CPopupResource* resource = GetPopupResource();
	if ( resource )
	{
		CPopupDef* popupDef = resource->GetPopupDef();
		if ( popupDef )
		{
			CR4GuiManager* guiManager = Cast< CR4GuiManager >( m_guiManagerHandle.Get() );
			if ( ! guiManager )
			{
				GUI_ERROR(TXT("CR4Popup::ApplyParams: No GUI manager found!"));
			}

			IPopupTimeParam* timeParam = popupDef->GetTimeParam();
			if ( timeParam )
			{
				if ( timeParam->IsA< CPopupTimeScaleParam >() )
				{
					CPopupTimeScaleParam* timeScaleParam = Cast< CPopupTimeScaleParam >( timeParam );
					Float timeScale = timeScaleParam->GetTimeScale();
					if ( onStart )
					{
						GGame->SetTimeScale( timeScale, CName( GetFriendlyName() ), m_popupTimeScalePriority );
					}
					else
					{
						GGame->RemoveTimeScale( CName( GetFriendlyName() ) );
					}
				}
				else if ( timeParam->IsA< CPopupPauseParam >() )
				{
					CPopupPauseParam* pauseParam = Cast< CPopupPauseParam >( timeParam );
					EPopupPauseType pauseType =  pauseParam->GetPauseType();
					switch ( pauseType )
					{
					case MPT_FullPause:
						if ( onStart )
						{
							GGame->Pause( GetFriendlyName() );
						}
						else
						{
							GGame->Unpause( GetFriendlyName() );
						}
						break;
					case MPT_ActivePause:
						if ( onStart )
						{
							GGame->SetActivePause( true );
						}
						else
						{
							GGame->SetActivePause( false );
						}
						break;
					case MPT_NoPause:
						// nothing
						break;
					}
				}
			}
		}
	}
}

void CR4Popup::OnClosing()
{
	ApplyParams( false );

	CallEvent( CNAME( OnClosingPopup ) );

	// notify gui manager
	CR4GuiManager* guiManager = Cast< CR4GuiManager >( m_guiManagerHandle.Get() );
	if ( ! guiManager )
	{
		GUI_ERROR( TXT("CR4Popup::OnClosing - no CGuiManager") );
		return;
	}
	guiManager->OnClosingPopup( this );
}

void CR4Popup::funcMakeModal( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, make, true );
	FINISH_PARAMETERS;

	RETURN_BOOL( MakeModal( make ) )
}
