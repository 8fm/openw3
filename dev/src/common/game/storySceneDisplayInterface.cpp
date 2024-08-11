
#include "build.h"
#include "storySceneDisplayInterface.h"
#include "storySceneSystem.h"

CGlobalStorySceneDisplay::CGlobalStorySceneDisplay( CStorySceneSystem* system )
	: m_system( system )
{

}

void CGlobalStorySceneDisplay::ShowPreviousDialogText( const String& text )
{
	m_system->ShowPreviousDialogText( text );
}

void CGlobalStorySceneDisplay::HidePreviousDialogText()
{
	m_system->HidePreviousDialogText();
}

void CGlobalStorySceneDisplay::ShowDialogText( const String& text, ISceneActorInterface* actor, EStorySceneLineType lineType, Bool alternativeUI)
{
	m_system->ShowDialogText( text, actor, lineType, alternativeUI );
}

void CGlobalStorySceneDisplay::HideDialogText( ISceneActorInterface* actor, EStorySceneLineType lineType )
{
	m_system->HideDialogText( actor, lineType );
}

void CGlobalStorySceneDisplay::HideAllDialogTexts()
{
	m_system->HideAllDialogTexts();
}

void CGlobalStorySceneDisplay::ShowChoiceTimer( Float timePercent )
{
	GCommonGame->GetSystem< CStorySceneSystem >()->ShowChoiceTimer( timePercent );
}

void CGlobalStorySceneDisplay::HideChoiceTimer()
{
	GCommonGame->GetSystem< CStorySceneSystem >()->HideChoiceTimer();
}

void CGlobalStorySceneDisplay::SetChoices( const TDynArray< SSceneChoice >& choices, Bool alternativeUI )
{
	GCommonGame->GetSystem< CStorySceneSystem >()->SetChoices( choices, alternativeUI );
}

void CGlobalStorySceneDisplay::SetSceneCameraMovable( Bool flag )
{
	GCommonGame->GetSystem< CStorySceneSystem >()->SetSceneCameraMovable( flag );
}

const String& CGlobalStorySceneDisplay::GetLastDialogText()
{
	return m_system->GetLastDialogText();
}

void CGlobalStorySceneDisplay::ShowDebugComment( CGUID commentId, const String& comment )
{}

void CGlobalStorySceneDisplay::HideDebugComment( CGUID commentId )
{}

void CGlobalStorySceneDisplay::HideAllDebugComments()
{}
