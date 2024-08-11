/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "../../common/engine/extAnimCutsceneBodyPartEvent.h"
#include "../../common/engine/extAnimCutsceneDialogEvent.h"

class CEdCutsceneTimeline::CTimelineItemBlocking : public ITimelineItem
{
public:
	CTimelineItemBlocking( Float start, Float duration )
		: m_start( start )
		, m_duration( duration )
	{
		
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemBlocking" ); }

	virtual void SetState( Int32 value ) {}
	virtual Int32 GetState() const { return 0; }

	virtual Bool GetTopText( String& text ) const { return false; }
	virtual Bool GetMiddleText( String& text ) const
	{
		text = String::Printf( TXT( "%.2f s" ), m_duration );
		return true;
	}
	//virtual Bool GetMiddleText( String& text ) const { return false; }
	virtual Bool GetTooltip( String& text ) const { return false; }

	virtual wxColor	GetColor() const
	{
		return wxColor( 126, 209, 124 );
	}

	virtual Bool IsLeftResizable() const
	{ return false; }

	virtual Bool IsRightResizable() const
	{ return false; }

	virtual Bool IsMovable() const
	{ return false; }

	virtual Bool IsEditable() const
	{ return false; }

	virtual Bool IsRemovable() const
	{ return false; }

	virtual Float GetStart() const
	{ return m_start; }

	virtual void UpdatePresentation()
	{ }

	virtual Float SetStart( Float start, Bool deepUpdate ) override
	{ 
		m_start = start;
		return m_start;
	}

	virtual Bool IsDuration() const
	{ return GetDuration() > 0.f; }

	virtual Float GetDuration() const
	{ return m_duration; }

	virtual Float SetDuration( Float duration )
	{ 
		m_duration = duration;
		return m_duration;
	}

	virtual String GetTrackName() const
	{ 
		return TXT("Camera"); 
	}

	virtual void SetTrackName( const String& trackName ) {}

	virtual Gdiplus::Bitmap* GetIcon() const { return NULL; }

	virtual const wxBitmap* GetWxIcon() const override { return nullptr; }

	virtual void SetProperty( IProperty* property, ITimelineItem* sourceItem )
	{
		// Do nothing, because we don't set properties of blocking items
	}

	virtual Bool IsCopyable() const
	{ return false; }

	virtual void CustomDraw( CEdTimeline* canvas, const wxRect& rect ) const {}

protected:
	Float	m_start;
	Float	m_duration;
};

class CEdCutsceneTimeline::CTimelineItemEvent : public ITimelineItem
{
public:
	CTimelineItemEvent( CExtAnimEvent* event )
		: m_event( event )
	{
		ASSERT( m_event != NULL );
	}

	CExtAnimEvent* GetEvent()
	{ return m_event; }

	const CExtAnimEvent* GetEvent() const
	{ return m_event; }

	virtual Bool IsLeftResizable() const
	{ return true; }

	virtual Bool IsRightResizable() const
	{ return true; }

	virtual Bool   IsMovable() const
	{ return true; }

	virtual Bool   IsEditable() const
	{ return true; }

	virtual Bool   IsRemovable() const
	{ return true; }

	virtual Bool IsCopyable() const
	{ return true; }

	virtual Float GetStart() const
	{ return m_event->GetStartTime(); }

	virtual Float SetStart( Float start, Bool deepUpdate ) override
	{
		m_event->SetStartTime( start );
		return start;
	}

	virtual Bool IsDuration() const
	{ return IsType< CExtAnimDurationEvent >( m_event ); }

	virtual Float GetDuration() const
	{ return static_cast< CExtAnimDurationEvent* >( m_event )->GetDuration(); }

	virtual Float SetDuration( Float duration )
	{
		static_cast< CExtAnimDurationEvent* >( m_event )->SetDuration( duration );
		return duration;
	}

	virtual Bool GetTopText( String& text ) const
	{ 
		text = m_event->GetEventName().AsString();
		return true;
	}

	virtual Bool GetTooltip( String& text ) const
	{ 
		text = String::Printf( TXT( "%s : %s" ), m_event->GetEventName().AsString().AsChar(),
			m_event->GetClass()->GetName().AsString().AsChar() );

		return true;
	}

	virtual Bool GetMiddleText( String& text ) const
	{
		return false;
	}

	virtual String GetTrackName() const
	{ return m_event->GetTrackName(); }

	virtual wxColor GetColor() const
	{ return wxColor( 100, 100, 100, 150 ); }

	virtual void SetTrackName( const String& trackName )
	{ m_event->SetTrackName( trackName ); }

	virtual const Char* GetIconName() const
	{ return NULL; }

	virtual Gdiplus::Bitmap* GetIcon() const
	{
		if( m_bitmap == NULL && GetIconName() != NULL )
		{
			m_wxBitmap = SEdResources::GetInstance().LoadBitmap( GetIconName() );
			m_bitmap = ConvertToGDI( m_wxBitmap );
		}

		return m_bitmap;
	}

