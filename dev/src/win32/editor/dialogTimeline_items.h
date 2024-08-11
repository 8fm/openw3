/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/curveEntity.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneCutscene.h"
#include "../../common/game/storySceneScriptLine.h"
#include "../../common/game/storyScenePauseElement.h"
#include "../../common/game/storySceneBlockingElement.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventDuration.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventEnterActor.h"
#include "../../common/game/storySceneEventExitActor.h"
#include "../../common/game/storySceneEventLookat.h"
#include "../../common/game/storySceneEventDespawn.h"
#include "../../common/game/storySceneEventFade.h"
#include "../../common/game/storySceneEventMimics.h"
#include "../../common/game/storySceneEventMimicsAnim.h"
#include "../../common/game/storySceneEventSound.h"
#include "../../common/game/storySceneEventRotate.h"
#include "../../common/game/storySceneEventCustomCamera.h"
#include "../../common/game/storySceneEventCustomCameraInstance.h"
#include "../../common/game/storySceneEventGameplayCamera.h"
#include "../../common/game/storySceneEventCameraBlend.h"
#include "../../common/game/storySceneEventChangePose.h"
#include "../../common/game/storySceneEventEquipItem.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneComment.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/actor.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneEventMimicPose.h"
#include "../../common/game/storySceneEventMimicFilter.h"
#include "../../common/game/storySceneEventOverridePlacement.h"
#include "../../common/game/storySceneEventGroup.h"
#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneEventPoseKey.h"

#include "timeline.h"
#include "dialogTimeline.h"
#include "timelineImpl/drawing.h"
#include "../../common/game/storySceneEventAttachPropToBone.h"
#include "../../common/game/storySceneEventCsCamera.h"
#include "../../common/game/StorySceneEventCameraLight.h"

namespace DialogTimelineItems
{
	enum EventState
	{
		EVENTSTATE_NORMAL,
		EVENTSTATE_MARKED,
		EVENTSTATE_LOCKED,
	};

class CTimelineItemBlocking : public ITimelineItem
{
public:
	CTimelineItemBlocking( CStorySceneElement* element, Float start, Float duration, Uint32 level = 0 )
		: m_element( element )
		, m_start( start )
		, m_duration( duration )
		, m_highlight( false )
		, m_eventPosition( 0.0f )
		, m_level( level )
		, m_state( EVENTSTATE_NORMAL )
	{
		ASSERT( m_element != NULL );
	}

	CStorySceneElement* GetElement() const
	{ return m_element; }

	void SetHighlight( Bool highlight, Float position )
	{ 
		m_highlight = highlight;
		m_eventPosition = position;
	}

	Bool IsHighlight() const
	{ return m_highlight; }

	virtual Bool IsLeftResizable() const
	{ return false; }

	virtual Bool IsRightResizable() const
	{ return false; }

	virtual Bool   IsMovable() const
	{ return false; }

	virtual Bool   IsEditable() const
	{ return true; }

	virtual Bool   IsRemovable() const
	{ return true; }

	virtual Float GetStart() const
	{ return m_start; }

	virtual void UpdatePresentation()
	{ }

	virtual Float SetStart( Float start, Bool deepUpdate ) override
	{ 
		m_start = start;
		return m_start;
	}

	virtual Bool GetMiddleText( String& text ) const
	{
		if( m_highlight )
		{
			text = String::Printf( TXT( "%.0f %%" ), m_eventPosition * 100.0f );
		}
		else
		{
			text = String::Printf( TXT( "%.2f s" ), m_duration );
		}

		return true;
	}

	virtual Bool  IsDuration() const
	{ return true; }

	virtual Float GetDuration() const
	{ return m_duration; }

	virtual Float SetDuration( Float duration )
	{ 
		m_duration = duration;
		return m_duration;
	}

	virtual String GetTrackName() const
	{ 
		if ( m_level == 0 )
		{
			return CEdTimeline::TIMELINE_DEFAULT_TRACK_NAME; 
		}
		else
		{
			return String::Printf( TXT( "%s [%d]"), CEdTimeline::TIMELINE_DEFAULT_TRACK_NAME.AsChar(), m_level + 1 );
		}
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

	virtual void SetState( Int32 value )
	{
		m_state = ( EventState )value;
	}

	virtual Int32 GetState() const 
	{
		return ( Int32 )m_state;
	}

protected:
	Float					m_start;
	Float					m_duration;
	EventState				m_state;

private:
	CStorySceneElement*     m_element;
	Bool					m_highlight;
	Float					m_eventPosition;
	Uint32					m_level;
};

/*

Note that when background flag is set then CTimelineItemLineElement changes its behavior.
In such case it behaves like a point item - IsDuration() returns false, GetDuration()
retursn 0.0f. This also means that item will report duration that is different from
duration of its CStorySceneLine.
*/
class CTimelineItemLineElement : public CTimelineItemBlocking
{
	wxBitmap*			m_wxImage;
	Gdiplus::Bitmap*	m_image;

	wxBitmap			m_wxBackgroundLineIcon;	// icon used when bg flag is set
	Gdiplus::Bitmap*	m_backgroundLineIcon;	// icon used when bg flag is set

	wxColor				m_color;

	Float				m_voiceStartRelPos;		// relative position at which voice starts
	Float				m_voiceEndRelPos;		// relative position at which voice ends
	TDynArray< TPair< Float, Float > >	m_voiceMaxAmpRelPos;	// relative position at which voice has max amplitude with the weight of the point
	Bool				m_isVOInvalid_BlendIn;
	Bool				m_isVOInvalid_WavDuration;

	Float				m_timeOffsetFront;
	Float				m_timeOffsetBack;

	CEdDialogTimeline*	m_dialogTimeline;

public:
	CTimelineItemLineElement( CEdDialogTimeline* timeline, CStorySceneLine* line, Float start, Float duration, Uint32 level = 0 );

