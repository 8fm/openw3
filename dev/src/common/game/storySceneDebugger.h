
#pragma once

#include "storyScenePlayer.h"
#include "sceneLog.h"

struct SStorySceneActorAnimationState;

class IStorySceneDebugger
{
public:
	virtual void PlayerLogicTickBegin( const CStoryScenePlayer* player ) {}
	virtual void PlayerLogicTickEnd( const CStoryScenePlayer* player ) {}

	virtual void BeginTickElement( const IStorySceneElementInstanceData* element, Float dt ) {}
	virtual void EndTickElement( const IStorySceneElementInstanceData* element, Float dt ) {}

	virtual void FireAllStartEvents( const IStorySceneElementInstanceData* element ) {}
	virtual void FireAllStopEvents( const IStorySceneElementInstanceData* element ) {}

	virtual void EventInit( const CStorySceneEvent* event ) {}
	virtual void EventDeinit( const CStorySceneEvent* event ) {}

	virtual void EventProcess( const CStorySceneEvent* event, Float eventTime, Float progress, Float timeDelta ) {}
	virtual void EventSkipped( const CStorySceneEvent* event, Float eventTime, Float timeDelta ) {}

	virtual void EventStart( const CStorySceneEvent* event ) {}
	virtual void EventEnd( const CStorySceneEvent* event ) {}

	virtual void OnCreatedSceneElementInstanceData( const CStorySceneElement* element, const IStorySceneElementInstanceData* instanceData ) {}

	virtual void SignalAcceptChoice( const SSceneChoice& choosenLine ) {}
	virtual void SignalSkipLine() {}
	virtual void SignalForceSkipSection() {}

	virtual void OnChangeSection( const CStorySceneSection* prev, const CStorySceneSection* next ) {}
	virtual void OnFinishChangingSection( const CStorySceneSection* prev, const CStorySceneSection* next ) {}

	virtual void OnStartFromInput( const CStorySceneInput* input ) {}
	virtual void OnFinishedAtOutput( const CStorySceneOutput* output ) {}

	virtual void OnExecutor_ProcessPlacements( const CEntity* entity, const Matrix& placementWS ) {}
	virtual void OnExecutor_ChangeIdle( const CEntity* entity, const SStorySceneActorAnimationState& newState ) {}
};
