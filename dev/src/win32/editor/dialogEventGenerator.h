/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dialogEventGeneratorInternals.h"
#include "dialogEventGeneratorRuleFactory.h"
#include "dialogEventGeneratorSetupDialog.h"
#include "..\..\common\game\storySceneAnimationList.h"

class CStorySceneSection;
class CStoryScenePlayer;
struct CDialogEventGeneratorConfig;

RED_DEFINE_STATIC_NAME( Reaction );
RED_DEFINE_STATIC_NAME( Listen );
RED_DEFINE_STATIC_NAME( Shout );
RED_DEFINE_STATIC_NAME( Question );
RED_DEFINE_STATIC_NAME( Talk );
RED_DEFINE_STATIC_NAME( Observer );
RED_DEFINE_STATIC_NAME( AccentsBrow );
RED_DEFINE_STATIC_NAME( AccentsHead );

class CStorySceneEventGenerator
{
protected:

	typedef CStorySceneEventGeneratorInternals::Context					SContext;
	typedef CStorySceneEventGeneratorInternals::EventChunkPosition		SPositionInChunk;
	typedef CStorySceneEventGeneratorInternals::EventSectionPosition	SPositionInSection;
	typedef CStorySceneEventGeneratorInternals::SectionChunk			SSectionChunk;


	struct AnimData
	{
		enum EEventType
		{
			Animation,
			Additive,
			Mimic,
		};

		AnimData() : animDuration( 1.f ), markerPosition( 0.f ), animWeight( 1.f ),	animSpeed(1.f), trackName( TXT(".animations") ), blendTimeIn( 0.f ), blendTimeOut( 0.f )
		{}
		AnimData( CName	_animName, EEventType _type, Float _weight, Float _speed, String _friendlyName, Float _markerPosition = 0.f )
			: animName( _animName ), animDuration( 1.f ), markerPosition( _markerPosition ), animWeight( _weight ), animSpeed( _speed ), type( _type ), blendTimeIn( 0.f ), blendTimeOut( 0.f ), friendlyName( _friendlyName )
		{
			trackName = type == Additive ? TXT(".additives") : type == Mimic ? TXT(".mimics") : TXT(".animations");
		}

		Float	animDuration;		
		Float	markerPosition;
		Float	animWeight;
		Float	animSpeed;
		Float	blendTimeIn;
		Float	blendTimeOut;
		CName	animName;
		String	trackName;
		String  friendlyName;
		EEventType	type;
		TDynArray< CName > additionalData;
	};

	struct AnimSearchContext
	{
		AnimSearchContext( TDynArray< AnimData >& _availableAnim, CName _actorName, SSectionChunk& _atChunk,  CSkeletalAnimationContainer* _cont, CName _tag, Bool _mustHaveMarker, Float _timeBefore, Float _timeAfter ) 
			: availableAnim( _availableAnim ), cont( _cont ), actorName( _actorName ), animTag( _tag ), mustHaveMarker( _mustHaveMarker ), timeAfter( _timeAfter ), timeBefore( _timeBefore ), atChunk( _atChunk )
		{}
		TDynArray< AnimData >& availableAnim;
		CSkeletalAnimationContainer* cont;
		CName animTag;		
		CName actorName;
		SSectionChunk& atChunk;
		Bool mustHaveMarker;
		Float timeAfter;
		Float timeBefore;
	};

	struct AnimSearchData
	{
		AnimSearchData( const CStorySceneAnimationList::AnimationBodyData& bodyData, AnimData::EEventType _eventType, const SStorySceneActorAnimationState& state, CName animType )
			: animationName( bodyData.m_animationName ), generatorData( bodyData.m_generatorData ), friendlyName( bodyData.m_friendlyName ), eventType( _eventType )
		{
			additionalData.PushBack( state.m_status );
			additionalData.PushBack( state.m_emotionalState );
			additionalData.PushBack( state.m_poseType );
			additionalData.PushBack( animType );
		}

		AnimSearchData( const CStorySceneAnimationList::AnimationMimicsData& mimicData, CName mimicTag )
			: animationName( mimicData.m_animationName ), generatorData( mimicData.m_generatorData ), friendlyName( mimicData.m_friendlyName ), eventType( AnimData::Mimic )
		{
			additionalData.PushBack( mimicTag );
		}

		CName animationName;
		const String& friendlyName;
		AnimData::EEventType eventType;
		const SStorySceneEventGeneratorAnimationState& generatorData;
		TDynArray<CName> additionalData;		
	};