	~CTimelineItemLineElement()
	{
		delete m_backgroundLineIcon;
		delete m_image;
		delete m_wxImage;
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemLineElement" ); }

	virtual Bool IsDuration() const override;
	virtual Float GetDuration() const override;

	virtual Gdiplus::Bitmap* GetIcon() const override;
	virtual const wxBitmap* GetWxIcon() const override;

	virtual Bool GetTopText( String& text ) const
	{
		CStorySceneLine* line = Cast< CStorySceneLine >( GetElement() );
		text = line->GetVoiceTag().AsString();

		return true;
	}

	virtual wxColor GetColor() const
	{
		if( IsHighlight() )
		{
			return wxColor( 195, 255, 195 );
		}
		else
		{
			CStorySceneLine* line = Cast< CStorySceneLine >( GetElement() );

			const CStorySceneSection* section = m_dialogTimeline->GetSection();
			const CStorySceneSectionVariantId variantId = m_dialogTimeline->GetSectionVariantId();
			const Float approvedDuration = section->GetElementApprovedDuration( variantId, line->GetElementID() );
			
			if( Abs( approvedDuration - GetDuration() ) > NumericLimits< Float >::Epsilon() )
			{
				return wxColor( 0, 0, 255 );
			}

			if ( m_isVOInvalid_BlendIn && m_isVOInvalid_WavDuration )
			{
				return wxColor( 255, 250, 250 );
			}
			else if ( m_isVOInvalid_BlendIn )
			{
				return wxColor( 255, 209, 124 );
			}
			else if ( m_isVOInvalid_WavDuration )
			{
				return wxColor( 160, 160, 160 );
			}

			return m_color;
		}
	}

	virtual Bool GetTooltip( String& text ) const
	{ 
		CStorySceneLine* line = Cast< CStorySceneLine >( GetElement() );
		text = line->GetContent();

		return true;
	}

	virtual void CustomDraw( CEdTimeline* canvas, const wxRect& rect ) const;

	virtual Bool IsRemovable() const override
	{
		return false;
	}

	Float GetVoiceStartRelPos() const
	{
		return m_voiceStartRelPos;
	}

	Float GetVoiceEndRelPos() const
	{
		return m_voiceEndRelPos;
	}

	const TDynArray< TPair< Float, Float > >& GetVoiceMaxAmpRelPos() const
	{
		return m_voiceMaxAmpRelPos;
	}

	Bool IsBackgroundLine() const;

private:
	void ReloadImage();
};

class CTimelineItemScriptLine : public CTimelineItemBlocking
{
public:
	CTimelineItemScriptLine( CStorySceneScriptLine* line, Float start, Float duration, Uint32 level = 0 )
		: CTimelineItemBlocking( line, start, duration, level )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemScriptLine" ); }

	virtual Bool GetTopText( String& text ) const
	{
		text = TXT( "Script" );

		return true;
	}

	virtual Bool GetTooltip( String& text ) const
	{
		CStorySceneScriptLine* scriptLine = Cast< CStorySceneScriptLine >( GetElement() );
		text = scriptLine->GetScriptString();

		return true;
	}

	virtual wxColor GetColor() const
	{
		if( IsHighlight() )
		{
			return wxColor( 201, 112, 198 );
		}
		else
		{
			return wxColor( 161, 72, 158 );
		}
	}
};

class CTimelineItemPause : public CTimelineItemBlocking
{
public:
	CTimelineItemPause( CStoryScenePauseElement* pause, Float start, Float duration, CEdDialogTimeline* timeline, Uint32 level = 0 )
		: CTimelineItemBlocking( pause, start, duration, level )
		, m_timeline( timeline )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemPause" ); }

	virtual Bool IsEditable() const override;

	virtual Bool IsRightResizable() const override;

	virtual Float SetDuration( Float duration );

	virtual Bool GetTopText( String& text ) const
	{
		text = TXT( "Pause" );

		return true;
	}

	virtual Bool GetTooltip( String& text ) const
	{
		text = String::Printf( TXT( "Pause for %.2f s" ), m_duration );

		return true;
	}

	virtual wxColor GetColor() const
	{
		if( IsHighlight() )
		{
			return wxColor( 255, 121, 139 );
		}
		else
		{
			return wxColor( 236, 69, 93 );
		}
	}

private:
	CEdDialogTimeline*						m_timeline;
};

class CTimelineItemCutscene : public CTimelineItemBlocking
{
public:
	CTimelineItemCutscene( CStorySceneCutscenePlayer* cutscene, Float start, Float duration, Uint32 level = 0 )
		: CTimelineItemBlocking( cutscene, start, duration, level )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemCutscene" ); }

	virtual Bool GetTopText( String& text ) const
	{
		text = static_cast< CStorySceneCutscenePlayer* >( GetElement() )->GetDescriptionText();
		return true;
	}

	virtual Bool GetTooltip( String& text ) const
	{
		CStorySceneCutscenePlayer* cutsceneLine = static_cast< CStorySceneCutscenePlayer* >( GetElement() );
		ASSERT( cutsceneLine != NULL );

		text = ( cutsceneLine->GetCutscene() != NULL ) ? cutsceneLine->GetCutscene()->GetFriendlyName()
			: TXT( "Empty" );

		return true;
	}

	virtual wxColor GetColor() const
	{
		if( IsHighlight() )
		{
			return wxColor( 126, 201, 102 );
		}
		else
		{
			return wxColor( 86, 161, 72 );
		}
	}
};

class CTimelineItemBlockingEvent : public CTimelineItemBlocking
{
public:
	CTimelineItemBlockingEvent( CStorySceneBlockingElement* blocking, Float start, Float duration, Uint32 level = 0 )
		: CTimelineItemBlocking( blocking, start, duration, level )
	{
	}

	virtual ~CTimelineItemBlockingEvent() { }

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemBlockingEvent" ); }

	virtual Bool IsLeftResizable() const
	{ return false; }

	virtual Bool IsRightResizable() const
	{ return false; }

	virtual Bool   IsMovable() const
	{ return false; }

