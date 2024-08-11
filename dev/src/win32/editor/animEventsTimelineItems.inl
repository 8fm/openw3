#include "../../common/engine/extAnimBehaviorEvents.h"
#include "../../common/engine/extAnimExplorationEvent.h"
/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

class CEdAnimEventsTimeline::CTimelineItemEvent : public ITimelineItem
{
public:
	CTimelineItemEvent( CExtAnimEvent* event )
		: m_event( event )
		, m_container( NULL )
	{
		ASSERT( m_event != NULL );
	}

	CExtAnimEvent* GetEvent()
	{ return m_event; }

	const IEventsContainer* GetContainer() const
	{ return m_container; }

	void SetContainer( const IEventsContainer* container, Bool mainContainer )
	{
		m_container = container; 
		m_mainCointainer = mainContainer;
		if ( m_container && ! m_mainCointainer )
		{
			m_containerName = m_container->GetParentResource()->GetFriendlyName();
			size_t at = -1;
			if ( m_containerName.FindCharacter( '.', at, 0, m_containerName.GetLength()-1, true ) )
			{
				m_containerName = m_containerName.LeftString( at ) + TXT(" - ") + m_containerName.MidString( at + 1, m_containerName.GetLength() - at - 2 );
			}
			if ( m_containerName.FindCharacter( '\\', at, 0, m_containerName.GetLength()-1, true ) )
			{
				m_containerName = m_containerName.RightString( m_containerName.GetLength() - at - 1 );
			}
			m_containerName = TXT("{") + m_containerName + TXT("}."); // to make it appear at the end
		}
		else
		{
			m_containerName = TXT("");
		}
	}

	virtual Bool IsLeftResizable() const
	{ return true; }

	virtual Bool IsRightResizable() const
	{ return true; }

	virtual Bool   IsMovable() const
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
	{
		return m_containerName + m_event->GetTrackName();
	}

	virtual void SetTrackName( const String& trackName )
	{ m_event->SetTrackName( trackName ); }

	virtual const Char* GetIconName() const = 0;

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

	virtual Bool IsEditable() const
	{ return true;	}

	virtual Bool IsRemovable() const
	{ return true; }

	virtual Bool IsCopyable() const
	{ return true; }

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
	const IEventsContainer*  m_container; // set only if not in main container
	Bool					 m_mainCointainer;
	String					 m_containerName;
	mutable wxBitmap		 m_wxBitmap;
	mutable Gdiplus::Bitmap* m_bitmap;
};

class CEdAnimEventsTimeline::CTimelineItemGhostEvent : public ITimelineItem
{
public:
	CTimelineItemGhostEvent( CTimelineItemEvent* baseEvent, IEventsContainer* baseResource )
		: m_baseEvent( baseEvent )
		, m_resource( baseResource->GetParentResource() )
	{
		ASSERT( m_baseEvent != NULL );
	}

	//CExtAnimEvent* GetEvent()
	//{ return m_event; }

	virtual String GetTypeName() const
	{ return m_baseEvent->GetTypeName(); }

	virtual Bool IsLeftResizable() const
	{ return m_baseEvent->IsLeftResizable(); }

	virtual Bool IsRightResizable() const
	{ return m_baseEvent->IsRightResizable(); }

	virtual Bool IsMovable() const
	{ return m_baseEvent->IsMovable(); }

	virtual Float GetStart() const
	{ return m_baseEvent->GetStart(); }

	virtual Float SetStart( Float start, Bool deepUpdate ) override
	{
		if( m_resource->MarkModified() )
		{
			return m_baseEvent->SetStart( start, deepUpdate );
		}
		else
		{
			return GetStart();
		}
	}

	virtual Bool IsDuration() const
	{ return m_baseEvent->IsDuration(); }

	virtual Float GetDuration() const
	{ return m_baseEvent->GetDuration(); }

	virtual Float SetDuration( Float duration )
	{
		if( m_resource->MarkModified() )
		{
			return m_baseEvent->SetDuration( duration );
		}
		else
		{
			return GetDuration();
		}
	}

