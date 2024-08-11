
#include "build.h"
#include "storySceneIncludes.h"
#include "../engine/mimicComponent.h"
#include "../engine/skeletalAnimationContainer.h"
#include "../engine/environmentManager.h"
#include "storyScenePlayer.h"
#include "movableRepresentationPathAgent.h"
#include "movingAgentComponent.h"
#include "storySceneInput.h"
#include "storySceneSection.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

namespace StorySceneUtils 
{
	void ConvertStateToAnimationFilters( const CName& state, CName& status, CName& emoState, CName& pose )
	{
		const String stateStr = state.AsString();
		TDynArray< String > tokens = stateStr.Split( TXT(" ") );
		if ( tokens.Size() == 3 )
		{
			status = CName( tokens[ 0 ] );
			emoState = CName( tokens[ 2 ] );
			pose = CName( tokens[ 1 ] );
		}
		else if ( tokens.Size() == 4 )
		{
			status = CName( tokens[ 0 ] );
			emoState = CName( tokens[ 3 ] );
			String str = tokens[ 1 ] + TXT(" ") + tokens[ 2 ];
			pose = CName( str );
		}
		else if ( tokens.Size() == 5 )
		{
			status = CName( tokens[ 0 ] );
			emoState = CName( tokens[ 4 ] );
			String str = tokens[ 1 ] + TXT(" ") + tokens[ 2 ] + TXT(" ") + tokens[ 3 ];
			pose = CName( str );
		}
	}



	Vector CalcEyesPosLS( CActor* actor, const CName& idleAnim )
	{
		if( !actor )
		{
			return Vector::ZERO_3D_POINT;
		}

		Vector result = actor->GetHeadPosition() - actor->GetWorldPosition();

		CSkeleton* mimicSkeleton = actor->GetMimicComponent() ? actor->GetMimicComponent()->GetSkeleton() : nullptr;
		if( !mimicSkeleton )
		{
			return result;
		}
		Int32 eyeL =	mimicSkeleton->FindBoneByName( CNAME( placer_left_eye ) );
		Int32 eyeR =	mimicSkeleton->FindBoneByName( CNAME( placer_right_eye ) );
		if( eyeR <= 0 || eyeL <= 0 || ! actor->GetMimicComponent()->GetTransformParent() )
		{
			return result;
		}
		RedQsTransform eyesPointLS = mimicSkeleton->GetBoneLS( eyeL );
		eyesPointLS.SetTranslation( Div( Add( eyesPointLS.GetTranslation(), mimicSkeleton->GetBoneLS( eyeR ).GetTranslation() ), 2.f ) );

		CAnimatedComponent* rootAc = actor->GetRootAnimatedComponent();
		//CName parentSlotName = actor->GetMimicComponent()->GetTransformParent()->GetParentSlotName();					
		//Int32 slotIndex = rootAc->GetSkeleton()->FindBoneByName( parentSlotName );	
		Int32 slotIndex = actor->GetHeadBone();

		SBehaviorGraphOutput pose;			
		CSkeletalAnimationSetEntry* animEntry =  rootAc->GetAnimationContainer()->FindAnimation( idleAnim );
		CSkeletalAnimation* bodyIdleAnim = animEntry ? animEntry->GetAnimation() : nullptr;
		if ( !bodyIdleAnim || slotIndex == -1 )
		{
			return result;
		}
		pose.Init( bodyIdleAnim->GetBonesNum(), 0 );
		bodyIdleAnim->Sample( 0.f, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks  );		
		if ( rootAc->UseExtractedTrajectory() && rootAc->HasTrajectoryBone() )
		{
			pose.ExtractTrajectory( rootAc );
		}

		Matrix slotBone = pose.GetBoneModelMatrix( rootAc, slotIndex );

		result = slotBone.TransformPoint( Vector( eyesPointLS.GetTranslation().AsFloat() ) );															
		return result;
	}

