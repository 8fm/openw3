
#include "build.h"
#include "mimicComponent.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphStack.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphAnimationSlotNode.h"
#include "meshComponent.h"
#include "mesh.h"
#include "normalBlendComponent.h"
#include "animMimicParam.h"
#include "entityTemplate.h"
#include "entity.h"
#include "world.h"
#include "layer.h"

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
extern Matrix anselCameraTransform;
#endif // USE_ANSEL

//#define DEBUG_MIMICS
#ifdef DEBUG_MIMICS
#pragma optimize("",off)
#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CMimicComponent );

#define LOD_SQR( x ) ((x)*(x))
#define RENDERING_LOD_1_TO_2_DIST	7.f
#define LOD_1_TO_2_DIST				RENDERING_LOD_1_TO_2_DIST + 2.0f
#define LOD_1_TO_2_DIST_THR			1.0f

RED_DEFINE_STATIC_NAMED_NAME( VAR_NAME_GAMEPLAY_WEIGHT, "gameplayWeight" );
RED_DEFINE_STATIC_NAMED_NAME( VAR_NAME_GAMEPLAY_LOD_WEIGHT, "gameplayLodWeight" );

const Float CMimicComponent::LOD_1_TO_2_DIST2 = LOD_SQR( LOD_1_TO_2_DIST + LOD_1_TO_2_DIST_THR );
const Float CMimicComponent::LOD_2_TO_1_DIST2 = LOD_SQR( LOD_1_TO_2_DIST - LOD_1_TO_2_DIST_THR );

#undef LOD_SQR
#undef RENDERING_LOD_1_TO_2_DIST
#undef LOD_1_TO_2_DIST
#undef LOD_1_TO_2_DIST_THR

RED_DEFINE_STATIC_NAME( MIMIC_SLOT_GMPL )

CMimicComponent::CMimicComponent()
	: m_cachedHeadIdx( -1 )
	, m_cachedNeckIdx( -1 )
	, m_categoryMimics( nullptr )
	, m_tempCanUseLod( false )
	, m_requestRefreshAllMeshedWithLod0( false )
	, m_firstUpdate( false )
	, m_mimicLod( EML_Lod_Unknown )
	, m_requestedMimicLod( EML_Lod_Unknown )
	, m_attachmentSlotName( CNAME( torso2 ) )
{
	m_allowPoseActions = false;
	m_allowScriptNotifications = false;
}

void CMimicComponent::OnInitialized()
{
	if ( !m_postInstanceInitializationDone )
	{
		ApplyMimicParams();
	}

	TBaseClass::OnInitialized();
}

void CMimicComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CMimicComponent_OnAttached );

	CacheNBComponentsAndSetupAreas();
}

void CMimicComponent::OnDetached( CWorld *world )
{
	m_cachedNBComponents.Clear();

	TBaseClass::OnDetached( world );
}

void CMimicComponent::OnAppearanceChanged( Bool added )
{
	if ( added )
	{
		CAnimatedComponent* animComponent = GetEntity()->GetRootAnimatedComponent();
		AnimatedAttachmentSpawnInfo sinfo;
		if ( animComponent != nullptr && !GetTransformParent() )
		{
			sinfo.m_parentSlotName = m_attachmentSlotName;
			animComponent->Attach( this, sinfo );
		}

		ApplyMimicParams();
		CacheNBComponentsAndSetupAreas();
	}
}

void CMimicComponent::OnStreamIn()
{
	if ( IsUsedInAppearance() )
	{
		CAnimatedComponent* animComponent = GetEntity()->GetRootAnimatedComponent();
		AnimatedAttachmentSpawnInfo sinfo;
		if ( animComponent != nullptr && !GetTransformParent() )
		{
			sinfo.m_parentSlotName = m_attachmentSlotName;
			animComponent->Attach( this, sinfo );
		}

		ApplyMimicParams();
		CacheNBComponentsAndSetupAreas();
	}
}

void CMimicComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	if ( m_firstUpdate )
	{
		m_requestRefreshAllMeshedWithLod0 = false;
	}
}

void CMimicComponent::OnPostInitializationDone()
{
	TBaseClass::OnPostInitializationDone();

	// Force T Pose
	const CSkeleton* skeleton = GetSkeleton();
	if ( skeleton && m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetSampledPose();
		pose.SetPose( m_skeleton.Get() );
	}

	m_requestRefreshAllMeshedWithLod0 = true;
}

Bool CMimicComponent::ShouldAddToTickGroups() const
{
	return false;
}

CSkeleton* CMimicComponent::GetSkeleton() const
{
	return m_mimicFace ? m_mimicFace->GetSkeleton() : NULL;
}

CSkeleton* CMimicComponent::GetMimicSkeleton() const
{
	return m_mimicFace ? m_mimicFace->GetFloatTrackSkeleton() : NULL;
}

Bool CMimicComponent::MimicHighOn()
{
	SetMimicLod( EML_Lod0_MimicHigh );

	ForceMeshHiResLOD( true );

	return true;
}

void CMimicComponent::MimicHighOff()
{
	const Bool isItemHead = GetEntity()->GetClass()->GetName() == TXT("CItemEntity");
	if ( GetRootEntity()->IsPlayer() || isItemHead )
	{
		return;
	}

#ifdef USE_GMPL_MIMICS
	if ( m_tempCanUseLod )
	{
		SetMimicLod( EML_Lod1_MimicLow );
	}
	else
#endif
	{
		SetMimicLod( EML_Lod2_NeckHead );
	}

	ForceMeshHiResLOD( false );
}

Bool CMimicComponent::HasMimicHigh() const
{
	return m_mimicLod == EML_Lod0_MimicHigh;
}

CAnimMimicParam* CMimicComponent::GetTemplateMimicParams()
{
	CEntity* entity = GetEntity();
	if ( entity )
	{
		CEntityTemplate* templ = entity->GetEntityTemplate();
		if ( templ )
		{
			return templ->FindParameter< CAnimMimicParam >();
		}
	}
	return nullptr;
}

CAnimMimicParam* CMimicComponent::GetParentTemplateMimicParams()
{
	// gets attachment
	CHardAttachment* attachment = GetTransformParent();
	if ( attachment )
	{
		// gets component we are attached to
		CNode* parent = attachment->GetParent();
		if ( parent )
		{
			CComponent* parentComponent = parent->AsComponent();
			{
				// gets component entity
				if ( parentComponent->GetEntity() )
				{
					// gets parent component entity template
					CEntityTemplate* templ = GetTransformParent()->GetParent()->AsComponent()->GetEntity()->GetEntityTemplate();
					if ( templ )
					{
						return templ->FindParameter< CAnimMimicParam >();
					}
				}
			}
		}
		
	}
	
	return nullptr;
}

void CMimicComponent::ApplyMimicParams()
{
	CAnimMimicParam* mimicParam = GetTemplateMimicParams();
	if ( !mimicParam )
	{
		// if can't find mimic param in template check template of entity we are attached to
		mimicParam = GetParentTemplateMimicParams();
	}
	if ( !mimicParam )
	{
		WARN_ENGINE( TXT("Couldn't setup head '%ls'. Actor '%ls' hasn't got CAnimMimicParam in entity template '%ls'"), 
		GetName().AsChar(), GetEntity()->GetName().AsChar(), GetEntity()->GetEntityTemplate() ? GetEntity()->GetEntityTemplate()->GetDepotPath().AsChar() : TXT("unknown") );
	}
	else
	{
		m_animationSets = mimicParam->GetAnimsets();
		m_behaviorInstanceSlots = mimicParam->GetInstanceSlots();
	}
}

