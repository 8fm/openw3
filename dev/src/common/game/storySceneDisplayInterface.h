
#pragma once

#include "storySceneIncludes.h"

class ISceneActorInterface;

class IStorySceneDisplayInterface
{
public:
	virtual void ShowDialogText( const String& text, ISceneActorInterface* actor, EStorySceneLineType lineType, Bool alternativeUI ) = 0;
	virtual void HideDialogText( ISceneActorInterface* actor, EStorySceneLineType lineType ) = 0;
	virtual void HideAllDialogTexts() = 0;

	virtual void ShowPreviousDialogText( const String& text ) = 0;
	virtual void HidePreviousDialogText() = 0;

	virtual void SetChoices( const TDynArray< SSceneChoice >& choices, Bool alternativeUI ) = 0;
	virtual void ShowChoiceTimer( Float timePercent ) = 0;
	virtual void HideChoiceTimer() = 0;

	virtual void SetSceneCameraMovable( Bool flag ) = 0;
	virtual const String& GetLastDialogText() = 0;

	virtual void ShowDebugComment( CGUID commentId, const String& comment ) = 0;
	virtual void HideDebugComment( CGUID commentId ) = 0;
	virtual void HideAllDebugComments() = 0;

public:
	virtual ~IStorySceneDisplayInterface() {}
};

//////////////////////////////////////////////////////////////////////////

class CStorySceneSystem;

class CGlobalStorySceneDisplay : public IStorySceneDisplayInterface
{
	CStorySceneSystem* m_system;

public:
	CGlobalStorySceneDisplay( CStorySceneSystem* system );

	virtual void ShowDialogText( const String& text, ISceneActorInterface* actor, EStorySceneLineType lineType, Bool alternativeUI );
	virtual void HideDialogText( ISceneActorInterface* actor, EStorySceneLineType lineType );
	virtual void HideAllDialogTexts();

	virtual void ShowPreviousDialogText( const String& text );
	virtual void HidePreviousDialogText();

	virtual void SetChoices( const TDynArray< SSceneChoice >& choices, Bool alternativeUI );
	virtual void ShowChoiceTimer( Float timePercent );
	virtual void HideChoiceTimer();

	virtual const String& GetLastDialogText();
	virtual void SetSceneCameraMovable( Bool flag );

	virtual void ShowDebugComment( CGUID commentId, const String& comment ) override;
	virtual void HideDebugComment( CGUID commentId ) override;
	virtual void HideAllDebugComments() override;
};
