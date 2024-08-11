/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "staticCamera.h"
#include "animationIterator.h"
#include "animMemStats.h"
#include "debugStat.h"
#include "debugCheckBox.h"
#ifndef NO_DEBUG_PAGES

#include "skeletalAnimationSet.h"
#include "baseEngine.h"
#include "inputBufferedInputEvent.h"
#include "inputKeys.h"
#include "poseProviderStats.h"
#include "poseProvider.h"
#include "skeleton.h"
#include "actorInterface.h"
#include "cutscene.h"
#include "animationManager.h"
#include "animatedComponent.h"
#include "game.h"
#include "renderFrame.h"
#include "world.h"
#include "tickManager.h"
#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#include "debugPage.h"
#include "debugPageManagerBase.h"
#include "../core/diskFile.h"

#endif

class CDebugCheckBoxAnimSets : public IDebugCheckBox
{
	struct AnimSetInfo
	{
		THandle< CSkeletalAnimationSet >		m_set;						//!< Handle to animation set
		String									m_name;						//!< Name of the loaded animation set
		Uint32									m_numActiveAnimations;		//!< Number of active animations in the animation set
		Uint32									m_totalUsedMemory;			//!< Total used memory for animations
		Uint32									m_totalUsedMemoryForAnimation;//!< Total used memory for animations motion extraction
		Uint32									m_totalUsedMemoryForMotionEx;//!< Total used memory for animations motion extraction
		Uint32									m_totalUsedMemoryForCompressedFrame;//!< Total used memory for animations motion extraction
		Uint32									m_totalUsedMemoryForCompressedFrameData;//!< Total used memory for animations motion extraction
		Uint32									m_activeUsedMemory;			//!< Memory used for active animations
		Float									m_highLightTimer;			//!< Highlight timer
		Color									m_highLightColor;			//!< Highlight color

		AnimSetInfo( CSkeletalAnimationSet* animSet )
			: m_set( animSet )
			, m_numActiveAnimations( 0 )
			, m_highLightColor( Color::RED )
			, m_highLightTimer( 2.0f )
		{
			CDiskFile* file = animSet->GetFile();
			if ( file )
			{
				CFilePath path( file->GetDepotPath() );
				m_name = path.GetFileName();
			}
			else
			{
				m_name = TXT("Unknown AnimSet");
			}
		}

		static int CompareActive( const void*a, const void* b )
		{
			AnimSetInfo* infoA = * ( AnimSetInfo** ) a;
			AnimSetInfo* infoB = * ( AnimSetInfo** ) b;
			if ( infoA->m_activeUsedMemory > infoB->m_activeUsedMemory ) return -1;
			if ( infoA->m_activeUsedMemory < infoB->m_activeUsedMemory ) return 1;
			return 0;
		}

		static int CompareTotal( const void*a, const void* b )
		{
			AnimSetInfo* infoA = * ( AnimSetInfo** ) a;
			AnimSetInfo* infoB = * ( AnimSetInfo** ) b;
			if ( infoA->m_totalUsedMemory > infoB->m_totalUsedMemory ) return -1;
			if ( infoA->m_totalUsedMemory < infoB->m_totalUsedMemory ) return 1;
			return 0;
		}

	};

	TDynArray< AnimSetInfo* >	m_animSets;

	Uint32						m_totalMemory;
	Uint32						m_totalAnimationsMemory;
	Uint32						m_totalAnimationsMotionExMemory;
	Uint32						m_totalAnimationsCompressedFrameMemory;
	Uint32						m_totalAnimationsCompressedFrameDataMemory;

	Uint32						m_csSize;
	Uint32						m_exDialogSize;

	Uint32						m_usedAnimationsMemory;
	Uint32						m_numActiveAnimations;

	Int32							m_offset;
	bool						m_sortTotal;
	Uint32						m_maxH;

	CClass*						DIALOGSET_CLASS;

public:
	CDebugCheckBoxAnimSets( IDebugCheckBox* parent, Uint32 maxH )
		: IDebugCheckBox( parent, TXT("Animsets"), true, false )
		, m_offset( 0 )
		, m_sortTotal( false )
		, m_maxH( maxH )
		, m_totalMemory( 0 )
		, m_totalAnimationsMemory( 0 )
		, m_totalAnimationsMotionExMemory( 0 )
		, m_totalAnimationsCompressedFrameMemory( 0 )
		, m_totalAnimationsCompressedFrameDataMemory( 0 )
		, m_usedAnimationsMemory( 0 )
		, m_numActiveAnimations( 0 )
		, m_csSize( 0 )
		, m_exDialogSize( 0 )
	{
		DIALOGSET_CLASS = SRTTI::GetInstance().FindClass( CName( TXT("CStorySceneDialogset" ) ) );
		ASSERT( DIALOGSET_CLASS );
	};

	Uint32 GetNumActiveAnimations()
	{
		return m_numActiveAnimations;
	}

	Uint32 GetUsedAnimationsMemory()
	{
		return m_usedAnimationsMemory;
	}

	Uint32 GetTotalMemory()
	{
		return m_totalMemory;
	}

	Uint32 GetTotalAnimationsMemory()
	{
		return m_totalAnimationsMemory;
	}

	Uint32 GetTotalAnimationsMotionExMemory()
	{
		return m_totalAnimationsMotionExMemory;
	}

	Uint32 GetTotalCompressedFramesMemory()
	{
		return m_totalAnimationsCompressedFrameMemory;
	}

	Uint32 GetTotalCompressedFramesDataMemory()
	{
		return m_totalAnimationsCompressedFrameDataMemory;
	}

	Uint32 GetCutsceneMemory()
	{
		return m_csSize;
	}

	Uint32 GetExDialogMemory()
	{
		return m_exDialogSize;
	}