	virtual Float SetStart( Float start, Bool deepUpdate ) override
	{ return m_start; }

	virtual Float SetDuration( Float duration )
	{ return m_duration; }

	virtual void UpdatePresentation()
	{

	}

	virtual Bool GetTopText( String& text ) const
	{
		CStorySceneBlockingElement* blocking = static_cast< CStorySceneBlockingElement* >( GetElement() );
		text = ( blocking->GetEvent() != NULL ) ? blocking->GetEvent()->GetEventName() : TXT( "Empty" );
		return true;
	}

	virtual Bool GetTooltip( String& text ) const
	{
		CStorySceneBlockingElement* blocking = static_cast< CStorySceneBlockingElement* >( GetElement() );
		text = String::Printf( TXT( "Blocking event '%s'" ),
			( blocking->GetEvent() != NULL ) ? blocking->GetEvent()->GetEventName() : TXT( "Empty" ) );
		return true;
	}

	virtual Bool GetMiddleText( String& text ) const
	{
		CStorySceneBlockingElement* blocking = static_cast< CStorySceneBlockingElement* >( GetElement() );
		if( blocking->GetEvent() != NULL )
		{
			// Call base method
			return CTimelineItemBlocking::GetMiddleText( text );
		}
		else
		{
			text = TXT( "Empty" );
		}
		return true;
	}

	virtual wxColor GetColor() const
	{
		if( IsHighlight() )
		{
			return wxColor( 106, 132, 211 );
		}
		else
		{
			return wxColor( 76, 102, 181 );
		}
	}
};

class CTimelineItemBlend;

class CTimelineItemEvent : public ITimelineItem
{
public:
	CTimelineItemEvent( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, 
			TDynArray< CTimelineItemBlocking* >& elements )
		: m_event( event )
		, m_elementItem( elementItem )
		, m_elements( elements )
		, m_state( EVENTSTATE_NORMAL )
		, m_timeline( timeline )
	{
		ASSERT( m_event != NULL );
		ASSERT( m_elementItem != NULL );
		// assert both CStorySceneEvent and CTimelineItemBlockng reference the same CStorySceneElement
		ASSERT( event->GetSceneElement() == elementItem->GetElement() );
	}

	CTimelineItemBlend* GetBlendParent();

	CStorySceneEvent* GetEvent()
	{ return m_event; }

	const CStorySceneEvent* GetEvent() const
	{ return m_event; }

	CTimelineItemBlocking* GetElementItem()
	{ return m_elementItem;	}

	virtual Bool IsLeftResizable() const override;
	virtual Bool IsRightResizable() const override;

	virtual Bool IsMovable() const override;

	virtual Bool IsEditable() const override;

	virtual Bool IsRemovable() const override;

	virtual Bool IsCopyable() const override { return !GetEvent()->HasBlendParent(); }

	virtual void UpdatePresentation();

	virtual Float GetStart() const
	{
		// TODO: we can get this from event
		Float sceneItemStart = m_elementItem->GetStart();
		Float sceneItemDuration = m_elementItem->GetDuration();
		return sceneItemStart + m_event->GetStartPosition() * sceneItemDuration;
	}

	virtual Float SetStart( Float start, Bool deepUpdate ) override;

	virtual Float GetEnd() const
	{
		// TODO: we can get this from event
		return GetStart() + GetDuration();
	}

	virtual Float CalcGoToThisEventTime() const { return GetStart(); }

	virtual Bool IsDuration() const override;
	virtual Float GetDuration() const override;

	virtual Float SetDuration( Float duration )
	{
		return duration;
	}

	virtual Bool GetTopText( String& text ) const
	{ 
		text = m_event->GetEventName();
		return true;
	}

