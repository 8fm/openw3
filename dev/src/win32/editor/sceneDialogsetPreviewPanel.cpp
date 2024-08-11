/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sceneDialogsetPreviewPanel.h"

#include "../../common/game/storySceneDialogset.h"
#include "../../common/core/depot.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/bitmapTexture.h"


CGatheredResource resDialogsetCharacterEdIcon( TXT("engine\\textures\\icons\\waypointicon.xbm"), RGF_NotCooked );
CGatheredResource resDialogsetCameraEdIcon( TXT("engine\\textures\\icons\\cameraicon.xbm"), RGF_NotCooked );

CEdSceneDialogsetPreviewPanel::CEdSceneDialogsetPreviewPanel( wxWindow* parent, CStorySceneDialogset* dialogset )
	: CEdPreviewPanel( parent, true )
	, m_dialogset( dialogset )
	, m_selectedCameraNumber( 0 )
	, m_placeableEntity( NULL )
{
	SetCameraPosition( Vector( -2.5f, 1.0f, 2.0f ) );
	SetCameraRotation( EulerAngles( 0.0f, -30.0f, -120.f ) );
}

CEdSceneDialogsetPreviewPanel::~CEdSceneDialogsetPreviewPanel()
{

}

void CEdSceneDialogsetPreviewPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// Base fragments
	CEdPreviewPanel::OnViewportGenerateFragments( view, frame );

	TDynArray< EngineTransform > characterTransforms;
	m_dialogset->GetSlotPlacements( characterTransforms );
	TDynArray< EngineTransform >& cameraTransforms = m_dialogset->GetCameraTrajectories();
	TDynArray< Vector >& cameraEyePositions = m_dialogset->GetCameraEyePositions();
	const TDynArray< StorySceneCameraDefinition >& cameraDefinitions = m_dialogset->GetCameraDefinitions();

	CHitProxyID hitProxyId;

	// Sprite icon
	CBitmapTexture* characterIcon = resDialogsetCharacterEdIcon.LoadAndGet< CBitmapTexture >();
	CBitmapTexture* cameraIcon = resDialogsetCameraEdIcon.LoadAndGet< CBitmapTexture >();
	if ( characterIcon != NULL )
	{
		// Draw slot icons
		Float iconSize = 0.5f;
		Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( Vector::ZERO_3D_POINT );
		const Float size = Max( iconSize, iconSize * screenScale * 0.33f ); 

		for ( Uint32 i = 0; i < characterTransforms.Size(); ++i )
		{
			Vector slotPosition = characterTransforms[ i ].GetPosition();

			if ( i >= 0 && i < m_characterGhosts.Size() )
			{
				m_characterGhosts[ i ]->SetPosition( slotPosition );
				m_characterGhosts[ i ]->SetRotation( characterTransforms[ i ].GetRotation() );
			}

			Matrix slotMatrix;
			characterTransforms[ i ].CalcLocalToWorld( slotMatrix );

			frame->AddDebugArrow( slotMatrix, Vector::EY, 1.0f, Color::GREEN );

			frame->AddSprite( slotPosition, size, Color::BLUE, hitProxyId, characterIcon );
			frame->AddDebugText( slotPosition, String::Printf( TXT( "Slot %d" ), i + 1 ), 0, 0, true, Color::LIGHT_BLUE );
		}
	}

	if ( cameraIcon != NULL )
	{
		// Draw camera icons
		Float iconSize = 0.5f;
		Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( Vector::ZERO_3D_POINT );
		const Float size = Max( iconSize, iconSize * screenScale * 0.33f ); 


		for ( Uint32 i = 0; i < cameraDefinitions.Size(); ++i )
		{

			Bool isCameraSelected = ( i + 1 == m_selectedCameraNumber );

			Color color = ( isCameraSelected == true ) ? Color::YELLOW : Color( 128, 128, 128, 128 );
			
			frame->AddDebugText( cameraDefinitions[ i ].m_cameraTransform.GetPosition(), cameraDefinitions[ i ].m_cameraName.AsString(), 0, 0, true, Color::LIGHT_BLUE );


			CRenderCamera camera;
			camera.Set( cameraDefinitions[ i ].m_cameraTransform.GetPosition(), 
				cameraDefinitions[ i ].m_cameraTransform.GetRotation(), cameraDefinitions[ i ].m_cameraFov, 
				1.77, 0.05 * 0.99f, 0.2f, 0.99f );
			frame->AddDebugFrustum( camera.GetScreenToWorld(), color, true, false, 1.0f );
			
		}
	}

}

void CEdSceneDialogsetPreviewPanel::TogglePlaceables( Bool enable )
{
	if ( enable == true )
	{
		String selectedResource;
		if ( GetActiveResource( selectedResource ) )
		{
			EntitySpawnInfo entitySpawnInfo;
			entitySpawnInfo.m_template = LoadResource< CEntityTemplate >( selectedResource );
			m_placeableEntity = GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( entitySpawnInfo );
			
		}
	}
	else
	{
		if ( m_placeableEntity != NULL )
		{
			m_placeableEntity->Destroy();
			m_placeableEntity = NULL;
		}
	}
}

void CEdSceneDialogsetPreviewPanel::ToggleCharacters( Bool enable )
{
	if ( enable == true )
	{
		TDynArray< EngineTransform > characterTransforms;
		m_dialogset->GetSlotPlacements( characterTransforms );
		for ( Uint32 i = 0; i < characterTransforms.Size(); ++i )
		{
			EntitySpawnInfo entitySpawnInfo;
			entitySpawnInfo.m_template = LoadResource< CEntityTemplate >( TXT( "characters\\templates\\man\\include_test.w2ent" ) );
			entitySpawnInfo.m_spawnPosition = characterTransforms[ i ].GetPosition();
			entitySpawnInfo.m_spawnRotation = characterTransforms[ i ].GetRotation();

			CEntity* characterGhost = GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( entitySpawnInfo );
			if ( characterGhost != NULL )
			{
				m_characterGhosts.PushBack( characterGhost );
			}
		}

	}
	else
	{
		for ( Uint32 j = 0; j < m_characterGhosts.Size(); ++j )
		{
			m_characterGhosts[ j ]->Destroy();
		}
		m_characterGhosts.Clear();
	}
}