	virtual void OnTick( Float timeDelta )
	{
		IDebugCheckBox::OnTick( timeDelta );

		m_totalMemory = 0;
		m_totalAnimationsMemory = 0;
		m_totalAnimationsMotionExMemory = 0;
		m_totalAnimationsCompressedFrameMemory = 0;
		m_totalAnimationsCompressedFrameDataMemory = 0;
		m_usedAnimationsMemory = 0;
		m_numActiveAnimations = 0;
		m_csSize = 0;
		m_exDialogSize = 0;

		// Unloaded animation set
		for ( Uint32 i=m_animSets.Size(); i>0; --i )
		{
			Int32 num = i-1;

			AnimSetInfo* info = m_animSets[num];
			if ( !info->m_set.Get() )
			{
				// Unloaded
				if ( info->m_highLightTimer > 0.0f )
				{
					// Process unload timer, when done, remove from list
					info->m_highLightTimer -= timeDelta;
					if ( info->m_highLightTimer <= 0.0f )
					{
						m_animSets.Erase( m_animSets.Begin() + num );
						continue;
					}
				}
				else
				{
					// Highlight unloaded animation set
					info->m_highLightColor = Color::GRAY;
					info->m_highLightTimer = 3.0f;
				}

				// Done
				continue;
			}
		}

		// Process list
		for ( ObjectIterator< CSkeletalAnimationSet > it; it; ++it )
		{
			CSkeletalAnimationSet* animSet = *it;

			// Find existing animation set entry
			AnimSetInfo* info = NULL;
			for ( Uint32 j=0; j<m_animSets.Size(); j++ )
			{
				AnimSetInfo* test = m_animSets[j];
				if ( test->m_set.Get() == animSet )
				{
					info = test;
					break;
				}
			}

			// Add new
			if ( !info )
			{
				info = new AnimSetInfo( animSet );
				m_animSets.PushBack( info );
			}
		}

		// Process fade
		for ( Uint32 j=0; j<m_animSets.Size(); j++ )
		{
			AnimSetInfo* test = m_animSets[j];

			CSkeletalAnimationSet* animSet = test->m_set.Get();
			if ( animSet )
			{
				// More animations
				TDynArray< CSkeletalAnimationSetEntry* > animations;
				animSet->GetAnimations( animations );

				// Count active animations
				Uint32 numUsedAnimations = 0;
				Uint32 totalDataSize = 0;
				Uint32 totalAnimationDataSize = 0;
				Uint32 totalAnimationMotionExDataSize = 0;
				Uint32 totalAnimationsCompressedFrameMemory = 0;
				Uint32 totalAnimationsCompressedFrameDataMemory = 0;
				Uint32 usedAnimationDataSize = 0;

				for ( Uint32 k=0; k<animations.Size(); k++ )
				{
					CSkeletalAnimationSetEntry* entry = animations[k];
					if ( entry && entry->GetAnimation() )
					{
						// Count total animation size
						CSkeletalAnimation* anim = entry->GetAnimation();

						AnimMemStats stats;
						anim->GetMemStats( stats );

						const Uint32 animSize = stats.m_animBufferNonStreamable + stats.m_animBufferStreamableLoaded;
						totalDataSize += animSize + stats.m_compressedPose + stats.m_motionExtraction;
						totalAnimationDataSize += animSize;
						totalAnimationsCompressedFrameMemory += stats.m_compressedPose;
						totalAnimationsCompressedFrameDataMemory += stats.m_compressedPoseData;
						totalAnimationMotionExDataSize += stats.m_motionExtraction;

						// Used ?
						if ( anim->GetLastTouchTime() == GEngine->GetCurrentEngineTick() )
						{
							usedAnimationDataSize += animSize;
							numUsedAnimations += 1;
						}
					}
				}

				// Update stats
				test->m_totalUsedMemory = totalDataSize;
				test->m_totalUsedMemoryForAnimation = totalAnimationDataSize;
				test->m_totalUsedMemoryForMotionEx = totalAnimationMotionExDataSize;
				test->m_activeUsedMemory = usedAnimationDataSize;
				test->m_totalUsedMemoryForCompressedFrame = totalAnimationsCompressedFrameMemory;
				test->m_totalUsedMemoryForCompressedFrameData = totalAnimationsCompressedFrameDataMemory;

				// Change count
				if ( numUsedAnimations > test->m_numActiveAnimations ) 
				{
					test->m_numActiveAnimations = numUsedAnimations;
					test->m_highLightColor = Color::RED;
					test->m_highLightTimer = 1.0f;
				}
				else if ( numUsedAnimations < test->m_numActiveAnimations )
				{
					test->m_numActiveAnimations = numUsedAnimations;
					test->m_highLightColor = Color::GREEN;
					test->m_highLightTimer = 1.0f;
				}

				// Process fade
				if ( test->m_highLightTimer > 0.0f )
				{
					test->m_highLightTimer -= timeDelta;
					if ( test->m_highLightTimer >= 0.0f )
					{
						test->m_highLightTimer = 0.0f;
					}
				}

				if ( animSet->IsA< CCutsceneTemplate >() )
				{
					m_csSize += test->m_totalUsedMemory;
				}
				else if ( animSet->GetClass() == DIALOGSET_CLASS )
				{
					m_exDialogSize += test->m_totalUsedMemory;
				}
			}

			m_totalMemory += test->m_totalUsedMemory;
			m_totalAnimationsMemory += test->m_totalUsedMemoryForAnimation;
			m_totalAnimationsMotionExMemory += test->m_totalUsedMemoryForMotionEx;
			m_totalAnimationsCompressedFrameMemory += test->m_totalUsedMemoryForCompressedFrame;
			m_totalAnimationsCompressedFrameDataMemory += test->m_totalUsedMemoryForCompressedFrameData;
			m_usedAnimationsMemory += test->m_activeUsedMemory;
			m_numActiveAnimations += test->m_numActiveAnimations;
		}

		if ( IsExpanded() )
		{
			// Sort
			if ( m_sortTotal )
			{
				qsort( m_animSets.TypedData(), m_animSets.Size(), sizeof( AnimSetInfo* ), &AnimSetInfo::CompareTotal );
			}
			else
			{
				qsort( m_animSets.TypedData(), m_animSets.Size(), sizeof( AnimSetInfo* ), &AnimSetInfo::CompareActive );
			}
		}
	}

	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
	{
		IDebugCheckBox::OnRender( frame, x, y, counter, options );

		if ( IsExpanded() )
		{
			static const Int32 xOffset = 30;
			static const Int32 xName = 70 + xOffset;
			static const Int32 xTotalMemory = 350 + xOffset;
			static const Int32 xUsedMemory = 450 + xOffset;
			static const Int32 xUsedAnimations = 550 + xOffset;

			String name = String::Printf( TXT("Animset name (%d)"), m_animSets.Size() );

			frame->AddDebugScreenFormatedText( xName, y, name.AsChar() );
			frame->AddDebugScreenFormatedText( xTotalMemory, y, TXT("Total used memory") );
			frame->AddDebugScreenFormatedText( xUsedMemory, y, TXT("Active used memory") );
			frame->AddDebugScreenFormatedText( xUsedAnimations, y, TXT("Number of active animations") );
			y += 20;

			// Clamp rendering
			const Int32 limit = Max< Int32 >( 0, (Int32)m_animSets.Size()-1 );
			m_offset = Clamp< Int32 >( m_offset, 0, limit );

			// List
			for ( Uint32 i=m_offset; i<m_animSets.Size(); i++ )
			{
				if ( y > m_maxH )
				{
					break;
				}

				AnimSetInfo* info = m_animSets[i];

				Color color = Color::WHITE;
				if ( info->m_highLightTimer > 0.0f )
				{
					color = info->m_highLightColor;
				}

				frame->AddDebugScreenFormatedText( xName, y, color, info->m_name.AsChar() );
				frame->AddDebugScreenFormatedText( xTotalMemory, y, color, TXT("%1.3f MB"), info->m_totalUsedMemory / ( 1024.0f*1024.0f ) );
				frame->AddDebugScreenFormatedText( xUsedMemory, y, color, TXT("%1.3f MB"), info->m_activeUsedMemory / ( 1024.0f*1024.0f ) );
				frame->AddDebugScreenFormatedText( xUsedAnimations, y, color, TXT("%i"), info->m_numActiveAnimations );
				y += 15;
			}
		}
	}

	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data )
	{
		if ( IsExpanded() )
		{
			// Move up
			if ( key == IK_Up && action == IACT_Press )
			{
				m_offset = Max< Int32 >( 0, m_offset-1 );
				return true;
			}

			// Move down
			if ( key == IK_Down && action == IACT_Press )
			{
				const Int32 limit = Max< Int32 >( 0, (Int32)m_animSets.Size()-1 );
				m_offset = Min< Int32 >( m_offset+1, limit );
				return true;
			}

			// Sort key
			if ( key == IK_S && action == IACT_Press )
			{
				m_sortTotal = !m_sortTotal;
				return true;
			}
		}
		
		return IDebugCheckBox::OnInput( key, action, data );
	}
};

