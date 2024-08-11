#pragma once

class CStoryScenePlayer;

/// Listens to scene playback events
class IStoryScenePlaybackListener
{
public:
	virtual ~IStoryScenePlaybackListener() {}
	 
	// called on scene end, stopped is true for scenes stopped manually
	virtual void OnEnd( CStoryScenePlayer* player, Bool stopped ) = 0;

	// called on injected returnable dialog end
	virtual void OnInjectedReturnDialogEnd( CStoryScenePlayer* player, const CName activatedOutputName, TSoftHandle< CStoryScene > injectedScene, TSoftHandle< CStoryScene > _targetScene ) = 0;
};