	virtual const wxBitmap* GetWxIcon() const override
	{
		if( m_bitmap == NULL && GetIconName() != NULL )
		{
			m_wxBitmap = SEdResources::GetInstance().LoadBitmap( GetIconName() );
			m_bitmap = ConvertToGDI( m_wxBitmap );
		}

		return m_bitmap? &m_wxBitmap : nullptr;
	}

	virtual void UpdatePresentation() {}

	virtual void SetProperty( IProperty* property, ITimelineItem* sourceItem )
	{
		CTimelineItemEvent* source = dynamic_cast< CTimelineItemEvent* >( sourceItem );
		if( source == NULL )
		{
			ASSERT( 0 && "Mismatched event types" );
			return;
		}

		// Get event properties
		TDynArray< CProperty* > props;
		m_event->GetClass()->GetProperties( props );

		for( TDynArray< CProperty* >::iterator propIter = props.Begin();
			propIter != props.End(); ++propIter )
		{
			CProperty* prop = *propIter;

			// Find matching property
			if( prop->GetName() != property->GetName() )
			{
				continue;
			}

			// Replace value
			prop->Set( m_event, property->GetOffsetPtr( source->GetEvent() ) );
		}
	}

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const {}

	virtual void SetState( Int32 value ) {}
	virtual Int32 GetState() const { return 0; }

private:
	CExtAnimEvent*			 m_event;
	mutable wxBitmap		 m_wxBitmap;
	mutable Gdiplus::Bitmap* m_bitmap;
};

class CEdCutsceneTimeline::CTimelineItemBodypart : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemBodypart( CExtAnimCutsceneBodyPartEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemBodypart" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 0, 58, 255, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_CUTSCENE_BODYPART" ); }
};

class CEdCutsceneTimeline::CTimelineItemResetCloth : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemResetCloth( CExtAnimCutsceneResetClothAndDangleEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemResetCloth" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 0, 58, 255, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_CUTSCENE_BODYPART" ); }
};

class CEdCutsceneTimeline::CTimelineItemDisableCloth : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemDisableCloth( CExtAnimCutsceneDisableClothEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CExtAnimCutsceneDisableClothEvent" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 0, 58, 255, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_CUTSCENE_BODYPART" ); }
};

class CEdCutsceneTimeline::CTimelineItemDisableDangle : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemDisableDangle( CExtAnimCutsceneDisableDangleEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CExtAnimCutsceneDisableDangleEvent" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 0, 58, 255, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_CUTSCENE_BODYPART" ); }
};

class CEdCutsceneTimeline::CTimelineItemEffect : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemEffect( CExtAnimEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemEffect" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 212, 25, 5, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_CUTSCENE_EFFECT" ); }
};

class CEdCutsceneTimeline::CTimelineItemFade : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemFade( CExtAnimEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemFade" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 80, 80, 80, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_CUTSCENE_FADE" ); }
};

class CEdCutsceneTimeline::CTimelineItemSlowMo : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemSlowMo( CExtAnimEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemSlowMo" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 180, 180, 180, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_CUTSCENE_EFFECT" ); }
};

class CEdCutsceneTimeline::CTimelineItemDialog : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemDialog( CExtAnimCutsceneDialogEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemDialog" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 72, 88, 143, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_CUTSCENE_DIALOG" ); }
};

class CEdCutsceneTimeline::CTimelineItemAnimation : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemAnimation( CExtAnimEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemAnimation" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 100, 100, 100, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_ANIMATION" ); }
};

class CEdCutsceneTimeline::CTimelineItemSound : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemSound( CExtAnimCutsceneSoundEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemSound" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ANIMATION_SOUND" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 72, 145, 235, 150 ); }

};

class CEdCutsceneTimeline::CTimelineItemEnvironment : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemEnvironment( CExtAnimEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemEnvironment" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 107, 156, 47, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_CUTSCENE_ENVIRONMENT" ); }
};

class CEdCutsceneTimeline::CTimelineItemDof : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemDof( CExtAnimEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemDof" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 107, 156, 47, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_CAMERA" ); }
};

class CEdCutsceneTimeline::CTimelineItemClippingPlanes : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemClippingPlanes( CExtAnimEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemClippingPlanes" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 153, 217, 234, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_CAMERA" ); }
};

class CEdCutsceneTimeline::CTimelineItemItem : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemItem( CExtAnimItemEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemItem" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 239, 213, 137, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ANIMATION_ITEM" ); }
};

class CEdCutsceneTimeline::CTimelineItemFootstep : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemFootstep( CExtAnimEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemFootstep" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 232, 125, 113, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ANIMATION_FOOTSTEP" ); }
};

class CEdCutsceneTimeline::CTimelineItemQuest : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemQuest( CExtAnimEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemQuest" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 255, 55, 255, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_QUEST" ); }
};

class CEdCutsceneTimeline::CTimelineItemReattachItem : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemReattachItem( CExtAnimReattachItemEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemReattachItem" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 239, 213, 137, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ANIMATION_ITEM" ); }
};

class CEdCutsceneTimeline::CTimelineItemMorphing : public CEdCutsceneTimeline::CTimelineItemEvent
{
public:
	CTimelineItemMorphing( CExtAnimEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemMorphing" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 174, 222, 244, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ANIMATION_EXPLORATION" ); }
};