class CDebugCheckBoxSkeletons : public IDebugCheckBox
{
	typedef StatChart< 128 > DefualtStatChar;

	class PoseStatsBar : public DefualtStatChar
	{
	public:
		PoseStatsBar()
			: DefualtStatChar( TXT(""), 100 )
		{}

		virtual void Update( CWorld* world )
		{
			//const CTickManager::STickGenericStats& stats = world->GetTickManager()->GetGroupTimersStats( m_tickGroup );
			//DefualtStatChar::Update( TicksToTime( stats.m_statsTime ), stats.m_statsCount );
		}
	};

	struct SkeletonInfo
	{
		THandle< CSkeleton >		m_skeleton;
		String						m_name;
		SPoseProviderStats			m_stats;
		PoseStatsBar				m_statBarCache;
		PoseStatsBar				m_statBarAlloc;

		SkeletonInfo( CSkeleton* skeleton )
			: m_skeleton( skeleton )
		{
			m_name = skeleton->GetDepotPath().StringAfter( TXT("\\"), true );
		}

		static int CompareNum( const void*a, const void* b )
		{
			SkeletonInfo* infoA = * ( SkeletonInfo** ) a;
			SkeletonInfo* infoB = * ( SkeletonInfo** ) b;
			if ( infoA->m_stats.m_numTotal > infoB->m_stats.m_numTotal ) return -1;
			else if ( infoA->m_stats.m_numTotal <= infoB->m_stats.m_numTotal ) return 1;
			return 0;
		}

		static int CompareMem( const void*a, const void* b )
		{
			SkeletonInfo* infoA = * ( SkeletonInfo** ) a;
			SkeletonInfo* infoB = * ( SkeletonInfo** ) b;
			if ( infoA->m_stats.m_memTotal > infoB->m_stats.m_memTotal ) return -1;
			else if ( infoA->m_stats.m_memTotal <= infoB->m_stats.m_memTotal ) return 1;
			return 0;
		}

	};

	TDynArray< SkeletonInfo* >	m_skeletons;

	Uint32						m_memTotal;
	Uint32						m_memAlloc;
	Uint32						m_memCached;

	Uint32						m_numTotal;
	Uint32						m_numCached;
	Uint32						m_numAlloc;

	Int32							m_offset;
	Uint32						m_maxH;

	Bool						m_sortMem;

public:
	CDebugCheckBoxSkeletons( IDebugCheckBox* parent, Uint32 maxH )
		: IDebugCheckBox( parent, TXT("Poses"), true, false )
		, m_offset( 0 )
		, m_sortMem( true )
		, m_maxH( maxH )
		, m_memTotal( 0 )
		, m_memAlloc( 0 )
		, m_memCached( 0 )
		, m_numTotal( 0 )
		, m_numCached( 0 )
		, m_numAlloc( 0 )
	{

	};

	Uint32 GetTotalPosesMem()
	{
		return m_memTotal;
	}

