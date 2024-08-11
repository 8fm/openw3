/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneElement.h"

class CStoryScenePauseElement : public CStorySceneElement
{
	DECLARE_ENGINE_CLASS( CStoryScenePauseElement, CStorySceneElement, 0 );

public:
	CStoryScenePauseElement();

	virtual Float CalculateDuration( const String& locale ) const override;
	void SetDuration( Float duration );

	//! Start playing of this dialog element
	virtual IStorySceneElementInstanceData* OnStart( CStoryScenePlayer* player ) const;

	//! Get list of element that can be scheduled via this element
	virtual void OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const;

	virtual Bool IsPlayable() const { return true; }

	virtual Bool CanBeDeleted() const { return true; }

protected:
	Float	m_duration;							// Pause duration. It's the same in all locales.
};

BEGIN_CLASS_RTTI( CStoryScenePauseElement );
	PARENT_CLASS( CStorySceneElement );
	PROPERTY( m_duration );
END_CLASS_RTTI();
