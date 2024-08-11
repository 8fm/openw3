/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "extAnimFootstepEvent.h"

#include "actorAnimationEventFilter.h"
#include "..\engine\soundStartData.h"
#include "..\engine\soundSystem.h"
#include "..\engine\dynamicDecal.h"
#include "..\engine\decalEmitter.h"
#include "..\engine\decalEmitter.h"

#include "..\engine\baseTree.h"
#include "..\engine\foliageBroker.h"
#include "..\engine\foliageCell.h"
#include "..\engine\foliageResource.h"
#include "..\engine\foliageScene.h"

#include "..\engine\game.h"
#include "vehicle.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimFootstepEvent );

CExtAnimFootstepEvent::CExtAnimFootstepEvent()
	: CExtAnimSoundEvent()
	, m_fx( true )
{
	m_reportToScript = false;
	m_filter = true;
	m_filterCooldown = 0.2f;
}

CExtAnimFootstepEvent::CExtAnimFootstepEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimSoundEvent( eventName, animationName, startTime, trackName )
{
	m_reportToScript = false;
}

CExtAnimFootstepEvent::~CExtAnimFootstepEvent()
{
}

void CExtAnimFootstepEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	component->MarkProcessPostponedEvents();
}

void CExtAnimFootstepEvent::ProcessPostponed( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CExtAnimFootstepEvent can only be called from Main Thread." );

	ASSERT( component != NULL );

	if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( component ) )
	{
		CEntity* entity = component->GetEntity();
		if ( !entity )
		{
			ASSERT( entity != NULL );
			return;
		}

		const Int32 bone = component->FindBoneByName( m_bone );

		static Float thr = 0.3f;
		const Matrix& localToWorld = entity->GetLocalToWorld();

		CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent( false );

#ifndef NO_EDITOR
		Int32 listMask = soundEmitterComponent ? soundEmitterComponent->GetListenerMask( ) : 0;
		if ( info.m_alpha > thr && !CSoundEmitterComponent::IsMaxDistanceReached( localToWorld.GetTranslationRef(), m_maxDistance, listMask ))
#else
		if ( info.m_alpha > thr && !CSoundEmitterComponent::IsMaxDistanceReached( localToWorld.GetTranslationRef(), m_maxDistance ) )
#endif
		{
			if( CActor* actor = Cast< CActor >( entity ) )
			{
				if( ( actor->GetHideReason() & CEntity::HR_Scene ) == CEntity::HR_Scene )
				{
					return;
				}
				if( m_filter && actor->UsesAnimationEventFilter() )
				{
					if( !actor->GetAnimationEventFilter()->CanTriggerEvent( GetSoundEventName(), bone, m_filterCooldown ) )
					{
						return;
					}
				}
			}
			
			const SPhysicalMaterial* material = mac->GetCurrentStandPhysicalMaterial();

			if ( m_fx && material && entity->IsPlayer() )
			{
				const CName effectName = m_customFxName.Empty() ? material->m_particleFootsteps : m_customFxName;
				if ( effectName )
				{
					entity->PlayEffect( effectName, m_bone, entity );
				}
			}

			// Add some foliage effects if available :) (BOB/GOTY only)
			ProcessFoliage( entity );
			
			CName materialName = material ? material->m_name : CNAME( default );

			//EP2 new-material overrides because we are unable to make the change to the objects in wwise
			if(materialName == CName(TXT("water_deep_river")))
			{
				materialName = CName(TXT("water_deep"));
			}
			else if(materialName == CName(TXT("dettlaff_flesh")))
			{
				materialName = CName(TXT("mud"));
			}

			Matrix bonePosition( localToWorld );
			const CAnimatedComponent* ac = entity->GetRootAnimatedComponent();
			if ( ac && bone != -1 )
			{
				bonePosition = ac->GetBoneMatrixWorldSpace( bone );
			}


#if !defined(RED_FINAL_BUILD)
			StringAnsi animationName = "";
			if(info.m_animInfo.m_animation && info.m_animInfo.m_animation->GetAnimation())
			{
				animationName = info.m_animInfo.m_animation->GetAnimation()->GetName().AsAnsiChar();
			}
			const StringAnsi soundObjectName(StringAnsi("AnimFootstepEvent: ") + StringAnsi(UNICODE_TO_ANSI(entity->GetName().AsChar()) + StringAnsi(", Anim Name:  ") + animationName + StringAnsi(", Event name: ") + info.GetEventName().AsAnsiChar()));
#else
			const StringAnsi soundObjectName;
#endif
			if(m_speed >= 0.f)
			{
				CAuxiliarySoundEmitter * auxEmitter = entity->GetSoundEmitterComponent()->GetAuxiliaryEmitter(bone);
				auxEmitter->SoundSwitch(CSoundSystem::PARAM_SURFACE, materialName.AsAnsiChar());
				entity->GetSoundEmitterComponent()->AuxiliarySoundEvent(GetSoundEventName().AsChar(), m_speed, m_decelDist, bone);
			}
			else
			{

				Float occlusion = 0.f, obstruction = 0.f;
				if(soundEmitterComponent)
				{
					SSoundAnimHistoryInfo prevEvent = soundEmitterComponent->GetPreviousFootstepEvent();
					Float engineTime = GGame->GetEngineTime();
					if(bone == prevEvent.bone && (engineTime - prevEvent.time < 0.1f) && GetSoundEventName() == prevEvent.eventName)
					{
						return;
					}

					soundEmitterComponent->NotifyFootstepEvent(GetSoundEventName(), bone);

					occlusion = soundEmitterComponent->GetCurrentOcclusion();
					obstruction = soundEmitterComponent->GetCurrentObstruction();
				}
#ifndef NO_EDITOR
				if( listMask )
				{
					CLightweightSoundEmitter( bonePosition.GetTranslationRef(), soundObjectName ).ListenerSwitch( listMask ).Switch( CSoundSystem::PARAM_SURFACE, materialName.AsAnsiChar() ).Event( GetSoundEventName().AsChar() ).Reverb( component->GetWorld()->GetTriggerManager(), bonePosition.GetTranslationRef()).OcclusionParams(occlusion, obstruction);
				}
				else
				{
					CLightweightSoundEmitter( bonePosition.GetTranslationRef(), soundObjectName ).Switch( CSoundSystem::PARAM_SURFACE, materialName.AsAnsiChar() ).Event( GetSoundEventName().AsChar() ).Reverb( component->GetWorld()->GetTriggerManager(), bonePosition.GetTranslationRef() ).OcclusionParams(occlusion, obstruction);
				}
#else
				CLightweightSoundEmitter( bonePosition.GetTranslationRef(), soundObjectName ).Switch( CSoundSystem::PARAM_SURFACE, materialName.AsAnsiChar() ).Event( GetSoundEventName().AsChar() ).Reverb( component->GetWorld()->GetTriggerManager(), bonePosition.GetTranslationRef() ).OcclusionParams(occlusion, obstruction);
#endif
			}
		}
	}
}