	virtual Bool GetTooltip( String& text ) const
	{ 
		text = String::Printf( TXT( "%s : %s" ), m_event->GetEventName().AsChar(),
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
	{ 
		return wxColor( 135, 207, 135 );
	}

	virtual wxColor	GetBorderColor() const 
	{
		if ( m_event->IsMuted() )
		{
			return wxColor( 200, 0, 0 );
		}
		else if ( m_event->HasLinkParent() )
		{
			return wxColor( 230, 230, 230 );
		}
		else
		{
			return wxColor( 0, 0, 0 );
		}
	}

	virtual void SetTrackName( const String& trackName )
	{ m_event->SetTrackName( trackName ); }

	virtual const Char* GetIconName() const
	{ return NULL; }

	virtual Gdiplus::Bitmap* GetIcon() const
	{
		if( m_bitmap == NULL && GetIconName() != NULL )
		{
			m_wxBitmap = SEdResources::GetInstance().LoadBitmap( GetIconName() );
			m_bitmap = CEdCanvas::ConvertToGDI( m_wxBitmap );
		}
		return m_bitmap;
	}

	virtual const wxBitmap* GetWxIcon() const override
	{
		if( m_bitmap == NULL && GetIconName() != NULL )
		{
			m_wxBitmap = SEdResources::GetInstance().LoadBitmap( GetIconName() );
			m_bitmap = CEdCanvas::ConvertToGDI( m_wxBitmap );
		}
		return m_bitmap? &m_wxBitmap : nullptr;
	}

	void ReloadIcon()
	{
		m_bitmap = nullptr;
	}

	virtual void SetProperty( IProperty* property, ITimelineItem* sourceItem );

	virtual void CustomDraw( CEdTimeline* canvas, const wxRect& rect ) const {}

	virtual void SetState( Int32 value )
	{
		m_state = ( EventState )value;
	}

	virtual Int32 GetState() const 
	{
		return ( Int32 )m_state;
	}

	virtual void OnDeleted() override;
	virtual void OnSelected() override;

	Bool ResetLinkParent();
	Bool SetLinkParent( const CGUID& parentId, Float timeoffset );
	Bool SetLinkParentTimeOffset( Float timeoffset );

	virtual Bool OnLinkParentReseted() { return false; }
	virtual Bool OnLinkParentSet( CTimelineItemEvent* parent, Float timeoffset ) { return false; }
	virtual Bool OnLinkParentTimeOffsetChanged( CTimelineItemEvent* parent, Float timeoffset ) { return false; }

	virtual Bool AutoLinkToThisEvent() const { return false; }

	virtual CClass* GetInterpolationEventClass() const;
	virtual CStorySceneEventInterpolation* CreateInterpolationEvent() const;

	virtual void GetMoveSet( TDynArray< ITimelineItem* >& outMoveSet ) override;
private:
	CTimelineItemBlocking*	GetBlockingItemAtTime( Float sectionTime )
	{
		for ( TDynArray< CTimelineItemBlocking* >::iterator itemIter = m_elements.Begin();
			itemIter != m_elements.End(); ++itemIter )
		{
			CTimelineItemBlocking* item = *itemIter;
			if ( sectionTime >= item->GetStart() && sectionTime < item->GetStart() + item->GetDuration() )
			{
				return item;
			}
		}
		return NULL;
	}

	void RefreshLinkedChildren( Bool deepUpdate );
	void RefreshLinkedParentTimeOffset( Float start );
	void RemoveAllLinkChildren();

protected:
	Gdiplus::Bitmap* ConvertToGrayscale( const wxBitmap& bmp ) const;

protected:
	CEdDialogTimeline*		m_timeline;
	CStorySceneEvent*		m_event;
	EventState				m_state;

	const static Float EVENT_ALPHA;

	CTimelineItemBlocking*					m_elementItem;

private:
	TDynArray< CTimelineItemBlocking* >&	m_elements;

	mutable wxBitmap		 m_wxBitmap;
	mutable Gdiplus::Bitmap* m_bitmap;
	mutable Gdiplus::Bitmap* m_bitmapLocked;
};

class CTimelineItemDefault : public CTimelineItemEvent
{
public:

	template< class T >
	CTimelineItemDefault( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements, const Char * iconName = nullptr )
		: CTimelineItemEvent( timeline, event, elementItem, elements ), m_iconName( iconName )
	{
		m_eventClass = T::GetStaticClass();
	}

	CTimelineItemDefault( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements, const Char * iconName, const CClass* evtClass )
		: CTimelineItemEvent( timeline, event, elementItem, elements ), m_iconName( iconName ), m_eventClass( evtClass )
	{
	}

	const CClass* m_eventClass;
	const Char* m_iconName;

	virtual String GetTypeName() const
	{ 
		return m_eventClass->GetName().AsString(); 
	}

	virtual Bool IsLeftResizable() const
	{ 
		return IsEditable() && IsDuration();
	}

	virtual Bool IsRightResizable() const
	{ 
		return IsEditable() && IsDuration();
	}

	virtual Bool IsDuration() const
	{ 
		return GetDuration() > 0.f || Cast< CStorySceneEventDuration >( m_event );
	}

	virtual const Char* GetIconName() const
	{
		return m_iconName;
	}

}; 


class CTimelineItemCsCamera : public CTimelineItemEvent
{
public:

	CTimelineItemCsCamera( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{}

	virtual String GetTypeName() const override
	{ 
		return CStorySceneEventCsCamera::GetStaticClass()->GetName().AsString(); 
	}

	virtual Bool   IsMovable() const override
	{ 
		return false; 
	}

	virtual Bool   IsEditable() const override
	{
		return false; 
	}

	virtual Bool IsCopyable() const override
	{
		return false;
	}

	virtual const Char* GetIconName() const override
	{
		return TXT("IMG_DIALOG_CAMERA");
	}
}; 


class CTimelineItemCameraLight : public CTimelineItemEvent
{
public:

	CTimelineItemCameraLight( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{}

	virtual String GetTypeName() const override
	{ 
		return CStorySceneEventCameraLight::GetStaticClass()->GetName().AsString(); 
	}

	virtual const Char* GetIconName() const override
	{
		return TXT("IMG_DIALOG_LIGHT");
	}

	virtual CClass* GetInterpolationEventClass() const override;
	virtual CStorySceneEventInterpolation* CreateInterpolationEvent() const override;
}; 


class CTimelineItemAttachToBone : public CTimelineItemEvent
{
public:
	CTimelineItemAttachToBone( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{}

	virtual String GetTypeName() const
	{ 
		return CStorySceneEventAttachPropToSlot::GetStaticClass()->GetName().AsString(); 
	}
	
	virtual void UpdatePresentation()
	{
		ReloadIcon(); 
		//reset icon it will be reloaded 
		CTimelineItemEvent::UpdatePresentation();
	}

	virtual const Char* GetIconName() const
	{
		return Cast<CStorySceneEventAttachPropToSlot>( m_event )->IsActivation() ? TXT("IMG_DIALOG_ATTACH") : TXT("IMG_DIALOG_DETACH");
	}
}; 


// TODO: obsolete?
class CTimelineItemCamera : public CTimelineItemEvent
{
public:
	CTimelineItemCamera( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemCamera" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_CAMERA" ); }

	virtual Bool AutoLinkToThisEvent() const { return true; }
};

class CTimelineItemGameplayCamera : public CTimelineItemEvent
{
public:
	CTimelineItemGameplayCamera( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemGameplayCamera" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_CUSTOM_CAMERA" ); }

	virtual Bool AutoLinkToThisEvent() const { return true; }

	virtual CClass* GetInterpolationEventClass() const override;
	virtual CStorySceneEventInterpolation* CreateInterpolationEvent() const override;
};

class CTimelineItemStartBlendToGameplayCamera : public CTimelineItemEvent
{
public:
	CTimelineItemStartBlendToGameplayCamera( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemStartBlendToGameplayCamera" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_START_BLEND_TO_GAMEPLAY_CAMERA" ); }

	virtual Bool AutoLinkToThisEvent() const { return true; }
};

class CTimelineItemCustomCamera : public CTimelineItemEvent
{
public:
	CTimelineItemCustomCamera( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemCustomCamera" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_CUSTOM_CAMERA" ); }

	virtual Bool AutoLinkToThisEvent() const { return true; }

	virtual CClass* GetInterpolationEventClass() const override;
	virtual CStorySceneEventInterpolation* CreateInterpolationEvent() const override;
};

class CTimelineItemCustomCameraInstance : public CTimelineItemEvent
{
	Bool m_isValid;

public:
	CTimelineItemCustomCameraInstance( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements );

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemCustomCameraInstance" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_CUSTOM_CAMERA_INSTANCE" ); }