	virtual void OnTick( Float timeDelta )
	{
		IDebugCheckBox::OnTick( timeDelta );

		m_memTotal = 0;
		m_memAlloc = 0;
		m_memCached = 0;

		m_numTotal = 0;
		m_numAlloc = 0;
		m_numCached = 0;

		for ( Uint32 i=m_skeletons.Size(); i>0; --i )
		{
			Int32 num = i-1;

			SkeletonInfo* info = m_skeletons[ num ];
			if ( !info->m_skeleton.Get() )
			{
				if ( info->m_stats.m_memTotal <= 0.0f || info->m_stats.m_numTotal <= 0 )
				{
					delete info;
					m_skeletons.Erase( m_skeletons.Begin() + num );
					continue;
				}

				continue;
			}
		}

		// Process list
		for ( ObjectIterator< CSkeleton > it; it; ++it )
		{
			CSkeleton* skeleton = *it;

			// Find existing skeletons
			SkeletonInfo* info = NULL;

			for ( Uint32 j=0; j<m_skeletons.Size(); j++ )
			{
				SkeletonInfo* test = m_skeletons[ j ];
				if ( test->m_skeleton.Get() == skeleton )
				{
					info = test;
					break;
				}
			}

			// Add new
			if ( !info )
			{
				info = new SkeletonInfo( skeleton );
				m_skeletons.PushBack( info );
			}
		}

		// Process
		for ( Uint32 j=0; j<m_skeletons.Size(); j++ )
		{
			SkeletonInfo* test = m_skeletons[ j ];

			CSkeleton* skeleton = test->m_skeleton.Get();
			if ( skeleton )
			{
				CPoseProvider* poseAlloc = skeleton->GetPoseProvider();
				if ( poseAlloc )
				{
					poseAlloc->GetStats( test->m_stats );

					m_numTotal += test->m_stats.m_numTotal;
					m_numAlloc += test->m_stats.m_numAlloc - test->m_stats.m_numCached;
					m_numCached += test->m_stats.m_numCached;

					m_memTotal += test->m_stats.m_memTotal;
					m_memAlloc += test->m_stats.m_memAlloc - test->m_stats.m_memCached;
					m_memCached += test->m_stats.m_memCached;

					//test->m_statBarCache.Update( .... ); dane do tego
					//test->m_statBarAlloc.Update( .... );
				}
			}
		}

		if ( IsExpanded() )
		{
			// Sort
			if ( m_sortMem )
			{
				qsort( m_skeletons.TypedData(), m_skeletons.Size(), sizeof( SkeletonInfo* ), &SkeletonInfo::CompareMem );
			}
			else
			{
				qsort( m_skeletons.TypedData(), m_skeletons.Size(), sizeof( SkeletonInfo* ), &SkeletonInfo::CompareNum );
			}
		}
	}

	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
	{
		IDebugCheckBox::OnRender( frame, x, y, counter, options );

		if ( IsExpanded() )
		{
			static const Int32 xOffset = 30;
			static const Int32 xName = 70 + xOffset;
			static const Int32 xTotalNum = 350 + xOffset;
			static const Int32 xTotalCached = 400 + xOffset;
			static const Int32 xTotalAlloc = 450 + xOffset;
			static const Int32 xTotalFreeCached = 500 + xOffset;
			static const Int32 xTotalFreeAlloc = 550 + xOffset;
			static const Int32 xBarCache = 650 + xOffset;
			static const Int32 xBarAlloc = 900 + xOffset;

			static const Float invMb = 1.f / ( 1024.f * 1024.f );

			String name = String::Printf( TXT("Allocators (%d) - Total poses %1.3f [MB] (%d), Cached poses %1.3f [MB] (%d), Alloc poses %1.3f [MB] (%d)"), 
				m_skeletons.Size(), 
				invMb * m_memTotal, m_numTotal,
				invMb * m_memCached, m_numCached,
				invMb * m_memAlloc, m_numAlloc );

			frame->AddDebugScreenFormatedText( xName, y, name.AsChar() );

			y += 16;

			frame->AddDebugScreenFormatedText( xName, y, TXT("Skeleton") );
			frame->AddDebugScreenFormatedText( xTotalNum, y, TXT("Poses") );
			frame->AddDebugScreenFormatedText( xTotalCached, y, TXT("Cache") );
			frame->AddDebugScreenFormatedText( xTotalAlloc, y, TXT("Alloc") );
			frame->AddDebugScreenFormatedText( xTotalFreeCached, y, TXT("F Cached") );
			frame->AddDebugScreenFormatedText( xTotalFreeAlloc, y, TXT("F Alloc") );

			y += 20;

			// Clamp rendering
			const Int32 limit = Max< Int32 >( 0, (Int32)m_skeletons.Size()-1 );
			m_offset = Clamp< Int32 >( m_offset, 0, limit );

			// List
			for ( Uint32 i=m_offset; i<m_skeletons.Size(); i++ )
			{
				if ( y > m_maxH )
				{
					break;
				}

				SkeletonInfo* info = m_skeletons[i];

				Color color = Color::WHITE;

				// Texts
				frame->AddDebugScreenFormatedText( xName, y, color, info->m_name.AsChar() );
				frame->AddDebugScreenFormatedText( xTotalNum, y, color, TXT("%d (%1.3f)"), info->m_stats.m_numTotal, info->m_stats.m_memTotal * invMb );
				frame->AddDebugScreenFormatedText( xTotalCached, y, color, TXT("%d (%1.3f)"), info->m_stats.m_numCached, info->m_stats.m_memCached * invMb );
				frame->AddDebugScreenFormatedText( xTotalAlloc, y, color, TXT("%d (%1.3f)"), info->m_stats.m_numAlloc, info->m_stats.m_memAlloc * invMb );
				frame->AddDebugScreenFormatedText( xTotalFreeCached, y, color, TXT("%d (%1.3f)"), info->m_stats.m_numFreeCached, info->m_stats.m_memFreeCached * invMb );
				frame->AddDebugScreenFormatedText( xTotalFreeAlloc, y, color, TXT("%d (%1.3f)"), info->m_stats.m_numFreeAlloc, info->m_stats.m_memFreeAlloc * invMb );

				// Bars
				const Uint32 barWidth = 200;

				//info->m_statBarCache.DrawBar( frame, xBarCache, y, barWidth );
				//info->m_statBarAlloc.DrawBar( frame, xBarAlloc, y, barWidth );

				y += 15;
			}
		}
	}

	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data )
	{
		if ( IsExpanded() )
		{
			// Move up
			if ( key == IK_Up && action == IACT_Press )
			{
				m_offset = Max< Int32 >( 0, m_offset-1 );
				return true;
			}

			// Move down
			if ( key == IK_Down && action == IACT_Press )
			{
				const Int32 limit = Max< Int32 >( 0, (Int32)m_skeletons.Size()-1 );
				m_offset = Min< Int32 >( m_offset+1, limit );
				return true;
			}

			// Sort key
			if ( key == IK_S && action == IACT_Press )
			{
				m_sortMem = !m_sortMem;
				return true;
			}
		}

		return IDebugCheckBox::OnInput( key, action, data );
	}
};

class CDebugCheckBoxAnimComponents : public IDebugCheckBox
{
	CClass* m_actorClass;
	CClass* m_itemClass;

	enum ECompType
	{
		CT_All,
		CT_StaticCamera,
		CT_Camera,
		CT_Actor,
		CT_Item,
		CT_Head,
		CT_Rest,
		CT_Last
	};

	struct CompInfo
	{
		Uint32	m_num;
		Uint32	m_frozenNum;
		Uint32	m_budgetedNum;

		Uint32	m_inCs;
		Uint32	m_inBehContr;
		Uint32	m_inWork;
	};

	CompInfo m_components[ CT_Last ];

public:
	CDebugCheckBoxAnimComponents( IDebugCheckBox* parent )
		: IDebugCheckBox( parent, TXT("Components"), true, false )
		, m_actorClass( NULL )
		, m_itemClass( NULL )
	{
		Reset();

		static const CName actorClassName( TXT("CActor") );
		static const CName itemClassName( TXT("CItemEntity") );

		m_actorClass = SRTTI::GetInstance().FindClass( actorClassName );
		m_itemClass = SRTTI::GetInstance().FindClass( itemClassName );

		ASSERT( m_actorClass );
		ASSERT( m_itemClass );
	};

