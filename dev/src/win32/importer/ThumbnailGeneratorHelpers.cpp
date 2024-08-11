
#include "Build.h"
#include "../../common/renderer/build.h"
#include "../../common/renderer/renderMaterial.h"

#include "../../common/gpuApiUtils/gpuApiMemory.h"
#include "../../common/gpuApiUtils/gpuApiTypes.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/depot.h"
#include "../../common/core/thumbnail.h"

#include "../editor/editorExternalResources.h" // HACK! dependency on Editor
#include "ThumbnailGeneratorHelpers.h"
#include "../../common/engine/environmentManager.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/environmentDefinition.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/entity.h"
#include "../../common/engine/worldTick.h"
#include "../../common/engine/meshComponent.h"

static CWorld* ThumbnailGeneratorWorld;
extern EShowFlags GShowGameMask[];

CWorld* ThumbnailGeneratorHelpers::PrepareFakeWorld()
{
	if ( ThumbnailGeneratorWorld == nullptr )
	{
		ThumbnailGeneratorWorld = CreateObject< CWorld >();

		WorldInitInfo initInfo;
		initInfo.m_previewWorld = true;
		ThumbnailGeneratorWorld->Init( initInfo );

		ThumbnailGeneratorWorld->AddToRootSet();
	}

	CLayer* layer = ThumbnailGeneratorWorld->GetDynamicLayer();
	TDynArray< CEntity* > entities;
	layer->GetEntities( entities );
	for ( auto it=entities.Begin(); it != entities.End(); ++it )
	{
		(*it)->Destroy();
	}
	ThumbnailGeneratorWorld->DelayedActions();

	if ( CDiskFile* thumbnailEnvFile = GDepot->FindFile( THUMBNAIL_GEN_ENVIRONMENT ) )
	{
		thumbnailEnvFile->Load();

		if ( CResource* res = thumbnailEnvFile->GetResource() )
		{
			if ( CEnvironmentDefinition* envDef = Cast< CEnvironmentDefinition >( res ) )
			{
  				CAreaEnvironmentParams aep = envDef->GetAreaEnvironmentParams();
				if ( aep.m_finalColorBalance.m_activatedBalanceMap )
				{
					if ( aep.m_finalColorBalance.m_balanceMap0 != nullptr ) aep.m_finalColorBalance.m_balanceMap0.Get();
					if ( aep.m_finalColorBalance.m_balanceMap1 != nullptr ) aep.m_finalColorBalance.m_balanceMap1.Get();
				}

				SWorldEnvironmentParameters wep = ThumbnailGeneratorWorld->GetEnvironmentParameters();
				wep.m_environmentDefinition = envDef;
				wep.m_environmentDefinition->SetAreaEnvironmentParams( aep );
				ThumbnailGeneratorWorld->SetEnvironmentParameters( wep );
			}
		}
	}

	return ThumbnailGeneratorWorld;
}

void ThumbnailGeneratorHelpers::SetUpGroundAndFoliage( CWorld* fakeWorld, const Box& box, const SThumbnailSettings& settings )
{
	if ( !(settings.m_flags & TF_UseGround) )
	{
		return;
	}

	Vector center = box.CalcCenter();
	Vector extents = box.CalcExtents();
	//Float min = Min( 0.f, box.Min.Z );
	Float min = box.Min.Z;

	EntitySpawnInfo einfo;
	if ( CEntity* groundEntity = fakeWorld->GetDynamicLayer()->CreateEntitySync( einfo ) )
	{
		if ( CMeshComponent* groundPlane = SafeCast< CMeshComponent >( groundEntity->CreateComponent( ClassID< CMeshComponent >(), SComponentSpawnInfo() ) ) )
		{
			groundPlane->SetResource( GDepot->LoadResource( THUMBNAIL_GEN_PLANE ) );
			groundPlane->SetNoDisolves( true );

			groundPlane->SetPosition( Vector( center.X, center.Y, min ) );

			Vector planeExtents = groundPlane->GetBoundingBox().CalcExtents();
			Float groundSize = 1.3f * Max( extents.X, extents.Y );
			groundPlane->SetScale( Vector( groundSize/planeExtents.X, groundSize/planeExtents.Y, 1.f ) );

			// plant the grass
#if 0
			const Uint32 foliageDensity = 80; // instances / m^2

			if ( CFoliageManager* foliageManager = fakeWorld->GetFoliageManager() )
			{
				CSRTBaseTree* grass = SafeCast< CSRTBaseTree >( GDepot->LoadResource( THUMBNAIL_GEN_GRASS ) );

				GEngine->GetRandomNumberGenerator().Seed( 8000 );
				
				Uint32 numOfInstances = static_cast< Uint32 >( foliageDensity * groundSize * groundSize );
				
				TDynArray< CFoliageContainer::SInstanceInfo > instances( numOfInstances );

				for ( Uint32 i = 0; i < numOfInstances; ++i )
				{
					CFoliageContainer::SInstanceInfo& instance = instances[i];

					Float posX = center.X + GEngine->GetRandomNumberGenerator().Get< Float >( -groundSize, groundSize );
					Float posY = center.Y + GEngine->GetRandomNumberGenerator().Get< Float >( -groundSize, groundSize );
					Float rot  = GEngine->GetRandomNumberGenerator().Get< Float >( 90.f );

					instance.m_position = Vector( posX, posY, min );
					instance.m_scale = 1.f;
					instance.m_rotation = EulerAngles( 0.f, 0.f, rot );
				}

				foliageManager->AddPreviewInstances( grass, instances );
			}
#endif
		}

	}

}

