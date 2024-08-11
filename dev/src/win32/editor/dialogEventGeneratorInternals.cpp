/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dialogEventGeneratorInternals.h"
#include "..\..\common\game\storySceneLine.h"
#include "..\..\common\game\storySceneSection.h"
#include "..\..\common\game\storyScene.h"
#include "..\..\common\game\storySceneInput.h"
#include "..\..\common\game\storySceneChoice.h"
#include "..\..\common\game\storySceneControlPartsUtil.h"
#include "..\..\common\game\storyScenePauseElement.h"
#include "..\..\common\engine\localizationManager.h"

namespace CStorySceneEventGeneratorInternals
{
	Float SectionChunk::Length() const
	{
		if( m_lengthCache < 0.f )
		{
			const String& currentLocale = SLocalizationManager::GetInstance().GetCurrentLocale();
			Float length = 0;
			for( Uint32 i = 0; i < m_elements.Size(); ++i )
			{
				length += m_elements[i]->CalculateDuration( currentLocale );
			}
			m_lengthCache = length;
		}
		
		return m_lengthCache;
	}

	CName SectionChunk::Speaker() const
	{
		for ( Uint32 i = 0; i < m_elements.Size(); i++ )
		{
			CStorySceneLine* line = Cast<CStorySceneLine>(m_elements[i]);
			if( line )
			{
				return line->GetVoiceTag();
			}			 
		}
		return CName::NONE;
	}

	Bool SectionChunk::IsChoice() const
	{
		if( m_elements.Size() > 0 )
		{
			return Cast<CStorySceneChoice>(m_elements[0]) != NULL;	 
		}
		return false;
	}


	CName SectionChunk::SpeakingTo() const
	{
		for ( Uint32 i = 0; i < m_elements.Size(); i++ )
		{
			CStorySceneLine* line = Cast<CStorySceneLine>(m_elements[i]);
			if( line )
			{
				return line->GetSpeakingTo();
			}			 
		}
		return CName::NONE;
	}

	Bool SectionChunk::ContainElement( const CStorySceneElement* el, Uint32& pos ) const
	{
		for( Uint32 i = 0; i < m_elements.Size(); ++i )
		{
			if( m_elements[i] == el )
			{
				pos = i;
				return true;
			}
		}
		pos = -1;
		return false;
	}

	/*
	Gets section events (those that belong to section variant that is associated with current locale id).
	*/
	void SectionChunk::GetEvents( TDynArray<const CStorySceneEvent*>& events ) const
	{
		for( Uint32 i = 0; i < m_elements.Size(); ++i )
		{
			CStorySceneSection* section = m_elements[i]->GetSection(); // TODO: why do we do this for each element? They all belong to the same section right? So it's enough to do this one time before loop.
			if( section )
			{
				const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
				CStorySceneSectionVariantId sectionVariantId = section->GetVariantUsedByLocale( currentLocaleId );

				section->GetEventsForElement( events, m_elements[i], sectionVariantId );
			}
		}
	}

	void Context::ProcessChunk( SectionChunk* element )
	{
		m_lastChunk = m_currentChunk;
		m_currentChunk = element ;
	}

	void Context::ProcessSection( CStorySceneSection* section )
	{
		m_section = section;
	}


	void Context::ProcessCamera( const StorySceneCameraDefinition* camera )
	{
		m_lastUsedCameras.Insert( 0, camera );
		if( m_lastUsedCameras.Size() > m_sizeOfCamMemory )
		{
			m_lastUsedCameras.PopBack();
		}		
	}