	void Reset()
	{
		for ( Uint32 i=0; i<CT_Last; ++i )
		{
			m_components[ i ].m_num = 0;
			m_components[ i ].m_frozenNum = 0;
			m_components[ i ].m_budgetedNum = 0;

			m_components[ i ].m_inCs = 0;
			m_components[ i ].m_inBehContr = 0;
			m_components[ i ].m_inWork = 0;
		}
	}

	// cs ac->IsUnderExternalControl

	virtual void OnTick( Float timeDelta )
	{
		IDebugCheckBox::OnTick( timeDelta );

		if ( IsExpanded() && GGame && GGame->IsActive() && GGame->GetActiveWorld() )
		{
			TDynArray< CAnimatedComponent* > animatedComponents;
			GGame->GetActiveWorld()->GetAttachedComponentsOfClass( animatedComponents );

			Reset();

			m_components[ CT_All ].m_num = animatedComponents.Size();

			const CTickManager* tickMgr = GGame->GetActiveWorld()->GetTickManager();

			for ( Uint32 i=0; i<animatedComponents.Size(); ++i )
			{
				CAnimatedComponent* ac = animatedComponents[ i ];

				const Bool isSuppressed = ac->IsTickSuppressed();
				if ( isSuppressed )
				{
					m_components[ CT_All ].m_frozenNum++;
				}

				const Bool isBudgeted = ac->IsTickBudgeted();
				if ( isBudgeted )
				{
					m_components[ CT_All ].m_budgetedNum++;
				}

				// Actor
				{
					if ( m_actorClass && ac->GetEntity()->GetClass()->IsA( m_actorClass ) )
					{
						m_components[ CT_Actor ].m_num++;
						if ( isSuppressed ) m_components[ CT_Actor ].m_frozenNum++;
						if ( isBudgeted ) m_components[ CT_Actor ].m_budgetedNum++;

						if ( ac->IsInCinematic() )
						{
							m_components[ CT_Actor ].m_inCs++;
						}

						IActorInterface* actor = ac->GetEntity()->QueryActorInterface();
						if ( actor )
						{
							if ( actor->IsWorking() ) m_components[ CT_Actor ].m_inWork++;
							if ( actor->IsInQuestScene() ) m_components[ CT_Actor ].m_inBehContr++;
						}

						continue;
					}
				}

				// Static Camera
				{
					CStaticCamera* cam = Cast< CStaticCamera >( ac->GetEntity() );
					if ( cam )
					{
						m_components[ CT_StaticCamera ].m_num++;
						if ( isSuppressed ) m_components[ CT_StaticCamera ].m_frozenNum++;
						if ( isBudgeted ) m_components[ CT_StaticCamera ].m_budgetedNum++;

						if ( ac->IsInCinematic() )
						{
							m_components[ CT_StaticCamera ].m_inCs++;
						}

						continue;
					}
				}

				// Camera
				{
					CCamera* cam = Cast< CCamera >( ac->GetEntity() );
					if ( cam )
					{
						m_components[ CT_Camera ].m_num++;
						if ( isSuppressed ) m_components[ CT_Camera ].m_frozenNum++;
						if ( isBudgeted ) m_components[ CT_Camera ].m_budgetedNum++;

						if ( ac->IsInCinematic() )
						{
							m_components[ CT_Camera ].m_inCs++;
						}

						continue;
					}
				}

				// Item
				{
					if ( m_itemClass && ac->GetEntity()->GetClass()->IsA( m_itemClass ) )
					{
						m_components[ CT_Item ].m_num++;
						if ( isSuppressed ) m_components[ CT_Item ].m_frozenNum++;
						if ( isBudgeted ) m_components[ CT_Item ].m_budgetedNum++;

						if ( ac->IsInCinematic() )
						{
							m_components[ CT_Item ].m_inCs++;
						}

						IActorInterface* actor = ac->GetEntity()->QueryActorInterface();
						if ( actor )
						{
							if ( actor->IsWorking() ) m_components[ CT_Item ].m_inWork++;
							if ( actor->IsInQuestScene() ) m_components[ CT_Item ].m_inBehContr++;
						}

						continue;
					}
				}
				
				// Rest
				m_components[ CT_Rest ].m_num++;
				if ( isSuppressed ) m_components[ CT_Rest ].m_frozenNum++;
				if ( isBudgeted ) m_components[ CT_Rest ].m_budgetedNum++;

				if ( ac->IsInCinematic() )
				{
					m_components[ CT_Rest ].m_inCs++;
				}

				IActorInterface* actor = ac->GetEntity()->QueryActorInterface();
				if ( actor )
				{
					if ( actor->IsWorking() ) m_components[ CT_Rest ].m_inWork++;
					if ( actor->IsInQuestScene() ) m_components[ CT_Rest ].m_inBehContr++;
				}
			}
		}
	}

	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
	{
		IDebugCheckBox::OnRender( frame, x, y, counter, options );

		static const Int32 xOffset = 70;
		static const Int32 xCol1 = 30 + xOffset;
		static const Int32 xCol2 = 100 + xOffset;
		static const Int32 xCol3 = 200 + xOffset;
		static const Int32 xCol4 = 260 + xOffset;
		static const Int32 xCol5 = 350 + xOffset;
		static const Int32 xCol6 = 390 + xOffset;
		static const Int32 xCol7 = 450 + xOffset;
		static const Int32 xCol8 = 490 + xOffset;

		if ( IsExpanded() )
		{
			String str;

			str = String::Printf( TXT("Animated Components %d    Active %d    Frozen %d    Budgeted %d"), m_components[ CT_All ].m_num, m_components[ CT_All ].m_num - m_components[ CT_All ].m_frozenNum, m_components[ CT_All ].m_frozenNum, m_components[ CT_All ].m_budgetedNum );
			frame->AddDebugScreenFormatedText( xCol1, y, str.AsChar() );

			y += 16;

			frame->AddDebugScreenFormatedText( xCol1, y, TXT("Type") );

			frame->AddDebugScreenFormatedText( xCol2, y, TXT("Active") );

			frame->AddDebugScreenFormatedText( xCol3, y, TXT("All") );

			frame->AddDebugScreenFormatedText( xCol4, y, TXT("Frozen") );

			frame->AddDebugScreenFormatedText( xCol5, y, TXT("Budgeted") );

			frame->AddDebugScreenFormatedText( xCol6, y, TXT("Work") );

			frame->AddDebugScreenFormatedText( xCol7, y, TXT("Cutscene") );

			frame->AddDebugScreenFormatedText( xCol8, y, TXT("Beh Contrl") );

			y += 16;

#define ADD_SECTION( name, enumName )																			\
{																												\
	frame->AddDebugScreenFormatedText( xCol1, y, TXT( name ) );													\
																												\
	str = String::Printf( TXT("%d"), m_components[ enumName ].m_num - m_components[ enumName ].m_frozenNum );	\
	frame->AddDebugScreenFormatedText( xCol2, y, str.AsChar() );												\
																												\
	str = String::Printf( TXT("%d"), m_components[ enumName ].m_num );											\
	frame->AddDebugScreenFormatedText( xCol3, y, str.AsChar() );												\
																												\
	str = String::Printf( TXT("%d"), m_components[ enumName ].m_frozenNum );									\
	frame->AddDebugScreenFormatedText( xCol4, y, str.AsChar() );												\
																												\
	str = String::Printf( TXT("%d"), m_components[ enumName ].m_budgetedNum );									\
	frame->AddDebugScreenFormatedText( xCol5, y, str.AsChar() );												\
																												\
	str = String::Printf( TXT("%d"), m_components[ enumName ].m_inWork );										\
	frame->AddDebugScreenFormatedText( xCol6, y, str.AsChar() );												\
																												\
	str = String::Printf( TXT("%d"), m_components[ enumName ].m_inCs );											\
	frame->AddDebugScreenFormatedText( xCol7, y, str.AsChar() );												\
																												\
	str = String::Printf( TXT("%d"), m_components[ enumName ].m_inBehContr );									\
	frame->AddDebugScreenFormatedText( xCol8, y, str.AsChar() );												\
																												\
	y += 16;																									\
}

			ADD_SECTION( "Actor", CT_Actor );
			ADD_SECTION( "Heads", CT_Head );
			ADD_SECTION( "Cameras", CT_Camera );
			ADD_SECTION( "SCameras", CT_StaticCamera );
			ADD_SECTION( "Rest", CT_Rest );

#undef ADD_SECTION
		}
	}
};

