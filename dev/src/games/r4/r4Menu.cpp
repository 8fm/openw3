/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "r4Menu.h"
#include "..\..\games\r4\r4GuiManager.h"
#include "..\..\common\engine\guiGlobals.h"
#include "..\..\common\engine\viewport.h"
#include "..\..\common\engine\renderCommands.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CR4Menu );

RED_DEFINE_STATIC_NAME( OnClosingMenu )

const Int32 CR4Menu::m_menuTimeScalePriority = 0x7FFFFFFF;

CR4Menu::CR4Menu()
	: m_gameRenderDeactivated(false)
{
	// JASON DONT FORGET KEY BIND FIXes
}

Bool CR4Menu::CanKeepHud() const
{
	const CMenuResource* resource = GetMenuResource();
	if ( !resource )
	{
		return false;
	}
	CMenuDef* menuDef = resource->GetMenuDef();
	if ( !menuDef )
	{
		return false;
	}
	IMenuDisplayParam* renderParam = menuDef->GetRenderParam();
	if ( !renderParam )
	{
		return false;
	}
	if ( !renderParam->IsA< CMenuHudParam >() )
	{
		// if there is no CMenuHudParam, return false since menu should hide hud by default
		return false;
	}
	CMenuHudParam* hudParam = Cast< CMenuHudParam >( renderParam );
	if ( !hudParam->GetKeepHud() )
	{
		// if keepHud is set to false - return false immediately, otherwise check possible child
		return false;
	}
	CR4Menu* childMenu = Cast< CR4Menu >( GetParentGuiObject() );
	if ( childMenu )
	{
		return childMenu->CanKeepHud();
	}
	return true;
}

Bool CR4Menu::RequestSubMenu( const CName& menuName, const THandle< IScriptable >& scriptInitData )
{
	CR4Menu* prevMenu = Cast< CR4Menu >( GetParentGuiObject() );
	if ( prevMenu )
	{
		// child menu exists, close it before creating other
		prevMenu->OnClosing();
		prevMenu->Discard();
		SetParentGuiObject( nullptr );
	}

	CR4GuiManager* guiManager = Cast< CR4GuiManager >( m_guiManagerHandle.Get() );
	if ( ! guiManager )
	{
		GUI_ERROR( TXT("CR4Menu::RequestSubMenu - no CGuiManager") );
		return false;
	}

	CR4Menu* menu = Cast< CR4Menu >( guiManager->CreateMenu( menuName, this ) );
	ASSERT( menu );

	if ( menu )
	{
		//TBD: menu parents
		VERIFY( menu->Init( menuName, m_guiManagerHandle.Get() ) );
		menu->SetMenuScriptData( scriptInitData );
		menu->ApplyParams( true );

		SetParentGuiObject( menu );
		guiManager->OnOpenedMenuEvent( menuName );
	}
	else
	{
		SetParentGuiObject( nullptr );
		return false;
	}

	return true;
}

void CR4Menu::CloseSubMenu()
{
	CR4Menu* menu = Cast< CR4Menu >( GetParentGuiObject() );
	if ( menu )
	{
		menu->OnClosing();
		menu->Discard();
	}
}

Bool CR4Menu::MakeModal( Bool make )
{
	CR4GuiManager* guiManager = Cast< CR4GuiManager >( m_guiManagerHandle.Get() );
	if ( ! guiManager )
	{
		GUI_ERROR( TXT("CR4Menu::MakeModal - no CGuiManager") );
		return false;
	}

	return guiManager->MakeModal( this, make );
}

void CR4Menu::EnableInput( Bool enable, Bool recursively )
{
	CFlashMovie* movie = GetFlashMovie();
	if ( movie )
	{
		movie->SetInputSourceFlags( enable? CFlashMovie::ISF_All: CFlashMovie::ISF_None );
	}

	if ( recursively )
	{
		CR4Menu* menu = Cast< CR4Menu >( GetParentGuiObject() );
		if ( menu )
		{
			menu->EnableInput( enable );
		}
	}
}