	virtual Bool AutoLinkToThisEvent() const { return true; }

	virtual CClass* GetInterpolationEventClass() const override;
	virtual CStorySceneEventInterpolation* CreateInterpolationEvent() const override;
	virtual Bool IsValid() const override;

private:
	void EvaluateValidity();
};

class CTimelineItemAnimClip : public CTimelineItemEvent
{
public:
	CTimelineItemAnimClip( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
		UpdatePresentation();
	}

	virtual const Char* GetIconName() const { return NULL; }

	virtual void UpdatePresentation();
	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const;

	virtual Bool IsLeftResizable() const override;
	virtual Bool IsRightResizable() const override;

	virtual Float SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;
	virtual Float SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;

	const TDynArray< Float >& GetKeyPoseMarkers() const { return m_dialogKeyPoseMarkers; }
	const TDynArray< TPair< Float, Float > >& GetKeyPoseTransitionDurations() const { return m_dialogKeyPoseTransitionDurations; }
	const TDynArray< TPair< Float, Float > >& GetKeyPoseKeyDurations() const { return m_dialogKeyPoseKeyDurations; }

protected:
	TDynArray< TPair< Float, Float > >  m_dialogAnimEvents;
	TDynArray< Float >					m_dialogKeyPoseMarkers;
	TDynArray< TPair< Float, Float > >	m_dialogKeyPoseTransitionDurations;
	TDynArray< TPair< Float, Float > >	m_dialogKeyPoseKeyDurations;
};

class CTimelineItemAnimation : public CTimelineItemAnimClip
{
public:
	CTimelineItemAnimation( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements );

	virtual String GetTypeName() const { return TXT( "CTimelineItemAnimation" ); }
	virtual wxColor GetColor() const { return wxColor( 110, 156, 195, EVENT_ALPHA ); }
	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const;
	virtual Bool IsValid() const override;

private:
	void EvaluateValidity();

	Bool m_isValid;
};

class CTimelineItemCurveAnimation : public CTimelineItemEvent
{
public:
	CTimelineItemCurveAnimation( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	~CTimelineItemCurveAnimation();

	virtual String GetTypeName() const { return TXT( "CTimelineItemCurveAnimation" ); }

	virtual wxColor GetColor() const { return wxColor( 180, 180, 20 ); }

};
class CTimelineItemEnterExit : public CTimelineItemAnimClip
{
public:
	CTimelineItemEnterExit( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemAnimClip( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemEnterExit" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 217, 183, 159, EVENT_ALPHA );
	}
};

class CTimelineItemLookat : public CTimelineItemEvent
{
public:
	CTimelineItemLookat( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemLookat" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_LOOKAT" ); }
};

class CTimelineItemLookatDuration : public CTimelineItemEvent
{
public:
	CTimelineItemLookatDuration( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual Float CalcGoToThisEventTime() const { return GetEnd(); }

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemLookatDuration" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_LOOKAT" ); }

	virtual Bool IsLeftResizable() const override;
	virtual Bool IsRightResizable() const override;

	virtual Float SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;
	virtual Float SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;

	virtual Bool IsDuration() const
	{ return CTimelineItemEvent::IsDuration() && !static_cast< CStorySceneEventLookAtDuration* >( m_event )->IsInstant(); }

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const;
};

class CTimelineItemLookatGameplay : public CTimelineItemEvent
{
public:
	CTimelineItemLookatGameplay( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual Float CalcGoToThisEventTime() const { return GetEnd(); }

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemLookatGameplay" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_LOOKAT" ); }

	virtual Bool IsLeftResizable() const override;
	virtual Bool IsRightResizable() const override;

	virtual Float SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;
	virtual Float SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;

	virtual Bool IsDuration() const
	{ return CTimelineItemEvent::IsDuration() && !static_cast< CStorySceneEventGameplayLookAt* >( m_event )->IsInstant(); }
};

class CTimelineItemVisibility : public CTimelineItemEvent
{
public:
	CTimelineItemVisibility( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemDespawn" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_VISIBILITY" ); }
};

class CTimelineItemLodOverride : public CTimelineItemEvent
{
public:
	CTimelineItemLodOverride( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
	: CTimelineItemEvent( timeline, event, elementItem, elements )
	{}

	virtual ~CTimelineItemLodOverride() override
	{}

	virtual String GetTypeName() const override
	{
		return TXT( "CTimelineItemLodOverride" );
	}

	virtual const Char* GetIconName() const override
	{
		return TXT( "IMG_DIALOG_VISIBILITY" );
	}
};

class CTimelineItemAppearance : public CTimelineItemEvent
{
public:
	CTimelineItemAppearance( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemAppearance" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_EFFECT" ); }
};

class CTimelineItemUseHiresShadows : public CTimelineItemEvent
{
public:
	CTimelineItemUseHiresShadows( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemUseHiresShadows" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_EFFECT" ); }
};

class CTimelineItemMimicLod : public CTimelineItemEvent
{
public:
	CTimelineItemMimicLod( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemMimicLod" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_EFFECT" ); }
};

class CTimelineItemChangeActorGameState : public CTimelineItemEvent
{
public:
	CTimelineItemChangeActorGameState( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemChangeActorGameState" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 217, 183, 159, EVENT_ALPHA );
	}

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_CHANGE_ACTOR_STATE" ); }
};

