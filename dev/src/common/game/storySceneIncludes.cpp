
#include "build.h"
#include "storySceneIncludes.h"
#include "storyScenePlayer.h"
#include "storySceneAnimationList.h"
#include "..\engine\mimicComponent.h"
#include "storySceneSystem.h"
#include "..\engine\skeletalAnimationContainer.h"
#include "extAnimItemEvents.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

//#define TRACE_SCENE_MEM_ALLOC

//////////////////////////////////////////////////////////////////////////

#ifdef TRACE_SCENE_MEM_ALLOC
#ifdef NO_LOG
#define LOG_TRACE_SCENE_MEM_ALLOC( format, ... );
#else
RED_DEFINE_STATIC_NAME( SceneMemAlloc );
#define LOG_TRACE_SCENE_MEM_ALLOC( format, ... ) RED_LOG( SceneMemAlloc, format, ## __VA_ARGS__ );
#endif
Int32 SCENE_OBJECT = 0;
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EDialogLookAtType );

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EStorySceneSignalType );

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EDialogResetClothAndDanglesType );

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EStorySceneAnimationType );
IMPLEMENT_RTTI_ENUM( EStorySceneMimicsKeyType );
IMPLEMENT_RTTI_ENUM( EStoryScenePoseKeyType );

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SSceneChoice )

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR
const CSkeleton* IDialogBodyPartOwner::GetBodyPartSkeleton( const CComponent* c ) const
{
	const CAnimatedComponent* ac = Cast< const CAnimatedComponent >( c );
	return ac ? ac->GetSkeleton() : nullptr;
}
#endif

void* IStorySceneObject::operator new( size_t size )
{
#ifdef TRACE_SCENE_MEM_ALLOC
	++SCENE_OBJECT;
	LOG_TRACE_SCENE_MEM_ALLOC( TXT("Object(s): %d"), SCENE_OBJECT );
#endif

	return RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Dialog, size );
}

void IStorySceneObject::operator delete( void *ptr )
{
#ifdef TRACE_SCENE_MEM_ALLOC
	--SCENE_OBJECT;
	LOG_TRACE_SCENE_MEM_ALLOC( TXT("Object(s): %d"), SCENE_OBJECT );
#endif

	RED_MEMORY_FREE( MemoryPool_Default, MC_Dialog, ptr );
}

//////////////////////////////////////////////////////////////////////////

void CallMyPlaceForBreakpoint( const Char* msgFile, const Uint32 lineNum, const Char* msg )
{
	SCENE_ERROR( TXT("ASSERT: file: %ls, line: %d, exp: %ls"), msgFile, lineNum, msg );
	Int32 i = 0;
	i++;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( ELightTrackingType )

IMPLEMENT_ENGINE_CLASS( SStorySceneAttachmentInfo )
IMPLEMENT_ENGINE_CLASS( SStorySceneLightTrackingInfo )

	Uint32 SStorySceneAttachmentInfo::GetAttachmentFlags() const
{
	Uint32 res = 0;
	if ( m_freeRotation ) res |= HAF_FreeRotation;
	if ( m_freePositionAxisX ) res |= HAF_FreePositionAxisX;
	if ( m_freePositionAxisY ) res |= HAF_FreePositionAxisY;
	if ( m_freePositionAxisZ ) res |= HAF_FreePositionAxisZ;
	return res;
}

//////////////////////////////////////////////////////////////////////////

CStorySceneAnimationContainer::CStorySceneAnimationContainer()
	: m_valid( true )
	, m_lastUnstreamedAnim( nullptr )
	, m_lastUnstreamedAnimDuration( 0.f )
{

}

CStorySceneAnimationContainer::~CStorySceneAnimationContainer()
{
	SCENE_ASSERT( m_anims.Size() == 0 );
	SCENE_ASSERT( m_corruptedAnims.Size() == 0 );
}

void CStorySceneAnimationContainer::UnloadAllAnimations()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "This is not thread safe code" );

	for ( const CSkeletalAnimation* a : m_anims )
	{
		if ( a )
		{
			a->ReleaseUsage();
		}
	}
	m_anims.ClearFast();

	m_valid = false;
	m_lastUnstreamedAnim = nullptr;
	m_lastUnstreamedAnimDuration = 0.f;
	m_corruptedAnims.Clear();
}

void CStorySceneAnimationContainer::AddBodyAnimation( const CName& actorId, const CName& animation )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "This is not thread safe code" );
	SCENE_ASSERT( m_valid );

	Record* r = FindRecord( actorId );
	r->AddBodyAnimation( animation );
}

void CStorySceneAnimationContainer::AddMimicAnimation( const CName& actorId, const CName& animation )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "This is not thread safe code" );
	SCENE_ASSERT( m_valid );

	Record* r = FindRecord( actorId );
	r->AddMimicAnimation( animation );
}

void CStorySceneAnimationContainer::AddBodyIdle( const CName& actorId, const CName& status, const CName& emotionalState, const CName& poseName )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "This is not thread safe code" );
	SCENE_ASSERT( m_valid );

	SStorySceneActorAnimationState state;
	state.m_status = status;
	state.m_emotionalState = emotionalState;
	state.m_poseType = poseName;

	Record* r = FindRecord( actorId );
	r->AddBodyIdle( state );
}

