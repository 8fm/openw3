/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storyScenePauseElement.h"
#include "storyScenePlayer.h"

IMPLEMENT_ENGINE_CLASS( CStoryScenePauseElement );

class StoryScenePauseInstanceData : public IStorySceneElementInstanceData
{
private:
	Float	m_timeToEnd;

public:
	StoryScenePauseInstanceData( const CStoryScenePauseElement* pauseElement, CStoryScenePlayer* player )
		: IStorySceneElementInstanceData( pauseElement, player )
		, m_timeToEnd( 0.f )
	{
		
	}

	virtual String GetName() const { return String( TXT("Pause") ); }

protected:
	virtual Bool OnInit( const String& /* locale */ ) override
	{
		SCENE_ASSERT( m_timeToEnd == 0.f );
		return true;
	}

	virtual void OnPlay()
	{
		m_timeToEnd = GetDuration();
		SCENE_ASSERT( m_timeToEnd > 0.f );
	}

	virtual Bool OnTick( Float timeDelta ) override
	{
		SCENE_ASSERT( m_timeToEnd > 0.f );

		m_timeToEnd -= timeDelta;

		if ( m_timeToEnd <= 0.0f )
		{
			return false;
		}

		return true;
	}
};

CStoryScenePauseElement::CStoryScenePauseElement()
	: m_duration( 0.0f )
{

}

/*
Calculates pause duration (in seconds).

\param locale For pauses this argument is ignored as pause duration is the same in all locales.
\return Pause duration in seconds.
*/
Float CStoryScenePauseElement::CalculateDuration( const String& /* locale */ ) const
{
	return m_duration;
}

/*
Sets pause duration (in seconds).

Note that pause duration is the same in all locales.
*/
void CStoryScenePauseElement::SetDuration( Float duration )
{
	m_duration = duration;
}

IStorySceneElementInstanceData* CStoryScenePauseElement::OnStart( CStoryScenePlayer* player ) const
{
	return new StoryScenePauseInstanceData( this, player );
}

void CStoryScenePauseElement::OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const
{
	elements.PushBack( this );
}