class CTimelineItemFade : public CTimelineItemEvent
{
public:
	CTimelineItemFade( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemFade" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_CUTSCENE_FADE" ); }

	virtual Bool IsLeftResizable() const override;
	virtual Bool IsRightResizable() const override;

	virtual Float SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;
	virtual Float SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;

	virtual Float SetDuration( Float duration )
	{
		SCENE_ASSERT( IsEditable() );

		// Assert that scaling factor is 1.0f - it can't be different as it would mean
		// that section is not approved and editing is disabled.
		SCENE_ASSERT( Abs( m_timeline->GetEventInstanceScalingFactor( *GetEvent() ) - 1.0f ) <= NumericLimits< Float >::Epsilon() );

		static_cast< CStorySceneEventFade* >( GetEvent() )->SetDuration( duration );

		return duration;
	}
};

class CTimelineItemEffect : public CTimelineItemEvent
{
public:
	CTimelineItemEffect( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemEffect" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_EFFECT" ); }
};

class CTimelineItemMimics : public CTimelineItemEvent
{
public:
	CTimelineItemMimics( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemMimics" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_MIMICS" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 205, 150, 178, EVENT_ALPHA );
	}
};

class CTimelineItemMimicAnimClip : public CTimelineItemAnimClip
{
	Bool m_isValid;

public:
	CTimelineItemMimicAnimClip( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements );

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemMimicAnimClip" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 205, 150, 178, EVENT_ALPHA );
	}

	virtual Bool IsValid() const override;

private:
	void EvaluateValidity();
};

class CTimelineItemCameraAnimClip : public CTimelineItemAnimClip
{
public:
	CTimelineItemCameraAnimClip( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemAnimClip( timeline, event, elementItem, elements )
	{
	}

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const override;

	Bool IsDuration() const;

	virtual const Char* GetIconName() const;

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemCameraAnimClip" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 250, 150, 150, EVENT_ALPHA );
	}
};

class CTimelineItemMusic : public CTimelineItemEvent
{
public:
	CTimelineItemMusic( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemMusic" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ANIMATION_MUSIC" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 53, 91, 230, 150 );
	}
};

class CTimelineItemSound : public CTimelineItemEvent
{
public:
	CTimelineItemSound( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemSound" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_ANIMATION_SOUND" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 53, 91, 230, 150 );
	}

};

class CTimelineItemRotate : public CTimelineItemEvent
{
public:
	CTimelineItemRotate( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemRotate" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_ROTATE" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 255, 0, 0, EVENT_ALPHA );
	}
};

class CTimelineItemLineEvent : public CTimelineItemEvent
{
public:
	CTimelineItemLineEvent( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemLineEvent" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_ANIMATION" ); }

	virtual Bool GetTooltip( String& text ) const;

	virtual wxColor GetColor() const
	{ 
		return wxColor( 255, 0, 0, EVENT_ALPHA );
	}
};

class CTimelineItemScenePropPlacement : public CTimelineItemEvent
{
public:
	CTimelineItemScenePropPlacement( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemScenePropPlacement" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_PROP_PLACEMENT" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 128, 128, 128, EVENT_ALPHA );
	}

	virtual CClass* GetInterpolationEventClass() const override;
	virtual CStorySceneEventInterpolation* CreateInterpolationEvent() const override;
};

class CTimelineItemLightProperty : public CTimelineItemEvent
{
public:
	CTimelineItemLightProperty( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual void UpdatePresentation()
	{
		ReloadIcon(); 
		//reset icon it will be reloaded 
		CTimelineItemEvent::UpdatePresentation();
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemLightProperty" ); }

	virtual const Char* GetIconName() const;

	virtual wxColor GetColor() const
	{ 
		return wxColor( 255, 128, 0, EVENT_ALPHA );
	}

	virtual CClass* GetInterpolationEventClass() const override;
	virtual CStorySceneEventInterpolation* CreateInterpolationEvent() const override;
};


class CTimelineItemOverridePlacement : public CTimelineItemEvent
{
public:
	CTimelineItemOverridePlacement( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemOverridePlacement" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_MOVE" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 255, 0, 0, EVENT_ALPHA );
	}

	virtual CClass* GetInterpolationEventClass() const override;
	virtual CStorySceneEventInterpolation* CreateInterpolationEvent() const override;
};

class CTimelineItemOverridePlacementDuration : public CTimelineItemEvent
{
public:
	CTimelineItemOverridePlacementDuration( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemOverridePlacementDuration" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_MOVE" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 255, 0, 0, EVENT_ALPHA );
	}
};

class CTimelineItemBlend : public CTimelineItemEvent
{
private:
	Bool m_isBeingInitialized;

public:
	CTimelineItemBlend( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
		, m_isBeingInitialized( false )
	{}

	virtual Bool Initialize( TDynArray< ITimelineItem* >& items );

	// Overridden from ITimelineItem
	virtual Float SetStart( Float start, Bool deepUpdate ) override;
	virtual void OnDeleted() override;
	virtual Float GetHeightScale() const override { return 0.5f; }
	virtual void UpdatePresentation() override;
	virtual void GetChildItems( TDynArray< ITimelineItem* >& childItems ) override;

	//! To be invoked whenever child start position moves
	virtual void OnChildChanged( CTimelineItemEvent* item );
	//! Attempts to add child to blend event; returns true on success
	virtual Bool AddChild( CTimelineItemEvent* item );
	//! Removes child from blend
	virtual void RemoveChild( CTimelineItemEvent* item );
	//! Removes all children from the blend
	void RemoveAllChildren();
	//! To be invoked whenever child gets selected
	virtual void OnChildSelected( CTimelineItemEvent* item );
	//! Refreshes state of the blend after change has been made
	virtual void Refresh();

	//! Gets items sorted by absolute time
	void GetItems( TDynArray< CTimelineItemEvent* >& items );

	Uint32 GetItemsCount();

	virtual void GetMoveSet( TDynArray< ITimelineItem* >& outMoveSet ) override;

	virtual Bool IsCopyable() const override;

protected:
	void RefreshStartPositionAndDuration();
	void DoAddChild( CTimelineItemEvent* item );
	void SortKeys();

	virtual void OnChildAdded( CTimelineItemEvent* item ) {}
	virtual void OnChildRemove( CTimelineItemEvent* item ) {}
};


class CTimelineItemCurveBlend : public CTimelineItemBlend, public ICurveChangeListener, public ICurveSelectionListener
{
public:
	CTimelineItemCurveBlend( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements );
	~CTimelineItemCurveBlend();

