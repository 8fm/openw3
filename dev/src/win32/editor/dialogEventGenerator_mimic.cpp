#include "build.h"
#include "dialogEventGenerator.h"

#include "dialogEventGeneratorInternals.h"
#include "dialogEventGeneratorSetupDialog.h"
#include "..\..\common\game\storySceneEventMimicsAnim.h"
#include "..\..\common\game\storySceneAnimationList.h"
#include "..\..\common\game\storySceneSystem.h"
#include "..\..\common\game\storySceneEventMimics.h"
#include "..\..\common\engine\localizationManager.h"
#include "../../common/engine/mimicComponent.h"

CName CStorySceneEventGenerator::GetSentenceInfoAt( SContext& context, const SPositionInSection& atPos, CName actorName )
{
	const SSectionChunk& chunk = atPos.GetChunk();
	CStorySceneElement* element = nullptr;
	Float pos;
	GetEventPosition( atPos, element, pos );
	CAbstractStorySceneLine* line = Cast< CAbstractStorySceneLine >( element );	
	CName res = CNAME( Listen );
	if ( line )
	{
		String content = line->GetContent();
		Int32 index = Int32( pos*content.GetLength() );
		for ( Uint32 i = index; i < content.GetLength(); i++)
		{
			if ( content[i] == TXT('!') )
			{		
				res = chunk.Speaker() == actorName ? CNAME( Shout ) : CNAME( Reaction );
				break;
			} 
			else if ( content[i] == TXT('?') )
			{
				res = chunk.Speaker() == actorName ? CNAME( Question ) : CNAME( Listen );
				break;
			}
			else if ( content[i] == TXT('.') || content[i] == TXT(';') )
			{
				res = chunk.Speaker() == actorName ? CNAME( Talk ) : CNAME( Listen );
				break;
			}
		}
	}
	return res;
}

void CStorySceneEventGenerator::GetSentenceInfos( SContext& context, const CStorySceneLine* line, TDynArray< TPair< Float, CName > >&  outData )
{
	if ( !line )
	{
		return;
	}
	CName actorName = line->GetVoiceTag();
	String content = line->GetContent();

	Bool newSentence = true;
	for ( Uint32 i = 0; i < content.GetLength(); i++ )
	{
		if ( newSentence )
		{
			outData.PushBack( TPair< Float, CName >( Float(i)/Float( content.GetLength() ), CName::NONE ) );
			newSentence = false;
		}
		if ( content[i] == TXT('!') )
		{		
			outData.Back().m_second = CNAME( Shout );
			newSentence = true;
		} 
		else if ( content[i] == TXT('?') )
		{
			outData.Back().m_second = CNAME( Question );
			newSentence = true;
		}
		else if ( content[i] == TXT('.') || content[i] == TXT(';') )
		{
			newSentence = true;
		}
	}
}

namespace
{
	struct CDataComparer
	{
		Bool m_sortByPos;
		CDataComparer( Bool sortByPos ) : m_sortByPos( sortByPos )
		{}
		Bool operator()( const TPair< CStorySceneEventGeneratorInternals::EventSectionPosition, Float >& x, const TPair< CStorySceneEventGeneratorInternals::EventSectionPosition, Float >& y ) const
		{
			return m_sortByPos ? x.m_first < y.m_first :  x.m_second > y.m_second;
		}
	};
}

