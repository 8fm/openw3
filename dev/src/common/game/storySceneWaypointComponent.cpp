/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "storySceneWaypointComponent.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/depot.h"
#include "../engine/renderFrame.h"
#include "../engine/bitmapTexture.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneWaypointComponent )

CGatheredResource resDialogsetAnimationsSet( TXT( "characters\\templates\\interaction\\dialogsets\\charactertrajectories.w2anims" ), 0 );
CGatheredResource resDialogsetList( TXT("gameplay\\globals\\scenes\\scene_dialogsets.csv"), 0 );

CGatheredResource resDialogsetCharacterIcon( TXT("engine\\textures\\icons\\waypointicon.xbm"), RGF_NotCooked );
CGatheredResource resDialogsetCameraIcon( TXT("engine\\textures\\icons\\cameraicon.xbm"), RGF_NotCooked );

CStorySceneWaypointComponent::CStorySceneWaypointComponent()
	: m_showCameras( false )
	, m_useDefaultDialogsetPositions( false )
{
	
}

void CStorySceneWaypointComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	// Base fragments
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	CStorySceneDialogset* dialogset = m_dialogset.Get();

	// Draw meshes if allowed
	if ( flags == SHOW_Sprites && dialogset != NULL )
	{
		TDynArray< EngineTransform > characterTransforms;
		dialogset->GetSlotPlacements( characterTransforms );
		const TDynArray< StorySceneCameraDefinition >& cameraDefinitions = dialogset->GetCameraDefinitions();
		
		//TDynArray< Vector >& cameraTransforms = dialogset->GetCameraEyePositions();

		// Sprite icon
		CBitmapTexture* characterIcon = resDialogsetCharacterIcon.LoadAndGet< CBitmapTexture >();
		CBitmapTexture* cameraIcon = resDialogsetCameraIcon.LoadAndGet< CBitmapTexture >();

		Float iconSize = 0.5f;
		Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( GetWorldPosition() );
		const Float size = Max( iconSize, iconSize * screenScale * 0.33f ); 

		if ( characterIcon != NULL )
		{
			Color color = IsSelected() ? Color::LIGHT_GREEN : Color::BLUE;

			// Draw slot icons
			for ( Uint32 i = 0; i < characterTransforms.Size(); ++i )
			{
				Vector slotPosition = GetLocalToWorld().TransformPoint( characterTransforms[ i ].GetPosition() );

				Matrix slotMatrix;
				characterTransforms[ i ].CalcLocalToWorld( slotMatrix );

#ifndef NO_COMPONENT_GRAPH
				frame->AddSprite( slotPosition, size, color, m_hitProxyId, characterIcon );
#endif
				frame->AddDebugAxis( slotPosition, GetLocalToWorld() * slotMatrix, 1.0f, true );
				frame->AddDebugText( slotPosition, String::Printf( TXT( "Slot %d" ), i + 1 ), 0, 0, true, Color::LIGHT_BLUE );
			}
		}

		if ( cameraIcon != NULL && m_showCameras == true )
		{
			for ( Uint32 i = 0; i < cameraDefinitions.Size(); ++i )
			{
				Color color = Color::YELLOW;

				Matrix cameraLocalTransform;
				cameraDefinitions[ i ].m_cameraTransform.CalcLocalToWorld( cameraLocalTransform );

				Matrix cameraWorldSpace( Matrix::Mul( GetLocalToWorld(), cameraLocalTransform ) );

				frame->AddDebugText( cameraWorldSpace.GetTranslationRef(), cameraDefinitions[ i ].m_cameraName.AsString(), 0, 0, true, Color::LIGHT_BLUE );

				CRenderCamera camera;
				camera.Set( cameraWorldSpace.GetTranslationRef(), 
					cameraWorldSpace.ToEulerAngles(), cameraDefinitions[ i ].m_cameraFov, 
					1.77f, 0.05f * 0.99f, 0.2f, 0.99f );

				frame->AddDebugFrustum( camera.GetScreenToWorld(), color, true, true, 1.0f );
			}
		}
	}
}

void CStorySceneWaypointComponent::RefreshPositions()
{
}

void CStorySceneWaypointComponent::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	if ( property->GetName().AsString() != TXT( "dialogsetName" ) )
	{
		return;
	}

	valueProperties.m_array = resDialogsetList.LoadAndGet< C2dArray >();
	valueProperties.m_descrColumnName = TXT("Id");
	valueProperties.m_valueColumnName = TXT("Name");
}

void CStorySceneWaypointComponent::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName().AsString() != TXT( "dialogsetName" ) )
	{
		return;
	}

	RefreshPositions();
}

void CStorySceneWaypointComponent::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	//RefreshPositions();
	if ( m_dialogset.Get() == NULL )
	{
		const String path = CStorySceneDialogset::GetDialogsetPathByName( m_dialogsetName );
		m_dialogset = LoadResource< CStorySceneDialogset >( path );
	}
}

void CStorySceneWaypointComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Sprites );
}

void CStorySceneWaypointComponent::OnDetached( CWorld* world )
{
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Sprites );

	TBaseClass::OnDetached( world );
}