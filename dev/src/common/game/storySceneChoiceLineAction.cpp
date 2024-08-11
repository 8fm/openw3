/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../core/contentManager.h"
#include "../engine/localizationManager.h"
#include "storySceneChoiceLineAction.h"

IMPLEMENT_RTTI_ENUM( EDialogActionIcon );
IMPLEMENT_ENGINE_CLASS( IStorySceneChoiceLineAction );
IMPLEMENT_ENGINE_CLASS( CStorySceneChoiceLineActionScripted );
IMPLEMENT_ENGINE_CLASS( CStorySceneChoiceLineActionScriptedContentGuard );
IMPLEMENT_ENGINE_CLASS( CStorySceneChoiceLineActionStallForContent );

#define CONTENT_MISSING_KEY TXT("panel_map_annot_tavel_downloading_content")

Bool CStorySceneChoiceLineActionScripted::CanUseAction()
{
	// DIALOG_TOMSIN_TODO
	Bool functionResult = true;
	CallFunctionRet< Bool >( this, CNAME( CanUseAction ), functionResult );
	return functionResult;
}

String CStorySceneChoiceLineActionScripted::GetActionText()
{
	// DIALOG_TOMSIN_TODO
	String functionResult = String::EMPTY;
	CallFunctionRet< String >( this, CNAME( GetActionText ), functionResult );
	return functionResult;
}

EDialogActionIcon CStorySceneChoiceLineActionScripted::GetActionIcon()
{
	// DIALOG_TOMSIN_TODO
	EDialogActionIcon functionResult = DialogAction_NONE;
	CallFunctionRet< EDialogActionIcon >( this, CNAME( GetActionIcon ), functionResult );
	return functionResult;
}

void CStorySceneChoiceLineActionScripted::PerformAction()
{
	// DIALOG_TOMSIN_TODO
	CallFunction( this, CNAME( PerformAction ) );
}

//////////////////////////////////////////////////////////////////////////

Bool CStorySceneChoiceLineActionScriptedContentGuard::CanUseAction()
{
	if ( m_playGoChunk && !GContentManager->IsContentAvailable( m_playGoChunk ) )
	{
		return false;
	}

	return TBaseClass::CanUseAction();
}

String CStorySceneChoiceLineActionScriptedContentGuard::GetActionText()
{
	/*if ( m_playGoChunk && !GContentManager->IsContentAvailable( m_playGoChunk ) )
	{
		return SLocalizationManager::GetInstance().GetStringByStringKeyCachedWithFallback( CONTENT_MISSING_KEY );
	}*/

	return TBaseClass::GetActionText();
}

EDialogActionIcon CStorySceneChoiceLineActionScriptedContentGuard::GetActionIcon()
{
	if ( m_playGoChunk && !GContentManager->IsContentAvailable( m_playGoChunk ) )
	{
		return DialogAction_CONTENT_MISSING;
	}

	return TBaseClass::GetActionIcon();
}

void CStorySceneChoiceLineActionScriptedContentGuard::PerformAction()
{
	TBaseClass::PerformAction();
}

CName CStorySceneChoiceLineActionScriptedContentGuard::GetPlayGoChunk()
{
	if ( m_playGoChunk )
	{
		return m_playGoChunk;
	}

	return TBaseClass::GetPlayGoChunk();
}

//////////////////////////////////////////////////////////////////////////

Bool CStorySceneChoiceLineActionStallForContent::CanUseAction()
{
	if ( GContentManager->GetStallForMoreContent() != eContentStall_None )
	{
		return false;
	}

	return TBaseClass::CanUseAction();
}

String CStorySceneChoiceLineActionStallForContent::GetActionText()
{
	// For now just disable it. Avoid TRC failures of making the user aware of the installation process.
	return TBaseClass::GetActionText();
}

EDialogActionIcon CStorySceneChoiceLineActionStallForContent::GetActionIcon()
{
	// For now no special icon. Avoid TRC failures of making the user aware of the installation process.
	return TBaseClass::GetActionIcon();
}

void CStorySceneChoiceLineActionStallForContent::PerformAction()
{
	TBaseClass::PerformAction();
}