void CExtAnimFootstepEvent::ProcessFoliage( CEntity* entity ) const
{
	CWorld* world = GGame->GetActiveWorld( );
	if( world )
	{
		CFoliageScene* foliage = world->GetFoliageScene( );
		if( foliage )
		{
			const CellHandle cellHandle = foliage->GetFoliageBroker( )->AcquireCell( entity->GetPosition( ) );
			if( cellHandle->IsResourceValid( ) )
			{
				CFoliageResource* foliageResource = cellHandle->GetFoliageResource( );
				if( foliageResource != nullptr )
				{
					Box entityBox = entity->CalcBoundingBox( );

					// Make the box thinner.
					{
						Vector quarter = ( entityBox.Max - entityBox.Min ) * 0.25f;
						quarter.Z = 0.0f;
						entityBox.Min += quarter;
						entityBox.Max -= quarter;
					}

					// Thresholds used to filter out false positives (rocks treated as trees, etc..).
					const Float entityBoxHeight = entityBox.Max.Z - entityBox.Min.Z;
					const Float treeMinHeight = entityBoxHeight * 0.5f;
					const Float treeMaxHeight = entityBoxHeight + entityBoxHeight * 0.75f;
					const Float loGrassMinHeight = entityBoxHeight * 0.35f;
					const Float hiGrassMinHeight = entityBoxHeight * 0.90f;

					const CFoliageResource::InstanceGroupContainer& treeGroup = foliageResource->GetAllTreeInstances( );
					for( auto group = treeGroup.Begin( ); group != treeGroup.End( ); ++group )
					{
						const FoliageInstanceContainer& instances = group->instances;
						for( auto instance = instances.Begin( ); instance != instances.End( ); ++instance )
						{
							const Float s = instance->GetScale( );
							const Box instanceBox = group->baseTree->GetBBox( ) * Vector( s, s, s ) + instance->GetPosition( );
							const Float instanceBoxHeight = instanceBox.Max.Z - instanceBox.Min.Z;

							if( instanceBox.Touches2D( entityBox ) )
							{
								if( instanceBoxHeight >= treeMinHeight && instanceBoxHeight <= treeMaxHeight )
								{
									CLightweightSoundEmitter( instance->GetPosition(), StringAnsi() ).Event( "g_bob_foliage_mv_through_bush" );
									return;
								}
							}
						}
					}

					const CFoliageResource::InstanceGroupContainer& grassGroup = foliageResource->GetAllGrassInstances( );
					for( auto group = grassGroup.Begin( ); group != grassGroup.End( ); ++group )
					{
						const FoliageInstanceContainer& instances = group->instances;
						for( auto instance = instances.Begin( ); instance != instances.End( ); ++instance )
						{
							const Float s = instance->GetScale( );
							const Box instanceBox = group->baseTree->GetBBox( ) * Vector( s, s, s ) + instance->GetPosition( );
							const Float instanceBoxHeight = instanceBox.Max.Z - instanceBox.Min.Z;

							if( instanceBox.Touches2D( entityBox ) )
							{
								if( instanceBoxHeight >= loGrassMinHeight )
								{
									if( instanceBoxHeight >= hiGrassMinHeight )
										CLightweightSoundEmitter( instance->GetPosition(), StringAnsi() ).Event( "g_bob_foliage_mv_through_grass_very_high" );
									else
										CLightweightSoundEmitter( instance->GetPosition(), StringAnsi() ).Event( "g_bob_foliage_mv_through_grass_high" );
									return;
								}
							}
						}
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////
//
//      CExtForcedLogicalFootstepAnimEvent
//
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtForcedLogicalFootstepAnimEvent );

CExtForcedLogicalFootstepAnimEvent::CExtForcedLogicalFootstepAnimEvent()
	: CExtAnimEvent()
{
}

//////////////////////////////////////////////////////////////////////////
CExtForcedLogicalFootstepAnimEvent::CExtForcedLogicalFootstepAnimEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
{
}

//////////////////////////////////////////////////////////////
void CExtForcedLogicalFootstepAnimEvent::Process( const CAnimationEventFired&, CAnimatedComponent* component ) const 
{
	component->MarkProcessPostponedEvents();
}
