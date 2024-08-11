
#include "build.h"
#include "storySceneEventsExecutor.h"
#include "storySceneEventsCollector.h"
#include "storySceneEventsCollector_events.h"
#include "storySceneDebugger.h"
#include "storyScenePlayer.h"
#include "storySceneUtils.h"
#include "storySceneVoicetagMapping.h"
#include "storySceneSystem.h"
#include "../engine/behaviorGraphStack.h"
#include "itemIterator.h"
#include "storySceneEventLightProperties.h"
#include "storySceneItems.h"
#include "../engine/lightComponent.h"
#include "../engine/spotLightComponent.h"
#include "../engine/environmentManager.h"
#include "../engine/dimmerComponent.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

namespace
{
	const CStorySceneLight* FindLightDefinition( const TDynArray< CStorySceneLight* >& lightDefinitions, CName lightId )
	{
		SCENE_ASSERT( lightId != CName::NONE );

		const Uint32 size = lightDefinitions.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const CStorySceneLight* def = lightDefinitions[ i ];
			if ( def && def->m_id == lightId )
			{
				return def;
			}
		}

		return nullptr;
	}

	Bool IsDefaultValue( const Color& color )
	{
		return color.R == 0 && color.G == 0 && color.B == 0;
	}

	Bool IsDefaultValue( Float val )
	{
		return val < 0.f;
	}

	Bool IsDefaultValue( const EngineTransform& val )
	{
		return val.IsIdentity();
	}
}