	virtual Bool GetTopText( String& text ) const
	{ return m_baseEvent->GetTopText( text ); }

	virtual Bool GetTooltip( String& text ) const
	{ return m_baseEvent->GetTooltip( text ); }

	virtual Bool GetMiddleText( String& text ) const
	{ return m_baseEvent->GetMiddleText( text ); }

	virtual String GetTrackName() const
	{ return m_baseEvent->GetTrackName(); }

	virtual void SetTrackName( const String& trackName )
	{ 
		if( m_resource->MarkModified() )
		{
			m_baseEvent->SetTrackName( trackName );
		}
	}

	virtual const Char* GetIconName() const
	{ return m_baseEvent->GetIconName(); }

	Gdiplus::Bitmap* ConvertToGrayscale( const wxBitmap& bmp ) const
	{
		Gdiplus::Bitmap *result = new Gdiplus::Bitmap(bmp.GetWidth(), bmp.GetHeight(), PixelFormat32bppARGB );
		wxImage image = bmp.ConvertToImage();
		if( image.HasAlpha() )
		{
			for (Int32 i = 0; i < bmp.GetWidth(); i++)
			{
				for (Int32 j = 0; j < bmp.GetHeight(); j++)
				{
					wxColour c( image.GetRed(i, j), image.GetGreen(i, j), image.GetBlue(i, j), image.GetAlpha(i, j) );
					if( c.Red() != 255 && c.Green() != 255 && c.Blue() != 255 )
					{
						c = CEdCanvas::ConvertToGrayscale( c );
					}
					Gdiplus::Color color(c.Alpha(), c.Red(), c.Green(), c.Blue() );
					result->SetPixel(i, j, color);
				};
			}
		}
		else
		{
			for (Int32 i = 0; i < bmp.GetWidth(); i++)
			{
				for (Int32 j = 0; j < bmp.GetHeight(); j++)
				{
					wxColour c( image.GetRed(i, j), image.GetGreen(i, j), image.GetBlue(i, j), 255 );
					if( c.Red() != 255 && c.Green() != 255 && c.Blue() != 255 )
					{
						c = CEdCanvas::ConvertToGrayscale( c );
					}
					Gdiplus::Color color(c.Alpha(), c.Red(), c.Green(), c.Blue() );
					result->SetPixel(i, j, color);
				};
			}
		}
		return result;
	}

	virtual Gdiplus::Bitmap* GetIcon() const
	{ 
		if( m_bitmap == NULL && GetIconName() != NULL )
		{
			// TODO: wxBitmap is not greyscaled
			m_wxBitmap = SEdResources::GetInstance().LoadBitmap( GetIconName() );
			m_bitmap = ConvertToGrayscale( m_wxBitmap );
		}

		return m_bitmap;
	}

	virtual const wxBitmap* GetWxIcon() const override
	{
		if( m_bitmap == NULL && GetIconName() != NULL )
		{
			// TODO: wxBitmap is not greyscaled
			m_wxBitmap = SEdResources::GetInstance().LoadBitmap( GetIconName() );
			m_bitmap = ConvertToGrayscale( m_wxBitmap );
		}

		return m_bitmap? &m_wxBitmap : nullptr;
	}

	virtual wxColour GetColor() const
	{ return wxColour( 50, 50, 50 ); }

	virtual void UpdatePresentation()
	{ m_baseEvent->UpdatePresentation(); }

	virtual Bool IsEditable() const
	{ return false;	}

	virtual Bool IsCopyable() const
	{ return false; }

	virtual Bool IsRemovable() const
	{ return false; }

	virtual void SetProperty( IProperty* property, ITimelineItem* sourceItem )
	{
		// Setting properties on ghost event is forbidden!
	}

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const {}

	virtual void SetState( Int32 value ) {}
	virtual Int32 GetState() const { return 0; }

private:
	mutable wxBitmap		 m_wxBitmap;
	mutable Gdiplus::Bitmap* m_bitmap;
	CTimelineItemEvent*		 m_baseEvent;
	CResource*				 m_resource;
};

class CEdAnimEventsTimeline::CTimelineItemAnimation : public CEdAnimEventsTimeline::CTimelineItemEvent
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