void CR4Menu::ApplyParams( Bool onStart )
{
	const CMenuResource* resource = GetMenuResource();
	if ( resource )
	{
		CMenuDef* menuDef = resource->GetMenuDef();
		if ( menuDef )
		{
			IMenuBackgroundVideoParam* bgVideoParam  = menuDef->GetBackgroundVideoParam();
			CR4GuiManager* guiManager = Cast< CR4GuiManager >( m_guiManagerHandle.Get() );
			if ( ! guiManager )
			{
				GUI_ERROR(TXT("CR4Menu::ApplyParams: No GUI manager found!"));
			}

			if ( guiManager && bgVideoParam )
			{
				if ( bgVideoParam->IsA< CMenuInheritBackgroundVideoParam >() )
				{
					// Do nothing, don't register/unregister any bg videos
				}
				else if ( bgVideoParam->IsA< CMenuClearBackgroundVideoParam >() )
				{
					if ( onStart )
					{
						guiManager->RegisterBackgroundVideo( this, String::EMPTY );
					}
					else
					{
						guiManager->UnregisterBackgroundVideo( this );
					}
				}
				else if ( CMenuBackgroundVideoFileParam* setVideoParam = Cast< CMenuBackgroundVideoFileParam >( bgVideoParam ) )
				{
					if ( onStart )
					{
						guiManager->RegisterBackgroundVideo( this, setVideoParam->GetVideoFile() );
					}
					else
					{
						guiManager->UnregisterBackgroundVideo( this );
					}
				}
				else if ( CMenuBackgroundVideoAliasParam* scriptVideoParam = Cast< CMenuBackgroundVideoAliasParam >( bgVideoParam ) )
				{
					if ( onStart )
					{
						//TODO! E.g., 'MainMenuBackground' -> "novigrad_loop.usm"
						///constString& videoFile = guiManager->GetVideoForName( scriptVideoParam->GetVideoName() );
						//guiManager->RegisterBackgroundVideo( this, videoFile );
					}
					else
					{
						//guiManager->UnregisterBackgroundVideo( this );
					}
				}
			}

			IMenuTimeParam* timeParam = menuDef->GetTimeParam();
			if ( timeParam )
			{
				if ( timeParam->IsA< CMenuTimeScaleParam >() )
				{
					CMenuTimeScaleParam* timeScaleParam = Cast< CMenuTimeScaleParam >( timeParam );
					Float timeScale = timeScaleParam->GetTimeScale();
					if ( onStart )
					{
						GGame->SetTimeScale( timeScale, CName( GetFriendlyName() ), m_menuTimeScalePriority );
					}
					else
					{
						GGame->RemoveTimeScale( CName( GetFriendlyName() ) );
					}
				}
				else if ( timeParam->IsA< CMenuPauseParam >() )
				{
					CMenuPauseParam* pauseParam = Cast< CMenuPauseParam >( timeParam );
					EMenuPauseType pauseType =  pauseParam->GetPauseType();
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

			IMenuDisplayParam* renderParam = menuDef->GetRenderParam();
			if ( renderParam )
			{
				if ( renderParam->IsA< CMenuRenderBackgroundParam >() )
				{
					CMenuRenderBackgroundParam* renderBackgroundParam = Cast< CMenuRenderBackgroundParam >( renderParam );
					Bool renderGameWorld = renderBackgroundParam->IsRenderGameWorld();
					if ( !renderGameWorld )
					{
						DisallowRenderGameWorld(onStart);
					}
				}
			}
		}
	}
}

void CR4Menu::SetRenderOverride( Bool overrideOn )
{
	if (m_gameRenderDeactivated && overrideOn)
	{
		DisallowRenderGameWorld(false);
	}
	else if (!overrideOn)
	{
		const CMenuResource* resource = GetMenuResource();
		if ( resource )
		{
			CMenuDef* menuDef = resource->GetMenuDef();
			if ( menuDef )
			{
				IMenuDisplayParam* renderParam = menuDef->GetRenderParam();
				if ( renderParam )
				{
					if ( renderParam->IsA< CMenuRenderBackgroundParam >() )
					{
						CMenuRenderBackgroundParam* renderBackgroundParam = Cast< CMenuRenderBackgroundParam >( renderParam );
						Bool renderGameWorld = renderBackgroundParam->IsRenderGameWorld();
						if ( !renderGameWorld )
						{
							DisallowRenderGameWorld(true);
						}
					}
				}
			}
		}
	}
}

void CR4Menu::OnClosing()
{
	// close submenu if there's any
	CloseSubMenu();

	ApplyParams( false );

	CallEvent( CNAME( OnClosingMenu ) );

	// notify gui manager
	CR4GuiManager* guiManager = Cast< CR4GuiManager >( m_guiManagerHandle.Get() );
	if ( ! guiManager )
	{
		GUI_ERROR( TXT("CR4Menu::OnClosing - no CGuiManager") );
		return;
	}
	guiManager->OnClosingMenu( this );
}

void CR4Menu::DisallowRenderGameWorld( Bool disallow )
{
	if (m_gameRenderDeactivated != disallow)
	{
		m_gameRenderDeactivated = disallow;

		// Prime texture streaming for the current frame. This way, if needed, things that are not in the current
		// frame will be unstreamed first. They won't be unstreamed until needed, though. Only do the priming if we're
		// a top-level menu. No reason to do it when we switch between sub-menus.
		IRenderScene* scene = nullptr;
		CRenderFrame* frame = nullptr;
		if ( m_gameRenderDeactivated && GGame->GetActiveWorld() && !GetParent()->IsA< CR4Menu >() )
		{
			scene = GGame->GetActiveWorld()->GetRenderSceneEx();
			CRenderFrameInfo frameInfo( GGame->GetViewport() );
			frame = GGame->GetActiveWorld()->GenerateFrame( GGame->GetViewport(), frameInfo );
		}

		if ( GGame->GetViewport() )
		{
			( new CRenderCommand_SuppressSceneRendering( GGame->GetViewport(), m_gameRenderDeactivated, scene, frame ) )->Commit();
		}

		SAFE_RELEASE( frame );
	}
}

void CR4Menu::funcGetSubMenu( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_HANDLE( CMenu, Cast< CMenu >( GetParentGuiObject() ) )
}

void CR4Menu::funcMakeModal( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, make, true );
	FINISH_PARAMETERS;

	RETURN_BOOL( MakeModal( make ) )
}

void CR4Menu::funcSetRenderGameWorldOverride( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, override, false );
	FINISH_PARAMETERS;

	SetRenderOverride(override);
}