	Bool Context::FirstSectionInScene()
	{		
		TDynArray< const CStorySceneLinkElement* > stack;
		TDynArray< const CStorySceneLinkElement* > visited;
		TDynArray< const CStorySceneLinkElement* > linkedElements;
		stack.PushBack( m_section );
		const CStorySceneInput* result = nullptr;

		while( !result && !stack.Empty() )
		{	
			const CStorySceneLinkElement* link = stack.PopBack();
			result = Cast<const CStorySceneInput>( link );
			if ( !result )
			{
				linkedElements.ClearFast();
				StorySceneControlPartUtils::GetPrevConnectedElements( link, linkedElements );
				for ( Int32 i = 0; i < linkedElements.SizeInt(); ++i )
				{					
					if ( linkedElements[i] && ! Cast<const CStorySceneSection>( linkedElements[i] ) && !visited.Exist( linkedElements[i] ) )
					{
						visited.PushBack( linkedElements[i] );
						stack.PushBack( linkedElements[i] );
					}				
				}
			}		
		}

		return result != NULL;
	}

	Bool Context::FirstChunkInSection()
	{	
		if( m_currentChunk && !m_currentChunk->m_elements.Empty() )
		{
			for( Uint32 i = 0; i < m_section->GetNumberOfElements(); ++i )		
			{
				CStorySceneLine* line = Cast<CStorySceneLine>( m_section->GetElement( i ) );
				if( line )
				{
					if( line == m_currentChunk->m_elements[0] )
					{
						return true;
					}
					else
					{
						return false;
					}
				}
			}
		}		
		return false;
	}

	Bool Context::FirstActorOccurence( Bool withThisListener )
	{
		if( !m_section || !m_currentChunk )
		{
			return false;
		}
		TDynArray< CStorySceneLinkElement* > queue;
		TDynArray< CStorySceneInput* > inputs;
		CName speaker = m_currentChunk->Speaker();
		m_section->GetScene()->CollectControlParts< CStorySceneInput >( inputs );
		CStorySceneLinkElement* link = Cast<CStorySceneLinkElement>( m_section );
		for( Uint32 i = 0; i < inputs.Size(); ++i  )
		{
			queue.PushBack( inputs[i] );
		}
		while ( !queue.Empty() )
		{
			CStorySceneLinkElement* link = queue.Back();
			CStorySceneSection* section = Cast<CStorySceneSection>( link );
			queue.PopBack();		
			Bool exist = false;
			if( section )
			{
				Uint32 num = section->GetNumberOfElements();
				for( Uint32 i = 0; i < num; ++i )
				{
					const CStorySceneLine * line = Cast<CStorySceneLine>( section->GetElement( i ) );
					if( section == m_section && !m_currentChunk->m_elements.Empty() && line == m_currentChunk->m_elements[0] )
					{
						return true;
					}
					if( line && line->GetVoiceTag() == m_currentChunk->Speaker() && ( !withThisListener || line->GetSpeakingTo() == m_currentChunk->SpeakingTo() ) )
					{
						exist = true;
					}
				}	
			}
			if( ! exist && link )
			{
				queue.PushBack( link->GetNextElement() );
			}							
		}
		return false;
	}


	const StorySceneCameraDefinition * Context::GetCameraFromHistory( Uint32 index )
	{
		if( index >= 0 && index < m_lastUsedCameras.Size() )
		{
			return m_lastUsedCameras[index];
		}
		return NULL;
	}

	void Context::MarkAnimationUsed( CName animName, SectionChunk* atChunk, CName actorName )
	{
		if ( animName && actorName && atChunk )
		{
			m_lastUsedAnimations.PushBackUnique( PrevUsedAnim( animName, actorName, atChunk ) );
		}		
	}

	Bool Context::IsAnimationAlreadyUsed( CName animName, SectionChunk* atChunk, CName actorName )
	{
		return m_lastUsedAnimations.Exist( PrevUsedAnim( animName, actorName, atChunk ) );
	}

	Uint32 Context::m_sizeOfCamMemory = 5;





	//===================================================================================================

