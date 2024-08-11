/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderInterface.h"
#include "renderTexture.h"
#include "renderTextureStreaming.h"
#include "renderMaterial.h"
#include "renderProxyTerrain.h"
#include "renderProxyWater.h"
#include "renderTextureArray.h"
#include "renderScene.h"


namespace Config
{
	TConfigVar< Bool >		cvInitialFrameUseMultipleRotations( "Streaming/Textures", "InitialFrameUseMultipleRotations", true );
	TConfigVar< Bool >		cvInitialFrameUseOcclusionForMultipleRotations( "Streaming/Textures", "InitialFrameUseOcclusionForMultipleRotations", false );
}


void CRenderInterface::ResetTextureStreamingDistances( CRenderSceneEx* scene )
{
	// Wait through the "unsafe" part of the update task, because we're about to update texture distances.
	m_textureStreamingManager->EnsureUpdateTaskIsSafe();

	// Reset all texture distances. This will let everything be properly prioritized, and allow us to unload unused textures if needed.
	{
		struct Visitor
		{
			void Visit( CRenderTextureBase *tex )
			{
				tex->ResetLastBindDistance();
			}
		};

		Visitor visitor;
		CRenderTextureBase::VisitResourcesAll( visitor );
	}


	// A couple things are always going to set the texture distances to 0, so set those here, to make sure they don't
	// get unstreamed or anything.
	if ( scene != nullptr )
	{
		if ( CRenderProxy_Terrain* terrain = scene->GetTerrain() )
		{
			if ( CRenderMaterial* mtl = terrain->GetNewestMaterial() )
			{
				for ( IRenderResource* resource : mtl->m_textures )
				{
					if ( CRenderTextureBase* texture = static_cast< CRenderTextureBase* >( resource ) )
					{
						texture->UpdateLastBindDistance( 0.0f );
					}
				}
			}
		}

		if ( scene->GetWaterProxy() != nullptr && scene->GetWaterProxy()->GetControlTexture() != nullptr )
		{
			scene->GetWaterProxy()->GetControlTexture()->UpdateLastBindDistance( 0.0f );
		}
	}
}


void CRenderInterface::SetupInitialFrame( CRenderFrame* frame, CRenderSceneEx* scene, Bool multipleAngles, Bool unloadExisting )
{
	ResetTextureStreamingDistances( scene );

	// HACK : Apply multiple prefetches, at various camera directions.
	const Bool doMultipleAngles = multipleAngles && Config::cvInitialFrameUseMultipleRotations.Get();
	{
		if ( doMultipleAngles )
		{
			// When doing multiple angles, especially with occlusion off, this may take a substantial amount of time. So just queue up
			// the prefetches and let them be done one per frame.

			// Don't use occlusion culling. We want EVERYTHING around us...
			const Bool useOcclusion = Config::cvInitialFrameUseOcclusionForMultipleRotations.Get();

			const CRenderCamera& origCamera = frame->GetFrameInfo().m_camera;

			// Limit far plane to max streaming distance. No point collected stuff farther than that.
			const Float farPlane	= Min( origCamera.GetFarPlane(), MSqrt( Config::cvTextureStreamingDistanceLimitSq.Get() ) );


			EulerAngles rotation	= origCamera.GetRotation();
			const Float vFov		= origCamera.GetFOV();

			const Float vSize = MTan( DEG2RAD( vFov * 0.5f ) );
			const Float hSize = vSize * origCamera.GetAspect();
			const Float hFov = RAD2DEG( MATan2( hSize, 1.0f ) * 2.0f );

			const Uint32 NUM_TIMES = (Uint32)( 360.0f / hFov );

			for ( Uint32 i = 0; i < NUM_TIMES; ++i )
			{
				CRenderFrameInfo frameInfo = frame->GetFrameInfo();
				frameInfo.m_camera.SetRotation( rotation );
				frameInfo.m_camera.SetFarPlane( farPlane );
				frameInfo.m_occlusionCamera = frameInfo.m_camera;
				CRenderFrame* newFrame = GRender->CreateFrame( nullptr, frameInfo );

				CRenderFramePrefetch* prefetch = new CRenderFramePrefetch( newFrame, scene, useOcclusion );
				GetRenderer()->EnqueueFramePrefetch( prefetch );

				prefetch->Release();
				newFrame->Release();

				rotation.Yaw += 360.0f / NUM_TIMES;
			}
		}
		else
		{
			// If there's just one camera angle to do, we can do it synchronously. It'll be fast, and there's probably a loading
			// screen or something.

			CRenderFramePrefetch* prefetch = new CRenderFramePrefetch( frame, scene );
			prefetch->DoSceneCollectSync();
			prefetch->ApplyTextureResults();
			prefetch->Release();
		}
	}


	// Unload anything that is not visible with this frame. Can only do this if we did the prefetch locally.
	if ( unloadExisting && !doMultipleAngles )
	{
		m_textureStreamingManager->ForceUnloadUnused();
	}
}