void CMimicComponent::CacheNBComponentsAndSetupAreas()
{
	m_tempCanUseLod = false;

	m_cachedNBComponents.Clear();

	TList< IAttachment* >::const_iterator attIt = GetChildAttachments().Begin();
	TList< IAttachment* >::const_iterator end = GetChildAttachments().End();

	for (  ; attIt != end; ++attIt )
	{
		IAttachment* attachment = *attIt;
		if ( attachment && attachment->GetChild()->IsA< CNormalBlendComponent >() )
		{
			CNormalBlendComponent* comp = static_cast< CNormalBlendComponent* >( attachment->GetChild() );

			if ( m_mimicFace )
			{
				TDynArray< Vector > nbAreas;
				m_mimicFace->GetNormalBlendAreasNormalized( nbAreas );
				comp->SetAreas( nbAreas );
			}

			m_cachedNBComponents.PushBack( comp );
		}
		else if ( attachment && attachment->GetChild()->IsA< CMeshComponent >() )
		{
			CMeshComponent* comp = static_cast< CMeshComponent* >( attachment->GetChild() );
			if ( comp )
			{
				const Uint8 lch = comp->GetLightChannels();

				const Bool hasCharLightChannel = 0 != ( lch & LC_Characters );
				if ( hasCharLightChannel )
				{
					if ( CMesh* mesh = comp->GetMeshNow() )
					{
						if ( mesh->GetNumLODLevels() > 2 )
						{
							const Float dist0 = mesh->GetLODLevel( 0 ).GetDistance();
							const Float dist1 = mesh->GetLODLevel( 1 ).GetDistance();
							const Float dist2 = mesh->GetLODLevel( 2 ).GetDistance();

							if ( dist0 < 0.01f && dist1 < 0.01f && MAbs( dist2 - 7.f ) < 0.01f )
							{
								m_tempCanUseLod = true;
							}
						}
					}
				}
			}
		}
	}

	const Bool isItemHead = GetEntity()->GetClass()->GetName() == TXT("CItemEntity");
	if ( GetRootEntity()->IsPlayer() || isItemHead )
	{
		MimicHighOn();
	}
	else
	{
		if ( !m_tempCanUseLod )
		{
			SetMimicLod( EML_Lod2_NeckHead );
		}
		else
		{
			SetMimicLod( EML_Lod1_MimicLow );
		}
	}
}

void CMimicComponent::ForceMeshHiResLOD( Bool flag )
{
	for ( TList< IAttachment* >::iterator it = m_childAttachments.Begin(); it != m_childAttachments.End(); ++it )
	{
		IAttachment* att = *it;

		if ( att && att->GetChild()->IsA< CMeshTypeComponent >() )
		{
			CMeshTypeComponent* mesh = static_cast< CMeshTypeComponent* >( att->GetChild() );
			mesh->SetForcedHighestLOD( flag );
		}
	}
}

void CMimicComponent::SetMimicLod( EMimicLOD lod )
{
	m_mimicLod = lod;
	m_requestedMimicLod = lod;

#ifdef USE_GMPL_MIMICS
	if ( m_tempCanUseLod )
	{
		if ( m_mimicLod == EML_Lod0_MimicHigh )
		{
			SetLod( BL_Lod0 );

			VERIFY( SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_WEIGHT ), 0.f ) );
			VERIFY( SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_LOD_WEIGHT ), 1.f ) );
		}
		else if ( m_mimicLod == EML_Lod1_MimicLow )
		{
			SetLod( BL_Lod1 );

			VERIFY( SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_WEIGHT ), 1.f ) );
			VERIFY( SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_LOD_WEIGHT ), 1.f ) );
		}
		else if ( m_mimicLod == EML_Lod2_NeckHead )
		{
			SetLod( BL_Lod2 );

			VERIFY( SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_WEIGHT ), 1.f ) );
			VERIFY( SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_LOD_WEIGHT ), 0.f ) );

			// Reset pose (hi-res bones), we will touch only low-res bones
			if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
			{
				SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetMainPose();
				pose.SetPose( GetSkeleton() );
			}
		}
	}
	else