// HACK
extern Uint8* GrabRenderSurfacesThumbnail( const GpuApi::Rect* srcRect, Uint32& size );

void ThumbnailGeneratorHelpers::UpdateWorld( CWorld* fakeWorld )
{
	CWorldTickInfo tickInfo( fakeWorld, 0.1f );
	tickInfo.m_updatePhysics = true;
	fakeWorld->Tick( tickInfo );
	fakeWorld->Tick( tickInfo );
}

CThumbnail* ThumbnailGeneratorHelpers::CreateThumbnail( CWorld* fakeWorld, const Box& box, Float distance, const SThumbnailSettings& settings )
{
	Float bsize = box.CalcExtents().Mag3();

	EulerAngles cameraRotation;
	Vector cameraPosition;
	EulerAngles sunRotation;
	Float cameraFov;

	cameraRotation = ( settings.m_customSettings & TCS_CameraRotation ) ? settings.m_cameraRotation : EulerAngles( 0.f, -30.f, 210.f );
	cameraFov = ( settings.m_customSettings & TCS_CameraFov ) ? settings.m_cameraFov : 70.f;
	sunRotation = ( settings.m_customSettings & TCS_LightPosition ) ? EulerAngles( 0.f,  30.f, settings.m_lightPosition ) : EulerAngles( 0.f,  30.f, 20.f );

	if ( settings.m_customSettings & TCS_CameraPosition )
	{
		cameraPosition = settings.m_cameraPosition;
	}
	else
	{
		Vector forward, right, up;
		cameraRotation.ToAngleVectors( &forward, &right, &up );
		cameraPosition = box.CalcCenter() - forward * bsize * distance;
	}

	// Set environment
	if ( CEnvironmentManager *envManager = fakeWorld->GetEnvironmentManager() )
	{		
		CGameEnvironmentParams params = envManager->GetGameEnvironmentParams();
 		params.m_dayCycleOverride.m_enableCustomSunRotation = true;
 		params.m_dayCycleOverride.m_customSunRotation = sunRotation;

		params.m_displaySettings.m_allowGlobalFog = true;
		params.m_displaySettings.m_allowBloom = true;
		params.m_displaySettings.m_disableTonemapping = false;

		params.m_displaySettings.m_gamma = 1.8;
		params.m_displaySettings.m_enableInstantAdaptation = true;
		params.m_displaySettings.m_allowDOF = false;
		params.m_displaySettings.m_allowCloudsShadow = false;
		params.m_displaySettings.m_allowVignette = false;
		params.m_displaySettings.m_forceCutsceneDofMode = false;
		params.m_displaySettings.m_displayMode = EMM_None;

		envManager->SetGameEnvironmentParams( params );

		if ( !settings.m_environmentPath.Empty() )
		{
			if ( CDiskFile* envFile = GDepot->FindFile( settings.m_environmentPath ) )
			{
				auto envDef = Cast< CEnvironmentDefinition >( envFile->Load() );
				envFile->GetResource();
				SWorldEnvironmentParameters wep = fakeWorld->GetEnvironmentParameters();
				CAreaEnvironmentParams aep = envDef->GetAreaEnvironmentParams();
				wep.m_environmentDefinition = envDef;
				if ( aep.m_finalColorBalance.m_activatedBalanceMap )
				{
					if ( aep.m_finalColorBalance.m_balanceMap0 != NULL ) aep.m_finalColorBalance.m_balanceMap0.Get();
					if ( aep.m_finalColorBalance.m_balanceMap1 != NULL ) aep.m_finalColorBalance.m_balanceMap1.Get();
				}
				wep.m_environmentDefinition->SetAreaEnvironmentParams( aep );
				fakeWorld->SetEnvironmentParameters( wep );
			}
		}
	}

	UpdateWorld( fakeWorld );

	const Uint32 SIZE = 1024;

	// Setup render frame
	CRenderFrameInfo info( SIZE, SIZE, RM_Shaded, GShowGameMask, CRenderCamera( cameraPosition, cameraRotation, cameraFov, 1.0f, 0.1f, 1000.0f ) );
	info.m_camera.SetNonDefaultNearRenderingPlane();
	info.m_camera.SetNonDefaultFarRenderingPlane();
	if ( settings.m_item )
	{
		info.m_clearColor = Color::CLEAR;
		info.m_customRenderResolution = true;
		info.m_tonemapFixedLumiance	= Config::cvInventoryFixedLuminance.Get();
		info.m_backgroundTextureColorScale = Vector( Config::cvInventoryBgColorScaleR.Get(), Config::cvInventoryBgColorScaleG.Get(), Config::cvInventoryBgColorScaleB.Get(), 1.0f );
		RED_ASSERT( info.m_renderTarget == nullptr );
		info.m_renderTarget = GRender->CreateGameplayRenderTarget( "thumbnail" );
		info.m_renderTarget->RequestResizeRenderSurfaces( SIZE, SIZE );
	}
	if ( settings.m_flags & TF_SetBackgroundColor )
	{
		if ( !info.m_renderTarget )
		{
			info.m_renderTarget = GRender->CreateGameplayRenderTarget( "thumbnail" );
			info.m_renderTarget->RequestResizeRenderSurfaces( SIZE, SIZE );
		}
		info.m_renderTarget->SetBackgroundColor( settings.m_backgroundColor );
	}

	// Render
	if ( CRenderFrame* frame = fakeWorld->GenerateFrame( nullptr, info ) )
	{
		Bool matAsyncMode = GetRenderer()->GetAsyncCompilationMode();

		GetRenderer()->SetAsyncCompilationMode( false );
		{
			( new CRenderCommand_ToggleCinematicMode( true ) )->Commit();
			( new CRenderCommand_PrepareInitialTextureStreaming( fakeWorld->GetRenderSceneEx(), true ) )->Commit();
			( new CRenderCommand_TickTextureStreaming() )->Commit();
			GRender->Flush();

			IRenderFramePrefetch* prefetch = GetRenderer()->CreateRenderFramePrefetch( frame, fakeWorld->GetRenderSceneEx(), false );
			( new CRenderCommand_StartFramePrefetch( prefetch ) )->Commit();
			( new CRenderCommand_TickTextureStreaming() )->Commit();
			GetRenderer()->Flush();

			for ( Uint32 i = 0; i < 100; ++i ) // HACK - I've tested it on smaller values, this one gives good results while insignificantly affecting the performance
			//while( GetRenderer()->IsStreamingTextures() ) <- nope
			//while( !prefetch->IsFinished() ) <- unfortunately not
			{
				( new CRenderCommand_TickTextureStreaming( true ) )->Commit();
				fakeWorld->RenderWorld( frame, false );
				GetRenderer()->Flush();
			}

			( new CRenderCommand_FinishInitialTextureStreaming() )->Commit();
			( new CRenderCommand_TickTextureStreaming() )->Commit();
			GetRenderer()->Flush();

			if ( settings.m_item )
				fakeWorld->RenderWorld( frame );
			else
				fakeWorld->RenderWorld( frame, true );
			GetRenderer()->Flush();
	 		frame->Release();
			prefetch->Release();
		}
		GetRenderer()->SetAsyncCompilationMode( matAsyncMode );

		// Grab thumbnail
		GpuApi::Rect rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = SIZE;
		rect.bottom = SIZE;

		if ( settings.m_item )
		{
			Uint32 memsize = 0;
			Uint8* buffer = nullptr;
			if( GpuApi::SaveTextureToMemory( info.m_renderTarget->GetGpuTexture(), GpuApi::SAVE_FORMAT_PNG, &rect, (void**)(&(buffer)), memsize ) && buffer && memsize )
			{
				// Create thumbnail from surface data
				CThumbnail* thumb = new CThumbnail( buffer, memsize );
				thumb->SetCameraPosition( cameraPosition );
				thumb->SetCameraRotation( cameraRotation );
				thumb->SetCameraFov( cameraFov );
				thumb->SetSunRotation( sunRotation );
				thumb->SetFlags( settings.m_flags );
				GpuApi::FreeTextureData( buffer );

				info.m_renderTarget->Release();
				return thumb;
			}
		}
		else
		{
			Uint32 memsize = 0;
			Uint8* buffer = GrabRenderSurfacesThumbnail( &rect, memsize );
			if ( buffer && memsize )
			{
				// Create thumbnail from surface data
				CThumbnail* thumb = new CThumbnail( buffer, memsize );
				thumb->SetCameraPosition( cameraPosition );
				thumb->SetCameraRotation( cameraRotation );
				thumb->SetCameraFov( cameraFov );
				thumb->SetSunRotation( sunRotation );
				thumb->SetFlags( settings.m_flags );
				GpuApi::FreeTextureData( buffer );
				return thumb;
			}
		}
	}
	
	if ( settings.m_item )
	{
		info.m_renderTarget->Release();
		info.m_renderTarget = nullptr;
	}

	return nullptr;
}
