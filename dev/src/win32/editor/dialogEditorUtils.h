
#pragma once

// forward declarations
class CStorySceneLine;
class CStorySceneSection;

#include "timeline.h"
#include "timelineImpl/drawing.h"
#include "dialogTimeline.h"
#include "dialogTimeline_items.h"

#include "../../common/game/storySceneEvent.h"

namespace TimelineImpl
{
	class CDrawBuffer;
}

namespace StorySceneEditorUtils
{
	/*
	Wraps string.

	\param result String object to which to store wrapped string.
	\param src String to be wrapped.
	\param maxWidth Max allowed width of string. Must be > 0.
	\param font Font used to display the string.

	It's ok to pass the same string as src and result arguments.

	WrapString() searches for spaces to wrap text between words. If it can't find spaces then it wraps
	text in arbitrary positions. This means that the function will work for any language. However, for
	languages that don't use spaces to separate words the result may not be correct in respect to wrapping
	rules for that language.
	*/
	void WrapString(String& result, const String& src, Uint32 maxWidth, const CFont& font);


	//////////////////////////////////////////////////////////////////////////

	Bool StartStorySceneDebug( const CStoryScene* scene );
	Bool StartStorySceneDebug( const CActor* actor );

	//////////////////////////////////////////////////////////////////////////

	Bool VoiceoverExists( const CStorySceneLine* sceneLine, const String& languageId );
	Uint32 CreateTemporaryLipsync( const CStorySceneSection* section, const String& languageId, Bool createNew, Bool recreateExisting );
	Uint32 DeleteTemporaryLipsync( const CStorySceneSection* section, const String& languageId );

	//////////////////////////////////////////////////////////////////////////

	CEntity* ExtractEntityFromComponent( const CComponent* c );

	//////////////////////////////////////////////////////////////////////////

	void DrawEventsCurve( const SCurveData* c, TimelineImpl::CDrawBuffer& drawBuf, const wxRect& rect, Int32 steps = 10 );
	
	template< typename T >
	Bool DrawDialogEventWithCurve( const DialogTimelineItems::CTimelineItemEvent* item, const CStorySceneEvent* e, CEdTimeline* timeline, const wxRect& rect )
	{
		const T* evt = static_cast< const T* >( e );
		if ( evt && evt->UseWeightCurve() )
		{
			// get draw group appropriate for drawing this item
			Track* track = timeline->GetItemTrack( item );
			TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

			StorySceneEditorUtils::DrawEventsCurve( evt->GetWeightCurve(), *drwGrp->GetDrawBuffer(), rect );

			return true;
		}

		return false;
	}

	template< typename T >
	void DrawDialogEventWithCurveOrLinearBlend( const DialogTimelineItems::CTimelineItemEvent* item, const CStorySceneEvent* e, CEdTimeline* timeline, const wxRect& rect )
	{
		if ( !DrawDialogEventWithCurve< T >( item, e, timeline, rect ) )
		{
			// get draw group appropriate for drawing this item
			Track* track = timeline->GetItemTrack( item );
			TimelineImpl::CDrawGroupTracks* drwGrp = timeline->GetTrackDrawGroup( track );

			drwGrp->GetDrawBuffer()->DrawLine( rect.GetLeft(), rect.GetBottom(), rect.GetRight(), rect.GetTop(), wxColor( 0, 0, 0 ) );
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void FindBodyAnimationData( const CName& animationName, TDynArray< CName >& extraData );
	void FindMimicsAnimationData( const CName& animationName, TDynArray< CName >& extraData );

}