#endif
	{
		if ( m_mimicLod == EML_Lod0_MimicHigh )
		{
			SetLod( BL_Lod0 );

			VERIFY( SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_WEIGHT ), 0.f ) );
			VERIFY( SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_LOD_WEIGHT ), 1.f ) );
		}
		else if ( m_mimicLod == EML_Lod1_MimicLow || m_mimicLod == EML_Lod2_NeckHead )
		{
			SetLod( BL_Lod0 );

			VERIFY( SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_WEIGHT ), 1.f ) );
			VERIFY( SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_LOD_WEIGHT ), 1.f ) );

			// Reset pose (hi-res bones), we will touch only low-res bones
			if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
			{
				SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetMainPose();
				pose.SetPose( GetSkeleton() );
			}
		}
	}
}

void CMimicComponent::SetMimicLodSmooth( EMimicLOD lod )
{
#ifdef USE_GMPL_MIMICS
	ASSERT( m_tempCanUseLod );
#endif

	m_requestedMimicLod = lod;
}

void CMimicComponent::InternalUpdateDestLodState( const Vector& cameraPosition, Float camFovFactor )
{
#ifdef USE_GMPL_MIMICS
	ASSERT( m_tempCanUseLod );
#endif

	const Vector& thisPos = GetWorldPositionRef();

	const Float dist2 = cameraPosition.DistanceSquaredTo( thisPos ) * camFovFactor;

	if ( m_mimicLod == EML_Lod1_MimicLow )
	{
		if ( dist2 > LOD_1_TO_2_DIST2 )
		{
			SetMimicLodSmooth( EML_Lod2_NeckHead );
		}
	}
	else if ( m_mimicLod == EML_Lod2_NeckHead )
	{
		if ( dist2 < LOD_2_TO_1_DIST2 )
		{
			SetMimicLodSmooth( EML_Lod1_MimicLow );
		}
	}
	else
	{
		ASSERT( 0 );
	}
}

#ifndef NO_EDITOR
void CMimicComponent::Editor_RefreshCameraPosition( const Vector& cameraPosition, Float cameraFov )
{
#ifdef USE_GMPL_MIMICS
	if ( m_tempCanUseLod && ( ( m_mimicLod >= EML_Lod1_MimicLow || m_requestedMimicLod >= EML_Lod1_MimicLow ) ) )
	{
		const Float camFovFactor = MeshUtilities::CalcFovDistanceMultiplier( cameraFov );
		InternalUpdateDestLodState( cameraPosition, camFovFactor );
	}
#endif
}

void CMimicComponent::Editor_GetStateDesc( String& desc ) const
{
#ifdef USE_GMPL_MIMICS
	desc = String::Printf( TXT("Mimics [%ls]: "), m_tempCanUseLod ? TXT("ON") : TXT("OFF") );
	if ( m_mimicLod == EML_Lod0_MimicHigh )
	{
		desc += TXT("Lod0_High");
	}
	else if ( m_mimicLod == EML_Lod1_MimicLow )
	{
		desc += TXT("Lod1_Low ");
	}
	if ( m_mimicLod == EML_Lod2_NeckHead )
	{
		desc += TXT("Lod2_Head");
	}

	if ( m_mimicLod != m_requestedMimicLod )
	{
		desc += TXT(" -> ");

		if ( m_requestedMimicLod == EML_Lod0_MimicHigh )
		{
			desc += TXT("Lod0_High");
		}
		else if ( m_requestedMimicLod == EML_Lod1_MimicLow )
		{
			desc += TXT("Lod1_Low ");
		}
		if ( m_requestedMimicLod == EML_Lod2_NeckHead )
		{
			desc += TXT("Lod2_Head");
		}
	}
#endif
}

#endif