class CEdAnimEventsTimeline::CTimelineItemFootstep : public CEdAnimEventsTimeline::CTimelineItemEvent
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

class CEdAnimEventsTimeline::CTimelineItemItem : public CEdAnimEventsTimeline::CTimelineItemEvent
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

class CEdAnimEventsTimeline::CTimelineItemItemEffect : public CEdAnimEventsTimeline::CTimelineItemEvent
{
public:
	CTimelineItemItemEffect( CExtAnimItemEffectEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemItemEffect" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 212, 25, 5, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_CUTSCENE_EFFECT" ); }
};

class CEdAnimEventsTimeline::CTimelineItemItemAnimation : public CEdAnimEventsTimeline::CTimelineItemEvent
{
public:
	CTimelineItemItemAnimation( CExtAnimItemAnimationEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemItemAnimation" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 200, 25, 54, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ITEM_ANIMATION" ); }
};

class CEdAnimEventsTimeline::CTimelineItemItemBehavior : public CEdAnimEventsTimeline::CTimelineItemEvent
{
public:
	CTimelineItemItemBehavior( CExtAnimItemBehaviorEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemItemBehavior" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 30, 200, 100, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ITEM_BEHAVIOR" ); }
};

class CEdAnimEventsTimeline::CTimelineItemItemDrop : public CEdAnimEventsTimeline::CTimelineItemEvent
{
public:
	CTimelineItemItemDrop( CExtAnimDropItemEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemItemDrop" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 80, 100, 255, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ITEM_DROP" ); }
};

class CEdAnimEventsTimeline::CTimelineItemExploration : public CEdAnimEventsTimeline::CTimelineItemEvent
{
public:
	CTimelineItemExploration( CExtAnimExplorationEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemExploration" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 174, 222, 244, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ANIMATION_EXPLORATION" ); }
};

class CEdAnimEventsTimeline::CTimelineItemMorphing : public CEdAnimEventsTimeline::CTimelineItemEvent
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

class CEdAnimEventsTimeline::CTimelineItemSound : public CEdAnimEventsTimeline::CTimelineItemEvent
{
public:
	CTimelineItemSound( CExtAnimSoundEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemSound" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ANIMATION_SOUND" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 53, 91, 230, 150 ); }

};

class CEdAnimEventsTimeline::CTimelineItemCombo : public CEdAnimEventsTimeline::CTimelineItemEvent
{
public:
	CTimelineItemCombo( CExtAnimComboEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemCombo" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 255, 179, 128, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ANIMATION_COMBO" ); }
};

class CEdAnimEventsTimeline::CTimelineItemHit : public CEdAnimEventsTimeline::CTimelineItemEvent
{
public:
	CTimelineItemHit( CExtAnimHitEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemHit" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 255, 179, 128, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ANIMATION_HIT" ); }
};

class CEdAnimEventsTimeline::CTimelineItemLookAt : public CEdAnimEventsTimeline::CTimelineItemEvent
{
public:
	CTimelineItemLookAt( CExtAnimLookAtEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemLookAt" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 255, 255, 255, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_LOOKAT" ); }
};

class CEdAnimEventsTimeline::CTimelineItemProjectile : public CEdAnimEventsTimeline::CTimelineItemEvent
{
public:
	CTimelineItemProjectile( CExtAnimProjectileEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemProjectile" ); }

	virtual wxColor GetColor() const
	{ return wxColor( 255, 179, 128, 150 ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_REACTION" ); }
};

class CEdAnimEventsTimeline::CTimelineItemSyncEvent : public CEdAnimEventsTimeline::CTimelineItemEvent
{
public:
	CTimelineItemSyncEvent( CExtAnimItemSyncEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	CTimelineItemSyncEvent( CExtAnimItemSyncDurationEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	CTimelineItemSyncEvent( CExtAnimItemSyncWithCorrectionEvent* event )
		: CTimelineItemEvent( event )
	{
	}

	virtual wxColor GetColor() const
	{ return wxColor( 100, 100, 100, 150 ); }

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemSync" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_SYNC" ); }
};