	struct CameraData
	{
		CameraData( const StorySceneCameraDefinition* data, Float _weight, String& debug, Int32 _quality = 0 ) : carriedData(data) , weight(_weight), debugString(debug), quality(_quality)
		{}
		CameraData()
		{}

		const StorySceneCameraDefinition*	carriedData;
		Float		weight;
		String		debugString;
		Int32		quality;
	};

	struct LookatData
	{
		LookatData( CName _target, const String& track, Float _speed = 0.f ) : target(_target), speed( _speed ), timelineTrack( track)
		{}

		CName			target;
		Float			speed;
		String			timelineTrack;
	};

	const CDialogEventGeneratorConfig& m_config;
	CEdSceneEditor* m_editor;
public:
	CStorySceneEventGenerator( CEdSceneEditor* editor, const CDialogEventGeneratorConfig & config );
	~CStorySceneEventGenerator();


	void GenerateEvents( CStorySceneSection* section );

protected:

	void GetEventPosition( const SSectionChunk* chunk, const SPositionInChunk& pos, CStorySceneElement*& outElement, Float& outPosInElement );
	void GetEventPosition( const SPositionInSection& pos, CStorySceneElement*& outElement, Float& outPosInElement );

	const CameraData* GetRandomElement( const TDynArray< CameraData > & elements );
	void RemoveEventsFromSection( SContext& context );

	void GenerateLookats( SContext& context );
	void GenerateLookatsForActor( SContext& context, CName actorName );
	void CreateLookatEvent( SContext& context, CName actorName, const LookatData& data, const SPositionInChunk& pos, const String& combinedDebugString  );
	
	void GenerateCameraForChunk( SContext& context );
	Bool ActionAxisRule( SContext& context, const StorySceneCameraDefinition & camera ) const;
	Int32 FindCameraForPosition( SContext& context, const SPositionInChunk & cameraPosition, CameraData & out, String& combinedDebugString );		
	void CreateCameraEvent( SContext& context, const CameraData& chosenCamData, const SPositionInChunk & cameraPosition, const String& combinedDebugString );
	Bool CanCameraBeBlended( SContext& context, const SPositionInChunk& posMarker, const StorySceneCameraDefinition& firstCamera );
	const StorySceneCameraDefinition* Get2ndCameraForBlend( SContext& context, const SPositionInChunk& posMarker, const StorySceneCameraDefinition& firstCamera );

	void GenerateAnimationEvents( SContext& context );
	void CreateAnimationEvent( SContext& context, CName actorName, const AnimData& animData, const SPositionInSection & eventPos, const String& debugString );
	void ProcessAnimationEntry( SContext& context, const AnimSearchContext& animContext, AnimSearchData& searchData );
	Float FindAnimation( SContext& context, const SPositionInSection & atPos, const SPositionInSection & fromPos, const SPositionInSection & toPos, CName actorName, AnimData& anim, Bool mustHaveMarker, Float overlap = 0.f );	
	void FirstPassAnimationEvents( SContext& context, const CStorySceneLine* line, const TDynArray< TPair< Float, Float > >& maxAmpPos );
	
	void GenerateMimicEvents( SContext& context );
	Float FindMimicAnimation( SContext& context, const SPositionInSection & atPos, const SPositionInSection & fromPos, const SPositionInSection & toPos,  CName actorName, AnimData& out, CName tag );
	void ChangeMimicIdle( SContext& context, const SPositionInSection& at, CName actorName );

	CName GetSentenceInfoAt( SContext& context, const SPositionInSection& atPos, CName actorName );
	void GetSentenceInfos( SContext& context, const CStorySceneLine* line, TDynArray< TPair< Float, CName > >& outData );

	void GenerateShake( SContext& context );