void CStorySceneAnimationContainer::AddMimicIdle( const CName& actorId, const CName& mimicsLayer_Eyes, const CName& mimicsLayer_Pose, const CName& mimicsLayer_Animation )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "This is not thread safe code" );
	SCENE_ASSERT( m_valid );

	SStorySceneActorAnimationState state;
	state.m_mimicsLayerEyes = mimicsLayer_Eyes;
	state.m_mimicsLayerPose = mimicsLayer_Pose;
	state.m_mimicsLayerAnimation = mimicsLayer_Animation;

	Record* r = FindRecord( actorId );
	r->AddMimicIdle( state );
}

void CStorySceneAnimationContainer::PreloadNewAnimations( const CStoryScenePlayer* p, SPreloadedAnimFunc* func )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "This is not thread safe code" );
	SCENE_ASSERT( m_valid );
	for ( Record& r : m_data )
	{
		const CEntity* e = p->GetSceneActorEntity( r.m_id );
		if( !e )
		{
			e = p->GetScenePropEntity( r.m_id );
		}		
		if ( e )
		{
			const CAnimatedComponent* ac = e->GetRootAnimatedComponent();
			const CMimicComponent* mc( nullptr );

			const CSkeletalAnimationContainer* bodyC = ac ? ac->GetAnimationContainer() : nullptr;
			const CSkeletalAnimationContainer* bodyM( nullptr );

			if ( const CActor* a = Cast< CActor >( e ) )
			{
				mc = a->GetMimicComponent();
				bodyM = mc ? mc->GetAnimationContainer() : nullptr;
			}

			if ( bodyC )
			{
				const Int32 numBodyIdles = r.m_bodyIdles.SizeInt();
				for ( Int32 i=r.m_bodyIdlesPreloadIdx; i<numBodyIdles; ++i )
				{
					const SStorySceneActorAnimationState& s = r.m_bodyIdles[ i ];

					CStorySceneAnimationList::IdleAndLookAtAnimationData queryData;
					GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandBodyIdleAnimation( s, queryData, ac );

					if ( queryData.m_idle )			r.AddBodyAnimation( queryData.m_idle );
					if ( queryData.m_lookAtBody )	r.AddBodyAnimation( queryData.m_lookAtBody );
					if ( queryData.m_lookAtHead )	r.AddBodyAnimation( queryData.m_lookAtHead );
				}
				r.m_bodyIdlesPreloadIdx = numBodyIdles;

				const Int32 numBodyAnimations = r.m_bodyAnimations.SizeInt();
				for ( Int32 i=r.m_bodyAnimationsPreloadIdx; i<numBodyAnimations; ++i )
				{
					const CName& animName = r.m_bodyAnimations[ i ];
					if ( animName )
					{
						if ( const CSkeletalAnimationSetEntry* anim = bodyC->FindAnimationRestricted( animName ) )
						{
							if ( const CSkeletalAnimation* a = anim->GetAnimation() )
							{
								if ( !m_anims.Exist( a ) )
								{
									a->AddUsage();
									m_anims.PushBack( a );
									if( func )
									{
										func->OnAnimationPreloaded( e, anim );
									}
								}
							}
						}
					}
				}
				r.m_bodyAnimationsPreloadIdx = numBodyAnimations;
			}

			if ( bodyM )
			{
				const Int32 numMimicIdles = r.m_mimicIdles.SizeInt();
				for ( Int32 i=r.m_mimicIdlesPreloadIdx; i<numMimicIdles; ++i )
				{
					const SStorySceneActorAnimationState& s = r.m_mimicIdles[ i ];

					CName animationMimicsEyes;
					CName animationMimicsPose;
					CName animationMimicsAnim;

					GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandMimicsIdleAnimation( s, animationMimicsEyes, mc, CStorySceneAnimationList::LAYER_EYES );
					GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandMimicsIdleAnimation( s, animationMimicsPose, mc, CStorySceneAnimationList::LAYER_POSE );
					GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().RandMimicsIdleAnimation( s, animationMimicsAnim, mc, CStorySceneAnimationList::LAYER_ANIMATION );

					if ( animationMimicsEyes ) r.AddMimicAnimation( animationMimicsEyes );
					if ( animationMimicsPose ) r.AddMimicAnimation( animationMimicsPose );
					if ( animationMimicsAnim ) r.AddMimicAnimation( animationMimicsAnim );
				}
				r.m_mimicIdlesPreloadIdx = numMimicIdles;

				const Int32 numMimicAnimations = r.m_mimicAnimations.SizeInt();
				for ( Int32 i=r.m_mimicAnimationsPreloadIdx; i<numMimicAnimations; ++i )
				{
					const CName& animName = r.m_mimicAnimations[ i ];
					if ( animName )
					{
						if ( const CSkeletalAnimationSetEntry* anim = bodyM->FindAnimationRestricted( animName ) )
						{
							if ( const CSkeletalAnimation* a = anim->GetAnimation() )
							{
								if ( !m_anims.Exist( a ) )
								{
									a->AddUsage();
									m_anims.PushBack( a );
								}
							}
						}
					}
				}
				r.m_mimicAnimationsPreloadIdx = numMimicAnimations;
			}
		}
	}
}