	Int32 PersistenceLayer::GenerateUniqueIndex( CName channel, Uint32 outOf )
	{
		TArrayMap<CName,TDynArray<Uint32>>::iterator usedIndices = m_usedIndices.Find( channel );
		if( usedIndices == m_usedIndices.End() )
		{
			usedIndices = m_usedIndices.Insert( channel, TDynArray<Uint32>() );
		}
		for( Uint32 i = outOf ; i >= 0; --i )
		{
			if( !usedIndices->m_second.Exist( i ) && GEngine->GetRandomNumberGenerator().Get< Uint32 >( i - usedIndices->m_second.Size() ) < 1 )
			{		
				usedIndices->m_second.PushBack( i );
				return i;
			}
		}		
		return -1;
	}

	void PersistenceLayer::ClearMemory()
	{
		m_usedIndices.Clear();
		m_usedNames.Clear();
	}

	void PersistenceLayer::ClearNameFilter( CName channel )
	{
		TArrayMap<CName,TDynArray<CName>>::iterator filterNames = m_usedNames.Find( channel );
		if( filterNames != m_usedNames.End() )
		{
			filterNames->m_second.Clear();
		}
	}

	void PersistenceLayer::AddNameToFilter( CName channel, CName name )
	{
		TArrayMap<CName,TDynArray<CName>>::iterator filterNames = m_usedNames.Find( channel );
		if( filterNames == m_usedNames.End() )
		{
			filterNames = m_usedNames.Insert( channel, TDynArray<CName>() );
		}
		filterNames->m_second.PushBack(name);
	}

	void PersistenceLayer::FilterNames( CName channel, TDynArray<CName> & names )
	{
		TArrayMap<CName,TDynArray<CName>>::iterator filterNames = m_usedNames.Find( channel );
		if( filterNames == m_usedNames.End() )
		{
			return;
		}

		for( Uint32 i = 0; i < filterNames->m_second.Size(); ++i )
		{
			names.RemoveFast( filterNames->m_second[i] );
		}

	}

	void EventSectionPosition::Normalize()
	{
		if ( !chunks || index == -1  )
		{
			return;
		}
		localPos.valAbs = localPos.PosAbs( (*chunks)[ index ] );
		for ( ; (*chunks)[ index ].Length() < localPos.valAbs && index < (*chunks).SizeInt() - 1 ; index++ )
		{
			localPos.valAbs -= (*chunks)[ index ].Length();		
		}
	}


	EventSectionPosition::EventSectionPosition( TDynArray<SectionChunk>& _chunks, Uint32 _index, Float _ChunkRelTime, Float _ChunkAbsTime )
		: localPos(_ChunkRelTime,  _ChunkAbsTime ), index( _index ) , chunks( &_chunks )
	{
		Normalize();
	}

	EventSectionPosition::EventSectionPosition( TDynArray<SectionChunk>& _chunks, Float _SecitonRelTime /*= 0.f*/, Float _SectionAbsTime /*= 0.f */ )
		: chunks( &_chunks )
	{
		Float sectionLength = CalculateChunksLength( chunks );
		Float sectionTime = _SecitonRelTime*sectionLength + _SectionAbsTime;
		Int32 i = 0;
		for( ; i < chunks->SizeInt()-1 && sectionTime - (*chunks)[i].Length() > 0.f ; ++i )
		{
			sectionTime -= (*chunks)[i].Length();
		}
		localPos = EventChunkPosition( 0, sectionTime );
		index = i;
	}


	EventSectionPosition::EventSectionPosition( TDynArray<SectionChunk>& _chunks, const CStorySceneEvent* event )
		: chunks( &_chunks )
	{
		const String& currentLocale = SLocalizationManager::GetInstance().GetCurrentLocale();

		CStorySceneElement* el = event->GetSceneElement();
		for( Uint32 i = 0; i < chunks->Size() ; ++i )
		{
			Uint32 pos;
			if( (*chunks)[i].ContainElement( el, pos ) )
			{
				index = i;
				for ( Uint32 j = 0; j < pos ; ++j )
				{
					localPos.valAbs += (*chunks)[i].m_elements[j]->CalculateDuration( currentLocale );
				}
				localPos.valAbs += event->GetStartPosition() * el->CalculateDuration( currentLocale );
				return;
			}
		}
	}