void CStorySceneEventGenerator::GenerateMimicEvents( SContext& context )
{
	TDynArray<SSectionChunk>& chunks = context.m_chunks;
	CStorySceneSection* section = context.m_section;
	Uint32 nrOfElem = section->GetNumberOfElements();
	if ( nrOfElem == 0 || chunks.Empty() ) //empty section ignore
	{
		return;
	}
	
	MarkExistingAsUsed<CStorySceneEventMimicsAnim>( context );
	TDynArray< CStorySceneLine* > lines;
	TDynArray< CName >		actors;
	TDynArray< Int32 >		linesForActor;

	String debug;	
	AnimData animData;

	const String& currentLocale = SLocalizationManager::GetInstance().GetCurrentLocale();

	for ( Uint32 k = 0; k < chunks.Size(); k++ )
	{
		const SPositionInSection from( chunks, k, 0.f, 0.f );
		const SPositionInSection to( chunks, k, 0.f, from.GetChunk().Length() ); 

		CName actor = chunks[k].Speaker();
		Uint32 nrOfElem = chunks[k].m_elements.Size();
		for ( Uint32 j = 0; j < nrOfElem; ++j )
		{
			CStorySceneLine*	line	= Cast<CStorySceneLine>( chunks[k].m_elements[j] );		
			if ( !line )
			{
				continue;
			}
			Float lineDuration = line->CalculateDuration( currentLocale );
			//collect data for change idle event
			lines.PushBack( line);
			Int32 i = actors.GetIndex( line->GetVoiceTag() );
			if( i != -1 )
			{
				linesForActor[i]++;
			}
			else
			{
				actors.PushBack( line->GetVoiceTag() );
				linesForActor.PushBack( 1 );
			}

			//animations based on tag snapped to start of sentence			
			TDynArray< TPair< Float, CName > >  textData;
			GetSentenceInfos( context, line, textData );
			for ( Uint32 i = 0; i < textData.Size(); ++i )
			{
				if( textData[i].m_second != CName::NONE )
				{
					SPositionInSection at( chunks, line, textData[i].m_first * lineDuration );
					SPositionInSection fromCopy( from ), toCopy( to );						
					if( IsSpaceAvailable< CStorySceneEventMimicsAnim >( context, at, actor, fromCopy, toCopy, true ) )
					{
						at -= lineDuration * textData[i].m_first;
						Float animDuration = FindMimicAnimation( context, at, fromCopy, toCopy, actor, animData, textData[i].m_second );						
						animData.blendTimeIn = 0.25f;
						animData.blendTimeOut = 0.4f;
						context.MarkAnimationUsed( animData.animName, &at.GetChunk(), actor );	
						CreateAnimationEvent( context,actor,animData,at,debug );
					}					
				}			
			}
		}


		TDynArray< TPair<SPositionInSection,SPositionInSection> >	 emptySpaces;
		CollectEmptySpaces< CStorySceneEventMimicsAnim >( context, actor, emptySpaces );						
		TDynArray< TDynArray< TPair< SPositionInSection, Float > > > markersForSpaces;
		markersForSpaces.Resize( emptySpaces.Size() );			
		for ( Uint32 j = 0; j < nrOfElem; j++)
		{
			CStorySceneLine* line = Cast<CStorySceneLine>( chunks[k].m_elements[j] );		
			if ( !line )
			{
				continue;
			}
			TDynArray< TPair< Float, Float > > voiceData;
			m_editor->OnEvtGenerator_GetVoiceDataPositions( line, &voiceData );

			for ( Uint32 i = 0; i < voiceData.Size(); i++)
			{
				SPositionInSection markerInSection( chunks, line, voiceData[i].m_first * line->CalculateDuration( currentLocale ) );					
				for ( Uint32 l = 0; l < markersForSpaces.Size(); l++)
				{
					if ( emptySpaces[l].m_first < markerInSection && emptySpaces[l].m_second > markerInSection )
					{
						markersForSpaces[l].PushBack( MakePair( markerInSection, voiceData[i].m_second ) );		
						break;
					}
				}						
			}
		}
		
		//animations based on voice markers 
		for ( Uint32 i = 0; i < emptySpaces.Size(); i++)
		{
			
			const SPositionInSection fromClip = from > emptySpaces[i].m_first ? from : emptySpaces[i].m_first;
			const SPositionInSection toClip   = to  <  emptySpaces[i].m_second ? to : emptySpaces[i].m_second;
			
			Sort( markersForSpaces[i].Begin(), markersForSpaces[i].End() , ::CDataComparer( false ) );

			Bool browMimicSimple = markersForSpaces[i].Size() < 3 || GEngine->GetRandomNumberGenerator().Get<Uint32>( 4 ) == 0;			
			for ( Uint32 j = 0; j < markersForSpaces[i].Size(); j++)
			{
				SPositionInSection fromCopy( fromClip ), toCopy( toClip ), at( markersForSpaces[i][j].m_first );
				if( IsSpaceAvailable< CStorySceneEventMimicsAnim >( context, at, actor, fromCopy, toCopy, true ) )
				{	
					SPositionInSection at2 = at;
					CName tag = GetSentenceInfoAt( context, at, actor );								
					//head anim
					Float animDuration = FindMimicAnimation( context, at, fromCopy, toCopy, actor, animData, CNAME( AccentsHead ) );
					Float offset = animData.markerPosition != 0.f ? animData.markerPosition / animData.animSpeed  : animData.blendTimeIn ; 
					debug = String::Printf( TXT("MarkVal-%f, offset-%f, From%s To%s At%s "), markersForSpaces[i][j].m_second, offset, fromCopy.ToString().AsChar(), toCopy.ToString().AsChar(), at.ToString().AsChar() );
					at -= offset;
					animData.blendTimeOut = animData.blendTimeIn = 0.2f;
					context.MarkAnimationUsed( animData.animName, &at.GetChunk(), actor );	
					CreateAnimationEvent( context,actor,animData,at, debug );

					if ( browMimicSimple ) // brow simple 25%
					{
						animDuration = FindMimicAnimation( context, at2, fromCopy, toCopy, actor, animData, CNAME( AccentsBrow ) );
						offset = animData.markerPosition != 0.f ? animData.markerPosition / animData.animSpeed  : animData.blendTimeIn ; 
						at2 -= offset;
						context.MarkAnimationUsed( animData.animName, &at2.GetChunk(), actor );	
						animData.blendTimeIn = 0.25f;
						animData.blendTimeOut = 0.4f;
						animData.trackName += TXT("2");//brow anim on second mimic track
						CreateAnimationEvent( context,actor,animData,at2, debug );			
					}										
				}
			}			
			if ( !browMimicSimple ) // brow adv 75%
			{
				Sort( markersForSpaces[i].Begin(), markersForSpaces[i].End() , ::CDataComparer( true ) );
				Int32 availableSlots = markersForSpaces[i].SizeInt(), lastEndInd = 0;
				while ( availableSlots >= 3 )
				{
					Uint32 beginInd =  GEngine->GetRandomNumberGenerator().Get<Uint32>( Min( availableSlots - 2 , 4 ) ) , endInd = beginInd + 2;
					SPositionInSection fromCopy( from ), toCopy( to ), begin( markersForSpaces[i][ lastEndInd + beginInd ].m_first ), end( markersForSpaces[i][ lastEndInd + endInd].m_first ) ;
					Float animDuration = FindMimicAnimation( context, begin, fromCopy, toCopy, actor, animData, CNAME( AccentsBrow ) );
					animData.animSpeed = animDuration / Abs( begin.DistanceTo( end ) );
					context.MarkAnimationUsed( animData.animName, &fromCopy.GetChunk(), actor );	
					animData.blendTimeIn = 0.25f;
					animData.blendTimeOut = 0.4f;
					animData.trackName += TXT("2");//brow anim on second mimic track
					CreateAnimationEvent( context, actor, animData, begin, debug );		
					lastEndInd += endInd;
					availableSlots -= endInd;
				}
			}							

		}		
	}

	// change mimic event 
	TDynArray< TPair< Float, Float > > data;
	for ( Uint32 i = 0; i < lines.Size() ; i++)
	{
		Int32 index = actors.GetIndex( lines[i]->GetVoiceTag() );
		if( index == -1  ||  linesForActor[index] <= 0)
		{
			continue;
		}
		
		if( ( !GEngine->GetRandomNumberGenerator().Get<Uint32>( linesForActor[index] ) ) && m_editor->OnEvtGenerator_GetVoiceDataPositions( lines[i], &data ) && data.Size() > 0 )
		{
			Float eventDuration = 1.f;
			actors[ index ] = CName::NONE;
			Float lineDuration = lines[i]->CalculateDuration( currentLocale );
			SPositionInSection from( chunks, lines[i], data[0].m_first * lineDuration - eventDuration );						
			ChangeMimicIdle( context, from, lines[i]->GetVoiceTag() );
		}
		linesForActor[index]--;
	}
}