	template< typename T >
	void CollectEmptySpaces( SContext& context, CName actor, TDynArray< TPair<SPositionInSection,SPositionInSection> >& outEmptySpaces )
	{
		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId sectionVariantId = context.m_section->GetVariantUsedByLocale( currentLocaleId );

		TDynArray<SSectionChunk>& chunks = context.m_chunks;
		SPositionInSection sectionBegin( chunks, 0.f, 0.f );
		SPositionInSection sectionEnd( chunks, 1.f, 0.f );
		outEmptySpaces.PushBack( TPair<SPositionInSection,SPositionInSection>( sectionBegin, sectionEnd ) );
		const TDynArray< CGUID >& evGuids = context.m_section->GetEvents( sectionVariantId );
		for ( Uint32 i = 0; i < evGuids.Size(); ++i )
		{
			T* anim = Cast<T>( context.m_section->GetEvent( evGuids[ i ] ) );
			if( anim && anim->GetActor() == actor )
			{

				SPositionInSection animBegin( chunks, anim );
				SPositionInSection animEnd = animBegin;
				animEnd += anim->GetDurationProperty(); // TODO: confirm it's ok

				for ( Uint32 j = 0; j < outEmptySpaces.Size(); ++j )
				{
					SPositionInSection& emptySpaceBegin = outEmptySpaces[j].m_first;
					SPositionInSection& emptySpaceEnd	= outEmptySpaces[j].m_second;

					if( animEnd > emptySpaceBegin && animBegin < emptySpaceEnd )
					{	
						if ( animBegin > emptySpaceBegin )
						{
							if ( animEnd < emptySpaceEnd )
							{
								outEmptySpaces.Insert( j + 1, TPair<SPositionInSection,SPositionInSection>( animEnd, emptySpaceEnd ) );
								outEmptySpaces[j].m_second = animBegin; 
								j++;							
							}
							else
							{
								emptySpaceEnd = animBegin;
							}
						}
						else
						{
							if ( animEnd > emptySpaceEnd )
							{
								outEmptySpaces.RemoveAt( j );
								j--;
							}
							else
							{							
								emptySpaceBegin = animEnd;
							}									
						}
					}
				}
			}			
		}
	}


	template< typename T >
	Bool IsSpaceAvailable( SContext& context, const SPositionInSection& atPos, CName forActor, SPositionInSection& fromPos, SPositionInSection& toPos, Bool noOverlap = false )
	{
		const Float allowedOverlap = noOverlap ? 0.f : m_config.m_overlap;
		TDynArray<SSectionChunk>& chunks = context.m_chunks;

		Float durationOfEventBefore = 0.f;
		Float durationOfEventAfter = 0.f;
		Bool spotEmpty  = true;

		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId sectionVariantId = context.m_section->GetVariantUsedByLocale( currentLocaleId );

		const TDynArray< CGUID >& evGuids = context.m_section->GetEvents( sectionVariantId );
		for ( Uint32 j = 0; j < evGuids.Size() && ( spotEmpty ) ; ++j )
		{
			const T* anim = Cast< const T >( context.m_section->GetEvent( evGuids[ j ] ) );
			if( anim && ( anim->GetActor() == forActor ) )
			{
				SPositionInSection animBegin( context.m_chunks, anim );
				SPositionInSection animEnd = animBegin;
				animEnd += anim->GetDurationProperty(); // TODO: confirm it's ok
				if( atPos > animBegin &&  atPos < animEnd )
				{
					spotEmpty  = false;
				}
				if( atPos > animEnd && animEnd > fromPos )
				{
					fromPos = animEnd;
					durationOfEventBefore = anim->GetDurationProperty(); // TODO: confirm it's ok
				}
				if( atPos < animBegin && animBegin < toPos )
				{
					toPos = animBegin;
					durationOfEventAfter = anim->GetDurationProperty(); // TODO: confirm it's ok
				}
			}
		}
		fromPos -= allowedOverlap * durationOfEventBefore;;
		toPos += allowedOverlap * durationOfEventAfter;
		return spotEmpty;
	}

	template< typename T >
	void MarkExistingAsUsed( SContext& context )
	{
		const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
		const CStorySceneSectionVariantId sectionVariantId = context.m_section->GetVariantUsedByLocale( currentLocaleId );

		TDynArray<SSectionChunk>& chunks = context.m_chunks;
		const TDynArray< CGUID >& evGuids = context.m_section->GetEvents( sectionVariantId );
		for ( Uint32 i = 0; i < evGuids.Size(); ++i )
		{
			const T* anim = Cast< const T >( context.m_section->GetEvent( evGuids[ i ] ) );
			if( anim )
			{			
				Uint32 pos ,j;
				for( j = 0; j < chunks.Size(); ++j )
				{
					if( chunks[j].ContainElement( anim->GetSceneElement(), pos ) )
					{
						break;
					}
				}	
				if ( j < chunks.Size() )
				{
					context.MarkAnimationUsed( anim->GetAnimationName(), &chunks[j], anim->GetActor() );	
				}					
			}
		}
	}
};