	Matrix CalcWSFromSS( const EngineTransform& transformSS, const EngineTransform& sceneLocalToWorld )
	{
		Matrix localOffset;
		Matrix l2w;

		const Vector& scale = sceneLocalToWorld.GetScale();
		if ( !Vector::Equal3( scale, Vector::ONES ) )
		{
			SCENE_ASSERT__FIXME_LATER( 0 );
		}
		if ( !Vector::Near3( scale, Vector::ONES ) )
		{
			SCENE_ASSERT( 0 );
		}

		transformSS.CalcLocalToWorld( localOffset );
		sceneLocalToWorld.CalcLocalToWorld( l2w );

		return Matrix::Mul( l2w, localOffset );
	}

	Vector CalcWSFromSS( const Vector& transformSS, const EngineTransform& sceneLocalToWorld )
	{
		Matrix localOffset;
		Matrix l2w;

		const Vector& scale = sceneLocalToWorld.GetScale();
		if ( !Vector::Equal3( scale, Vector::ONES ) )
		{
			SCENE_ASSERT( 0 );
		}
		if ( !Vector::Near3( scale, Vector::ONES ) )
		{
			SCENE_ASSERT( 0 );
		}

		sceneLocalToWorld.CalcLocalToWorld( l2w );

		return l2w.TransformPoint( transformSS );
	}

	EngineTransform CalcSSFromWS( const EngineTransform& transformWS, const EngineTransform& sceneLocalToWorld )
	{
		Matrix transWS;
		Matrix l2w;

		transformWS.CalcLocalToWorld( transWS );
		sceneLocalToWorld.CalcLocalToWorld( l2w );

		Matrix mat = Matrix::Mul( l2w.FullInverted(), transWS );

		return EngineTransform( mat );
	}

	Matrix CalcL2WForAttachedObject( const CAnimatedComponent* ac, CName boneName, Uint32 attachmentFlags )
	{
		Matrix toWorldSpace( Matrix::IDENTITY );
		Int32 bone = ac->FindBoneByName( boneName );
		if ( bone >= 0 )
		{
			toWorldSpace = ac->GetBoneMatrixWorldSpace( bone );
			if ( attachmentFlags & HAF_FreeRotation )
			{
				Vector translation = toWorldSpace.GetTranslation();
				toWorldSpace.SetIdentity();
				toWorldSpace.SetTranslation( translation );
			}
			if ( attachmentFlags & ~HAF_FreeRotation )
			{
				Vector translation = toWorldSpace.GetTranslation();
				if ( attachmentFlags & HAF_FreePositionAxisX ) translation.X = 0.0f;
				if ( attachmentFlags & HAF_FreePositionAxisY ) translation.Y = 0.0f;
				if ( attachmentFlags & HAF_FreePositionAxisZ ) translation.Z = 0.0f;
				toWorldSpace.SetTranslation( translation );
			}
		}
		return toWorldSpace;
	}

	Color GetEnvLightVal( CLayer* layer, Int32 index )
	{
		if( CWorld* world = layer->GetWorld() )
		{
			if ( CEnvironmentManager* mgr = world->GetEnvironmentManager() )
			{
				GameTime time = mgr->GetCurrentGameTime();
				const CAreaEnvironmentParams& params = mgr->GetCurrentAreaEnvironmentParams();		
				Float floatTime = time.ToFloat() / (24.f * 60.f * 60.f);
				Vector value;
				switch ( index )
				{
				case 0:
					value = params.m_dialogLightParams.m_lightColor.GetValue( floatTime );				
					break;
				case 1:
					value = params.m_dialogLightParams.m_lightColor2.GetValue( floatTime );				
					break;
				case 2:
					value = params.m_dialogLightParams.m_lightColor3.GetValue( floatTime );				
					break;
				default:
					value = Vector::ZEROS;
				}				 
				return Color ( (Uint8) Clamp( value.X, 0.f, 255.f ), (Uint8) Clamp( value.Y, 0.f, 255.f ), (Uint8) Clamp( value.Z, 0.f, 255.f ) );
			}
		}

		return Color::WHITE;
	}

