
#include "build.h"
#include "dialogEditorUtils.h"
#include "dialogEditor.h"
#include "voice.h"

#include "../../common/game/storyScenePlayer.h"
#include "../../common/game/storySceneVoicetagMapping.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/engine/fonts.h"

extern CEdFrame* wxTheFrame;

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

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
	void WrapString(String& result, const String& src, Uint32 maxWidth, const CFont& font)
	{
		ASSERT(maxWidth > 0);

		// we'll work on a copy of src string
		result = src;

		Bool whitespaceFound = true;		// assume true so first iteration works ok
		size_t startPos = 0;				// starting position of current chunk of text
		size_t fitLen = 0;					// length of text from current chunk that fits width
		size_t extLen = 0;					// extended length of text from current chunk - this is to be checked
		Uint32 textLen = src.GetLength();

		// while not whole text is processed, wrap text at whitespaces
		while(startPos + extLen < textLen)
		{
			// search for next whitespace (but don't do it if prev search didn't find any ws)
			size_t whitespacePos = -1;
			if(whitespaceFound)
			{
				// TODO: space is not the only whitespace
				whitespaceFound = result.FindCharacter(TXT(' '), whitespacePos, startPos + fitLen + 1);
			}

			// extend chunk to next whitespace (ws not included) or to the end of the text
			extLen = whitespaceFound? whitespacePos - startPos : textLen - startPos;

			Int32 unused;
			Uint32 width, height;
			font.GetTextRectangle(result.AsChar() + startPos, extLen, unused, unused, width, height);

			if(width <= maxWidth)
			{
				// extended chunk fits, store this state
				fitLen = extLen;
			}
			else if(fitLen > 0)
			{
				// extended chunk doesn't fit, wrap text using last fit state
				result[startPos + fitLen] = TXT('\n');

				// start next chunk
				startPos += fitLen + 1;
				fitLen = 0;
				extLen = 0;
			}
			else
			{
				// extended chunk doesn't fit and we don't have any fit state stored so we
				// stop this loop so we can start wrapping the text in arbitrary positions
				extLen = textLen - startPos;
			}
		}

		if(fitLen > 0)
		{
			// we've processed whole text and current chunk still fits so we're done
			return;
		}

		// now, startPos points to part of the text that doesn't fit and that couldn't be wrapped at whitespaces

		// extLen needs to be zeroed
		extLen = 0;

		// while not whole text fits, wrap text at arbitrary positions
		while(startPos + fitLen < textLen)
		{
			// extend chunk by one char
			++extLen;

			Int32 unused;
			Uint32 width, height;
			font.GetTextRectangle(result.AsChar() + startPos, extLen, unused, unused, width, height);

			if(width <= maxWidth)
			{
				// extended chunk fits, store this state
				fitLen = extLen;
			}
			else if(fitLen > 0)
			{
				// extended chunk doesn't fit, wrap text using last fit state
				result.Insert(startPos + fitLen, TXT('\n'));
				++textLen;

				// start next chunk
				startPos += fitLen + 1;
				fitLen = 0;
				extLen = 0;
			}
			else
			{
				// extended chunk doesn't fit and and we don't have any fit state stored,
				// the chunk is one char only and it doesn't fit - there's not much we can
				// do besides stopping
				fitLen = textLen - startPos;
			}
		}
	}


	//////////////////////////////////////////////////////////////////////////

	Bool StartStorySceneDebug( const CStoryScene* scene )
	{
		return false;
	}

	Bool StartStorySceneDebug( const CActor* actor )
	{
		const TDynArray< CStorySceneController* >& currScenes = actor->GetStoryScenes();

		TDynArray< String > sceneNames;

		Uint32 best = 0;

		for ( Uint32 i=0; i<currScenes.Size(); ++i )
		{
			if ( currScenes[ i ] && currScenes[ i ]->GetPlayer() )
			{
				sceneNames.PushBack( currScenes[ i ]->GetPlayer()->GetName() );

				if ( !currScenes[ i ]->GetPlayer()->IsPaused() && !currScenes[ i ]->IsPaused() )
				{
					best = i;
				}
			}
		}

		if ( sceneNames.Size() > 0 )
		{
			String out = InputComboBox( NULL, TXT("Scene debugger"), TXT("Please choose scene to debug"), sceneNames[ best ], sceneNames );

			Int32 index = (Int32)sceneNames.GetIndex( out );
			if ( index != -1 )
			{
				CStorySceneController* c = currScenes[ index ];
				{
					CEdSceneEditor* editor = new CEdSceneEditor( wxTheFrame, c );
					editor->Center();
					editor->Show();
					editor->SetFocus();

					return true;
				}
			}
		}

		return false;
	}

	//////////////////////////////////////////////////////////////////////////

	/*
	Returns whether voiceover for specified scene line exists.

	\param Scene line to be checked. Must not be nullptr.
	\param languageId Language to be checked.
	\return True if voiceover for scene line exists, otherwise - false.
	*/
	Bool VoiceoverExists( const CStorySceneLine* sceneLine, const String& languageId )
	{
		ASSERT( sceneLine );

		Bool voExists = false;
		String voName = sceneLine->GetVoiceFileName();

		// check if wav voiceover exists
		String voAbsPath = SEdLipsyncCreator::GetInstance().GetWavPath( voName, languageId.ToLower() );
		voExists = GFileManager->FileExist( voAbsPath );

		// check if ogg voiceover exists
		if( !voExists )
		{
			voAbsPath = voAbsPath.StringBefore( TXT( "." ), true ) + TXT( ".ogg" );
			voExists = GFileManager->FileExist( voAbsPath );
		}

		return voExists;
	}

	/*
	Creates/recreates temporary lipsync for all text lines of specified section.

	\param section Section for which temporary lipsync is to be created/recreated. Must not be nullptr.
	\param languageId Language in which to create/recreate temporary lipsync.
	\param createNew Determines whether new temporary lipsync is to be created if it doesn't exist.
	\param recreateExisting Determines whether existing temporary lipsync is to be recreated.
	\return Number of lipsyncs processed (created/recreated).

	This function creates/recreates temporary lipsync only, i.e. lipsync that's created using autogenerated
	temporary voiceover. Temporary voiceover is created when lipsync is needed but no voiceover is available.
	Temporary voiceover is deleted right after lipsync is created so the easiest way to find out whether
	lipsync is temporary or not is to check voiceover existence.
	*/
	Uint32 CreateTemporaryLipsync( const CStorySceneSection* section, const String& languageId, Bool createNew, Bool recreateExisting )
	{
		ASSERT( section );

		EdLipsyncCreator& lipsyncCreator = SEdLipsyncCreator::GetInstance();
		String languageIdLowercase = languageId.ToLower();

		TDynArray< CAbstractStorySceneLine* > sectionLine;
		section->GetLines( sectionLine );

		Uint32 numLipsyncsProcessed = 0;

		for( auto it = sectionLine.Begin(), end = sectionLine.End(); it != end; ++it )
		{
			if( CStorySceneLine* sceneLine = Cast< CStorySceneLine >( *it ) )
			{
				String lineText = sceneLine->GetLocalizedContent()->GetString();
				String voName = sceneLine->GetVoiceFileName();
				Bool voExists = VoiceoverExists( sceneLine, languageIdLowercase );

				if( !voExists )
				{
					// Voiceover doesn't exist. Create temporary voiceover and use it to create new temporary lipsync
					// or recreate existing one. Delete temporary voiceover after use as it's not going to be needed.

					Bool lipsyncExists = GFileManager->FileExist( lipsyncCreator.GetLipsPath( voName, languageIdLowercase ) );
					if( ( !lipsyncExists && createNew ) || ( lipsyncExists && recreateExisting ) )
					{
						Bool tempVoCreated = lipsyncCreator.CreateWav( voName, languageIdLowercase, lineText );
						if( tempVoCreated )
						{
							lipsyncCreator.CreateLipsync( voName, languageIdLowercase, lineText );
							GFileManager->DeleteFile( lipsyncCreator.GetWavPath( voName, languageIdLowercase ) );
							++numLipsyncsProcessed;
						}
					}
				}
			}
		}

		return numLipsyncsProcessed;
	}

	/*
	Deletes temporary lipsync for all text lines of specified section.

	\param section Section for which temporary lipsync is to be deleted. Must not be nullptr.
	\param languageId Language of lipsync to be deleted.
	\return Number of lipsyncs deleted.

	This function deletes temporary lipsync only, i.e. lipsync that's created using autogenerated
	temporary voiceover. Temporary voiceover is created when lipsync is needed but no voiceover is
	available. Temporary voiceover is deleted right after lipsync is created so the easiest way to
	find out whether lipsync is temporary or not is to check voiceover existence.
	*/
	Uint32 DeleteTemporaryLipsync( const CStorySceneSection* section, const String& languageId )
	{
		ASSERT( section );

		EdLipsyncCreator& lipsyncCreator = SEdLipsyncCreator::GetInstance();
		String languageIdLowercase = languageId.ToLower();

		TDynArray< CAbstractStorySceneLine* > sectionLine;
		section->GetLines( sectionLine );

		Uint32 numLipsyncsProcessed = 0;

		for( auto it = sectionLine.Begin(), end = sectionLine.End(); it != end; ++it )
		{
			if( CStorySceneLine* sceneLine = Cast< CStorySceneLine >( *it ) )
			{
				String lineText = sceneLine->GetLocalizedContent()->GetString();
				String voName = sceneLine->GetVoiceFileName();
				Bool voExists = VoiceoverExists( sceneLine, languageIdLowercase );

				if( !voExists )
				{
					// Voiceover doesn't exist. If lipsync exists then delete it as it's temporary.

					String lipsyncPath = lipsyncCreator.GetLipsPath( voName, languageIdLowercase );
					Bool lipsyncExists = GFileManager->FileExist( lipsyncPath );
					if( lipsyncExists )
					{
						GFileManager->DeleteFile( lipsyncPath );
						++numLipsyncsProcessed;
					}
				}
			}
		}

		return numLipsyncsProcessed;
	}

	//////////////////////////////////////////////////////////////////////////

	void DrawEventsCurve( const SCurveData* c, TimelineImpl::CDrawBuffer& drawBuf, const wxRect& rect, Int32 steps )
	{
		if ( steps < 2 )
		{
			return;
		}

		const Int32 l = rect.GetLeft();
		const Int32 r = rect.GetRight();
		const Int32 t = rect.GetTop();
		const Int32 b = rect.GetBottom();

		const Int32 rangeH = r - l;
		SCENE_ASSERT( rangeH > 0 );
		const Int32 rangeV = t - b;
		SCENE_ASSERT( rangeV < 0 );

		Int32 pX = (Int32)((Float)rangeH * 0.f) + l;
		Int32 pY = (Int32)((Float)rangeV * Clamp( c->GetFloatValue( 0.f ), 0.f, 1.f )) + b;

		const Float duration = c->GetMaxTime();
		const Float stepF = duration / (Float)(steps-1);

		for ( Int32 i=0; i<steps; ++i )
		{
			const Float w = (Float)i / (Float)steps;
			const Float f = (Float)i * stepF;

			Int32 x = (Int32)((Float)rangeH * w) + l;
			Int32 y = (Int32)((Float)rangeV * Clamp( c->GetFloatValue( f ), 0.f, 1.f )) + b;

			drawBuf.DrawLine( pX, pY, x, y, wxColor( 0, 0, 0 ) );

			pX = x;
			pY = y;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	CEntity* ExtractEntityFromComponent( const CComponent* c )
	{
		CEntity*  ret = nullptr;
		if ( CItemEntity* item = c->FindParent< CItemEntity >() )
		{
			ret = item->GetItemProxy() ? item->GetItemProxy()->GetParentEntity() : nullptr;
		}
		if ( !ret )
		{
			ret = c->GetEntity();
		}	
		return ret;
	}

	//////////////////////////////////////////////////////////////////////////

	void FindBodyAnimationData( const CName& animationName, TDynArray< CName >& extraData )
	{
		extraData.Resize( 6 );
		extraData[ 5 ] = animationName;

		const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();
		for ( CStorySceneAnimationList::AllBodyAnimationsIterator it( list ); it; ++it )
		{
			const CStorySceneAnimationList::AnimationBodyData& anim = *it;
			if ( anim.m_animationName == animationName )
			{
				extraData[ 0 ] = it.GetStatus();
				extraData[ 1 ] = it.GetEmoState();
				extraData[ 2 ] = it.GetPose();
				extraData[ 3 ] = it.GetTypeName();
				extraData[ 4 ] = CName( anim.m_friendlyName );
				extraData[ 5 ] = anim.m_animationName;
			}
		}
	}

	void FindMimicsAnimationData( const CName& animationName, TDynArray< CName >& extraData )
	{
		extraData.Resize( 3 );
		extraData[ 2 ] = animationName;

		const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();
		for ( CStorySceneAnimationList::AllMimicsAnimationsIterator it( list ); it; ++it )
		{
			const CStorySceneAnimationList::AnimationMimicsData& anim = *it;
			if ( anim.m_animationName == animationName )
			{
				extraData[ 0 ] = it.GetActionType();
				extraData[ 1 ] = anim.m_friendlyNameAsName;
				extraData[ 2 ] = anim.m_animationName;
			}
		}
	}

} // namespace StorySceneUtils

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