Float CStorySceneEventGenerator::FindMimicAnimation( SContext& context, const SPositionInSection & atPos, const SPositionInSection & fromPos, const SPositionInSection & toPos,  CName actorName, AnimData& out, CName tag )
{
	CActor* actor = m_editor->OnEvtGenerator_GetSceneActor( actorName );
	Float animDuration = 1.f;
	if( !actor )
	{
		return animDuration;
	}
	CAnimatedComponent* animatedComponent  = actor->GetMimicComponent();
	if( !animatedComponent )
	{
		return animDuration; 
	}
	CSkeletalAnimationContainer* cont = animatedComponent->GetAnimationContainer();
	if ( !cont )
	{
		return animDuration;
	}
	const CStorySceneAnimationList& list = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList();

	TDynArray< AnimData > availableAnim;
	Float timeBefore = fromPos.DistanceTo( atPos );
	Float timeAfter = atPos.DistanceTo( toPos );
	AnimSearchContext searchContext( availableAnim, actorName, atPos.GetChunk(), cont, CName::NONE, false, timeBefore, timeAfter ) ;
	for ( CStorySceneAnimationList::MimicsAnimationIteratorByAction it( list, tag ); it; ++it )
	{			
		AnimSearchData searchData( *it, tag );
		ProcessAnimationEntry( context, searchContext, searchData );
	}

	//chose one events from selected 
	const CSkeletalAnimationSetEntry* anim = nullptr;
	if ( availableAnim.Size() > 0)
	{	
		out = availableAnim[ GEngine->GetRandomNumberGenerator().Get< Uint32 >( availableAnim.Size() ) ];
		return out.animDuration;	
	}
	out = AnimData();
	return out.animDuration;
}