void CMimicComponent::UpdateMimicLodState()
{
	if ( ( m_mimicLod >= EML_Lod1_MimicLow || m_requestedMimicLod >= EML_Lod1_MimicLow ) && !GetLayer()->GetWorld()->GetPreviewWorldFlag() &&  GetLayer()->GetWorld()->GetCameraDirector() )
	{
		Vector camPos;
		Float camFovFactor;
		if ( !GGame->IsFreeCameraEnabled() )
		{
			const CCameraDirector* camDir = GetLayer()->GetWorld()->GetCameraDirector();
			camPos = camDir->GetCameraPosition();
			if( !( GetLayer()->GetWorld()==GGame->GetActiveWorld() && GGame->IsActive() ) )
			{
				camPos = GGame->GetActiveWorld()->GetCameraPosition();
			}
			camFovFactor = camDir->GetFovDistanceMultiplier();
		}
		else
		{
			GGame->GetFreeCameraWorldPosition( &camPos, NULL, NULL );
			camFovFactor = GGame->GetFreeCameraFovDistanceMultiplier();
		}
#ifdef USE_ANSEL
		if ( isAnselSessionActive )
		{
			camPos = anselCameraTransform.GetTranslation();
		}
#endif // USE_ANSEL
		InternalUpdateDestLodState( camPos, camFovFactor );
	}

	if ( m_mimicLod != m_requestedMimicLod )
	{
		if ( m_requestedMimicLod == EML_Lod0_MimicHigh )
		{
			SetMimicLod( m_requestedMimicLod );
		}
		else if ( m_requestedMimicLod == EML_Lod1_MimicLow )
		{
			SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_LOD_WEIGHT ), 1.f );

			//if ( GetMimicInternalVariable( CNAME( VAR_NAME_GAMEPLAY_LOD_WEIGHT_FEEDBACK ) ) >= 1.f - FLT_EPSILON )
			{
				SetMimicLod( m_requestedMimicLod );
			}
		}
		else if ( m_requestedMimicLod == EML_Lod2_NeckHead )
		{
			SetMimicVariable( CNAME( VAR_NAME_GAMEPLAY_LOD_WEIGHT ), 0.f );

			//if ( GetMimicInternalVariable( CNAME( VAR_NAME_GAMEPLAY_LOD_WEIGHT_FEEDBACK ) ) <= FLT_EPSILON )
			{
				SetMimicLod( m_requestedMimicLod );
			}
		}
	}
}