	Bool CalcTrackedPosition( const SStorySceneLightTrackingInfo& info, const CStoryScenePlayer* player, Vector& outPos, EulerAngles& outRot )
	{
		const CWorld* world = player->GetLayer()->GetWorld();
		const CEnvironmentManager *envManager = world->GetEnvironmentManager();
		if ( envManager )
		{
			EulerAngles sunRotation;
			Vector lightDir;
			GameTime time = envManager->GetCurrentGameTime();
			if ( envManager->GetGameEnvironmentParams().m_dayCycleOverride.m_enableCustomSunRotation )
			{
				sunRotation = envManager->GetGameEnvironmentParams().m_dayCycleOverride.m_customSunRotation;			
				lightDir = sunRotation.TransformVector( Vector::EY );
			}
			else
			{
				lightDir = world->GetEnvironmentParameters().m_globalLightingTrajectory.GetLightDirection( time );
				sunRotation = lightDir.ToEulerAngles();
			}									

			if ( info.m_trackingType == LTT_Reverse )
			{
				lightDir = -lightDir;
			}

			const Float importTime = time.ToFloat() / (24.f * 60.f * 60.f);
			Float distance = info.m_radius.GetFloatValue( importTime );
			Float angleOffset = Clamp( info.m_angleOffset.GetFloatValue( importTime ), -90.f, 90.f );
			outRot = -sunRotation;
			Vector posBeforeRot = -( lightDir.Normalized3() * distance );						
			Vector axisVec = Vector::Cross( Vector::EZ, posBeforeRot );
			Float qSin = MSin( DEG2RAD( angleOffset ) );
			Float qCos = MCos( DEG2RAD( angleOffset ) );
			Vector quat( axisVec.X * qSin, axisVec.Y * qSin, axisVec.Z * qSin, qCos );
			Matrix m;
			m.BuildFromQuaternion( quat );
			outPos = m.TransformPoint( posBeforeRot );

			return true;
		}
		return false;
	}

	Bool DoTraceZTest( const CActor* actor, const Vector& inPos, Vector& outPos )
	{
		// Old version
		//if ( CMovingPhysicalAgentComponent* pmac = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() ) )
		//{
		//	return pmac->DoTraceZTest( currPosition, newPosition );
		//}

		// New version
		if ( CMovingAgentComponent* actorMovingAgent = actor->GetMovingAgentComponent() )
		{
			if ( CPathAgent* pathAgent = actorMovingAgent->GetPathAgent() )
			{
				SCENE_ASSERT( actor->GetRootAnimatedComponent() );

				CPathLibWorld* pathlib = pathAgent->GetPathLib();

				const PathLib::AreaId areaId = pathAgent->GetCachedAreaId();
				const Float searchRadius = 6.0f;
				Float minZ = inPos.Z - 2.0f;
				Float maxZ = inPos.Z + 1.f;

				Vector3 safePosition3( 0.f, 0.f, 0.f );
				if ( !pathlib->FindSafeSpot( areaId, inPos.AsVector3(), searchRadius, pathAgent->GetPersonalSpace(), safePosition3, &minZ, &maxZ, PathLib::CT_IGNORE_METAOBSTACLE | pathAgent->GetCollisionFlags() ) )
				{
					ERR_GAME( TXT("Actor '%ls' cannot be teleported to a safe position after scene. Please check navigation data."), actor->GetFriendlyName().AsChar() );
					SCENE_ERROR( TXT("Actor '%ls' cannot be teleported to a safe position after scene. Please check navigation data."), actor->GetFriendlyName().AsChar() );
					return false;
				}

				pathAgent->DoZCorrection( safePosition3 );

				outPos = safePosition3;
				outPos.W = 1.f;
				return true;
			}
		}

		return false;
	}

#ifndef NO_EDITOR
	void ShouldFadeOnLoading( const CStoryScene* scene, TDynArray< CName >& names, TDynArray< Bool >& values )
	{
		for ( CStoryScene::TConstControlPartIterator< CStorySceneInput > it( scene ); it; ++it )
		{
			const CStorySceneInput* input = *it;
			
			const CName name( input->GetName() );
			Bool isGameplay = input->IsGameplay();

			if ( const CStorySceneSection* section = Cast< const CStorySceneSection >( input->GetNextElement() ) )
			{
				isGameplay |= section->IsGameplay();
			}

			names.PushBack( name );
			values.PushBack( !isGameplay );
		}
	}
#endif
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