	//! Creates key item
	virtual CTimelineItemEvent* CreateKeyItem() = 0;
	virtual void OnChildSelected( CTimelineItemEvent* item ) override;

	// Overridden from CTimelineItemBlend
	virtual Bool Initialize( TDynArray< ITimelineItem* >& items ) override;
	virtual void OnChildAdded( CTimelineItemEvent* item ) override;
	virtual void OnChildRemove( CTimelineItemEvent* item ) override;
	virtual void Refresh() override;

	// Overridden from ITimelineItemEvent
	virtual void UpdatePresentation() override;

	// Overridden from ICurveChangeListener
	virtual void OnCurveChanged( SMultiCurve* curve ) override;

	// Overridden from ICurveSelectionListener
	virtual void OnCurveSelectionChanged( CCurveEntity* curveEntity, const TDynArray< Uint32 >& selectedControlPointIndices ) override;

protected:
	void RefreshCurveFromKeys();
	void RefreshKeysFromCurve();
};

class CTimelineItemOverridePlacementBlend : public CTimelineItemCurveBlend
{
public:
	CTimelineItemOverridePlacementBlend( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemCurveBlend( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemOverridePlacementBlend" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 233, 213, 168 );
	}

	// Overridden from CTimelineItemBlend
	virtual Bool AddChild( CTimelineItemEvent* item ) override;
	virtual CTimelineItemEvent* CreateKeyItem() override;
};


class CTimelineItemChoice : public CTimelineItemBlocking
{
public:
	CTimelineItemChoice( CStorySceneChoice* choice, Float start, Float duration, CEdDialogTimeline* timeline, Uint32 level = 0 )
		: CTimelineItemBlocking( choice, start, duration, level )
		, m_timeline( timeline )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemChoice" ); }

	virtual Bool GetTopText( String& text ) const
	{
		text = !IsCutsceneChoice() ? TXT( "Choice" ) : TXT( "Cutscene Choice" );
		return true;
	}

	virtual Bool IsRightResizable() const;

	virtual wxColor GetColor() const
	{
		if( IsHighlight() )
		{
			return wxColor( 240, 170, 255 );
		}
		else
		{
			return !IsCutsceneChoice() ? wxColor( 235, 145, 255 ) : wxColor( 185, 95, 205 );
		}
	}

	virtual Bool GetTooltip( String& text ) const
	{ 
		text = !IsCutsceneChoice() ? TXT( "Choice" ) : TXT( "Cutscene Choice" );
		return true;
	}

	virtual void UpdatePresentation()
	{

	}

	virtual Float SetDuration( Float duration );

	virtual Bool IsRemovable() const override;

private:
	Bool IsCutsceneChoice() const;

protected:
	CEdDialogTimeline* m_timeline;
};

/*

This is used only for old style camera blends (CStorySceneEventCameraBlend).
*/
class CTimelineItemCameraBlend : public CTimelineItemEvent
{
public:
	CTimelineItemCameraBlend( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemCameraBlend" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_CAMERA" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 233, 213, 168 );
	}
};

class CTimelineItemEnhancedCameraBlend : public CTimelineItemCurveBlend
{
public:
	CTimelineItemEnhancedCameraBlend( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemCurveBlend( timeline, event, elementItem, elements )
	{
	}

	void GenerateCameraFragments( IViewport* view, CRenderFrame* frame );

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemEnhancedCameraBlend" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 233, 213, 168 );
	}

	// Overridden from CTimelineItemBlend
	virtual Bool AddChild( CTimelineItemEvent* item ) override;
	virtual CTimelineItemEvent* CreateKeyItem() override;
	virtual void Refresh() override;
};

/*

This is used for new style camera blends (CStorySceneCameraBlendEvent).
*/
class CTimelineItemCameraBlendEvent : public CTimelineItemBlend
{
public:
	CTimelineItemCameraBlendEvent( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemBlend( timeline, event, elementItem, elements )
	{}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemCameraBlendEvent" ); }

	virtual Bool AddChild( CTimelineItemEvent* item );

	virtual void Refresh();
	virtual void OnChildAdded( CTimelineItemEvent* item );
	virtual void OnChildRemove( CTimelineItemEvent* item );

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const;
};

class CTimelineItemPoseChange : public CTimelineItemAnimClip
{
public:
	CTimelineItemPoseChange( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemAnimClip( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemPoseChange" ); }

	virtual wxColor GetColor() const
	{ 
		return  wxColor( 141, 196, 172 );
	}

	virtual const Char* GetIconName() const;

	virtual Bool IsDuration() const;

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const;
};

class CTimelineItemMimicsTick : public CTimelineItemEvent
{
public:
	CTimelineItemMimicsTick( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemMimics" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_MIMICS" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 205, 150, 178, EVENT_ALPHA );
	}
};

class CTimelineItemMimicsDuration : public CTimelineItemAnimClip
{
public:
	CTimelineItemMimicsDuration( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemAnimClip( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemMimics" ); }

	virtual const Char* GetIconName() const;

	virtual Bool IsDuration() const;

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const;

	virtual wxColor GetColor() const
	{ 
		return wxColor( 205, 150, 178, EVENT_ALPHA );
	}
};

class CTimelineItemMimicsPose : public CTimelineItemMimicsTick
{
public:
	CTimelineItemMimicsPose( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemMimicsTick( timeline, event, elementItem, elements )
	{
	}

	virtual Bool IsDuration() const;

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const;
};

class CTimelineItemMimicsFilter : public CTimelineItemMimicsPose
{
public:
	CTimelineItemMimicsFilter( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemMimicsPose( timeline, event, elementItem, elements )
	{
	}

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const;
};

class CTimelineItemEquipItem : public CTimelineItemEvent
{
public:
	CTimelineItemEquipItem( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemEquipItem" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_EQUIP_ITEM" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 156, 42, 111 );
	}
};

class CTimelineItemMorphItem : public CTimelineItemEvent
{
public:
	CTimelineItemMorphItem( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemMorphItem" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_MORPH" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 156, 42, 111 );
	}
	CStorySceneEventInterpolation* CreateInterpolationEvent() const;
	CClass* GetInterpolationEventClass() const;
};

class CTimelineItemSurfaceEffect : public CTimelineItemEvent
{
public:
	CTimelineItemSurfaceEffect( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemSurfaceEffect" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_SURFACE_EFFECT" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 156, 42, 111 );
	}
};

class CTimelineItemWeatherChange : public CTimelineItemEvent
{
public:
	CTimelineItemWeatherChange( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemWeatherChange" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_WEATHER" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 156, 42, 111 );
	}
};

class CTimelineItemDisablePhysicsCloth : public CTimelineItemEvent
{
public:
	CTimelineItemDisablePhysicsCloth( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemDisablePhysicsCloth" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_DISABLE_CLOTH" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 156, 42, 111 );
	}