Bool CStorySceneAnimationContainer::WaitForAllAnimations() const
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "This is not thread safe code" );

	for ( const CSkeletalAnimation* a : m_anims )
	{
		if ( a && !a->IsFullyLoaded() )
		{
			CStorySceneAnimationContainer* c = const_cast< CStorySceneAnimationContainer* >( this );

			if ( c->m_corruptedAnims.Find( a ) != c->m_corruptedAnims.End() )
			{
				continue;
			}

			if ( m_lastUnstreamedAnim == a )
			{
				SCENE_ASSERT( m_lastUnstreamedAnimDuration >= 0.f );

				c->m_lastUnstreamedAnimDuration += GEngine->GetLastTimeDelta();
				if ( m_lastUnstreamedAnimDuration > 3.f )
				{
					SCENE_ASSERT( 0 );
					const String animset = a->GetEntry() && a->GetEntry()->GetAnimSet() ? a->GetEntry()->GetAnimSet()->GetDepotPath() : TXT("<invalid>");
					SCENE_ERROR( TXT("Story scene animation '%s' in animset '%s' can not be stream - please debug it"), a->GetName().AsChar(), animset.AsChar() );

					c->m_corruptedAnims.Insert( a );

					c->m_lastUnstreamedAnim = nullptr;
					c->m_lastUnstreamedAnimDuration = 0.f;
				}
			}
			else
			{
				c->m_lastUnstreamedAnim = a;
				c->m_lastUnstreamedAnimDuration = 0.f;
			}

			return false;
		}
	}

	return true;
}

Bool CStorySceneAnimationContainer::HasAnimation( const CSkeletalAnimationSetEntry* anim ) const
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "This is not thread safe code" );

	for ( const CSkeletalAnimation* a : m_anims )
	{
		if ( a == anim->GetAnimation() )
		{
			return true;
		}
	}

	return false;
}

CStorySceneAnimationContainer::Record* CStorySceneAnimationContainer::FindRecord( const CName& id )
{
	for ( Record& r : m_data )
	{
		if ( r.m_id == id )
		{
			return &r;
		}
	}

	Record* newRecord = new ( m_data ) Record();
	newRecord->m_id = id;

	return newRecord;
}

CStorySceneAnimationContainer::Record::Record()
	: m_bodyAnimationsPreloadIdx( 0 )
	, m_mimicAnimationsPreloadIdx( 0 )
	, m_bodyIdlesPreloadIdx( 0 )
	, m_mimicIdlesPreloadIdx( 0 )
{

}

void CStorySceneAnimationContainer::Record::AddBodyAnimation( const CName& animation )
{
	if ( !m_bodyAnimations.Exist( animation ) )
	{
		if ( m_bodyAnimationsPreloadIdx == -1 )
		{
			m_bodyAnimationsPreloadIdx = m_bodyAnimations.SizeInt();
		}

		m_bodyAnimations.PushBack( animation );
	}
}

void CStorySceneAnimationContainer::Record::AddMimicAnimation( const CName& animation )
{
	if ( !m_mimicAnimations.Exist( animation ) )
	{
		if ( m_mimicAnimationsPreloadIdx == -1 )
		{
			m_mimicAnimationsPreloadIdx = m_mimicAnimations.SizeInt();
		}

		m_mimicAnimations.PushBack( animation );
	}
}

void CStorySceneAnimationContainer::Record::AddBodyIdle( const SStorySceneActorAnimationState& state )
{
	for ( const SStorySceneActorAnimationState& s : m_bodyIdles )
	{
		if ( s.IsBodyEqual( state ) )
		{
			return;
		}
	}

	if ( m_bodyIdlesPreloadIdx == -1 )
	{
		m_bodyIdlesPreloadIdx = m_bodyIdles.SizeInt();
	}

	m_bodyIdles.PushBack( state );
}

void CStorySceneAnimationContainer::Record::AddMimicIdle( const SStorySceneActorAnimationState& state )
{
	for ( const SStorySceneActorAnimationState& s : m_mimicIdles )
	{
		if ( s.IsMimicEqual( state ) )
		{
			return;
		}
	}

	if ( m_mimicIdlesPreloadIdx == -1 )
	{
		m_mimicIdlesPreloadIdx = m_mimicIdles.SizeInt();
	}

	m_mimicIdles.PushBack( state );
}

CStorySceneAnimationContainer::Iterator::Iterator( CStorySceneAnimationContainer& c )
	: m_container( c )
	, m_index( 0 )
{
	
}

CStorySceneAnimationContainer::Iterator::operator Bool () const
{
	return m_index < m_container.m_anims.Size();
}

void CStorySceneAnimationContainer::Iterator::operator++ ()
{
	++m_index;
}

const CSkeletalAnimation* CStorySceneAnimationContainer::Iterator::operator*() const
{
	return m_container.m_anims[ m_index ];
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
