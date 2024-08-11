
#include "build.h"

#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneEventDialogLine.h"
#include "../../common/game/storySceneEventEnhancedCameraBlend.h"
#include "../../common/game/storySceneEventCameraInterpolation.h"
#include "../../common/game/storySceneEventPlacementInterpolation.h"
#include "../../common/game/storySceneEventPropPlacementInterpolation.h"
#include "../../common/game/storySceneEventLightPropertiesInterpolation.h"
#include "../../common/game/storySceneEventCameraAnimation.h"
#include "../../common/game/storySceneEventPoseKey.h"
#include "../../common/game/storySceneEventClothDisablingInterpolation.h"
#include "../../common/game/storySceneEventDangleInterpolation.h"
#include "../../common/game/storySceneEventMorphInterpolation.h"
#include "../../common/game/storySceneEventLightProperties.h"
#include "../../common/game/storySceneEventCameraLightInterpolation.h"

#include "dialogTimeline_items.h"
#include "dialogTimeline_includes.h"
#include "voice.h"
#include "dialogEditor.h"
#include "dialogEditorUtils.h"

#include "dialogTimelineItemInterpolationEvent.h"

#include "../importer/REAAnimImporter.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/engine/skeletalAnimationContainer.h"
#include "../../common/game/gameTypeRegistry.h"
#include "../../common/engine/renderFrame.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

namespace DialogTimelineItems
{
	const Float CTimelineItemEvent::EVENT_ALPHA = 150.f;

	CTimelineItemLineElement::CTimelineItemLineElement( CEdDialogTimeline* timeline, CStorySceneLine* line, Float start, Float duration, Uint32 level /*= 0 */ ) : CTimelineItemBlocking( line, start, duration, level )
		, m_image( NULL )
		, m_backgroundLineIcon( NULL )
		, m_voiceStartRelPos(0.0f)
		, m_voiceEndRelPos(1.0f)
		, m_voiceMaxAmpRelPos(0.0f)
		, m_color( 126, 209, 124 )
		, m_timeOffsetBack( 0.f )
		, m_timeOffsetFront( 0.f )
		, m_dialogTimeline( timeline )
		, m_isVOInvalid_BlendIn( false )
		, m_isVOInvalid_WavDuration( false )
	{
		ReloadImage();

		CStorySceneLine* lineElement = Cast< CStorySceneLine >( GetElement() ); // Why don't use line?
		if( lineElement )
		{
			timeline->GetVoiceDataPositions( lineElement, &m_voiceMaxAmpRelPos, &m_voiceStartRelPos, &m_voiceEndRelPos );
		}

		SCENE_ASSERT( line == lineElement ); // Why don't use line?

		m_wxBackgroundLineIcon = SEdResources::GetInstance().LoadBitmap( TXT( "IMG_ARROW_DOWN" ) );
		m_backgroundLineIcon = CEdCanvas::ConvertToGDI( m_wxBackgroundLineIcon );

		if ( timeline->UseLocColors() && line && line->GetLocalizedContent() )
		{
			m_color = timeline->FindLocColor( line->GetLocalizedContent()->GetIndex() );
		}

		if ( const StorySceneLineInstanceData* instanceData = static_cast< const StorySceneLineInstanceData* >( timeline->FindElementInstance( line ) ) )
		{
			instanceData->GetTimeOffsets( m_timeOffsetFront, m_timeOffsetBack );
		}
	}

	Gdiplus::Bitmap* DrawImageFromData( TDynArray< Float >& data )
	{
		const Uint32 width = data.Size();
		const Uint32 height = 24;
		const Uint32 height2 = height / 2;

		Gdiplus::Bitmap* image = new Gdiplus::Bitmap( width, height, PixelFormat32bppARGB );

		Float signalMax = 0.f;
		for ( Uint32 i=0; i<width; ++i )
		{
			if ( data[ i ] > signalMax )
			{
				signalMax = data[ i ];
			}
		}

		for ( Uint32 i=0; i<width; ++i )
		{
			const Uint32 max = (Uint32)( height * Clamp( data[ i ] / signalMax, 0.f, 1.f ));

			for ( Uint32 j=max; j<height-1; ++j )
			{
				Gdiplus::Color color( 0, 0, 0, 0 );

				const Uint32 jj1 = j / 2 + height2;
				const Uint32 jj2 = height2 - j / 2;

				image->SetPixel( i, jj1, color );
				image->SetPixel( i, jj2, color );
			}

			for ( Uint32 j=0; j<max; ++j )
			{
				Gdiplus::Color color( 128, 0, 0, 0 );

				const Uint32 jj1 = j / 2 + height2;
				const Uint32 jj2 = height2 - j / 2;

				image->SetPixel( i, jj1, color );
				image->SetPixel( i, jj2, color );
			}
		}

		return image;
	}

	/*

	\return Bitmap. A mask is associated with this bitmap. Caller acquires ownership of the bitmap.
	*/
	wxBitmap* DrawWxImageFromData( TDynArray< Float >& data)
	{
		const Uint32 width = data.Size();
		const Uint32 height = 24;
		const Uint32 height2 = height / 2;

		wxPen penBg( wxColour( 0, 0, 0 ) );
		wxPen penFg( wxColour( 140, 140, 160 ) );
		wxPen maskPenBg( *wxBLACK );
		wxPen maskPenFg( *wxWHITE );

		wxBitmap* bm = new wxBitmap( width, height );
		wxMemoryDC memDC( *bm );

		wxBitmap maskBm( width, height, 1 );
		wxMemoryDC maskMemDC( maskBm );

		Float signalMax = 0.f;
		for ( Uint32 i=0; i<width; ++i )
		{
			if ( data[ i ] > signalMax )
			{
				signalMax = data[ i ];
			}
		}

		for ( Uint32 i=0; i<width; ++i )
		{
			const Uint32 max = (Uint32)( height * Clamp( data[ i ] / signalMax, 0.f, 1.f ));

			memDC.SetPen( penBg );
			maskMemDC.SetPen(maskPenBg);

			for ( Uint32 j=max; j<height-1; ++j )
			{
				const Uint32 jj1 = j / 2 + height2;
				const Uint32 jj2 = height2 - j / 2;

				memDC.DrawPoint( i, jj1 );
				memDC.DrawPoint( i, jj2 );

				maskMemDC.DrawPoint( i, jj1 );
				maskMemDC.DrawPoint( i, jj2 );
			}

			memDC.SetPen( penFg );
			maskMemDC.SetPen(maskPenFg);

			for ( Uint32 j=0; j<max; ++j )
			{
				const Uint32 jj1 = j / 2 + height2;
				const Uint32 jj2 = height2 - j / 2;

				memDC.DrawPoint( i, jj1 );
				memDC.DrawPoint( i, jj2 );

				maskMemDC.DrawPoint( i, jj1 );
				maskMemDC.DrawPoint( i, jj2 );
			}
		}

		maskMemDC.SelectObject(wxNullBitmap);
		wxMask* mask = new wxMask(maskBm);
		bm->SetMask(mask);

		return bm;
	}

	void CTimelineItemLineElement::ReloadImage()
	{
		m_isVOInvalid_BlendIn = false;
		m_isVOInvalid_WavDuration = false;

		if ( m_image )
		{
			delete m_image;
			m_image = NULL;

			delete m_wxImage;
			m_wxImage = nullptr;
		}

		CStorySceneLine* line = Cast< CStorySceneLine >( GetElement() );

		static Int32 curveIndex = 146;
		static Int32 curveVOContolIndex = 147;

		String path = SEdLipsyncCreator::GetInstance().GetLipsPath( line->GetVoiceFileName(), SLocalizationManager::GetInstance().GetCurrentLocale().ToLower() );
		if ( GFileManager->FileExist( path ) )
		{
			static Bool DO_WAV = true;
			if ( DO_WAV )
			{
				TDynArray< Float > data;
				if ( CREAAnimImporter::ImportSingleCurve( path, curveIndex, data ) )
				{
					m_image = DrawImageFromData( data );
					m_wxImage = DrawWxImageFromData( data );
				}

				TDynArray< Float > dataVO;
				CREAAnimImporter::ImportSingleCurve( path, curveVOContolIndex, dataVO );
				if ( dataVO.Size() > 0 && dataVO[ 0 ] > 0.1f )
				{
					m_isVOInvalid_BlendIn = true;
				}

				if ( data.Size() == dataVO.Size() )
				{
					const Float lineDuration = GetDuration();
					const Float lipsyncDuration = dataVO.Size() * ( 1.f / 30.f );
					if ( MAbs( lineDuration - lipsyncDuration ) > 0.1f )
					{
						//m_isVOInvalid_WavDuration = true;
					}
				}
				else
				{
					SCENE_ASSERT( 0 );
				}
			}

			static Bool DO_ANA = false;
			if ( DO_ANA )
			{
				const String pathWav = SEdLipsyncCreator::GetInstance().GetWavPath( line->GetVoiceFileName(), SLocalizationManager::GetInstance().GetCurrentLocale().ToLower() );

				TDynArray< Float > data;
				if ( SEdLipsyncCreator::GetInstance().CreateWavAna( pathWav, data ) )
				{
					m_image = DrawImageFromData( data );
					m_wxImage = DrawWxImageFromData( data );
				}
			}
		}
	}

	void CTimelineItemLineElement::CustomDraw( CEdTimeline* canvas, const wxRect& rect ) const
	{
		TimelineImpl::CDrawGroupTracks* drwGrp = nullptr;

		const Float duration = GetDuration() > 0.f ? GetDuration() : 1.f;

		if ( m_image )
		{
			// get draw group appropriate for drawing this item
			Track* track = canvas->GetItemTrack( this );
			drwGrp = canvas->GetTrackDrawGroup( track );

			wxRect imgRect( rect );
			if ( m_timeOffsetFront > 0.f || m_timeOffsetBack > 0.f )
			{
				const Float x1	= rect.GetX();
				const Float y1 = rect.GetY();

				const Float pF = m_timeOffsetFront / duration;
				SCENE_ASSERT( pF >= 0.f && pF <= 1.f );
				const Float pB = m_timeOffsetBack / duration;
				SCENE_ASSERT( pB >= 0.f && pB <= 1.f );

				imgRect.SetX( x1 + pF*rect.GetWidth() );
				imgRect.SetWidth( (1.f-pF-pB)*rect.GetWidth() );
			}

			if( drwGrp->GetDrawBuffer()->GetType() == CanvasType::gdi)
			{
				drwGrp->GetDrawBuffer()->DrawImageWithMask( m_wxImage, imgRect );
			}
			else
			{
				drwGrp->GetDrawBuffer()->DrawImage( m_image, imgRect );
			}

			Float y0 = rect.y+2;
			Float y1 = rect.y + rect.height - 2;
			//Float x = rect.x + m_voiceStartRelPos * rect.width;
			//drwGrp->GetDrawBuffer()->DrawLine(x, y0, x, y1, wxColour(0, 0, 255), 2.f);
			//x = rect.x + m_voiceEndRelPos * rect.width;
			//drwGrp->GetDrawBuffer()->DrawLine(x, y0, x, y1, wxColour(0, 0, 255), 2.f);
			
			for ( Uint32 i = 0; i < m_voiceMaxAmpRelPos.Size(); ++i )
			{				
				Float scale = Clamp<Float>( m_voiceMaxAmpRelPos[i].m_second * 9.f , 0.7f, 1.f );
				Float y1scaled = rect.y + scale*rect.height - 2;
				Float xoffset  = 2 + (7 * scale);

				Float x	= rect.x + m_voiceMaxAmpRelPos[i].m_first * rect.width;
				drwGrp->GetDrawBuffer()->FillTriangle( x, y1scaled, x+xoffset, y0, x-xoffset, y0, wxColour( 209, 124, 143, 150 ) );
			}
			
			//drwGrp->GetDrawBuffer()->DrawLine( x, y0, x, y1, wxColour(255, 0, 0)  );
		}

		if ( m_timeOffsetFront > 0.f )
		{
			if ( !drwGrp )
			{
				Track* track = canvas->GetItemTrack( this );
				drwGrp = canvas->GetTrackDrawGroup( track );
			}

			const wxColor fillColor( 0, 0, 0, 128 );
			const Float p = m_timeOffsetFront / duration;
			SCENE_ASSERT( p >= 0.f && p <= 1.f );
			const Float x1	= rect.GetX();
			const Float y1 = rect.GetY();
			drwGrp->GetDrawBuffer()->FillRect( x1, y1, p*rect.GetWidth(), rect.GetHeight(), fillColor );
		}
		if ( m_timeOffsetBack > 0.f )
		{
			if ( !drwGrp )
			{
				Track* track = canvas->GetItemTrack( this );
				drwGrp = canvas->GetTrackDrawGroup( track );
			}

			const wxColor fillColor( 0, 0, 0, 128 );
			const Float p = m_timeOffsetBack / duration;
			SCENE_ASSERT( p >= 0.f && p <= 1.f );
			const Float x1	= rect.GetX() + (1.f - p) * rect.GetWidth();
			const Float y1 = rect.GetY();
			drwGrp->GetDrawBuffer()->FillRect( x1, y1, p*rect.GetWidth(), rect.GetHeight(), fillColor );
		}
	}