void CMimicComponent::ProcessMimicData( const SMimicPostProcessData* mimicData, SBehaviorGraphOutput* pose, SBehaviorGraphOutput* parentsPose, const BoneMappingContainer& mapping )
{
	// if ( proper lod )
	{
		if ( pose && parentsPose && m_mimicFace )
		{
			if ( m_cachedHeadIdx == -1 && m_cachedHeadIdx != -2 )
			{
				m_cachedHeadIdx = -2;

				Int32 neck, head;
				m_mimicFace->GetNeckAndHead( neck, head );

				if ( neck != -1 && head != -1 )
				{
					Uint32 count = 0;

					const Uint32 s = mapping.Size();
					for ( Uint32 i=0; i<s; ++i )
					{
						const SBoneMapping& boneM = mapping[ i ];

						if ( boneM.m_boneA == neck )
						{
							count++;

							m_cachedNeckIdx = i;
						}
						else if ( boneM.m_boneA == head )
						{
							count++;

							m_cachedHeadIdx = i;
						}

						if ( count == 2 )
						{
							break;
						}
					}

					if ( count != 2 )
					{
						m_cachedHeadIdx = -2;
					}
				}
			}

			if ( m_cachedHeadIdx >= 0 )
			{
				ASSERT( m_cachedHeadIdx < mapping.SizeInt() );
				ASSERT( m_cachedNeckIdx < mapping.SizeInt() );

				const SBoneMapping& neckM = mapping[ m_cachedNeckIdx ];
				const SBoneMapping& headM = mapping[ m_cachedHeadIdx ];

				if ( neckM.m_boneB != -1 && neckM.m_boneB < (Int32)parentsPose->m_numBones && headM.m_boneB != -1 && headM.m_boneB < (Int32)parentsPose->m_numBones )
				{
					// 2. Lipsync offset
					{
						/*const Float maxAngle = 5.f;
						const Float val = maxAngle * mimicData->m_mimicLipsyncOffset;

						RedQuaternion rotationQuat( RedVector4( 0.f, 0.f, 1.f ), DEG2RAD( val ) );
						RedQsTransform rotation( RedVector4( 0.0f, 0.0f, 0.0f, 0.0f ), rotationQuat, RedVector4( 1.0f, 1.0f, 1.0f, 1.0f ) );

						ASSERT( pose->m_outputPose[ headM.m_boneA ].IsAlmostEqual( parentsPose->m_outputPose[ headM.m_boneB ] ) );

						pose->m_outputPose[ headM.m_boneA ].SetMul( pose->m_outputPose[ headM.m_boneA ], rotation );

						parentsPose->m_outputPose[ headM.m_boneB ].SetMul( parentsPose->m_outputPose[ headM.m_boneB ], rotation );

						ASSERT( pose->m_outputPose[ headM.m_boneA ].IsAlmostEqual( parentsPose->m_outputPose[ headM.m_boneB ] ) );*/
					}

					// 3. Bones
					{
						COMPILE_ASSERT( MIMIC_POSE_BONES_NUM == 0 );

						/*
						RedQsTransform t0 = mimicData->m_mimicBones[ 0 ];
						RedQsTransform t1 = mimicData->m_mimicBones[ 1 ];

						// was it inverted for any particular reason? without inversion it looks as intended
						//t0.Rotation.SetInverse( t0.Rotation );
						//t1.Rotation.SetInverse( t1.Rotation );

						ASSERT( pose->m_outputPose[ neckM.m_boneA ].IsAlmostEqual( parentsPose->m_outputPose[ neckM.m_boneB ] ) );
						ASSERT( pose->m_outputPose[ headM.m_boneA ].IsAlmostEqual( parentsPose->m_outputPose[ headM.m_boneB ] ) );

						//pose->m_outputPose[ neckM.m_boneA ].SetMul( pose->m_outputPose[ neckM.m_boneA ], t0 ); // neck
						//pose->m_outputPose[ headM.m_boneA ].SetMul( pose->m_outputPose[ headM.m_boneA ], t1 ); // head

						parentsPose->m_outputPose[ neckM.m_boneB ].SetMul( parentsPose->m_outputPose[ neckM.m_boneB ], t0 ); // neck
						parentsPose->m_outputPose[ headM.m_boneB ].SetMul( parentsPose->m_outputPose[ headM.m_boneB ], t1 ); // head

						pose->m_outputPose[ neckM.m_boneA ] = parentsPose->m_outputPose[ neckM.m_boneB ];
						pose->m_outputPose[ headM.m_boneA ] = parentsPose->m_outputPose[ headM.m_boneB ];

						//ASSERT( pose->m_outputPose[ neckM.m_boneA ].IsAlmostEqual( parentsPose->m_outputPose[ neckM.m_boneB ] ) );
						//ASSERT( pose->m_outputPose[ headM.m_boneA ].IsAlmostEqual( parentsPose->m_outputPose[ headM.m_boneB ] ) );
						*/

						parentsPose->m_outputPose[ neckM.m_boneB ] = pose->m_outputPose[ neckM.m_boneA ];
						parentsPose->m_outputPose[ headM.m_boneB ] = pose->m_outputPose[ headM.m_boneA ];
					}
				}
			}
		}

		// 4. Normal blend areas
		COMPILE_ASSERT( MIMIC_POSE_AREAS_NUM == NUM_NORMALBLEND_AREAS );
		const Uint32 size  = m_cachedNBComponents.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			m_cachedNBComponents[ i ]->UpdateDataManually( mimicData->m_mimicAreas );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CMimicComponent::PlayMimicAnimation( const CName& animation, const CName& slotName, Float blendTime, Float offset )
{
	SBehaviorSlotSetup slotSetup;
	slotSetup.m_blendIn = blendTime;
	slotSetup.m_blendOut = blendTime;
	slotSetup.m_offset = offset;

	return m_behaviorGraphStack ? m_behaviorGraphStack->PlaySlotAnimation( slotName, animation, &slotSetup ) : false;
}

Bool CMimicComponent::StopLipsyncAnimation()
{
	Bool ret( false );

	if ( m_behaviorGraphStack )
	{
		ret |= m_behaviorGraphStack->StopAllSlotAnimation( CNAME( MIMIC_SLOT ), 0.f, false );
		ret |= m_behaviorGraphStack->StopAllSlotAnimation( CNAME( MIMIC_SLOT_GMPL ), 0.f, false );
	}
	else
	{
		ret = true;
	}

	return ret;
}

Bool CMimicComponent::PlayLipsyncAnimation( CSkeletalAnimationSetEntry* skeletalAnimation, Float offset )
{
	SBehaviorSlotSetup slotSetup;
	slotSetup.m_blendIn = 0.f;
	slotSetup.m_blendOut = 0.1f;
	slotSetup.m_offset = offset;

	Bool ret( false );

	if ( m_mimicLod == EML_Lod0_MimicHigh || m_requestedMimicLod == EML_Lod0_MimicHigh )
	{
		ret = m_behaviorGraphStack ? m_behaviorGraphStack->PlaySlotAnimation( CNAME( MIMIC_SLOT ), skeletalAnimation, &slotSetup, false ) : false;
	}
	else
	{
		ret = m_behaviorGraphStack ? m_behaviorGraphStack->PlaySlotAnimation( CNAME( MIMIC_SLOT_GMPL ), skeletalAnimation, &slotSetup, false ) : false;
	}

	return ret;
}

void CMimicComponent::StopMimicAnimation( const CName& slotName )
{
	if ( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->StopAllSlotAnimation( slotName );
	}
}

Bool CMimicComponent::HasMimicAnimation( const CName& slotName ) const
{
	return m_behaviorGraphStack ? m_behaviorGraphStack->HasSlotAnimation( slotName ) : false;
}

Bool CMimicComponent::SetMimicVariable( const CName varName, Float value )
{
	return m_behaviorGraphStack ? m_behaviorGraphStack->SetBehaviorVariable( varName, value ) : false;
}

Bool CMimicComponent::RaiseMimicEvent( const CName& eventName, Bool force )
{
	if ( !force )
	{
		return m_behaviorGraphStack ? m_behaviorGraphStack->GenerateBehaviorEvent( eventName ) : false;
	}
	else
	{
		return m_behaviorGraphStack ? m_behaviorGraphStack->GenerateBehaviorForceEvent( eventName ) : false;
	}
}

Float CMimicComponent::GetMimicInternalVariable( const CName varName ) const
{
	return m_behaviorGraphStack ? m_behaviorGraphStack->GetBehaviorInternalFloatVariable( varName ) : 0.f;
}

//////////////////////////////////////////////////////////////////////////

void CMimicComponent::OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{

}

void CMimicComponent::OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{

}

void CMimicComponent::OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{

}

void CMimicComponent::OnParentUpdatedAttachedAnimatedObjectsLSConstrainted( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	PC_SCOPE_PIX( MimicComponent_Update );

	if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() && poseLS )
	{
		m_behaviorGraphSampleContext->SetupBoneCorrection( *poseLS, bones );
	}

#ifdef USE_GMPL_MIMICS
	if ( m_tempCanUseLod )
	{
		UpdateMimicLodState();
	}
#endif

	if ( m_mimicLod == EML_Lod0_MimicHigh || m_mimicLod == EML_Lod1_MimicLow || m_requestedMimicLod == EML_Lod0_MimicHigh || m_requestedMimicLod == EML_Lod1_MimicLow )
	{
		InternalUpdateAndSample( dt );

		if ( CanUseBehavior() )
		{
			SBehaviorGraphOutput& output = m_behaviorGraphSampleContext->GetMainPose();
			ProcessMimicData( m_behaviorGraphSampleContext->GetMimicPostProcessData(), &output, poseLS, bones );
		}
		else if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
		{
			SBehaviorGraphOutput& output = m_behaviorGraphSampleContext->GetMainPose();
			m_behaviorGraphSampleContext->SetPoseCorrection( output );
		}
	}
	// EML_Lod2_NeckHead
	else if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() && m_behaviorGraphSampleContext->ShouldCorrectPose() )
	{
		SBehaviorGraphOutput& output = m_behaviorGraphSampleContext->GetMainPose();
		m_behaviorGraphSampleContext->SetPoseCorrection( output );
	}

	m_firstUpdate = true;
}