class CDebugCheckBoxCutscenes : public IDebugCheckBox
{
	TDynArray< String >		m_csNames;
	TDynArray< Uint32 >		m_csSizes;

public:
	CDebugCheckBoxCutscenes( IDebugCheckBox* parent )
		: IDebugCheckBox( parent, TXT("Cutscenes"), true, false )
	{

	};

	void Reset()
	{
		m_csNames.Clear();
		m_csSizes.Clear();
	}

	virtual void OnTick( Float timeDelta )
	{
		IDebugCheckBox::OnTick( timeDelta );

		if ( IsExpanded() )
		{
			m_csNames.Clear();
			m_csSizes.Clear();

			for ( ObjectIterator< CCutsceneTemplate > it; it; ++it )
			{
				const CCutsceneTemplate* templ = *it;
				
				m_csNames.PushBack( templ->GetDepotPath() );
				m_csSizes.PushBack( 0 );
			}
		}
	}

	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
	{
		IDebugCheckBox::OnRender( frame, x, y, counter, options );

		static const Int32 xOffset = 70;
		static const Int32 xCol1 = 30 + xOffset;
		static const Int32 xCol2 = 100 + xOffset;
		static const Int32 xCol3 = 200 + xOffset;

		if ( IsExpanded() )
		{
			String str;

			// Instances
			{
				frame->AddDebugScreenFormatedText( xCol1, y, TXT("Instances:") );

				TDynArray< String > csInstances;
				GGame->CollectCutsceneInstancesName( csInstances );

				for ( Uint32 i=0; i<csInstances.Size(); ++i )
				{
					y += 16;
					frame->AddDebugScreenFormatedText( xCol2, y, csInstances[i].AsChar() );
				}
			}

			y += 16;

			// Templates
			{
				frame->AddDebugScreenFormatedText( xCol1, y, TXT("Resources:") );

				for ( Uint32 i=0; i<m_csNames.Size(); ++i )
				{
					y += 16;

					String str = String::Printf( TXT("%1.3f[MB]"), m_csSizes[i] );

					frame->AddDebugScreenFormatedText( xCol2, y, str.AsChar() );
					frame->AddDebugScreenFormatedText( xCol3, y, m_csNames[i].AsChar() );
				}
			}
		}
	}
};

class CDebugCheckBoxAnimBehaviors : public IDebugCheckBox
{
public:
	CDebugCheckBoxAnimBehaviors( IDebugCheckBox* parent )
		: IDebugCheckBox( parent, TXT("Behaviors"), true, false )
	{

	};

	virtual void OnTick( Float timeDelta )
	{
		IDebugCheckBox::OnTick( timeDelta );

		if ( IsExpanded() )
		{

		}
	}

	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
	{
		IDebugCheckBox::OnRender( frame, x, y, counter, options );

		if ( IsExpanded() )
		{

		}
	}
};

/*class CDebugCheckBoxMemBar : public IDebugCheckBox
{
public:
	CDebugCheckBoxMemBar( IDebugCheckBox* parent )
		: IDebugCheckBox( parent, TXT("Memory"), true, false )
	{

	};

	virtual void OnTick( Float timeDelta )
	{
		IDebugCheckBox::OnTick( timeDelta );

		if ( IsExpanded() )
		{

		}
	}

	virtual void OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
	{
		IDebugCheckBox::OnRender( frame, x, y, counter, options );

		if ( IsExpanded() )
		{

		}
	}
};*/

//////////////////////////////////////////////////////////////////////////

/// Debug page that display used animations
class CDebugPageAnimations : public IDebugPage
{
public:
	CDebugOptionsTree*			m_tree;
	Bool						m_showTree;

	CDebugCheckBoxSkeletons*	m_optionPoses;
	CDebugCheckBoxAnimSets*		m_optionAnimsets;

	CProfilerStatBox*			m_statBoxBehavior;
	CProfilerStatBox*			m_statBoxSkeleton;
	CProfilerStatBox*			m_statBoxMovement;

public:
	CDebugPageAnimations()
		: IDebugPage( TXT("Animations") )
		, m_tree( NULL )
		, m_optionPoses( NULL )
		, m_optionAnimsets( NULL )
		, m_statBoxBehavior( NULL )
		, m_statBoxSkeleton( NULL )
		, m_statBoxMovement( NULL )
		, m_showTree( false )
	{
	};