	Bool CTimelineItemLineElement::IsBackgroundLine() const
	{
		CStorySceneLine* line = Cast< CStorySceneLine >( GetElement() );
		return line->IsBackgroundLine();
	}

	Bool CTimelineItemLineElement::IsDuration() const
	{
		if( !IsBackgroundLine() )
		{
			// if background flag is not set then this behaves like a duration item
			return true;
		}
		else
		{
			// if background flag is set then this behaves like a point item
			return false;
		}
	}

	Float CTimelineItemLineElement::GetDuration() const
	{
		if( !IsBackgroundLine() )
		{
			// if background flag is not set then this behaves like a duration item
			return m_duration;
		}
		else
		{
			// if background flag is set then this behaves like a point item
			return 0.0f;
		}
	}

	Gdiplus::Bitmap* CTimelineItemLineElement::GetIcon() const
	{
		if( !IsBackgroundLine() )
		{
			return nullptr;
		}
		else
		{
			return m_backgroundLineIcon;
		}
	}

	const wxBitmap* CTimelineItemLineElement::GetWxIcon() const
	{
		if( !IsBackgroundLine() )
		{
			return nullptr;
		}
		else
		{
			return &m_wxBackgroundLineIcon;
		}
	}

	CTimelineItemBlend* CTimelineItemEvent::GetBlendParent()
	{
		if ( GetEvent()->HasBlendParent() )
		{
			return static_cast< CTimelineItemBlend* >( m_timeline->FindItemEvent( GetEvent()->GetBlendParentGUID() ) );
		}
		return NULL;
	}

	void CTimelineItemEvent::SetProperty( IProperty* property, ITimelineItem* sourceItem )
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

	Gdiplus::Bitmap* CTimelineItemEvent::ConvertToGrayscale( const wxBitmap& bmp ) const
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

	Bool CTimelineItemEvent::IsRemovable() const
	{
		return IsEditable();
	}

	Bool CTimelineItemEvent::IsLeftResizable() const
	{
		return false;
	}

	Bool CTimelineItemEvent::IsRightResizable() const
	{
		return false;
	}

	Bool CTimelineItemEvent::IsMovable() const
	{
		return IsEditable();
	}

	Bool CTimelineItemEvent::IsEditable() const
	{
		return m_timeline->IsTimelineEditingEnabled();
	}

	Float CTimelineItemEvent::SetStart( Float startTime, Bool deepUpdate )
	{
		CTimelineItemBlocking* newElementItem = GetBlockingItemAtTime( startTime );
		if ( newElementItem != NULL && newElementItem != m_elementItem /*&& wxIsShiftDown() == false */)
		{
			m_elementItem->SetHighlight( false, 0.0f );
			m_elementItem = newElementItem;
			m_event->SetSceneElement( newElementItem->GetElement() );

			//m_timeline->OnItemParentElementChanged( this );
		}

		// Calculate percentage event start
		Float position = ( startTime - m_elementItem->GetStart() ) / m_elementItem->GetDuration();
		m_event->SetStartPosition( Clamp( position, 0.0f, 1.0f ) );
		m_timeline->SetEventInstanceStartTime( *GetEvent(), startTime );

		// Update highlight
		m_elementItem->SetHighlight( true, m_event->GetStartPosition() );

		if( m_event->IsInterpolationEventKey() )
		{
			CTimelineItemInterpolationEvent* tiInterpolationEvent = static_cast< CTimelineItemInterpolationEvent* >( m_timeline->FindItemEvent( m_event->GetInterpolationEventGUID() ) );
			CStorySceneEventInterpolation* scInterpolationEvent = static_cast< CStorySceneEventInterpolation* >( tiInterpolationEvent->GetEvent() );

			// Notify interpolation event that key has changed.
			scInterpolationEvent->OnKeyChanged( m_event->GetGUID() );
			tiInterpolationEvent->Reinitialize();

			// Update interpolation event instance.
			const CStorySceneSection* section = scInterpolationEvent->GetSceneElement()->GetSection();
			const CStorySceneEvent* firstKey = section->GetEvent( scInterpolationEvent->GetKeyGuid( 0 ) );
			const CStorySceneEvent* lastKey = section->GetEvent( scInterpolationEvent->GetKeyGuid( scInterpolationEvent->GetNumKeys() - 1 ) );
			const Float firstKeyStartTime = m_timeline->GetEventInstanceStartTime( *firstKey );
			const Float lastKeyStartTime = m_timeline->GetEventInstanceStartTime( *lastKey );
			m_timeline->SetEventInstanceStartTime( *scInterpolationEvent, firstKeyStartTime );
			m_timeline->SetEventInstanceDuration( *scInterpolationEvent, lastKeyStartTime - firstKeyStartTime );
		}

		if ( CTimelineItemBlend* blendParent = GetBlendParent() )
		{
			blendParent->OnChildChanged( this );
		}

		if ( m_event->HasLinkParent() )
		{
			RefreshLinkedParentTimeOffset( startTime );
		}

		if ( m_event->HasLinkChildren() )
		{
			RefreshLinkedChildren( deepUpdate );
		}

		return GetStart();
	}

	void CTimelineItemEvent::GetMoveSet( TDynArray< ITimelineItem* >& outMoveSet )
	{
		outMoveSet.PushBackUnique( this );

		const TDynArray< CGUID >& children = m_event->GetLinkChildrenGUID();
		for( auto it = children.Begin(), end = children.End(); it != end; ++it )
		{
			CTimelineItemEvent* ch = m_timeline->FindItemEvent( *it );
			ch->GetMoveSet( outMoveSet );
		}
	}

	void CTimelineItemEvent::UpdatePresentation()
	{
		// Clamp original value, because user could enter something stupid
		m_event->SetStartPosition( Clamp( m_event->GetStartPosition(), 0.0f, 1.0f ) );
	
		if ( CTimelineItemBlend* blendParent = GetBlendParent() )
		{
			blendParent->OnChildChanged( this );
		}

		if ( m_event->HasLinkChildren() )
		{
			RefreshLinkedChildren( true );
		}
	}

	Bool CTimelineItemEvent::IsDuration() const
	{
		return m_timeline->GetEventInstanceDuration( *GetEvent() ) > 0.f;
	}

	/*
	Returns duration of event.

	\return Duration of event. Scaled! TODO: improve docs.
	*/
	Float CTimelineItemEvent::GetDuration() const
	{ 
		return m_timeline->GetEventInstanceDuration( *GetEvent() );
	}

	void CTimelineItemEvent::OnDeleted()
	{
		if( GetEvent()->IsInterpolationEventKey() )
		{
			CTimelineItemInterpolationEvent* tiInterpolationEvent = static_cast< CTimelineItemInterpolationEvent* >( m_timeline->FindItemEvent( GetEvent()->GetInterpolationEventGUID() ) );
			CStorySceneEventInterpolation* scInterpolationEvent = tiInterpolationEvent->GetSceneInterpolationEvent();

			// We should not try to detach key from interpolation event if it has two keys only.
			ASSERT( tiInterpolationEvent->GetNumKeys() > 2 );

			scInterpolationEvent->DetachKey( GetEvent() );
			tiInterpolationEvent->Reinitialize();
		}

		if ( CTimelineItemBlend* blendParent = GetBlendParent() )
		{
			blendParent->RemoveChild( this );
		}

		if ( m_event->HasLinkParent() )
		{
			CTimelineItemEvent* pEvt = m_timeline->FindItemEvent( m_event->GetLinkParentGUID() );
			SCENE_ASSERT( pEvt );

			pEvt->GetEvent()->RemoveLinkChildGUID( m_event->GetGUID() );
			ResetLinkParent();
		}

		if ( m_event->HasLinkChildren() )
		{
			RemoveAllLinkChildren();
		}
	}

	void CTimelineItemEvent::OnSelected()
	{
		if ( CTimelineItemBlend* blendParent = GetBlendParent() )
		{
			blendParent->OnChildSelected( this );
		}
	}

	Bool CTimelineItemEvent::ResetLinkParent()
	{
		m_event->ResetLinkParent();

		return OnLinkParentReseted();
	}

	Bool CTimelineItemEvent::SetLinkParent( const CGUID& parentId, Float timeoffset )
	{
		m_event->SetLinkParent( parentId, timeoffset );

		CTimelineItemEvent* parent = m_timeline->FindItemEvent( m_event->GetLinkParentGUID() );
		return OnLinkParentSet( parent, timeoffset );
	}

	Bool CTimelineItemEvent::SetLinkParentTimeOffset( Float timeoffset )
	{
		m_event->SetLinkParentTimeOffset( timeoffset );

		CTimelineItemEvent* parent = m_timeline->FindItemEvent( m_event->GetLinkParentGUID() );
		return OnLinkParentTimeOffsetChanged( parent, timeoffset );
	}

	void CTimelineItemEvent::RefreshLinkedChildren( Bool deepUpdate )
	{
		TDynArray< CGUID > toRemove;

		const TDynArray< CGUID >& linkedChildren = m_event->GetLinkChildrenGUID();
		const Uint32 num = linkedChildren.Size();
		for ( Uint32 i=0; i<num; ++i )
		{
			CTimelineItemEvent* chItem = m_timeline->FindItemEvent( linkedChildren[ i ] );
			//SCENE_ASSERT( chItem );

			if ( chItem )
			{
				//SCENE_ASSERT( chItem->GetEvent()->GetLinkParentGUID() == m_event->GetGUID() );

				if( deepUpdate )
				{
					// Move link child to keep the offset.
					const Float newTimeForChild = GetStart() + chItem->GetEvent()->GetLinkParentTimeOffset();
					chItem->SetStart( newTimeForChild, true );
				}
				else
				{
					// Update offset, don't move child.
					chItem->SetLinkParentTimeOffset( chItem->GetStart() - GetStart() );
				}
			}
			else
			{
				toRemove.PushBack( linkedChildren[ i ] );
			}
		}

		if ( toRemove.Size() > 0 )
		{
			for ( Uint32 i=0; i<toRemove.Size(); ++i )
			{
				m_event->RemoveLinkChildGUID( toRemove[ i ] );
			}
		}
	}

	void CTimelineItemEvent::RemoveAllLinkChildren()
	{
		const TDynArray< CGUID >& linkedChildren = m_event->GetLinkChildrenGUID();		
		for ( Int32 i = linkedChildren.SizeInt() - 1; i >= 0; --i )
		{
			CTimelineItemEvent* chItem = m_timeline->FindItemEvent( linkedChildren[ i ] );
			SCENE_ASSERT( chItem );

			if ( chItem )
			{
				SCENE_ASSERT( chItem->GetEvent()->GetLinkParentGUID() == m_event->GetGUID() );

				chItem->ResetLinkParent();
				m_event->RemoveLinkChildGUID( chItem->GetEvent()->GetGUID() );
			}
		}

		SCENE_ASSERT( !m_event->HasLinkChildren() );
		m_event->RemoveAllLinkChildren();

		RefreshLinkedChildren( true );
	}

	void CTimelineItemEvent::RefreshLinkedParentTimeOffset( Float start )
	{
		CTimelineItemEvent* pEvt = m_timeline->FindItemEvent( m_event->GetLinkParentGUID() );
		SCENE_ASSERT( pEvt );

		if ( pEvt )
		{
			const Float timeOffset = start - pEvt->GetStart();
			SetLinkParentTimeOffset( timeOffset );
		}
		else
		{
			ResetLinkParent();
		}
	}

	/*
	If it returns nullptr then this item type can't be interpolated.
	*/
	CClass* CTimelineItemEvent::GetInterpolationEventClass() const
	{
		return nullptr;
	}

	/*
	If it returns nullptr then this item type can't be interpolated.
	*/
	CStorySceneEventInterpolation* CTimelineItemEvent::CreateInterpolationEvent() const
	{
		return nullptr;
	}

	Bool CTimelineItemAnimClip::IsLeftResizable() const
	{
		return IsEditable();
	}

	Bool CTimelineItemAnimClip::IsRightResizable() const
	{
		return IsEditable();
	}