void CMimicComponent::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* parentsPoseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	
}

Int32 CMimicComponent::GetLodBoneNum() const
{
	const CSkeleton* s = GetSkeleton();
	if ( s )
	{
		if ( m_requestRefreshAllMeshedWithLod0 )
		{
			return s->GetLodBoneNum_0();
		}

		//-- HACK!!---
		// This hack here exist only because there is the same hack in CMesh_RenderProxy, in calculating lod section, that forces highest
		// lod if material replacement exist.
		// Remember that condition for this hack should be exactly the same as in CMesh_RenderProxy.
		if ( GetEntity()->HasMaterialReplacement() )
		{
			return s->GetLodBoneNum_0();
		}
		//---------------

		const EBehaviorLod lod = GetLod();

		// We support lod 1 and lod 2
		if ( lod >= BL_Lod2 )
		{
			return LOD_2_NUM_BONES;
		}
		else if ( lod == BL_Lod1 )
		{
			return LOD_1_NUM_BONES;
		}
		else
		{
			return s->GetLodBoneNum_0();
		}
	}

	return 0;
}

CExtendedMimics CMimicComponent::GetExtendedMimics() const
{
	return CExtendedMimics( m_mimicFace.Get(), m_categoryMimics.Get() );
}

void CMimicComponent::OnParentAttachmentAdded( IAttachment* attachment )
{
	// Pass to the base class
	TBaseClass::OnParentAttachmentAdded( attachment );

	// GetTemplateMimicParams() returns false because 'templ' is null ( CEntityTemplate* templ = entity->GetEntityTemplate() ).
	// Entity template should be null here but it is and we can not fix this because entity system is broken (hacked).
	// We need to have below functionality only for item heads so we will check it. This is also a hack.
	const Bool isItemHead = GetEntity()->GetClass()->GetName() == TXT("CItemEntity");
	if ( !GetTemplateMimicParams() && isItemHead )
	{
		const CEntity* parent = attachment->FindParent< CEntity >();
		// if it's ok for player or player preview
		ASSERT( parent->IsPlayer() || parent->GetTemplate() && parent->GetTemplate()->GetTemplateInstance() && 
			parent->GetTemplate()->GetTemplateInstance()->IsA( CEntity::GetStaticClass() ) && 
			(Cast< CEntity >( parent->GetTemplate()->GetTemplateInstance() ) )->IsPlayer() );

		// Don't reset all stuff when we are playing animations
		StopAllAnimationsOnSkeleton();

		ClearAnimContainer();
		ClearBehaviorGraphs();
		DestroyBehaviorContext();

		ApplyMimicParams();// if our entity template doesn't have mimic params try to get it from entity template we are attached to (if any).
		
		CreateBehaviorContext();
		BuildAnimContainer();
		LoadBehaviorGraphs();

		CacheNBComponentsAndSetupAreas();
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif

#ifdef DEBUG_MIMICS
#pragma optimize("",on)
#endif