void CStorySceneEventGenerator::ChangeMimicIdle( SContext& context, const SPositionInSection& at, CName actorName )
{	
	String trackName = actorName.AsString() + TXT(".mimics2");

	const CStorySceneDialogsetInstance* dialogset = context.m_section->GetScene()->GetFirstDialogsetAtSection( context.m_section );
	if ( !dialogset )
	{
		return;								
	}
	const CStorySceneDialogsetSlot* slot = dialogset->GetSlotByActorName( actorName );
	if ( !slot )
	{
		return;
	}

	Float poseWeight = slot->GetMimicsPoseWeight();

	CStorySceneElement* element = NULL;
	Float pos = 0.f;
	GetEventPosition( at, element, pos );

	if( element )
	{
		CStorySceneEventMimics* event = new CStorySceneEventMimics( String::EMPTY, element, pos, actorName, trackName );
		TDynArray<CName>	data;
		Float				poseWeight = event->GetMimicPoseWeight();
		m_editor->OnEvtGenerator_GetMimicIdleData( data, poseWeight, actorName );
		Float diff = GEngine->GetRandomNumberGenerator().Get< Float >( 0.25f, 0.35f );
		Float sign = 0.f;
		if( poseWeight > 1.f - diff )
		{
			sign = -1.f;
		}
		else if ( poseWeight < 0.15f + diff )
		{
			sign = 1.f;
		}
		else
		{
			GEngine->GetRandomNumberGenerator().Get< Uint16 >( 2 ) ? -1 : 1;
		}			
		event->SetMimicData( data  );
		event->SetMimicPoseWeight( poseWeight + ( diff * sign ) );

		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId sectionVariantId = context.m_section->GetVariantUsedByLocale( currentLocaleId );

		context.m_section->AddEvent( event, sectionVariantId );
	}
}