	EventSectionPosition::EventSectionPosition( TDynArray<SectionChunk>& _chunks, const CStorySceneElement* element, Float _ChunkAbsTime )
		: chunks( &_chunks )
	{
		const String& currentLocale = SLocalizationManager::GetInstance().GetCurrentLocale();

		for( Uint32 i = 0; i < chunks->Size() ; ++i )
		{
			Uint32 pos;
			if( (*chunks)[i].ContainElement( element, pos ) )
			{	
				index = i;
				for ( Uint32 j = 0; j < pos ; ++j )
				{
					localPos.valAbs += (*chunks)[i].m_elements[j]->CalculateDuration( currentLocale );
				}
				localPos.valAbs += _ChunkAbsTime;
				Normalize();
				return;
			}
		}
	}


	EventSectionPosition::EventSectionPosition()
		: chunks( NULL ), index(-1)
	{}

	void EventSectionPosition::operator+=( Float time )
	{
		if( !chunks || index == -1 )
		{
			return;
		}
		Float length = (*chunks)[index].Length();	
		Float toEndOfChunk = length - (localPos.valAbs + localPos.valRel*length );
		if( time < toEndOfChunk )
		{
			localPos.valAbs += time;
			return;
		}
		if( index == chunks->Size() -1 )
		{
			localPos.valAbs += time;
		}
		else
		{
			time -= toEndOfChunk;
			index++;
			for( ; index < chunks->SizeInt() && time - (*chunks)[index].Length() > 0 ; ++index )
			{			

				time -= (*chunks)[index].Length();
			}	
			if( index == chunks->Size() )
			{
				index--;
			}
			localPos = EventChunkPosition( 0, time );
		}
	}

	void EventSectionPosition::operator-=( Float time )
	{
		if( !chunks || index == -1)
		{
			return;
		}
		Float length = (*chunks)[index].Length();	
		Float absPos = localPos.valAbs + localPos.valRel*length;
		if( time < absPos )
		{
			localPos.valAbs -= time;
			return;
		}
		if( index == 0 )
		{
			localPos.valAbs -= time;
		}
		else
		{
			time -= absPos;
			index--;
			for( ; index >= 0 && time - (*chunks)[index].Length() > 0 ; --index )
			{			
				time -= (*chunks)[index].Length();
			}	
			if( index == -1 )
			{
				index++;
			}
			localPos = EventChunkPosition( 0, (*chunks)[index].Length() - time );
		}
	}

	Float EventSectionPosition::PosAbs() const
	{
		Float sectionTime = 0.f;
		for( Int32 i = 0; i < index ; ++i )
		{
			sectionTime += (*chunks)[i].Length();
		}
		sectionTime += localPos.PosAbs( (*chunks)[index] );
		return sectionTime;
	}

	Float EventSectionPosition::DistanceTo( const EventSectionPosition& sec ) const
	{
		return sec.PosAbs() - PosAbs();
	}

	Bool EventSectionPosition::operator>( const EventSectionPosition& sec ) const
	{
		if( index == sec.index && chunks && sec.chunks )
		{
			return localPos.PosAbs( (*chunks)[index] ) > sec.localPos.PosAbs( (*sec.chunks)[sec.index] );
		}
		return index > sec.index;
	}


	Bool EventSectionPosition::operator<( const EventSectionPosition& sec ) const
	{
		return sec > *this;
	}

	

	Float CalculateChunksLength( TDynArray<SectionChunk>* chunks )
	{
		if( !chunks )
		{
			return 1.f;
		}
		Float sectionLength = 0.f;
		for( Uint32 i = 0; i < chunks->Size(); ++i )
		{
			sectionLength += (*chunks)[i].Length();
		}
		return sectionLength;
	}

	const EventSectionPosition& EventSectionPosition::operator=( const EventSectionPosition& sec )
	{
		localPos = sec.localPos; 
		index    = sec.index;
		chunks	 = sec.chunks;
		return *this;
	}
}