	void CTimelineItemAnimClip::UpdatePresentation()
	{
		CTimelineItemEvent::UpdatePresentation();

		if ( m_timeline->GetEventInstanceDuration( *GetEvent() ) > 0 )
		{
			CStorySceneEventAnimClip* animClipEvent = Cast< CStorySceneEventAnimClip >( m_event );
			
			const Float animationDuration = GetDuration();
			const Float clipStartTime = animClipEvent->GetAnimationClipStart();
			const Float clipEndTime = Clamp( animClipEvent->GetAnimationClipEnd(), clipStartTime, animClipEvent->GetCachedAnimationDuration() );

			TDynArray< TPair< Float, Float > > absoultePositions;
			TDynArray< Float > absPosExprPoints;
			TDynArray< TPair< Float, Float > > absPosExprDurationsTransitions;
			TDynArray< TPair< Float, Float > > absPosExprDurationsPoses;
			CStorySceneEventAnimation* bodyClipEvent = Cast< CStorySceneEventAnimation >( m_event );
			if ( bodyClipEvent != NULL && m_timeline )
			{
				m_timeline->GetDisableDialogLookatEventsPositions( bodyClipEvent->GetActor(), bodyClipEvent->GetAnimationName() ,absoultePositions, false );	
				m_timeline->GetKeyPoseMarkersEventsPositions( bodyClipEvent->GetActor(), bodyClipEvent->GetAnimationName() ,absPosExprPoints, false );	
				m_timeline->GetKeyPoseDurationsEventsPositions( bodyClipEvent->GetActor(), bodyClipEvent->GetAnimationName() ,absPosExprDurationsTransitions, absPosExprDurationsPoses, false );	
			}
			CStorySceneEventMimicsAnim* mimicClipEvent = Cast< CStorySceneEventMimicsAnim >( m_event );
			if ( mimicClipEvent != NULL && m_timeline )
			{
				m_timeline->GetDisableDialogLookatEventsPositions( mimicClipEvent->GetActor(), mimicClipEvent->GetAnimationName() ,absoultePositions, true );	
				m_timeline->GetKeyPoseMarkersEventsPositions( mimicClipEvent->GetActor(), mimicClipEvent->GetAnimationName() ,absPosExprPoints, true );	
				m_timeline->GetKeyPoseDurationsEventsPositions( mimicClipEvent->GetActor(), mimicClipEvent->GetAnimationName() ,absPosExprDurationsTransitions, absPosExprDurationsPoses, true );	
			}

			{
				m_dialogAnimEvents.ClearFast();

				for ( Uint32 i = 0; i < absoultePositions.Size(); ++i )
				{
					if( absoultePositions[i].m_second > clipStartTime &&  absoultePositions[i].m_first < clipEndTime )
					{					
						Float startClipped	  = absoultePositions[i].m_first - clipStartTime;
						Float endClipped	  = absoultePositions[i].m_second;
						Float startRel = startClipped/animationDuration * animClipEvent->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *GetEvent() );
						Float endRel =  1.f - ( clipEndTime - endClipped ) /animationDuration * animClipEvent->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *GetEvent() );
						m_dialogAnimEvents.PushBack( TPair<Float, Float>( startRel, endRel ) );
					}				
				}
			}
			{
				m_dialogKeyPoseMarkers.ClearFast();

				for ( Uint32 i = 0; i < absPosExprPoints.Size(); ++i )
				{
					if( absPosExprPoints[i] > clipStartTime &&  absPosExprPoints[i] < clipEndTime )
					{					
						Float startClipped = absPosExprPoints[i] - clipStartTime;
						Float startRel = startClipped / animationDuration * animClipEvent->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *GetEvent() );
						m_dialogKeyPoseMarkers.PushBack(  startRel );
					}				
				}
			}
			{
				m_dialogKeyPoseTransitionDurations.ClearFast();

				for ( Uint32 i = 0; i < absPosExprDurationsTransitions.Size(); ++i )
				{
					const Float timeA = absPosExprDurationsTransitions[ i ].m_first;
					const Float timeB = absPosExprDurationsTransitions[ i ].m_second;

					if ( timeA > clipStartTime && timeA < clipEndTime && timeB > clipStartTime && timeB < clipEndTime )
					{					
						const Float startClippedA = timeA - clipStartTime;
						const Float startRelA = startClippedA/animationDuration * animClipEvent->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *GetEvent() );
						const Float startClippedB = timeB - clipStartTime;
						const Float startRelB = startClippedB/animationDuration * animClipEvent->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *GetEvent() );

						m_dialogKeyPoseTransitionDurations.PushBack( TPair< Float, Float >( startRelA, startRelB ) );
					}				
				}
			}
			{
				m_dialogKeyPoseKeyDurations.ClearFast();

				for ( Uint32 i = 0; i < absPosExprDurationsPoses.Size(); ++i )
				{
					const Float timeA = absPosExprDurationsPoses[ i ].m_first;
					const Float timeB = absPosExprDurationsPoses[ i ].m_second;

					if ( timeA > clipStartTime && timeA < clipEndTime && timeB > clipStartTime && timeB < clipEndTime )
					{					
						const Float startClippedA = timeA - clipStartTime;
						const Float startRelA = startClippedA/animationDuration * animClipEvent->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *GetEvent() );
						const Float startClippedB = timeB - clipStartTime;
						const Float startRelB = startClippedB/animationDuration * animClipEvent->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *GetEvent() );

						m_dialogKeyPoseKeyDurations.PushBack( TPair< Float, Float >( startRelA, startRelB ) );
					}				
				}
			}
		}
	}

	void CTimelineItemAnimClip::CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{
		CStorySceneEventAnimClip* animClipEvent = Cast< CStorySceneEventAnimClip >( m_event );

		// get draw group appropriate for drawing this item
		Track* track = timeline->GetItemTrack( this );
		TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

		for( Uint32 i = 0; i < m_dialogAnimEvents.Size(); ++i )
		{
			Float startTime = Clamp<Float>( m_dialogAnimEvents[i].m_first,  0.f, 1.f );
			Float endTime   = Clamp<Float>( m_dialogAnimEvents[i].m_second - startTime , 0.f , 1.f - startTime );
			wxRect block( rect.GetX() + startTime * rect.GetWidth(), rect.GetY() + 0.35f*rect.GetHeight(), rect.GetWidth()* endTime, 0.65f*rect.GetHeight() );

			CStorySceneEventAnimClip* animClipEvent = Cast< CStorySceneEventAnimClip >( m_event );
			wxColour fillColor = animClipEvent && animClipEvent->GetAllowedLookatLevel() != LL_Body  ?  wxColour( 227, 70, 70 ) : wxColour( 187, 183, 111 );
			drwGrp->GetDrawBuffer()->FillRect( block, fillColor );
		}

		for( Uint32 i = 0; i < m_dialogKeyPoseTransitionDurations.Size(); ++i )
		{
			const Float startTime = Clamp<Float>( m_dialogKeyPoseTransitionDurations[i].m_first, 0.f, 1.f );
			const Float endTime = Clamp<Float>( m_dialogKeyPoseTransitionDurations[i].m_second, 0.f, 1.f );

			const Float x1 = rect.GetX() + startTime * rect.GetWidth();
			const Float x2 = rect.GetX() + endTime * rect.GetWidth();
			const Float y1 = rect.GetY()+1;
			const Float y2 = y1 + rect.GetHeight()-2;

			wxColor fillColor = GetColor();
			Uint8 cr = fillColor.Red();
			Uint8 cg = fillColor.Green();
			Uint8 cb = fillColor.Blue();
			fillColor = wxColor( (Uint8)(cr*0.9f), (Uint8)(cg*0.9f), (Uint8)(cb*0.9f));

			drwGrp->GetDrawBuffer()->FillRect( x1, y1, x2-x1, y2-y1, fillColor );
		}

		for( Uint32 i = 0; i < m_dialogKeyPoseKeyDurations.Size(); ++i )
		{
			const Float startTime = Clamp<Float>( m_dialogKeyPoseKeyDurations[i].m_first, 0.f, 1.f );
			const Float endTime = Clamp<Float>( m_dialogKeyPoseKeyDurations[i].m_second, 0.f, 1.f );

			const Float x1 = rect.GetX() + startTime * rect.GetWidth();
			const Float x2 = rect.GetX() + endTime * rect.GetWidth();
			const Float y1 = rect.GetY()+1;
			const Float y2 = y1 + rect.GetHeight()-2;

			wxColor fillColor = GetColor();
			Uint8 cr = fillColor.Red();
			Uint8 cg = fillColor.Green();
			Uint8 cb = fillColor.Blue();
			fillColor = wxColor( (Uint8)(cr*0.8f), (Uint8)(cg*0.8f), (Uint8)(cb*0.8f));

			drwGrp->GetDrawBuffer()->FillRect( x1, y1, x2-x1, y2-y1, fillColor );
		}

		for( Uint32 i = 0; i < m_dialogKeyPoseMarkers.Size(); ++i )
		{
			Float startTime = Clamp<Float>( m_dialogKeyPoseMarkers[i],  0.f, 1.f );

			Float x		= rect.GetX() + startTime * rect.GetWidth();
			Float y1	= rect.GetY()+1;
			Float y2	= y1 + rect.GetHeight()-2;
			
			wxColor fillColor = GetColor();
			Uint8 cr = fillColor.Red();
			Uint8 cg = fillColor.Green();
			Uint8 cb = fillColor.Blue();
			fillColor = wxColor( (Uint8)(cr*0.75f), (Uint8)(cg*0.75f), (Uint8)(cb*0.75f));

			drwGrp->GetDrawBuffer()->FillTriangle( x, y1, x+4, y2, x-4, y2, fillColor );
		}

		const Float instanceAnimationStretch = animClipEvent->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *animClipEvent );

		// draw blend in line
		const Float blendInEndRelPos = Clamp< Float >( animClipEvent->GetAnimationBlendIn() * instanceAnimationStretch / GetDuration(), 0.0f, 1.0f );
		const Int32 blendInEndPos = rect.GetX() + rect.GetWidth() * blendInEndRelPos;
		if ( blendInEndPos > 0 )
		{
			drwGrp->GetDrawBuffer()->DrawLine( rect.GetLeft(), rect.GetBottom(), blendInEndPos, rect.GetTop(), wxColor( 0, 0, 0 ) );
		}

		// draw blend out line
		const Float blendOutStartRelPos = Clamp< Float >( ( GetDuration() - animClipEvent->GetAnimationBlendOut() * instanceAnimationStretch ) / GetDuration(), 0.0f, 1.0f );
		const Int32 blendOutStartPos = rect.GetX() + rect.GetWidth() * blendOutStartRelPos;
		if ( blendOutStartPos < rect.GetRight() )
		{
			drwGrp->GetDrawBuffer()->DrawLine( blendOutStartPos, rect.GetTop(), rect.GetRight(), rect.GetBottom(), wxColor( 0, 0, 0 ) );
		}

		// draw stretch factor
		const Uint32 stretchFactor = instanceAnimationStretch * 100.0f;
		if ( stretchFactor != 100 )
		{
			if(drwGrp->GetDrawBuffer()->GetType() == CanvasType::gdi)
			{
				drwGrp->GetDrawBuffer()->DrawText( wxPoint( ( rect.GetLeft() + rect.GetRight() ) / 2, rect.GetBottom() ), timeline->GetWxBoldFont(), String::Printf( TXT( "%d%%" ), stretchFactor ), wxColour( 0, 0 ,0), CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Center );
			}
			else
			{
				drwGrp->GetDrawBuffer()->DrawText( wxPoint( ( rect.GetLeft() + rect.GetRight() ) / 2, rect.GetBottom() ), timeline->GetGdiBoldFont(), String::Printf( TXT( "%d%%" ), stretchFactor ), wxColour( 0, 0 ,0), CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Center );
			}
		}

		if ( animClipEvent->GetAnimationClipStart() > NumericLimits< Float >::Epsilon() )
		{
			if(drwGrp->GetDrawBuffer()->GetType() == CanvasType::gdi)
			{
				drwGrp->GetDrawBuffer()->DrawText( wxPoint( rect.GetLeft(), rect.GetBottom() ), timeline->GetWxBoldFont(), String::Printf( TXT( "%.2fs" ), animClipEvent->GetAnimationClipStart() ), wxColour( 0, 0 ,0), CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Left );
			}
			else
			{
				drwGrp->GetDrawBuffer()->DrawText( wxPoint( rect.GetLeft(), rect.GetBottom() ), timeline->GetGdiBoldFont(), String::Printf( TXT( "%.2fs" ), animClipEvent->GetAnimationClipStart() ), wxColour( 0, 0 ,0), CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Left );
			}
		}

		// Draw animation clip end value but only if animation clip is actually clipped.
		if ( Abs( animClipEvent->GetAnimationClipEnd() - animClipEvent->GetCachedAnimationDuration() ) > NumericLimits< Float >::Epsilon() )
		{
			if(drwGrp->GetDrawBuffer()->GetType() == CanvasType::gdi)
			{
				drwGrp->GetDrawBuffer()->DrawText( wxPoint( rect.GetRight(), rect.GetBottom() ), timeline->GetWxBoldFont(), String::Printf( TXT( "%.2fs" ), animClipEvent->GetAnimationClipEnd() ), wxColour( 0, 0 ,0), CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Right );
			}
			else
			{
				drwGrp->GetDrawBuffer()->DrawText( wxPoint( rect.GetRight(), rect.GetBottom() ), timeline->GetGdiBoldFont(), String::Printf( TXT( "%.2fs" ), animClipEvent->GetAnimationClipEnd() ), wxColour( 0, 0 ,0), CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Right );
			}
		}

		{
			CStorySceneEventAnimClip* animClipEvent = Cast< CStorySceneEventAnimClip >( m_event );
			if ( animClipEvent->GetVoiceWeightCurve().m_useCurve )
			{
				// get draw group appropriate for drawing this item
				Track* track = timeline->GetItemTrack( this );
				TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

				StorySceneEditorUtils::DrawEventsCurve( &(animClipEvent->GetVoiceWeightCurve().m_curve), *drwGrp->GetDrawBuffer(), rect, 100 );
			}
		}
	}

	/*
	Tries to set leftedge to given positon.

	\return Final position of left edge.
	*/
	Float CTimelineItemAnimClip::SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
	{
		SCENE_ASSERT( IsEditable() );

		CStorySceneEventAnimClip* ev = Cast< CStorySceneEventAnimClip >( GetEvent() );

		// Section is approved, these should be the same.
		SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

		enum class Mode { clipping, stretching };
		Mode mode = keyModifiers.ctrl? Mode::stretching : Mode::clipping;

		if( mode == Mode::clipping )
		{
			const Float animationStretch = ev->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *GetEvent() );
			const Float requestedFrontClip = ( requestedTimePos - GetStart() ) / animationStretch + ev->GetAnimationClipStart();
			const Float effectiveFrontClip = Clamp( requestedFrontClip, 0.0f, ev->GetAnimationClipEnd() );
			const Float effectiveTimePos = GetStart() + ( effectiveFrontClip - ev->GetAnimationClipStart() ) * animationStretch;

			ev->SetAnimationClipStart( effectiveFrontClip );
			// duration has changed so we must update it (it would be best if we edited instance event
			// so that evInst->SetAnimationClipStart() would do everything that's necessary)
			m_timeline->SetEventInstanceDuration( *ev, ev->GetDurationProperty() ); // ok since scaling factor is 1.0f

			SetStart( effectiveTimePos, true );

			UpdatePresentation();

			return effectiveTimePos;
		}
		else // Mode::stretching
		{
			const Float effectiveDuration = GetEnd() - requestedTimePos;
			const Float unstretchedDuration = m_timeline->GetEventInstanceDuration( *GetEvent() ) / ( ev->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *GetEvent() ) );
			const Float effectiveStretch = effectiveDuration / unstretchedDuration;

			ev->SetAnimationStretch( effectiveStretch );
			// duration has changed so we must update it (it would be best if we edited instance event
			// so that evInst->SetAnimationStretch() would do everything that's necessary)
			m_timeline->SetEventInstanceDuration( *ev, ev->GetDurationProperty() ); // ok since scaling factor is 1.0f
			
			SetStart( requestedTimePos, true );

			UpdatePresentation();

			return requestedTimePos;
		}
	}

	Float CTimelineItemAnimClip::SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
	{
		SCENE_ASSERT( IsEditable() );

		CStorySceneEventAnimClip* ev = Cast< CStorySceneEventAnimClip >( GetEvent() );

		// Section is approved, these should be the same.
		SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

		enum class Mode { clipping, stretching };
		Mode mode = keyModifiers.ctrl? Mode::stretching : Mode::clipping;

		if( mode == Mode::clipping )
		{
			const Float animationStretch = ev->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *GetEvent() );
			const Float requestedEndClip = ( requestedTimePos - GetStart() ) / animationStretch + ev->GetAnimationClipStart();
			const Float effectiveEndClip = Clamp( requestedEndClip, ev->GetAnimationClipStart(), m_timeline->GetAnimationDurationFromEvent( *ev ) );
			const Float effectiveTimePos = GetStart() + ( effectiveEndClip - ev->GetAnimationClipStart() ) * animationStretch;

			ev->SetAnimationClipEnd( effectiveEndClip );
			// duration has changes so we must update it (it would be best if we edited instance event
			// so that evInst->SetAnimationClipStart() would do everything that's necessary)
			m_timeline->SetEventInstanceDuration( *ev, ev->GetDurationProperty() ); // ok since scaling factor is 1.0f

			UpdatePresentation();

			return effectiveTimePos;
		}
		else // Mode::stretching
		{
			const Float effectiveDuration = requestedTimePos - m_timeline->GetEventInstanceStartTime( *GetEvent() );
			const Float unstretchedDuration = m_timeline->GetEventInstanceDuration( *GetEvent() ) / ( ev->GetAnimationStretch() * m_timeline->GetEventInstanceScalingFactor( *GetEvent() ) );
			const Float effectiveStretch = effectiveDuration / unstretchedDuration;

			ev->SetAnimationStretch( effectiveStretch );
			// duration has changed so we must update it (it would be best if we edited instance event
			// so that evInst->SetAnimationStretch() would do everything that's necessary)
			m_timeline->SetEventInstanceDuration( *ev, ev->GetDurationProperty() ); // ok since scaling factor is 1.0f

			UpdatePresentation();

			return requestedTimePos;
		}
	}

	void CTimelineItemEventGroup::DrawEmbeddedItem( CTimelineItemEvent* item, Float startTime, CEdDialogTimeline* timeline ) const
	{
		wxRect rect;
		if ( timeline->CalculateEventItemRect( item, rect ) == false )
		{
			return;
		}

		// get draw group appropriate for drawing this item
		Track* track = timeline->GetItemTrack( item );
		TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

		//rect.x = timeline->ToPixelCoords( GetStart() + startTime );

		//rect.y += rect.height / 4;
		//rect.height /= 2;

		Int32 alphaChanel = 150;

		wxColour color = item->GetColor();
		color.SetRGBA( color.GetRGBA() & 0x00FFFFFF | ( alphaChanel << 24 ) );

		if( item->IsDuration() )
		{
			// Draw event rectangle
			drwGrp->GetDrawBuffer()->FillRect( rect, color );
			drwGrp->GetDrawBuffer()->DrawRect( rect, wxColour( 0, 0, 0, alphaChanel ) );

			// Draw icons
			if( drwGrp->GetDrawBuffer()->GetType() == CanvasType::gdi )
			{
				if( item->GetWxIcon() )
				{
					for( Int32 i = rect.x + 30; i < rect.GetRight() - 30; i += 48 )
					{
						drwGrp->GetDrawBuffer()->DrawImage( item->GetWxIcon(), i, rect.y, wxColour( 255, 255, 255, alphaChanel ) );
					}
				}
			}
			else
			{
				if( item->GetIcon() )
				{
					for( Int32 i = rect.x + 30; i < rect.GetRight() - 30; i += 48 )
					{
						drwGrp->GetDrawBuffer()->DrawImage( item->GetIcon(), i, rect.y, wxColour( 255, 255, 255, alphaChanel ) );
					}
				}
			}

			// Draw text
			String text;
			if( item->GetTopText( text ) )
			{
				wxPoint textPos(rect.x + 3, drwGrp->GetDispTrackLocalPos(track).y + 2);

				if(drwGrp->GetDrawBuffer()->GetType() == CanvasType::gdi)
				{
					drwGrp->GetDrawBuffer()->DrawText( textPos, rect.width - 3, 15, timeline->GetWxDrawFont(), text, wxColour( 255, 255, 255, alphaChanel ) );
				}
				else
				{
					drwGrp->GetDrawBuffer()->DrawText( textPos, rect.width - 3, 15, timeline->GetGdiDrawFont(), text, wxColour( 255, 255, 255, alphaChanel ) );
				}
			}
			if( item->GetMiddleText( text ) )
			{
				if(drwGrp->GetDrawBuffer()->GetType() == CanvasType::gdi)
				{
					drwGrp->GetDrawBuffer()->DrawText( wxPoint( rect.x + 5, rect.y ), rect.width - 5, 15, timeline->GetWxDrawFont(), text, wxColour( 0, 0, 0, alphaChanel ) );
				}
				else
				{
					drwGrp->GetDrawBuffer()->DrawText( wxPoint( rect.x + 5, rect.y ), rect.width - 5, 15, timeline->GetGdiDrawFont(), text, wxColour( 0, 0, 0, alphaChanel ) );
				}
			}
		}
		else
		{
			// point-events (without duration)

			wxPoint pt0( rect.x, rect.y - 11 );
			wxPoint pt1( rect.x - 6, rect.y - 2 );
			wxPoint pt2( rect.x + 6, rect.y - 2 );

			drwGrp->GetDrawBuffer()->FillTriangle( pt0.x, pt0.y, pt1.x, pt1.y, pt2.x, pt2.y, color );
			drwGrp->GetDrawBuffer()->DrawTriangle( pt0.x, pt0.y, pt1.x, pt1.y, pt2.x, pt2.y, wxColour( 0, 0, 0, alphaChanel ) );

			if(drwGrp->GetDrawBuffer()->GetType() == CanvasType::gdi)
			{
				if( item->GetWxIcon() != NULL )
				{
					drwGrp->GetDrawBuffer()->DrawImage( item->GetWxIcon(), rect.x - 12, rect.y, wxColour( 255, 255, 255, alphaChanel ) );
				}
			}
			else
			{
				if( item->GetIcon() != NULL )
				{
					drwGrp->GetDrawBuffer()->DrawImage( item->GetIcon(), rect.x - 12, rect.y, wxColour( 255, 255, 255, alphaChanel ) );
				}
			}
		}

		item->CustomDraw( timeline, rect );
	}

	void CTimelineItemLookatDuration::CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{
		const CStorySceneEventLookAtDuration* evt = static_cast< CStorySceneEventLookAtDuration* >( m_event );
		if ( evt && evt->UseBlink() )
		{
			Track* track = timeline->GetItemTrack( this );
			TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

			const Int32 w = 5;
			const Int32 h = 5;
			const Int32 o = 3;
			const Int32 x = rect.GetX() + rect.GetWidth() - o - w;
			const Int32 y = rect.GetY() + o;

			drwGrp->GetDrawBuffer()->DrawRect( x, y, w, h, wxColor(0,0,0) );
			drwGrp->GetDrawBuffer()->FillRect( x+1, y+1, w-2, h-2, wxColor(100,100,100) );
		}
	}

	Bool CTimelineItemPause::IsEditable() const
	{
		return m_timeline->IsTimelineEditingEnabled();
	}

	Bool CTimelineItemPause::IsRightResizable() const
	{
		return IsEditable();
	}

	Float CTimelineItemPause::SetDuration( Float duration )
	{
		if( duration < 0.1f )
		{
			duration = 0.1f;
		}

		Float offset = duration - m_duration;
		m_duration = duration;

		// Update element duration
		static_cast< CStoryScenePauseElement* >( GetElement() )->SetDuration( duration );

		// Approve pause duration in all variants of current section (pause duration is always
		// considered approved because its duration never changes without user knowing about this).
		CStorySceneSection* section = GetElement()->GetSection();
		TDynArray< CStorySceneSectionVariantId > variantIds;
		section->EnumerateVariants( variantIds );
		for( CStorySceneSectionVariantId variantId : variantIds )
		{
			section->ApproveElementDuration( variantId, GetElement()->GetElementID(), duration );
		}

		m_timeline->OnPauseItemDurationChange( this, offset );

		return duration;
	}

	Bool CTimelineItemPoseChange::IsDuration() const
	{
		return !( GetEvent() && m_timeline->GetEventInstanceDuration( *GetEvent() ) < 0.01f );
	}

	const Char* CTimelineItemPoseChange::GetIconName() const
	{ 
		if ( !IsDuration() )
		{
			return TXT( "IMG_DIALOG_POSE_CHANGE" ); 
		}
		else
		{
			return nullptr;
		}
	}

	void CTimelineItemPoseChange::CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{
		const CStorySceneEventChangePose* evtPose = static_cast< CStorySceneEventChangePose* >( m_event );
		if ( evtPose )
		{
			if ( evtPose->HasAnimation() )
			{
				CTimelineItemAnimClip::CustomDraw( timeline, rect );
			}
			else
			{
				// get draw group appropriate for drawing this item
				Track* track = timeline->GetItemTrack( this );
				TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

				if ( !evtPose->UseWeightCurve() )
				{
					drwGrp->GetDrawBuffer()->DrawLine( rect.GetLeft(), rect.GetBottom(), rect.GetRight(), rect.GetTop(), wxColor( 0, 0, 0 ) );
				}
				else
				{
					StorySceneEditorUtils::DrawEventsCurve( evtPose->GetWeightCurve(), *drwGrp->GetDrawBuffer(), rect );
				}
			}
		}
	}

	CTimelineItemMimicAnimClip::CTimelineItemMimicAnimClip( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
	: CTimelineItemAnimClip( timeline, event, elementItem, elements )
	, m_isValid( true )
	{
		EvaluateValidity();
	}

	Bool CTimelineItemMimicAnimClip::IsValid() const
	{
		return m_isValid;
	}

	void CTimelineItemMimicAnimClip::EvaluateValidity()
	{
		m_isValid = true;

		const CStorySceneEventAnimation* animationEv = static_cast< const CStorySceneEventAnimation* >( GetEvent() );
		const CName animationName = animationEv->GetAnimationName();
		if( animationName != CName::NONE )
		{
			const CName actorName = animationEv->GetActor();
			const CAnimatedComponent* ac = m_timeline->GetHeadComponent( actorName );
			if( !ac || !ac->GetAnimationContainer() || !ac->GetAnimationContainer()->HasAnimation( animationName ) )
			{
				m_isValid = false;
			}
		}
	}

	void CTimelineItemCameraAnimClip::CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{
		const CStorySceneEventCameraAnim* evt = static_cast< CStorySceneEventCameraAnim* >( m_event );
		if ( evt )
		{
			if ( !evt->IsIdle() )
			{
				CTimelineItemAnimClip::CustomDraw( timeline, rect );
			}
			else
			{
				Track* track = timeline->GetItemTrack( this );
				TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

				drwGrp->GetDrawBuffer()->DrawLine( rect.GetLeft(), rect.GetBottom(), rect.GetRight(), rect.GetTop(), wxColor( 0, 0, 0 ) );
			}
		}
	}

	Bool CTimelineItemCameraAnimClip::IsDuration() const
	{
		return !( GetEvent() && m_timeline->GetEventInstanceDuration( *GetEvent() ) < 0.01f );
	}

	const Char* CTimelineItemCameraAnimClip::GetIconName() const
	{
		if ( !IsDuration() )
		{
			return TXT( "IMG_DIALOG_SURFACE_EFFECT" ); 
		}
		else
		{
			return nullptr;
		}
	}

	Bool CTimelineItemMimicsDuration::IsDuration() const
	{
		;
		return !( GetEvent() && m_timeline->GetEventInstanceDuration( *GetEvent() ) < 0.01f );
	}

	const Char* CTimelineItemMimicsDuration::GetIconName() const
	{ 
		if ( !IsDuration() )
		{
			return TXT( "IMG_DIALOG_MIMICS" ); 
		}
		else
		{
			return nullptr;
		}
	}

	void CTimelineItemMimicsDuration::CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{
		const CStorySceneEventMimics* evtPose = static_cast< CStorySceneEventMimics* >( m_event );
		if ( evtPose )
		{
			if ( evtPose->HasAnimation() )
			{
				CTimelineItemAnimClip::CustomDraw( timeline, rect );
			}
			else
			{
				// get draw group appropriate for drawing this item
				Track* track = timeline->GetItemTrack( this );
				TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

				if ( !evtPose->UseWeightCurve() )
				{
					drwGrp->GetDrawBuffer()->DrawLine( rect.GetLeft(), rect.GetBottom(), rect.GetRight(), rect.GetTop(), wxColor( 0, 0, 0 ) );
				}
				else
				{
					StorySceneEditorUtils::DrawEventsCurve( evtPose->GetWeightCurve(), *drwGrp->GetDrawBuffer(), rect );
				}
			}
		}
	}

	void CTimelineItemAnimation::CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{
		const CStorySceneEventAnimation* evtAnim = static_cast< CStorySceneEventAnimation* >( m_event );
		if ( evtAnim )
		{
			if ( !evtAnim->UseWeightCurve() )
			{
				CTimelineItemAnimClip::CustomDraw( timeline, rect );
			}
			else
			{
				// get draw group appropriate for drawing this item
				Track* track = timeline->GetItemTrack( this );
				TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

				StorySceneEditorUtils::DrawEventsCurve( evtAnim->GetWeightCurve(), *drwGrp->GetDrawBuffer(), rect );
			}
		}
	}

	/*
	Ctor.
	*/
	CTimelineItemAnimation::CTimelineItemAnimation( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
	: CTimelineItemAnimClip( timeline, event, elementItem, elements )
	, m_isValid( true )
	{
		EvaluateValidity();
	}

	Bool CTimelineItemAnimation::IsValid() const
	{
		return m_isValid;
	}

	void CTimelineItemAnimation::EvaluateValidity()
	{
		m_isValid = true;

		const CStorySceneEventAnimation* animationEv = static_cast< const CStorySceneEventAnimation* >( GetEvent() );
		const CName animationName = animationEv->GetAnimationName();
		if( animationName != CName::NONE )
		{
			const CName actorName = animationEv->GetActor();
			const CAnimatedComponent* ac = m_timeline->GetBodyComponent( actorName );
			if( !ac || !ac->GetAnimationContainer() || !ac->GetAnimationContainer()->HasAnimation( animationName ) )
			{
				m_isValid = false;
			}
		}
	}

	Bool CTimelineItemMimicsPose::IsDuration() const
	{
		return GetDuration() > 0.f;
	}

	void CTimelineItemMimicsPose::CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{
		StorySceneEditorUtils::DrawDialogEventWithCurveOrLinearBlend< CStorySceneEventMimicsPose >( this, m_event, timeline, rect );
	}

	void CTimelineItemMimicsFilter::CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{
		StorySceneEditorUtils::DrawDialogEventWithCurveOrLinearBlend< CStorySceneEventMimicsFilter >( this, m_event, timeline, rect );
	}

	Bool CTimelineItemLineEvent::GetTooltip( String& text ) const
	{ 
		const CStorySceneEventDialogLine* evtLine = static_cast< const CStorySceneEventDialogLine* >( m_event );
		text = evtLine->GetLineText();
		return true;
	}

	//////////////////////////////////////////////////////////////////////////

	const Char* CTimelineItemPoseKey::GetIconName() const
	{ 
		CStorySceneEventPoseKey* evt = static_cast< CStorySceneEventPoseKey* >( m_event );
		if ( evt->IsMimic() && !evt->IsBody() )
		{
			return TXT( "IMG_DIALOG_MIMICS_KEY" );
		}
		else
		{
			return TXT( "IMG_DIALOG_POSE_KEY" ); 
		}
	}

	Bool CTimelineItemPoseKey::IsLeftResizable() const
	{
		return IsEditable();
	}

	Bool CTimelineItemPoseKey::IsRightResizable() const
	{
		return IsEditable();
	}

	Float CTimelineItemPoseKey::SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
	{
		SCENE_ASSERT( IsEditable() );

		CStorySceneEventDuration* ev = Cast< CStorySceneEventDuration >( GetEvent() );

		// Section is approved, these should be the same.
		SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

		const Float oldDuration = m_timeline->GetEventInstanceDuration( *GetEvent() );
		const Float requestedDuration = GetEnd() - requestedTimePos;

		if( CStorySceneEventPoseKey* poseKey = Cast< CStorySceneEventPoseKey >( GetEvent() ) )
		{
			poseKey->FitBlends( oldDuration, requestedDuration );
		}

		ev->SetDuration( requestedDuration );
		// duration has changes so we must update it (it would be best if we edited instance event
		// so that evInst->SetDuration() would do everything that's necessary)
		m_timeline->SetEventInstanceDuration( *ev, requestedDuration ); // ok since scaling factor is 1.0f

		SetStart( requestedTimePos, true );

		UpdatePresentation();

		return requestedTimePos;
	}

	Float CTimelineItemPoseKey::SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
	{
		SCENE_ASSERT( IsEditable() );

		CStorySceneEventDuration* ev = Cast< CStorySceneEventDuration >( GetEvent() );

		// Section is approved, these should be the same.
		SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

		const Float oldDuration = m_timeline->GetEventInstanceDuration( *GetEvent() );
		const Float requestedDuration = requestedTimePos - m_timeline->GetEventInstanceStartTime( *GetEvent() );

		if( CStorySceneEventPoseKey* poseKey = Cast< CStorySceneEventPoseKey >( GetEvent() ) )
		{
			poseKey->FitBlends( oldDuration, requestedDuration );
		}

		ev->SetDuration( requestedDuration );
		// duration has changes so we must update it (it would be best if we edited instance event
		// so that evInst->SetDuration() would do everything that's necessary)
		m_timeline->SetEventInstanceDuration( *ev, requestedDuration ); // ok since scaling factor is 1.0f

		UpdatePresentation();

		return requestedTimePos;
	}

	Bool CTimelineItemPoseKey::OnLinkParentSet( CTimelineItemEvent* parent, Float timeoffset )
	{
		if ( CTimelineItemAnimClip* animClip = dynamic_cast< CTimelineItemAnimClip* >( parent ) )
		{
			const TDynArray< Float >& parentMarkers = animClip->GetKeyPoseMarkers();

			const Float parentDuration = animClip->GetDuration();
			const Float parentStart = animClip->GetStart();
			const Float parentEnd = parentStart + parentDuration;
			const Float start = GetStart();

			Float finalDuration = parentDuration;
			Float finalStart = parentStart;
			Float finalEnd = parentEnd;
			Bool calcBlends = false;
			Float blendIn = 0.f;
			Float blendOut = 0.f;

			Float snapTime = Clamp( start, parentStart, parentEnd );
			if ( parentMarkers.Size() > 0 )
			{
				Float bestMarker = -1.f;
				Float bestValue = FLT_MAX;

				for ( Uint32 i=0; i<parentMarkers.Size(); ++i )
				{
					const Float m = parentStart + finalDuration * parentMarkers[ i ];
					const Float dist = MAbs( m - start );
					if ( dist < bestValue )
					{
						bestValue = dist;
						bestMarker = m;
					}
				}

				if ( bestMarker > 0.f )
				{
					snapTime = bestMarker;

					Bool hasTransition = false;

					const TDynArray< TPair< Float, Float > >& parentDurations = animClip->GetKeyPoseTransitionDurations();
					const Uint32 numDurations = parentDurations.Size();
					if ( numDurations > 0 )
					{
						for ( Uint32 i=0; i<numDurations; ++i )
						{
							Float timeA = parentDurations[ i ].m_first;
							Float timeB = parentDurations[ i ].m_second;

							timeA = parentStart + parentDuration * timeA;
							timeB = parentStart + parentDuration * timeB;

							if ( snapTime >= timeA && snapTime <= timeB )
							{
								finalStart = timeA;
								finalEnd = timeB;
								finalDuration = Clamp( finalEnd - finalStart, 0.f, parentDuration );
								hasTransition = true;

								break;
							}
						}
					}

					if ( hasTransition )
					{
						const TDynArray< TPair< Float, Float > >& parentKeyDurations = animClip->GetKeyPoseKeyDurations();
						const Uint32 numKeyDurations = parentKeyDurations.Size();
						for ( Uint32 i=0; i<numKeyDurations; ++i )
						{
							Float timeA = parentKeyDurations[ i ].m_first;
							Float timeB = parentKeyDurations[ i ].m_second;

							timeA = parentStart + parentDuration * timeA;
							timeB = parentStart + parentDuration * timeB;

							if ( timeA >= finalStart && timeB <= finalEnd )
							{
								blendIn = Clamp( timeA - finalStart, 0.f, finalDuration );
								blendOut = Clamp( finalEnd - timeB, 0.f, finalDuration );
								calcBlends = true;

								break;
							}
						}
					}
				}
			}

			if ( !calcBlends )
			{
				blendIn = Clamp( snapTime - finalStart, 0.f, finalDuration );
				blendOut = Clamp( finalEnd - snapTime, 0.f, finalDuration );
			}

			if ( CTimelineItemPoseChange* poseChangeEvt = dynamic_cast< CTimelineItemPoseChange* >( animClip ) )
			{
				blendOut = 0.f;
			}

			SCENE_ASSERT( blendIn >= 0.f );
			SCENE_ASSERT( blendOut >= 0.f );

			if ( CStorySceneEventPoseKey* evt = dynamic_cast< CStorySceneEventPoseKey* >( m_event ) )
			{
				evt->SetBlends( blendIn, blendOut );
				evt->SetDuration( finalDuration );
			}

			SetStart( finalStart, true );

			UpdatePresentation();

			return true;
		}

		return false;
	}

	void CTimelineItemPoseKey::UpdatePresentation()
	{
		CTimelineItemEvent::UpdatePresentation();

		Float blendIn = 0.f;
		Float blendOut = 0.f;

		if ( CStorySceneEventPoseKey* evt = dynamic_cast< CStorySceneEventPoseKey* >( m_event ) )
		{
			evt->GetBlends( blendIn, blendOut );
		}

		const Float duration = m_timeline->GetEventInstanceDuration( *GetEvent() );
		if ( duration > 0.f )
		{
			m_blendInEndRelPos = Clamp< Float >( blendIn / duration, 0.0f, 1.0f );
			m_blendOutStartRelPos = Clamp< Float >( ( duration - blendOut ) / duration, 0.0f, 1.0f );
		}
	}

	void CTimelineItemPoseKey::CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{
		if ( ICurveDataOwner* evt = dynamic_cast< ICurveDataOwner* >( m_event ) )
		{
			Track* track = timeline->GetItemTrack( this );
			TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

			if ( !evt->UseCurveData() )
			{
				Int32 blendInEndPos = rect.GetX() + rect.GetWidth() * m_blendInEndRelPos;
				Int32 blendOutStartPos = rect.GetX() + rect.GetWidth() * m_blendOutStartRelPos;

				if ( blendInEndPos > 0 )
				{
					drwGrp->GetDrawBuffer()->DrawLine( rect.GetLeft(), rect.GetBottom(), blendInEndPos, rect.GetTop(), wxColor( 0, 0, 0 ) );
				}

				if ( blendOutStartPos < rect.GetRight() )
				{
					drwGrp->GetDrawBuffer()->DrawLine( blendOutStartPos, rect.GetTop(), rect.GetRight(), rect.GetBottom(), wxColor( 0, 0, 0 ) );
				}
			}
			else
			{
				StorySceneEditorUtils::DrawEventsCurve( evt->GetCurveData(), *drwGrp->GetDrawBuffer(), rect, 100 );
			}
		}
	}

	////////////////////////////////////////////////////

	CTimelineItemCurveAnimation::~CTimelineItemCurveAnimation()
	{
		if ( CWorld* world = m_timeline->GetPreviewWorld() )
		{
			CStorySceneEventCurveAnimation* event = Cast< CStorySceneEventCurveAnimation >( GetEvent() );
			CCurveEntity::DeleteEditor( world, &event->m_curve );
		}
	}

	////////////////////////////////////////////////////

	Bool CTimelineItemBlend::Initialize( TDynArray< ITimelineItem* >& items )
	{
		ASSERT( !m_isBeingInitialized );
		m_isBeingInitialized = true;

		CStorySceneEventBlend* event = Cast< CStorySceneEventBlend >( GetEvent() );

		for ( auto it = items.Begin(); it != items.End(); ++it )
		{
			AddChild( static_cast< CTimelineItemEvent* >( *it ) );
		}

		m_isBeingInitialized = false;
		return event->m_keys.Size() >= 2;
	}

	void CTimelineItemBlend::GetChildItems( TDynArray< ITimelineItem* >& childItems )
	{
		TDynArray< CTimelineItemEvent* >* _childItems = reinterpret_cast< TDynArray< CTimelineItemEvent* >* >( &childItems );
		GetItems( *_childItems );
	}

	void CTimelineItemBlend::UpdatePresentation()
	{
		CTimelineItemEvent::UpdatePresentation();
		Refresh();
	}

	void CTimelineItemBlend::OnChildChanged( CTimelineItemEvent* item )
	{
		SortKeys();
		Refresh();
	}

	void CTimelineItemBlend::SortKeys()
	{
		// Grab all items

		CStorySceneEventBlend* event = Cast< CStorySceneEventBlend >( GetEvent() );

		TDynArray< CTimelineItemEvent* > items;
		for ( auto it = event->m_keys.Begin(); it != event->m_keys.End(); ++it )
		{
			items.PushBack( m_timeline->FindItemEvent( *it ) );
		}

		// Sort items

		Sort( items.Begin(), items.End(), TimelineItemStartSorter() );

		// Write back GUIDs of sorted events
		
		for ( Uint32 i = 0; i < event->m_keys.Size(); i++ )
		{
			event->m_keys[ i ] = items[ i ]->GetEvent()->GetGUID();
		}
	}

	void CTimelineItemBlend::DoAddChild( CTimelineItemEvent* item )
	{
		CStorySceneEvent* event = item->GetEvent();

		// Remove from old blend

		if ( CTimelineItemBlend* blendParent = GetBlendParent() )
		{
			blendParent->RemoveChild( item );
		}

		// Add to new blend

		CStorySceneEventBlend* blendParentEvent = Cast< CStorySceneEventBlend >( GetEvent() );
		event->SetBlendParentGUID( blendParentEvent->GetGUID() );
		blendParentEvent->m_keys.PushBack( event->GetGUID() );
		SortKeys();

		OnChildAdded( item );

		// Refresh

		if ( !m_isBeingInitialized )
		{
			Refresh();
		}
	}

	Bool CTimelineItemBlend::AddChild( CTimelineItemEvent* item )
	{
		return false;
	}

	void CTimelineItemBlend::RemoveChild( CTimelineItemEvent* item )
	{
		if ( item )
		{
			CStorySceneEvent* event = item->GetEvent();
			CStorySceneEventBlend* blendParent = Cast< CStorySceneEventBlend >( GetEvent() );

			OnChildRemove( item );

			event->SetBlendParentGUID( CGUID::ZERO );
			blendParent->m_keys.Remove( event->GetGUID() ); // After this order of items should be preserved

			Refresh();
		}
	}

	void CTimelineItemBlend::RemoveAllChildren()
	{
		CStorySceneEventBlend* event = Cast< CStorySceneEventBlend >( GetEvent() );

		while ( !event->m_keys.Empty() )
		{
			RemoveChild( m_timeline->FindItemEvent( event->m_keys.PopBack() ) );
		}
	}

	void CTimelineItemBlend::OnChildSelected( CTimelineItemEvent* item )
	{
	}

	void CTimelineItemBlend::GetItems( TDynArray< CTimelineItemEvent* >& items )
	{
		CStorySceneEventBlend* event = Cast< CStorySceneEventBlend >( GetEvent() );

		for ( auto it = event->m_keys.Begin(); it != event->m_keys.End(); ++it )
		{
			items.PushBack( m_timeline->FindItemEvent( *it ) );
		}
	}

	Uint32 CTimelineItemBlend::GetItemsCount()
	{
		CStorySceneEventBlend* event = Cast< CStorySceneEventBlend >( GetEvent() );
		return event->m_keys.Size();
	}

	void CTimelineItemBlend::Refresh()
	{
		RefreshStartPositionAndDuration();
	}

	void CTimelineItemBlend::RefreshStartPositionAndDuration()
	{
		static const Float minDuration = 0.1f; // Make sure it's visible on the timeline

		CStorySceneEventBlend* event = Cast< CStorySceneEventBlend >( GetEvent() );

		// Get sorted items

		TDynArray< CTimelineItemEvent* > items;
		GetItems( items );

		if ( items.Empty() )
		{
			event->SetDuration( minDuration );
			return;
		}

		// Determine start position and duration of the whole blend

		Float startPosition = FLT_MAX;
		Float endPosition = -FLT_MAX;
		for ( auto it = items.Begin(); it != items.End(); ++it )
		{
			const Float childStartPosition = (*it)->GetStart();

			startPosition = Min( startPosition, childStartPosition );
			endPosition = Max( endPosition, childStartPosition );
		}

		CTimelineItemEvent::SetStart( startPosition, true );
		event->SetDuration( Max(endPosition - startPosition, minDuration) );
	}

	void CTimelineItemBlend::OnDeleted()
	{
		TDynArray< CTimelineItemEvent* > items;
		GetItems( items );

		// Make all key events aware that they are no longer attached as keys.
		for ( auto it = items.Begin(); it != items.End(); ++it )
		{
			( *it )->GetEvent()->SetBlendParentGUID( CGUID::ZERO );
		}

		CTimelineItemEvent::OnDeleted();
	}

	Float CTimelineItemBlend::SetStart( Float start, Bool deepUpdate )
	{
		CStorySceneEventBlend* event = Cast< CStorySceneEventBlend >( GetEvent() );

		const Float oldStart = GetStart();
		const Float newStart = CTimelineItemEvent::SetStart( start, deepUpdate );
		const Float offset = newStart - oldStart;

		// Also move all linked key items

		TDynArray< CTimelineItemEvent* > items;
		GetItems( items );
		for ( auto it = items.Begin(); it != items.End(); ++it )
		{
			(*it)->GetEvent()->SetBlendParentGUID( CGUID::ZERO ); // A bit of a hack: prevent full Refresh() on this
			(*it)->SetStart( (*it)->GetStart() + offset, deepUpdate );
			(*it)->GetEvent()->SetBlendParentGUID( event->GetGUID() );
		}

		Refresh();

		return newStart;
	}

	void CTimelineItemBlend::GetMoveSet( TDynArray< ITimelineItem* >& outMoveSet )
	{
		// Note we're not doing this:
		// outMoveSet.PushBackUnique( this );

		CStorySceneEventBlend* blendEv = Cast< CStorySceneEventBlend >( GetEvent() );
		for ( auto it = blendEv->m_keys.Begin(), end = blendEv->m_keys.End(); it != end; ++it )
		{
			CTimelineItemEvent* key = m_timeline->FindItemEvent( *it );
			key->GetMoveSet( outMoveSet );
		}

		const TDynArray< CGUID >& children = m_event->GetLinkChildrenGUID();
		for( auto it = children.Begin(), end = children.End(); it != end; ++it )
		{
			CTimelineItemEvent* ch = m_timeline->FindItemEvent( *it );
			ch->GetMoveSet( outMoveSet );
		}
	}

	Bool CTimelineItemBlend::IsCopyable() const
	{
		// Copying blend events is not yet supported (first we need to clean up some crappy code).
		return false;
	}

	////////////////////////////////////////////////////

	CTimelineItemCurveBlend::CTimelineItemCurveBlend( CEdDialogTimeline* timeline, CStorySceneEvent* _event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements )
		: CTimelineItemBlend( timeline, _event, elementItem, elements )
	{
		SMultiCurve::RegisterChangeListener( this );
		CCurveEntity::RegisterSelectionListener( this );
	}

	CTimelineItemCurveBlend::~CTimelineItemCurveBlend()
	{
		CCurveEntity::UnregisterSelectionListener( this );
		SMultiCurve::UnregisterChangeListener( this );

		if ( CWorld* world = m_timeline->GetPreviewWorld() )
		{
			CStorySceneEventCurveBlend* event = Cast< CStorySceneEventCurveBlend >( GetEvent() );
			CCurveEntity::DeleteEditor( world, &event->m_curve );
		}
	}

	void CTimelineItemCurveBlend::OnChildSelected( CTimelineItemEvent* item )
	{
		CTimelineItemBlend::OnChildSelected( item );

		// Select corresponding control point on editable curve in preview panel

		if ( CWorld* world = m_timeline->GetPreviewWorld() )
		{
			CStorySceneEventCurveBlend* event = Cast< CStorySceneEventCurveBlend >( GetEvent() );
			if ( CCurveEntity* curveEntity = CCurveEntity::FindCurveEditor( world, &event->m_curve ) )
			{
				for ( Uint32 i = 0; i < event->m_keys.Size(); i++ )
				{
					if ( event->m_keys[ i ] == item->GetEvent()->GetGUID() )
					{
						curveEntity->SelectControlPointByIndex( i );
						break;
					}
				}

			}
		}
	}

	Bool CTimelineItemCurveBlend::Initialize( TDynArray< ITimelineItem* >& items )
	{
		CStorySceneEventCurveBlend* event = Cast< CStorySceneEventCurveBlend >( GetEvent() );
		event->InitCurve();

		return CTimelineItemBlend::Initialize( items );
	}

	void CTimelineItemCurveBlend::OnChildAdded( CTimelineItemEvent* item )
	{
		CStorySceneEventCurveBlend* event = Cast< CStorySceneEventCurveBlend >( GetEvent() );
		const Uint32 index = event->GetKeyIndex( item->GetEvent()->GetGUID() );
		event->m_curve.AddControlPointAt( index );
	}

	void CTimelineItemCurveBlend::OnChildRemove( CTimelineItemEvent* item )
	{
		CStorySceneEventCurveBlend* event = Cast< CStorySceneEventCurveBlend >( GetEvent() );
		const Uint32 index = event->GetKeyIndex( item->GetEvent()->GetGUID() );
		event->m_curve.RemoveControlPoint( index );
	}

	void CTimelineItemCurveBlend::UpdatePresentation()
	{
		CTimelineItemBlend::UpdatePresentation();

		// Prevent the user from changing curve length; they should only change item duration to control animation length
		CStorySceneEventCurveBlend* event = Cast< CStorySceneEventCurveBlend >( GetEvent() );
		event->m_curve.SetTotalTime( GetDuration() );
	}

	void CTimelineItemCurveBlend::Refresh()
	{
		CTimelineItemBlend::Refresh();
		RefreshCurveFromKeys();
	}

	void CTimelineItemCurveBlend::RefreshCurveFromKeys()
	{
		CStorySceneEventCurveBlend* event = Cast< CStorySceneEventCurveBlend >( GetEvent() );

		TDynArray< CTimelineItemEvent* > items;
		GetItems( items );

		// Get keys' range

		Float startPosition = FLT_MAX;
		Float endPosition = -FLT_MAX;
		for ( auto it = items.Begin(); it != items.End(); ++it )
		{
			const Float childStartPosition = (*it)->GetStart();

			startPosition = Min( startPosition, childStartPosition );
			endPosition = Max( endPosition, childStartPosition );
		}

		if ( startPosition == endPosition )
		{
			return;
		}

		// Update the curve total time

		event->m_curve.SetTotalTime( GetDuration() );

		// Set values for all curve control points

		ASSERT( items.Size() == event->m_curve.Size() );
		for ( Uint32 i = 0; i < items.Size(); i++ )
		{
			CTimelineItemEvent* keyEvent = items[i];

			// Determine curve time

			const Float localTime = keyEvent->GetStart() - startPosition;

			// Copy transform and extra data from key to curve

			CStorySceneEventCurveBlend::CurveKeyData data;
			event->GetKeyDataFromEvent( keyEvent->GetEvent(), data );

			event->m_curve.SetControlPointTime( i, localTime );
			event->m_curve.SetControlPointTransform( i, data.m_transform );
			event->m_curve.SetExtraDataValues( i, data.m_extraData );
		}

		// Refresh editable curve

		if ( CWorld* world = m_timeline->GetPreviewWorld() )
		{
			CCurveEntity::RefreshEditor( world, &event->m_curve );
		}
	}

	void CTimelineItemCurveBlend::RefreshKeysFromCurve()
	{
		CStorySceneEventCurveBlend* blendEvent = Cast< CStorySceneEventCurveBlend >( GetEvent() );

		// Add keys

		if ( blendEvent->m_keys.Size() < blendEvent->m_curve.Size() )
		{
			for ( Uint32 i = blendEvent->m_keys.Size(); i < blendEvent->m_curve.Size(); i++ )
			{
				const Float absolutePosition = GetStart() + blendEvent->m_curve.GetControlPointTime( i );

				CTimelineItemEvent* item = CreateKeyItem();
				item->SetStart( absolutePosition, true );

				blendEvent->m_keys.PushBack( item->GetEvent()->GetGUID() );
				item->GetEvent()->SetBlendParentGUID( blendEvent->GetGUID() );
			}
		}

		// Remove keys

		else if ( blendEvent->m_keys.Size() > blendEvent->m_curve.Size() )
		{
			for ( Uint32 i = blendEvent->m_curve.Size(); i < blendEvent->m_keys.Size(); i++ )
			{
				CTimelineItemEvent* item = m_timeline->FindItemEvent( blendEvent->m_keys[ i ] );
				CStorySceneEvent* event = item->GetEvent();
				event->SetBlendParentGUID( CGUID::ZERO );
				m_timeline->RemoveItem( item );
				delete item;
			}
			blendEvent->m_keys.Resize( blendEvent->m_curve.Size() );
		}

		// Copy curve data into events

		ASSERT( blendEvent->m_keys.Size() == blendEvent->m_curve.Size() );

		for ( Uint32 i = 0; i < blendEvent->m_keys.Size(); i++ )
		{
			CStorySceneEventCurveBlend::CurveKeyData data;
			blendEvent->m_curve.GetControlPointTransform( i, data.m_transform );
			blendEvent->m_curve.GetControlPointExtraDataValues( i, data.m_extraData );

			CTimelineItemEvent* item = m_timeline->FindItemEvent( blendEvent->m_keys[ i ] );
			CStorySceneEvent* event = item->GetEvent();

			const Float globalTime = GetStart() + blendEvent->m_curve.GetControlPointTime( i );

			event->SetBlendParentGUID( CGUID::ZERO ); // A bit of a hack: prevent full Refresh() on this
			item->SetStart( globalTime, true );
			event->SetBlendParentGUID( blendEvent->GetGUID() );

			blendEvent->SetKeyDataToEvent( event, data );
		}
	}

	void CTimelineItemCurveBlend::OnCurveChanged( SMultiCurve* curve )
	{
		CStorySceneEventCurveBlend* blendEvent = Cast< CStorySceneEventCurveBlend >( GetEvent() );
		if ( curve != &blendEvent->m_curve )
		{
			return;
		}

		RefreshKeysFromCurve();
	}

	void CTimelineItemCurveBlend::OnCurveSelectionChanged( CCurveEntity* curveEntity, const TDynArray< Uint32 >& selectedControlPointIndices )
	{
		CStorySceneEventCurveBlend* blendEvent = Cast< CStorySceneEventCurveBlend >( GetEvent() );
		if ( curveEntity->GetCurve() != &blendEvent->m_curve )
		{
			return;
		}

		// Get all items

		TDynArray< CTimelineItemEvent* > items;
		GetItems( items );

		// Pick items to be selected based on provided indices

		TDynArray< ITimelineItem* > itemsToSelect;
		for ( auto it = selectedControlPointIndices.Begin(); it != selectedControlPointIndices.End(); ++it )
		{
			itemsToSelect.PushBack( items[ *it ] );
		}

		// Select items

		m_timeline->SetSelection( itemsToSelect, false );
	}

	////////////////////////////////////////////////////

	CTimelineItemEvent* CTimelineItemEnhancedCameraBlend::CreateKeyItem()
	{
		CStorySceneEventEnhancedCameraBlend* event = Cast< CStorySceneEventEnhancedCameraBlend >( GetEvent() );
		return m_timeline->CreateCustomCameraEvent( event->m_baseCameraDefinition );
	}

	void CTimelineItemEnhancedCameraBlend::Refresh()
	{
		CTimelineItemCurveBlend::Refresh();

		// Refresh base camera definition

		CStorySceneEventEnhancedCameraBlend* event = Cast< CStorySceneEventEnhancedCameraBlend >( GetEvent() );
		if ( event->m_keys.Size() >= 1 )
		{
			CStorySceneEventCamera* camera = Cast< CStorySceneEventCamera >( m_timeline->FindItemEvent( event->m_keys[ 0 ] )->GetEvent() );
			event->m_baseCameraDefinition = *camera->GetCameraDefinition();
		}
	}

	Bool CTimelineItemEnhancedCameraBlend::AddChild( CTimelineItemEvent* item )
	{
		// Already belongs to other blend?

		if ( item->GetEvent()->HasBlendParent() )
		{
			return false;
		}

		// Check event type compatibility
		if ( !IsType< CStorySceneEventCamera >( item->GetEvent() ) )
		{
			return false;
		}

		DoAddChild( item );
		return true;
	}

	void CTimelineItemEnhancedCameraBlend::GenerateCameraFragments( IViewport* view, CRenderFrame* frame )
	{
		// Only draw debug camera in free camera mode

		if ( m_timeline->GetCameraMode() != SPCM_FREE_CAM )
		{
			return;
		}

		// Get camera state

		const Float localTime = m_timeline->GetCurrentPosition() - GetStart();
		if ( localTime < 0.0f || localTime >= GetDuration() )
		{
			return;
		}

		CStorySceneEventEnhancedCameraBlend* event = Cast< CStorySceneEventEnhancedCameraBlend >( GetEvent() );
		StorySceneCameraDefinition cameraState;
		event->GetAbsoluteCameraStateAt( localTime, &cameraState );

		// Draw the camera

		const Box boundingBox = Box( Vector::ZEROS, 0.15f );

		Matrix worldMatrix;
		cameraState.m_cameraTransform.CalcLocalToWorld( worldMatrix );

		CRenderCamera camera;
		camera.Set(
			worldMatrix.GetTranslationRef(),
			worldMatrix.ToEulerAngles(),
			cameraState.m_cameraFov,
			1.77f,						// Aspect ratio
			0.05f * 0.99f,				// Near plane
			0.5f,						// Far plane
			0.99f );					// Zoom

		frame->AddDebugFrustum( camera.GetScreenToWorld(), Color(50, 50, 255, 50), true );
	}

	//////////////////////////////////////////////////////////////////////////

	Bool CTimelineItemCameraBlendEvent::AddChild( CTimelineItemEvent* item )
	{
		// Already belongs to other blend?
		if ( item->GetEvent()->HasBlendParent() )
		{
			return false;
		}

		// Check event type compatibility

		if ( !IsType< CStorySceneEventCamera >( item->GetEvent() ) )
		{
			return false;
		}

		DoAddChild( item );
		return true;
	}

	void CTimelineItemCameraBlendEvent::Refresh()
	{
		CTimelineItemBlend::Refresh();

		/*CStorySceneCameraBlendEvent* blendEvent = Cast< CStorySceneCameraBlendEvent >( GetEvent() );

		TDynArray< const CStorySceneEventCustomCamera* > camKeys;
		TDynArray< Float > camTimes;
		{
			for ( Uint32 i = 0; i < blendEvent->GetKeysNum(); i++ )
			{
				CTimelineItemEvent* item = m_timeline->FindItemEvent( blendEvent->GetKeys()[ i ] );
				CStorySceneEvent* childEvt = item->GetEvent();
				SCENE_ASSERT( childEvt->GetClass()->IsA< CStorySceneEventCustomCamera >() );
				
				const CStorySceneEventCustomCamera* camEvt = static_cast< const CStorySceneEventCustomCamera* >( childEvt );
				camKeys.PushBack( camEvt );
				camTimes.PushBack( item->GetStart() );
			}
		}

		blendEvent->OnRefreshedItems( camKeys, camTimes );*/
	}

	void CTimelineItemCameraBlendEvent::OnChildAdded( CTimelineItemEvent* item )
	{
		//CStorySceneCameraBlendEvent* event = Cast< CStorySceneCameraBlendEvent >( GetEvent() );
		//SCENE_ASSERT( item->GetEvent()->GetClass()->IsA< CStorySceneEventCustomCamera >() );
		//event->OnKeyAdded( static_cast< const CStorySceneEventCustomCamera* >( item->GetEvent() ) );
	}

	void CTimelineItemCameraBlendEvent::OnChildRemove( CTimelineItemEvent* item )
	{
		//CStorySceneCameraBlendEvent* event = Cast< CStorySceneCameraBlendEvent >( GetEvent() );
		//SCENE_ASSERT( item->GetEvent()->GetClass()->IsA< CStorySceneEventCustomCamera >() );
		//event->OnKeyRemoved( static_cast< const CStorySceneEventCustomCamera* >( item->GetEvent() ) );
	}

	///////////////////////////////////////////////////////////////////

	Bool CTimelineItemOverridePlacementBlend::AddChild( CTimelineItemEvent* item )
	{
		// Already belongs to other blend?

		if ( item->GetEvent()->HasBlendParent() )
		{
			return false;
		}

		// Check event type compatibility

		if ( !IsType< CStorySceneEventOverridePlacement >( item->GetEvent() ) )
		{
			return false;
		}

		DoAddChild( item );
		return true;
	}

	CTimelineItemEvent* CTimelineItemOverridePlacementBlend::CreateKeyItem()
	{
		CStorySceneOverridePlacementBlend* event = Cast< CStorySceneOverridePlacementBlend >( GetEvent() );
		return m_timeline->CreateOverridePlacementEvent( event->GetTrackName(), event->GetSubject() );
	}

	void CTimelineItemCameraBlendEvent::CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{
		CTimelineItemBlend::CustomDraw( timeline, rect );

		Track* track = timeline->GetItemTrack( this );
		TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

		/*wxColour fillColor = wxColour( 0, 0, 0, EVENT_ALPHA );

		const CStorySceneCameraBlendEvent* evt = static_cast< CStorySceneCameraBlendEvent* >( m_event );

		if ( evt->GetKeysNum() == 1 )
		{
			{
				const Float p = evt->GetFirstPointOfInterpolation();
				const Float x1	= rect.GetX() + p * rect.GetWidth();
				const Float y1 = rect.GetY();
				drwGrp->GetDrawBuffer()->FillRect( x1, y1, 2, rect.GetHeight(), fillColor );
			}
		}
		else if ( evt->GetKeysNum() > 1 )
		{
			{
				const Float p = evt->GetFirstPointOfInterpolation();
				const Float x1	= rect.GetX() + p * rect.GetWidth();
				const Float y1 = rect.GetY();
				drwGrp->GetDrawBuffer()->FillRect( x1, y1, 2, rect.GetHeight(), fillColor );
			}

			{
				const Float p = evt->GetLastPointOfInterpolation();
				const Float x1	= rect.GetX() + p * rect.GetWidth();
				const Float y1 = rect.GetY();
				drwGrp->GetDrawBuffer()->FillRect( x1, y1, 2, rect.GetHeight(), fillColor );
			}
		}*/
	}

	//////////////////////////////////////////////////////////////////////////

	void CTimelineItemEventGroup::CustomDraw( CEdTimeline* timeline, const wxRect& rect ) const
	{
		CEdDialogTimeline* dialogTimeline = static_cast< CEdDialogTimeline* >( timeline );

		for( TDynArray< CTimelineItemEvent* >::const_iterator itemIter = m_embeddedItems.Begin();
			itemIter != m_embeddedItems.End(); ++itemIter )
		{
			DrawEmbeddedItem( (*itemIter), 0.0f, dialogTimeline );
		}
	}

	//////////////////////////////////////////////////////////////////////////

	Bool CTimelineItemChoice::IsCutsceneChoice() const
	{
		return GetElement()->GetSection()->IsA< CStorySceneCutsceneSection >();
	}

	Bool CTimelineItemChoice::IsRightResizable() const
	{ 
		return !IsCutsceneChoice();
	}

	Float CTimelineItemChoice::SetDuration( Float duration )
	{
		SCENE_ASSERT( !IsCutsceneChoice() );

		Float offset = duration - m_duration;
		m_duration = duration;

		// Update element duration
		static_cast< CStorySceneChoice* >( GetElement() )->SetDuration( duration );

		m_timeline->OnBlockingItemDurationChange( this, offset );

		return duration;
	}

	Bool CTimelineItemChoice::IsRemovable() const
	{
		// Choices can't be removed using timeline, they should be removed using screenplay panel.
		return false;
	}

	//////////////////////////////////////////////////////////////////////////

	CClass* CTimelineItemGameplayCamera::GetInterpolationEventClass() const
	{
		return CStorySceneEventCameraInterpolation::GetStaticClass();
	}

	CStorySceneEventInterpolation* CTimelineItemGameplayCamera::CreateInterpolationEvent() const
	{
		return new CStorySceneEventCameraInterpolation;
	}

	/*
	If it returns nullptr then this item type can't be interpolated.
	*/
	CClass* CTimelineItemCustomCamera::GetInterpolationEventClass() const
	{
		// This indicates that custom camera items can be interpolated (because != nullptr)
		// and also is a marker to compare whether several items are compatible with regards
		// to interpolation.
		return CStorySceneEventCameraInterpolation::GetStaticClass();
	}

	/*
	If it returns nullptr then this item type can't be interpolated.

	\return CStorySceneEventCameraInterpolation object. Dynamically allocated. Caller takes ownership.
	User needs to call CStorySceneEventInterpolation::Initialize() to initialize the object.
	*/
	CStorySceneEventInterpolation* CTimelineItemCustomCamera::CreateInterpolationEvent() const
	{
		return new CStorySceneEventCameraInterpolation;
	}

	/*
	If it returns nullptr then this item type can't be interpolated.
	*/
	CClass* CTimelineItemCustomCameraInstance::GetInterpolationEventClass() const
	{
		// This indicates that custom camera items can be interpolated (because != nullptr)
		// and also is a marker to compare whether several items are compatible with regards
		// to interpolation.
		return CStorySceneEventCameraInterpolation::GetStaticClass();
	}

	/*
	If it returns nullptr then this item type can't be interpolated.

	\return CStorySceneEventCameraInterpolation object. Dynamically allocated. Caller takes ownership.
	User needs to call CStorySceneEventInterpolation::Initialize() to initialize the object.
	*/
	CStorySceneEventInterpolation* CTimelineItemCustomCameraInstance::CreateInterpolationEvent() const
	{
		return new CStorySceneEventCameraInterpolation;
	}


	CTimelineItemCustomCameraInstance::CTimelineItemCustomCameraInstance( CEdDialogTimeline* timeline, CStorySceneEvent* event, CTimelineItemBlocking* elementItem, TDynArray< CTimelineItemBlocking* >& elements ) 
		: CTimelineItemEvent( timeline, event, elementItem, elements )
	{
		EvaluateValidity();
	}

	Bool CTimelineItemCustomCameraInstance::IsValid() const
	{
		return m_isValid;
	}

	void CTimelineItemCustomCameraInstance::EvaluateValidity()
	{
		m_isValid = true;

		const CStorySceneEventCustomCameraInstance* evt = static_cast< const CStorySceneEventCustomCameraInstance* >( GetEvent() );
		if( ! evt->GetCameraDefinition() )
		{
			m_isValid = false;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	/*
	TODO: docs.
	*/
	CClass* CTimelineItemOverridePlacement::GetInterpolationEventClass() const
	{
		return CStorySceneEventPlacementInterpolation::GetStaticClass();
	}

	/*
	TODO: docs.
	*/
	CStorySceneEventInterpolation* CTimelineItemOverridePlacement::CreateInterpolationEvent() const
	{
		return new CStorySceneEventPlacementInterpolation;
	}

	//////////////////////////////////////////////////////////////////////////

	CClass* CTimelineItemScenePropPlacement::GetInterpolationEventClass() const
	{
		return CStorySceneEventPropPlacementInterpolation::GetStaticClass();
	}

	CStorySceneEventInterpolation* CTimelineItemScenePropPlacement::CreateInterpolationEvent() const
	{
		return new CStorySceneEventPropPlacementInterpolation;
	}

	//////////////////////////////////////////////////////////////////////////

	const Char* CTimelineItemLightProperty::GetIconName() const
	{ 
		const CStorySceneEventLightProperties* lEvt = static_cast< const CStorySceneEventLightProperties* >( GetEvent() );
		return lEvt->GetEnabled() ? TXT( "IMG_DIALOG_LIGHT" ) : TXT( "IMG_DIALOG_LIGHT_DISABLED" );
	}

	CClass* CTimelineItemLightProperty::GetInterpolationEventClass() const
	{
		return CStorySceneEventLightPropertiesInterpolation::GetStaticClass();
	}

	CStorySceneEventInterpolation* CTimelineItemLightProperty::CreateInterpolationEvent() const
	{
		return new CStorySceneEventLightPropertiesInterpolation;
	}

	//////////////////////////////////////////////////////////////////////////

	CClass* CTimelineItemDisablePhysicsCloth::GetInterpolationEventClass() const
	{
		return CStorySceneEventClothDisablingInterpolation::GetStaticClass();
	}

	CStorySceneEventInterpolation* CTimelineItemDisablePhysicsCloth::CreateInterpolationEvent() const
	{
		return new CStorySceneEventClothDisablingInterpolation;
	}

	//////////////////////////////////////////////////////////////////////////

	CClass* CTimelineItemDisableDangle::GetInterpolationEventClass() const
	{
		return CStorySceneEventDangleDisablingInterpolation::GetStaticClass();
	}

	CStorySceneEventInterpolation* CTimelineItemDisableDangle::CreateInterpolationEvent() const
	{
		return new CStorySceneEventDangleDisablingInterpolation;
	}

	//////////////////////////////////////////////////////////////////////////

	CClass* CTimelineItemCameraLight::GetInterpolationEventClass() const 	
	{
		return CStorySceneEventCameraLightInterpolation::GetStaticClass();
	}
	
	CStorySceneEventInterpolation* CTimelineItemCameraLight::CreateInterpolationEvent() const
	{
		return new CStorySceneEventCameraLightInterpolation;
	}

	//////////////////////////////////////////////////////////////////////////

	CClass* CTimelineItemMorphItem::GetInterpolationEventClass() const
	{
		return CStorySceneEventMorphInterpolation::GetStaticClass();
	}

	CStorySceneEventInterpolation* CTimelineItemMorphItem::CreateInterpolationEvent() const
	{
		return new CStorySceneEventMorphInterpolation;
	}

	//////////////////////////////////////////////////////////////////////////

	CClass* CTimelineItemDanglesShake::GetInterpolationEventClass() const
	{
		return CStorySceneDanglesShakeEventInterpolation::GetStaticClass();
	}

	CStorySceneEventInterpolation* CTimelineItemDanglesShake::CreateInterpolationEvent() const
	{
		return new CStorySceneDanglesShakeEventInterpolation;
	}

	//////////////////////////////////////////////////////////////////////////

	Bool CTimelineItemLookatDuration::IsLeftResizable() const
	{
		return IsEditable();
	}

	Bool CTimelineItemLookatDuration::IsRightResizable() const
	{
		return IsEditable();
	}

	Float CTimelineItemLookatDuration::SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
	{
		SCENE_ASSERT( IsEditable() );

		CStorySceneEventLookAtDuration* ev = Cast< CStorySceneEventLookAtDuration >( GetEvent() );

		// Section is approved, these should be the same.
		SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

		const Float requestedDuration = GetEnd() - requestedTimePos;
		ev->SetDuration( requestedDuration );
		// duration has changes so we must update it (it would be best if we edited instance event
		// so that evInst->SetDuration() would do everything that's necessary)
		m_timeline->SetEventInstanceDuration( *ev, requestedDuration ); // ok since scaling factor is 1.0f

		SetStart( requestedTimePos, true );

		UpdatePresentation();

		return requestedTimePos;
	}

	Float CTimelineItemLookatDuration::SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
	{
		SCENE_ASSERT( IsEditable() );

		CStorySceneEventLookAtDuration* ev = Cast< CStorySceneEventLookAtDuration >( GetEvent() );

		// Section is approved, these should be the same.
		SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

		const Float requestedDuration = requestedTimePos - m_timeline->GetEventInstanceStartTime( *GetEvent() );
		ev->SetDuration( requestedDuration );
		// duration has changes so we must update it (it would be best if we edited instance event
		// so that evInst->SetDuration() would do everything that's necessary)
		m_timeline->SetEventInstanceDuration( *ev, requestedDuration ); // ok since scaling factor is 1.0f

		UpdatePresentation();

		return requestedTimePos;
	}

	//////////////////////////////////////////////////////////////////////////

	Bool CTimelineItemLookatGameplay::IsLeftResizable() const
	{
		return IsEditable();
	}

	Bool CTimelineItemLookatGameplay::IsRightResizable() const
	{
		return IsEditable();
	}

	Float CTimelineItemLookatGameplay::SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
	{
		SCENE_ASSERT( IsEditable() );

		CStorySceneEventGameplayLookAt* ev = Cast< CStorySceneEventGameplayLookAt >( GetEvent() );

		// Section is approved, these should be the same.
		SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

		const Float requestedDuration = GetEnd() - requestedTimePos;
		ev->SetDuration( requestedDuration );
		// duration has changes so we must update it (it would be best if we edited instance event
		// so that evInst->SetDuration() would do everything that's necessary)
		m_timeline->SetEventInstanceDuration( *ev, requestedDuration ); // ok since scaling factor is 1.0f

		SetStart( requestedTimePos, true );

		UpdatePresentation();

		return requestedTimePos;
	}

	Float CTimelineItemLookatGameplay::SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
	{
		SCENE_ASSERT( IsEditable() );

		CStorySceneEventGameplayLookAt* ev = Cast< CStorySceneEventGameplayLookAt >( GetEvent() );

		// Section is approved, these should be the same.
		SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

		const Float requestedDuration = requestedTimePos - m_timeline->GetEventInstanceStartTime( *GetEvent() );
		ev->SetDuration( requestedDuration );
		// duration has changes so we must update it (it would be best if we edited instance event
		// so that evInst->SetDuration() would do everything that's necessary)
		m_timeline->SetEventInstanceDuration( *ev, requestedDuration ); // ok since scaling factor is 1.0f

		UpdatePresentation();

		return requestedTimePos;
	}

	//////////////////////////////////////////////////////////////////////////

	Bool CTimelineItemFade::IsLeftResizable() const
	{
		return IsEditable();
	}

	Bool CTimelineItemFade::IsRightResizable() const
	{
		return IsEditable();
	}

	Float CTimelineItemFade::SetLeftEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
	{
		SCENE_ASSERT( IsEditable() );

		CStorySceneEventFade* ev = Cast< CStorySceneEventFade >( GetEvent() );

		// Section is approved, these should be the same.
		SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

		const Float requestedDuration = GetEnd() - requestedTimePos;
		ev->SetDuration( requestedDuration );
		// duration has changes so we must update it (it would be best if we edited instance event
		// so that evInst->SetDuration() would do everything that's necessary)
		m_timeline->SetEventInstanceDuration( *ev, requestedDuration ); // ok since scaling factor is 1.0f

		SetStart( requestedTimePos, true );

		UpdatePresentation();

		return requestedTimePos;
	}

	Float CTimelineItemFade::SetRightEdge( Float requestedTimePos, TimelineKeyModifiers keyModifiers )
	{
		SCENE_ASSERT( IsEditable() );

		CStorySceneEventFade* ev = Cast< CStorySceneEventFade >( GetEvent() );

		// Section is approved, these should be the same.
		SCENE_ASSERT( Abs( ev->GetDurationProperty() - m_timeline->GetEventInstanceDuration( *GetEvent() ) ) <= NumericLimits< Float >::Epsilon() );

		const Float requestedDuration = requestedTimePos - m_timeline->GetEventInstanceStartTime( *GetEvent() );
		ev->SetDuration( requestedDuration );
		// duration has changes so we must update it (it would be best if we edited instance event
		// so that evInst->SetDuration() would do everything that's necessary)
		m_timeline->SetEventInstanceDuration( *ev, requestedDuration ); // ok since scaling factor is 1.0f

		UpdatePresentation();

		return requestedTimePos;
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
