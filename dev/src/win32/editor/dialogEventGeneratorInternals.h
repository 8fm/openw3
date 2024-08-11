/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "..\..\common\game\storyScenePlayerInterface.h"
#include "..\..\common\game\storySceneCameraDefinition.h"
#include "..\..\common\game\storySceneLine.h"
#include "dialogEditor.h"

class CStorySceneSection;
class CStorySceneElement;

namespace CStorySceneEventGeneratorInternals
{
	struct SectionChunk
	{
		SectionChunk() : m_lengthCache(-1.f)
		{}

		TDynArray<CStorySceneElement*>	m_elements;
		Float	Length() const;
		CName	Speaker() const;
		CName	SpeakingTo() const;
		Bool	IsChoice() const;
		Bool	ContainElement( const CStorySceneElement* el, Uint32& pos ) const;
		void	GetEvents( TDynArray<const CStorySceneEvent*>& events ) const;

		mutable Float m_lengthCache; //must be mutable so Length() can cache result 
	};

	struct PersistenceLayer
	{
		Int32 GenerateUniqueIndex( CName channel, Uint32 outOf );

		void	FilterNames( CName channel, TDynArray<CName> & names );
		void	AddNameToFilter( CName channel, CName name );
		void	ClearNameFilter( CName channel );

		void ClearMemory();

		TArrayMap<CName, TDynArray<Uint32>>		m_usedIndices;
		TArrayMap<CName, TDynArray<CName>>		m_usedNames;
	};

	struct EventChunkPosition
	{
		EventChunkPosition( Float _valRel = 0.f, Float _valAbs = 0.f ) : valRel(_valRel), valAbs( _valAbs ), durationRel(0.f), durationAbs(0.f)
		{}

		Float			valAbs;
		Float			valRel;
		TagList			tags;

		Float			durationRel;
		Float			durationAbs;

		Float PosAbs( SectionChunk& chunk ) const 
		{
			return chunk.Length()*valRel + valAbs;
		}
	};

	struct EventSectionPosition
	{
		EventSectionPosition();
		EventSectionPosition( TDynArray<SectionChunk>& _chunks, const CStorySceneEvent* event  );
		EventSectionPosition( TDynArray<SectionChunk>& _chunks, const CStorySceneElement* element, Float _ChunkAbsTime = 0.f );
		EventSectionPosition( TDynArray<SectionChunk>& _chunks, Uint32 _index, Float _ChunkRelTime, Float _ChunkAbsTime );
		EventSectionPosition( TDynArray<SectionChunk>& _chunks, Float _SecitonRelTime = 0.f, Float _SectionAbsTime = 0.f );

		void operator+=( Float time );
		void operator-=( Float time );
		void Normalize();
		const EventSectionPosition& operator=( const EventSectionPosition&  sec);

		Bool operator>( const EventSectionPosition& sec ) const;
		Bool operator<( const EventSectionPosition& sec ) const;
		Float PosAbs() const;
		Float DistanceTo( const EventSectionPosition& sec ) const;
		Float ToEndOfChunk() const 
		{
			return chunks ? (*chunks)[index].Length() - localPos.PosAbs( (*chunks)[index] ) : -1.f;
		}
		SectionChunk& GetChunk()  const 
		{
			return (*chunks)[index];
		} 
		String ToString()
		{
			return String::Printf( TXT("< i-%d, locV-%f, absV-%f >"), index, localPos.PosAbs( (*chunks)[index] ), PosAbs() );
		}

		EventChunkPosition		 localPos;
		Int32					 index;
		TDynArray<SectionChunk>* chunks;
	};


	struct Context
	{
		CStorySceneSection*			m_section;
		TDynArray<SectionChunk>		m_chunks;

		SectionChunk*				m_currentChunk;
		SectionChunk*				m_lastChunk;
		
		static Uint32 m_sizeOfCamMemory;
		TDynArray<const StorySceneCameraDefinition*>	m_lastUsedCameras;
		TDynArray<StorySceneCameraDefinition>			m_generatedBlendCameras;

		CName						m_lastLookatTarget;

		struct PrevUsedAnim 
		{
			CName animName;
			CName actorName;
			SectionChunk* chunk;
			PrevUsedAnim( CName _animName, CName _actorName, SectionChunk* _chunk ) : animName(_animName), actorName(_actorName), chunk(_chunk)
			{}
			Bool operator==( const PrevUsedAnim & sec) const 
			{
				return 	animName == sec.animName && actorName == sec.actorName && chunk == sec.chunk;
			}
		};

		TDynArray< PrevUsedAnim >		m_lastUsedAnimations;

		Context()
			: m_section( NULL )
			, m_currentChunk( NULL )
			, m_lastChunk( NULL )
		{}

		const StorySceneCameraDefinition * LastLineCamera() 	{ return GetCameraFromHistory( 0 ); }
		const StorySceneCameraDefinition * GetCameraFromHistory( Uint32 index );

		void ProcessSection( CStorySceneSection* section );
		void ProcessChunk( SectionChunk* element );
		void ProcessCamera( const StorySceneCameraDefinition* camera  );
		void MarkAnimationUsed( CName animName, SectionChunk* atChunk, CName actorName );

		Bool IsAnimationAlreadyUsed( CName animName, SectionChunk* atChunk, CName actorName );

		Bool FirstSectionInScene();
		Bool FirstChunkInSection();
		Bool FirstActorOccurence( Bool withThisListener = false );
		

		TagList  m_nextCameraPositionTags;
		TagList  m_prevCameraPositionTags;
	};

	Float CalculateChunksLength( TDynArray<SectionChunk>* chunks );
}
