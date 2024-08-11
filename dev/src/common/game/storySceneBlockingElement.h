/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneElement.h"

class CStorySceneBlockingElement : public CStorySceneElement
{
	DECLARE_ENGINE_CLASS( CStorySceneBlockingElement, CStorySceneElement, 0 );

public:
	CStorySceneBlockingElement();

	CStorySceneBlockingElement( CStorySceneEvent* event );

	virtual ~CStorySceneBlockingElement();

	//! Calculate element duration
	virtual Float CalculateDuration( const String& locale ) const override;

	//! Start playing of this dialog element
	virtual IStorySceneElementInstanceData* OnStart( CStoryScenePlayer* player ) const;

	virtual void OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const;

	virtual Bool IsPlayable() const { return true; }

	virtual Bool CanBeDeleted() const { return true; }

	RED_INLINE CStorySceneEvent* GetEvent() const
	{return m_event; }

	void SetEvent( CStorySceneEvent* event );

	// Custom serializer for events
	virtual void OnSerialize( IFile& file );

protected:
	CStorySceneEvent*	m_event;
};

BEGIN_CLASS_RTTI( CStorySceneBlockingElement );
	PARENT_CLASS( CStorySceneElement );
	PROPERTY_INLINED_NOSERIALIZE( m_event, TXT( "Event to run" ) );
END_CLASS_RTTI();