void CStorySceneEventsExecutor::ProcessLightProperties( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector )
{
	const CStoryScene* scene = player->GetCurrentStoryScene();
	const TDynArray< CStorySceneLight* >& lightDefinitions = scene->GetSceneLightDefinitions();

	const Uint32 size = collector.m_lightProperties.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::LightProperty& evt = collector.m_lightProperties[ i ];
		if ( evt.m_actorId )
		{
			CEntity* light = FindActorByType( evt.m_actorId, AT_LIGHT ).Get();
			if( !light )
			{
				light = player->SpawnSceneLightEntity( evt.m_actorId );
			}
			if ( light )
			{
				CLightComponent*		lc	= light->FindComponent< CLightComponent >();
				CSpotLightComponent*	slc = Cast< CSpotLightComponent >( lc );
				CDimmerComponent*		dc	= light->FindComponent< CDimmerComponent >();

				if ( evt.m_reset )
				{
					if ( const CStorySceneLight* lightDefinition = FindLightDefinition( lightDefinitions, evt.m_actorId ) )
					{
						TLightPropState* s = FindOrCreateActorCachedData( light, m_lightsStates );
						ASSERT( s );
						s->m_data.m_attachment = SStorySceneAttachmentInfo();

						if( light->GetTransformParent() )
						{
 							light->GetTransformParent()->Break();
						}

						if ( lc )
						{

							lc->SetEnabled( false );
							lc->SetColor( Color::WHITE );
							lc->SetBrightness( 100.f );
							lc->SetRadius( 10.f );
							lc->SetAttenuation( 0.5f );
							lc->SetAllowDistantFade( false );
							lc->SetAutoHideDistance( 2000.f, 1.f );

							lc->SetShadowCastingMode( lightDefinition->m_shadowCastingMode );
							lc->SetShadowFadeDistance( lightDefinition->m_shadowFadeDistance );
							lc->SetShadowFadeRange( lightDefinition->m_shadowFadeRange );																					

							if ( slc )
							{
								slc->SetInnerAngle( lightDefinition->m_innerAngle );
								slc->SetOuterAngle( lightDefinition->m_outerAngle );
								slc->SetSoftness( lightDefinition->m_softness );
							}
							lc->RefreshRenderProxies();
						}
						else if( dc )
						{
							dc->SetEnabled( false );
							dc->SetAmbientLevel( 0.f );
							dc->SetMarginFactor( 1.f );
							dc->RefreshRenderProxies();
						}

						//light->ForceUpdateTransform();
						//light->ForceUpdateBounds();						
					}
				}
				else
				{
					// NOTE: In theory these values should be compared to CStorySceneEventLightProperties::GetClass()->GetDefaultObject().
					// However, since we know the default values of color should be 0,0,0 and default value of all floats should be -1.0f

#define CHANGE_LIGHT_PARAM( x ) evt.m_enabled && ( ( evt.m_additiveChanges && !IsDefaultValue( evt.x ) ) || !evt.m_additiveChanges )
					const Bool changeColor = CHANGE_LIGHT_PARAM( m_color );
					const Bool changeRadius = CHANGE_LIGHT_PARAM( m_radius );
					const Bool changeBrightness = CHANGE_LIGHT_PARAM( m_brightness );
					const Bool changeInnerAngle = slc && CHANGE_LIGHT_PARAM( m_innerAngle );
					const Bool changeOuterAngle = slc && CHANGE_LIGHT_PARAM( m_outerAngle );
					const Bool changeSoftness = slc && CHANGE_LIGHT_PARAM( m_softness );				
#undef CHANGE_LIGHT_PARAM

					TLightPropState* s = FindOrCreateActorCachedData( light, m_lightsStates );
					ASSERT( s );
					s->m_data.m_attachment = evt.m_attachment;
					s->m_data.m_transform = evt.m_placementSS;

					Matrix finalPosition;
					EngineTransform placementNoScale = evt.m_placementSS;
					placementNoScale.SetScale( Vector::ONES );

					if (  light->GetTransformParent() != nullptr )
					{
						light->GetTransformParent()->Break();
					}
					if ( !evt.m_attachment.Empty() )
					{
						const SStorySceneAttachmentInfo& satInfo = evt.m_attachment;
						if ( CEntity* parent = FindActorByType( satInfo.m_attachTo, AT_ACTOR | AT_PROP | AT_EFFECT ).Get() )
						{
							HardAttachmentSpawnInfo ainfo;
							ainfo.m_parentSlotName = satInfo.m_parentSlotName;
							ainfo.m_freeRotation = satInfo.m_freeRotation;
							ainfo.m_freePositionAxisX = satInfo.m_freePositionAxisX;
							ainfo.m_freePositionAxisY = satInfo.m_freePositionAxisY;
							ainfo.m_freePositionAxisZ = satInfo.m_freePositionAxisZ;

							if ( CComponent* parentComponent = parent->GetRootAnimatedComponent() )
							{
								parentComponent->Attach( light, ainfo );
							}
							else
							{
								parent->Attach( light, ainfo );
							}
						}
						placementNoScale.CalcLocalToWorld( finalPosition );
					}
					else
					{
						finalPosition = StorySceneUtils::CalcWSFromSS( placementNoScale, evt.m_sceneTransformWS );
					}
						
					if( evt.m_lightTracker.m_enable )
					{
						Vector pos;
						EulerAngles rot;
						if( StorySceneUtils::CalcTrackedPosition( evt.m_lightTracker, player, pos, rot ) )
						{								
							light->Teleport( finalPosition.GetTranslation() + pos, rot );
						}								
					}
					else
					{												
						light->Teleport( finalPosition.GetTranslation(), finalPosition.ToEulerAngles() );
					}

					if( lc )
					{
						lc->SetEnabled( evt.m_enabled );

						if ( changeColor )
						{
							lc->SetColor( evt.m_color );
						}

						if ( changeRadius )
						{
							lc->SetRadius( evt.m_radius );
						}

						lc->SetAttenuation( evt.m_attenuation );

						if ( changeBrightness )
						{
							lc->SetBrightness( evt.m_brightness );
						}

						lc->SetLightFlickering( evt.m_flickering );

						if ( slc )
						{
							if ( changeInnerAngle )
							{
								slc->SetInnerAngle( evt.m_innerAngle );
							}

							if ( changeOuterAngle )
							{
								slc->SetOuterAngle( evt.m_outerAngle );
							}

							if ( changeSoftness )
							{
								slc->SetSoftness( evt.m_softness );
							}
						}

						lc->RefreshRenderProxies();
					}
					else if( dc )
					{
						dc->SetAmbientLevel( evt.m_ambientLevel );
						dc->SetMarginFactor( evt.m_marginFactor );
						dc->SetScale( evt.m_placementSS.GetScale() );
					}

					light->ForceUpdateTransformNodeAndCommitChanges();
					light->ForceUpdateBoundsNode();
				}
			}
		}
	}

	CWorld* world = player->GetLayer()->GetWorld();
	if( world && !player->IsGameplayNow() )
	{
		if( CEnvironmentManager* mgr = world->GetEnvironmentManager() )
		{
			const Uint32 size = collector.m_cameralightProperties.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				SCameraLightsModifiersSetup cameraSetup = collector.m_cameralightProperties[ i ].cameraSetup;
				cameraSetup.SetScenesSystemActiveFactor( 1.f );
				mgr->SetCameraLightsModifiers( cameraSetup );				
			}
		}		
	}
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