	~CDebugPageAnimations()
	{
		delete m_tree;
		delete m_statBoxBehavior;
		delete m_statBoxMovement;
		delete m_statBoxSkeleton;
	}

	void DrawCounter( CRenderFrame* frame, Int32 x, Int32& y, Float current, Float max, const String& name, const String& unit )
	{
		const Int32 width = 300;
		const Int32 height = 15;
		const Float prc = current / max;

		// Set color
		Color color;
		if ( prc > 0.75f )
		{
			color = Color( 128, 0, 0 );
		}
		else if ( prc > 0.5f )
		{
			color = Color( 128, 128, 0 );
		}
		else
		{
			color = Color( 0, 128, 0 );
		}

		DrawProgressBar( frame, x, y, width, height, prc, color );

		// Draw text
		String text = String::Printf( TXT( "%s: %.2f / %.2f %s" ), name.AsChar(), current, max, unit.AsChar() );
		frame->AddDebugScreenText( x+5, y+10, text );

		// Over budget :)
		if ( prc > 1.0f )
		{
			// Draw text
			frame->AddDebugScreenText( x + 5 + width, y+10, TXT("OVER BUDGET!"), Color::RED );
		}
	}

	void DrawSingleCounter( CRenderFrame* frame, Int32 x, Int32& y, Float current, Float max, const String& name, const String& unit )
	{
		DrawCounter( frame, x, y, current, max, name, unit);
		y += 20;
	}

	void DrawCounterSeparator( Int32& y )
	{
		y += 20;
	}

	void DrawFirstCounter( CRenderFrame* frame, Int32 x, Int32& y, Float current, Float max, const String& name, const String& unit )
	{
		DrawCounter( frame, x, y, current, max, name, unit);
	}

	void DrawSecondCounter( CRenderFrame* frame, Int32 x, Int32& y, Float current, Float max, const String& name, const String& unit )
	{
		x += 350;
		DrawCounter( frame, x, y, current, max, name, unit);
	}