	virtual CClass* GetInterpolationEventClass() const;
	virtual CStorySceneEventInterpolation* CreateInterpolationEvent() const;
};

class CTimelineItemDanglesShake : public CTimelineItemEvent
{
public:
	CTimelineItemDanglesShake( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemDanglesShake" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_RESET_CLOTH" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 156, 42, 111 );
	}

	virtual CClass* GetInterpolationEventClass() const;
	virtual CStorySceneEventInterpolation* CreateInterpolationEvent() const;
};

class CTimelineItemDisableDangle : public CTimelineItemEvent
{
public:
	CTimelineItemDisableDangle( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemDisableDangle" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_DISABLE_CLOTH" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 156, 42, 111 );
	}

	virtual CClass* GetInterpolationEventClass() const;
	virtual CStorySceneEventInterpolation* CreateInterpolationEvent() const;
};

class CTimelineItemResetClothAndDangles : public CTimelineItemEvent
{
public:
	CTimelineItemResetClothAndDangles( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemResetClothAndDangles" ); }

	virtual const Char* GetIconName() const
	{ return TXT( "IMG_DIALOG_RESET_CLOTH" ); }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 156, 42, 111 );
	}
};

class CTimelineItemPoseKey : public CTimelineItemEvent
{
	String	m_iconName;

	Float	m_blendInEndRelPos;
	Float	m_blendOutStartRelPos;

public:
	CTimelineItemPoseKey( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
		, m_blendInEndRelPos( 0.0f )
		, m_blendOutStartRelPos( 1.0f )
	{
		m_iconName = TXT( "IMG_DIALOG_POSE_KEY" ); 
		UpdatePresentation();
	}

	virtual Float CalcGoToThisEventTime() const { return GetStart() + GetDuration()* m_blendOutStartRelPos; }

	virtual Bool IsLeftResizable() const override;
	virtual Bool IsRightResizable() const override;

	virtual Float SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;
	virtual Float SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers ) override;

	virtual Bool OnLinkParentSet( CTimelineItemEvent* parent, Float timeoffset ) override;

	virtual void UpdatePresentation();

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const;

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemPoseKey" ); }

	virtual const Char* GetIconName() const;

	virtual wxColor GetColor() const
	{ 
		return wxColor( 255, 42, 111 );
	}
};

class CTimelineItemEventGroup : public CTimelineItemEvent
{
protected:
	TDynArray< CTimelineItemEvent* > m_embeddedItems;
	TDynArray< Float >				m_embeddedItemsTimes;

public:
	CTimelineItemEventGroup( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
	}

	~CTimelineItemEventGroup()
	{
		while ( m_embeddedItems.Empty() == false )
		{
			delete m_embeddedItems.PopBack();
		}
	}

	virtual String GetTypeName() const
	{ return TXT( "CTimelineItemEventGroup" ); }

	virtual const Char* GetIconName() const
	{ return NULL; }

	virtual wxColor GetColor() const
	{ 
		return wxColor( 63, 63, 63 );
	}

	Float SetStart( Float start, Bool deepUpdate )
	{
		Float oldStart = GetStart();

		Float newStart = CTimelineItemEvent::SetStart( start, deepUpdate );

		for( TDynArray< CTimelineItemEvent* >::const_iterator itemIter = m_embeddedItems.Begin();
			itemIter != m_embeddedItems.End(); ++itemIter )
		{
			Float itemOffset = (*itemIter)->GetStart() - oldStart;
			(*itemIter)->SetStart( newStart + itemOffset, deepUpdate );
		}
		return newStart;
	}

	void UpdatePresentation()
	{
		
	}

	void AddItem( CTimelineItemEvent* item, Float timeOffset ) 
	{	
		item->SetStart( GetStart() + timeOffset, true );
		m_embeddedItems.PushBack( item ); 
		m_embeddedItemsTimes.PushBack( timeOffset );
	}

	RED_INLINE void CleartItems() { m_embeddedItems.Clear(); }
	RED_INLINE const TDynArray< CTimelineItemEvent* >& GetEmbeddedItems() const { return m_embeddedItems; }

	void DrawEmbeddedItem( CTimelineItemEvent* item, Float startTime, CEdDialogTimeline* timeline ) const;

	virtual void CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const;

	void UpdateGroupTimes() 
	{
		Float groupStartTime = GetStart();
		ASSERT( m_embeddedItems.Size() == m_embeddedItemsTimes.Size() );
		for( Uint32 i = 0; i < m_embeddedItems.Size(); ++i )
		{
			m_embeddedItems[ i ]->SetStart( groupStartTime + m_embeddedItemsTimes[ i ], true );
		}
	}
};

}
