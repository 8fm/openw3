/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEvent.h"
#include "storySceneEventDuration.h"
#include "storySceneBlockingElement.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneBlockingElement );

class StorySceneBlockingInstanceData : public IStorySceneElementInstanceData
{
public:
	//! Create line
	StorySceneBlockingInstanceData( const CStorySceneBlockingElement* blockingElement,
			CStoryScenePlayer* player )
		: IStorySceneElementInstanceData( blockingElement, player )
	{

	}

	virtual String GetName() const { return String( TXT("Blocking element") ); }

protected:
	virtual Bool OnTick( Float timeDelta ) override
	{
		return true;
	}
};


CStorySceneBlockingElement::CStorySceneBlockingElement()
	: m_event( NULL )
{

}

CStorySceneBlockingElement::CStorySceneBlockingElement( CStorySceneEvent* event )
	: m_event( event )
{
}

CStorySceneBlockingElement::~CStorySceneBlockingElement()
{
	delete m_event;
}

Float CStorySceneBlockingElement::CalculateDuration( const String& locale ) const
{
	if( m_event == NULL )
	{
		// Use default
		return CStorySceneElement::CalculateDuration( locale );
	}

	if( IsType< CStorySceneEventDuration >( m_event ) )
	{
		// We don't have instance buffer here so we can't return duration of event.
		// CStorySceneBlockingElement is not used and is scheduled for removal so
		// there's no point sweating any smarter solution that this:
		SCENE_ASSERT( false && "CStorySceneBlockingElement::m_event used." );
		return CStorySceneElement::CalculateDuration( locale );
	}
	else
	{
		// Use default
		return CStorySceneElement::CalculateDuration( locale );
	}
}

void CStorySceneBlockingElement::SetEvent( CStorySceneEvent* event )
{
	delete m_event;
	m_event = event;
}

void CStorySceneBlockingElement::OnSerialize( IFile& file )
{
	// Serialize base
	TBaseClass::OnSerialize( file );

	// Write to file
	if( file.IsWriter() )
	{
		// Serialize type
		CName type = ( m_event != NULL ) ? m_event->GetClass()->GetName() : CName::NONE;
		file << type;

		if( m_event != NULL )
		{
			// Serialize object
			m_event->GetClass()->Serialize( file, m_event );
		}
	}
	// Read from file
	else if ( file.IsReader() )
	{
		// Read type
		CName eventType;
		file << eventType;

		if( eventType != CName::NONE )
		{
			// Find type in RTTI
			CClass* theClass = SRTTI::GetInstance().FindClass( eventType );
			ASSERT( theClass != NULL );

			// Create object
			CStorySceneEvent* m_event = theClass->CreateObject< CStorySceneEvent >() ;
			ASSERT( m_event != NULL );

			// Serialize object
			theClass->Serialize( file, m_event );
		}
		else
		{
			m_event = NULL;
		}
	}
}

IStorySceneElementInstanceData* CStorySceneBlockingElement::OnStart( CStoryScenePlayer* player ) const
{
	return new StorySceneBlockingInstanceData( this, player );
}

void CStorySceneBlockingElement::OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const
{
	elements.PushBack( this );
}