	void DrawThirdCounter( CRenderFrame* frame, Int32 x, Int32& y, Float current, Float max, const String& name, const String& unit )
	{
		x += 700;
		DrawCounter( frame, x, y, current, max, name, unit);
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is converted to debug window. If you want use it, click key: Enter.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 60, 120, Color(127, 255, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif

		Int32 y = 50;
		const Int32 xMargin = 50;
		const Int32 xMargin2 = 500;

		Uint32 numActiveAnimations = m_optionAnimsets ? m_optionAnimsets->GetNumActiveAnimations() : 0;
		Uint32 usedAnimationsMemory = m_optionAnimsets ? m_optionAnimsets->GetUsedAnimationsMemory() : 0;

		Uint32 totalAnimationsMemory = m_optionAnimsets? m_optionAnimsets->GetTotalAnimationsMemory() : 0;
		Uint32 totalAnimationsMotionExMemory = m_optionAnimsets? m_optionAnimsets->GetTotalAnimationsMotionExMemory() : 0;
		Uint32 totalAnimationsCompressedFrameMemory = m_optionAnimsets ? m_optionAnimsets->GetTotalCompressedFramesMemory() : 0;
		Uint32 totalAnimationsCompressedFrameDataMemory = m_optionAnimsets ? m_optionAnimsets->GetTotalCompressedFramesDataMemory() : 0;
		Uint32 totalMemory = m_optionAnimsets ? m_optionAnimsets->GetTotalMemory() : 0;

		Uint32 loadedCutscene = m_optionAnimsets ? m_optionAnimsets->GetCutsceneMemory() : 0;
		Uint32 loadedExDialogs = m_optionAnimsets ? m_optionAnimsets->GetExDialogMemory() : 0;

		Int32 graphSize = GAnimationManager ? GAnimationManager->GetGraphStat().m_size : 0;
		Int32 instancesSize = GAnimationManager ? GAnimationManager->GetGraphInstanceStat().m_size : 0;
		Int32 animsStaticSize = GAnimationManager ? GAnimationManager->GetAnimStaticStat().m_size : 0;
		Uint32 totalStreamedAnimationsMemory = GAnimationManager ? GAnimationManager->GetCurrentPoolSize() : 0;

		Int32 graphNum = GAnimationManager ? GAnimationManager->GetGraphStat().m_num : 0;
		Int32 instancesNum = GAnimationManager ? GAnimationManager->GetGraphInstanceStat().m_num : 0;
		Int32 animsStaticNum = GAnimationManager ? GAnimationManager->GetAnimStaticStat().m_num : 0;

		Uint32 numStreamingAnimation = 0;
		Uint32 numLoadedAnimation = 0;
		Float loadedAnimationSize = 0.f;
		CalcNumStreamingAndLoadedAnim( numStreamingAnimation, numLoadedAnimation, loadedAnimationSize );

		const Float megInv = 1.0f / ( 1024.0f * 1024.0f );

		Float posesMem = m_optionPoses ? m_optionPoses->GetTotalPosesMem() : 0.f;
		Float poolSize = GAnimationManager ? (Float)GAnimationManager->GetMaxPoolSize() * megInv: 30.f;

		DrawFirstCounter( frame, xMargin, y, totalMemory * megInv, poolSize, TXT("Total size"), TXT("MB") );
		DrawSecondCounter( frame, xMargin, y, (Float)numLoadedAnimation, 500, TXT("Loaded anims"), TXT("") );
		DrawThirdCounter( frame, xMargin, y, (Float)loadedCutscene * megInv, poolSize, TXT("Loaded cutscenes"), TXT("MB") );
		DrawCounterSeparator( y );
		
		//if ( m_statBoxBehavior ) frame->AddDebugScreenText( xMargin2, y, String::Printf(TXT("Behavior	   ms(%.3lf)    c(%.0lf)"), m_statBoxBehavior->GetAverageTime(), m_statBoxBehavior->GetAverageHitCount() ) );

		DrawFirstCounter( frame, xMargin, y, totalAnimationsMemory * megInv, poolSize, TXT("Anim size"), TXT("MB") );
		DrawSecondCounter( frame, xMargin, y, (Float)numStreamingAnimation, 25.0f, TXT("Streaming anims"), TXT("") );
		DrawThirdCounter( frame, xMargin, y, (Float)loadedExDialogs * megInv, poolSize, TXT("Loaded ex dialogs"), TXT("MB") );
		DrawCounterSeparator( y );
		
		//if ( m_statBoxSkeleton ) frame->AddDebugScreenText( xMargin2, y, String::Printf(TXT("Skeleton	   ms(%.3lf)    c(%.0lf)"), m_statBoxSkeleton->GetAverageTime(), m_statBoxSkeleton->GetAverageHitCount() ) );

		DrawFirstCounter( frame, xMargin, y, totalAnimationsMotionExMemory * megInv, 2.5f, TXT("Motion extraction size"), TXT("MB") );
		DrawSecondCounter( frame, xMargin, y, totalStreamedAnimationsMemory * megInv, poolSize, TXT("Anim size - streamed"), TXT("MB") );
		DrawThirdCounter( frame, xMargin, y, totalStreamedAnimationsMemory * megInv, poolSize, TXT("Anim size - streamed"), TXT("MB") );
		DrawCounterSeparator( y );
		
		//if ( m_statBoxMovement ) frame->AddDebugScreenText( xMargin2, y, String::Printf(TXT("Pose&Con	   ms(%.3lf)    c(%.0lf)"), m_statBoxMovement->GetAverageTime(), m_statBoxMovement->GetAverageHitCount() ) );

		String cfmStr = String::Printf( TXT("Compressed frames size (%1.2f)"), totalAnimationsCompressedFrameDataMemory * megInv );
		DrawFirstCounter( frame, xMargin, y, (Float)totalAnimationsCompressedFrameMemory * megInv, 2.5f, cfmStr, TXT("MB") );
		DrawSecondCounter( frame, xMargin, y, usedAnimationsMemory * megInv, poolSize, TXT("Active size"), TXT("MB") );
		DrawThirdCounter( frame, xMargin, y, (Float)loadedAnimationSize * megInv, poolSize, TXT("Anim size - loaded size"), TXT("MB") );
		DrawCounterSeparator( y );

		DrawFirstCounter( frame, xMargin, y, posesMem * megInv, GAnimationManager->GetPosesPoolSize() * megInv, TXT("Poses"), TXT("MB") );
		DrawSecondCounter( frame, xMargin, y, (Float)numActiveAnimations, 200.0f, TXT("Active animations"), TXT("") );
		DrawThirdCounter( frame, xMargin, y, (Float)totalAnimationsMemory * megInv, poolSize, TXT("Anim size - total"), TXT("MB") );
		DrawCounterSeparator( y );

		// Draw memory bar
		{
			//...
		}

		// Create tree
		if ( !m_tree )
		{
			const Uint32 width = frame->GetFrameOverlayInfo().m_width - 100;
			const Uint32 height = frame->GetFrameOverlayInfo().m_height - 80 - y;

			m_tree = new CDebugOptionsTree( 50, y, width, height, this );

			m_optionPoses = new CDebugCheckBoxSkeletons( NULL, y + height - 100 );
			m_tree->AddRoot( m_optionPoses );

			m_tree->AddRoot( new CDebugCheckBoxAnimComponents( NULL ) );

			m_tree->AddRoot( new CDebugCheckBoxAnimBehaviors( NULL ) );

			m_optionAnimsets = new CDebugCheckBoxAnimSets( NULL, y + height - 16 );
			m_tree->AddRoot( m_optionAnimsets );

			m_tree->AddRoot( new CDebugCheckBoxCutscenes( NULL ) );
		}

		// Render tree
		if ( m_showTree )
		{
			m_tree->OnRender( frame );
		}
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter )
		{
			GDebugWin::GetInstance().SetVisible( true );
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_Animations );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		if ( m_showTree && m_tree && m_tree->OnInput( key, action, data ) )
		{
			return true;
		}

		if ( ( key == IK_R || key == IK_Pad_DigitDown ) && action == IACT_Press )
		{
			if ( GAnimationManager )
			{
				GAnimationManager->MarkToUnloadAllAnimations();
			}

			return true;
		}
		else if ( key == IK_LeftBracket && action == IACT_Press )
		{
			if ( GAnimationManager )
			{
				GAnimationManager->Debug_AddPoolSize( -1 );
			}

			return true;
		}
		else if ( key == IK_RightBracket && action == IACT_Press )
		{
			if ( GAnimationManager )
			{
				GAnimationManager->Debug_AddPoolSize( 1 );
			}

			return true;
		}
		else if ( ( key == IK_T && action == IACT_Press ) || ( key == IK_Pad_DigitRight && action == IACT_Press ) )
		{
			m_showTree = !m_showTree;
		}

		// Not processed
		return false;
	}

	virtual void OnTick( Float timeDelta )
	{
		if ( m_statBoxBehavior )
		{
			m_statBoxBehavior->Tick();
		}
		else
		{
			CPerfCounter* p = CProfiler::GetCounter( "ACUpdateAndSampleBehavior" );
			if ( p )
			{
				m_statBoxBehavior = new CProfilerStatBox( p );
			}
		}

		if ( m_statBoxSkeleton )
		{
			m_statBoxSkeleton->Tick();
		}
		else
		{
			CPerfCounter* p = CProfiler::GetCounter( "ACUpdateAndSampleSkeleton" );
			if ( p )
			{
				m_statBoxSkeleton = new CProfilerStatBox( p );
			}
		}

		if ( m_statBoxMovement )
		{
			m_statBoxMovement->Tick();
		}
		else
		{
			CPerfCounter* p = CProfiler::GetCounter( "FinalizeMovement" );
			if ( p )
			{
				m_statBoxMovement = new CProfilerStatBox( p );
			}
		}

		if ( m_tree )
		{
			m_tree->OnTick( timeDelta );
		}
	}

	void CalcNumStreamingAndLoadedAnim( Uint32& numStr, Uint32& numLoaded, Float& loadedAnimationSize ) const
	{
		for ( AnimationIterator it( false ); it; ++it )
		{
			CSkeletalAnimation* anim = (*it);
			if ( anim )
			{
				if ( anim->IsLoaded() )
				{
					numLoaded++;
					loadedAnimationSize += anim->GetDataSize();
				}

				if ( anim->HasStreamingPending() )
				{
					numStr++;
					loadedAnimationSize += anim->GetDataSize();
				}
			}
		}
	}
};

#ifndef NO_DEBUG_WINDOWS

void CreateDebugPageAnimations()
{
	IDebugPage* page = new CDebugPageAnimations();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif

#endif