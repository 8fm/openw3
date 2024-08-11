/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storySceneIncludes.h"
#include "storyScene.h"

class CStoryScenePlayer;
class CStorySceneSection;
class IStorySceneElementInstanceData;

/**
 *	Dialog interception helper class.
 *
 *	Interceptions are used when the player walks away from the NPC (exceeds certain distance). In that case NPC stops its dialog and starts
 *	(any of) "interception section" (e.g. "This way!").
 */
class CStorySceneInterceptionHelper
{
private:
	CStoryScenePlayer*					m_player;
	Float								m_interceptTimeout;
	const CStorySceneSection*			m_currentInterceptingSection;
	CStorySceneSection*					m_currentInterceptedSection;
	Bool								m_isComingBackFromInterception;

public:
	CStorySceneInterceptionHelper( CStoryScenePlayer* player );
	void OnResetPlayer();
	void OnPlayerTick( Float timeDelta );
	Bool OnGoToNextElement( Bool& changedSection );
	Bool OnChangeSection();
	Bool OnCheckElement( IStorySceneElementInstanceData* currElem );
	Bool OnPlayElement();

	Bool IsEmptyInterceptionInProgress();
	Bool TickEmptyInterception();

private:
	RED_FORCE_INLINE Bool IsInterceptionInProgress() const { return m_currentInterceptedSection != nullptr; }
	Bool AreAllSectionActorsWithinDistanceFromPlayer( const CStorySceneSection* section, Float radius );
	void BeginInterception();
	void EndInterception();
};