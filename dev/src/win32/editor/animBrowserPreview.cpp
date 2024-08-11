/*
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "timeline.h"
#include "animBrowserPreview.h"
#include "viewportWidgetMoveAxis.h"
#include "viewportWidgetRotateAxis.h"
#include "viewportWidgetScaleAxis.h"
#include "../../common/game/actor.h"
#include "../../common/engine/cameraDirector.h"
#include "../../common/engine/behaviorGraphContext.h"
#include "../../common/core/gatheredResource.h"
#include <shellapi.h>
#include "../../common/engine/cameraComponent.h"
#include "../../common/engine/playedAnimation.h"
#include "../../common/engine/mimicComponent.h"
#include "../../common/engine/renderer.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/worldTick.h"
#include "../../common/engine/poseProvider.h"



const Vector		CEdAnimBrowserPreview::CAMERA_POS_OFFSET = Vector( 0.f, 4.f, 2.f );
const EulerAngles	CEdAnimBrowserPreview::CAMERA_ROT_OFFSET = EulerAngles( 0.f, -10.f, 180.f );

CGatheredResource resMaterialRedShader( TXT("characters\\shaders\\special\\preview_red.w2mg"), RGF_Startup ); 
CGatheredResource resMaterialGreenShader( TXT("characters\\shaders\\special\\preview_green.w2mg"), RGF_Startup ); 

CEdAnimBrowserPreview::CEdAnimBrowserPreview( wxWindow* parent, ISkeletonPreviewControl* skeletonPreviewControl, IAnimBrowserPreviewListener* listener )
	: CEdPreviewPanel( parent, true )
	, m_usePlayerCamera( false )
	, m_contextMenu( NULL )
	, m_cameraEntity( NULL )
	, m_cameraComponent( NULL )
	, m_isCameraAlign( false )
	, m_prevEntityPos( 0,0,0 )
	, m_timeMultiplier( 1.0f )
	, m_playedAnimation( NULL )
	, m_skeletonPreviewControl( skeletonPreviewControl )
	, m_showBBox( false )
	, m_listener( listener )
	, m_playerAnimationPreviousTime( 0.0f )
{
	SetCameraPosition( CAMERA_POS_OFFSET );
	SetCameraRotation( CAMERA_ROT_OFFSET );

	// Add widgets - move only
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EX, Color::RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EY, Color::GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EZ, Color::BLUE ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EY, Vector::EZ, Color::LIGHT_RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EZ, Color::LIGHT_GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EY, Color::LIGHT_BLUE ) );

	// Set local space
	m_widgetManager->SetWidgetSpace( RPWS_Local );

	// Select only components
	GetSelectionManager()->SetGranularity( CSelectionManager::SG_Components );

	// Register as event listener
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
}

CEdAnimBrowserPreview::~CEdAnimBrowserPreview()
{
	DestroyClones();

	m_listener = NULL;

	SEvents::GetInstance().UnregisterListener( this );
}

void CEdAnimBrowserPreview::OnLoadEntity( CEntity* entityCamera )
{
	CWorldTickInfo info( m_previewWorld, 0.1f );
	m_previewWorld->Tick( info );

	// Find default camera
	if ( entityCamera )
	{
		m_cameraEntity = entityCamera;

		TDynArray< CComponent* > components;
		CollectEntityComponents( entityCamera, components );

		for( Uint32 i=0; i<components.Size(); ++i )
		{
			if ( components[i]->IsA<CCameraComponent>() )
			{
				CCameraComponent *camComponent = SafeCast< CCameraComponent >( components[i] );
				if (camComponent->IsDefaultCamera())
				{
					m_cameraComponent = camComponent;
				}
			}
		}
	}

	RecreateClones();

	m_ghosts.Reset();
}

void CEdAnimBrowserPreview::OnUnloadEntity()
{
	DestroyClones();
	m_cameraComponent = NULL;
	ResetCamera();
}

void CEdAnimBrowserPreview::SetUsePlayerCamera( Bool flag )
{
	m_usePlayerCamera = flag;
}

void CEdAnimBrowserPreview::AlignCamera( const Vector& pos )
{
	if ( m_isCameraAlign )
	{
		m_cameraPosition = m_cameraPosition - m_prevEntityPos + pos;
		m_prevEntityPos = pos;
	}
	else
	{
		m_cameraPosition = pos + CAMERA_POS_OFFSET;
		m_isCameraAlign = true;
	}

	OnCameraMoved();
}

void CEdAnimBrowserPreview::ResetCamera()
{
	m_isCameraAlign = false;

	m_cameraPosition = CAMERA_POS_OFFSET;
	m_cameraRotation = CAMERA_ROT_OFFSET;
	m_cameraZoom = 3.0f;
	m_prevEntityPos = Vector::ZERO_3D_POINT;

	ResetCameraMoveKeys();
	OnCameraMoved();
}

void CEdAnimBrowserPreview::OnViewportTick( IViewport* view, Float timeDelta )
{
	timeDelta *= m_timeMultiplier;

	// Tick world
	CWorldTickInfo info( m_previewWorld, timeDelta );
	info.m_updatePhysics = true;
	if( m_playedAnimation )
	{
		if( m_playerAnimationPreviousTime == m_playedAnimation->GetTime() )
		{
			info.m_updatePhysics = false;
		}
		m_playerAnimationPreviousTime = m_playedAnimation->GetTime();
	}
	m_previewWorld->Tick( info );
	if ( m_listener )
	{
		m_listener->Tick( timeDelta );
	}

	if ( m_usePlayerCamera && m_cameraEntity )
	{
		AlignCamera( m_cameraEntity->GetPosition() );
	}

	// Tick panel
	CEdRenderingPanel::OnViewportTick( view, timeDelta );

	// Update item entities
	SItemEntityManager::GetInstance().OnTick( timeDelta );

	// Update clones
	UpdateClones( timeDelta );

	if( m_animatedComponents.Size() == 1 )
	{
		m_ghosts.UpdateGhosts( timeDelta, m_animatedComponents[ 0 ] );
	}
}

void CEdAnimBrowserPreview::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	if ( m_usePlayerCamera && m_cameraComponent )
	{
		m_cameraComponent->OnViewportCalculateCamera( view, camera );
		return;
	}	

	CEdPreviewPanel::OnViewportCalculateCamera( view, camera );

	if( !m_cameraEntity ) return;

	Vector forward, up;
	camera.GetRotation().ToAngleVectors( &forward, NULL, &up );

	UpdateSoundListener( camera.GetPosition(), up, forward, m_cameraEntity->GetSoundEmitterComponent( false ) );

}

void CEdAnimBrowserPreview::HandleContextMenu( Int32 x, Int32 y )
{
	CEdPreviewPanel::HandleContextMenu(x,y);

	if ( m_contextMenu != NULL )
	{
		PopupMenu( m_contextMenu, x, y );
	}
}

void CEdAnimBrowserPreview::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	if ( m_listener )
	{
		m_listener->HandleSelection( objects );
	}
}

void CEdAnimBrowserPreview::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( SelectionChanged ) )
	{
		// Selection has changed
		typedef CSelectionManager::SSelectionEventData SEventData;
		const SEventData& eventData = GetEventData< SEventData >( data );
		if ( eventData.m_world == m_previewWorld )
		{
			TDynArray< CNode* > nodes;
			GetSelectionManager()->GetSelectedNodes( nodes );

			if ( nodes.Size() > 0 )
			{
				ASSERT( nodes.Size() == 1 );

				if ( m_listener )
				{
					m_listener->SelectItem( nodes[0] );
				}
			}
		}
	}
}

void CEdAnimBrowserPreview::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// Scene
	CEdPreviewPanel::OnViewportGenerateFragments( view, frame );

	frame->AddDebugAxisOnScreen( 0.2f, 0.2f, Matrix::IDENTITY, 0.1f );

	// Display skeleton
	if ( m_animatedComponents.Size() == 1 )
	{
		CAnimatedComponent* ac = m_animatedComponents[ 0 ];

		DisplaySkeleton( frame, ac );

		m_ghosts.Draw( frame, ac );

		if ( m_showBBox && m_playedAnimation && 
			m_playedAnimation->GetAnimationEntry() && ac &&
			m_playedAnimation->GetAnimationEntry()->GetAnimation()->HasBoundingBox() )
		{
			Box bbox = m_playedAnimation->GetAnimationEntry()->GetAnimation()->GetBoundingBox();
			frame->AddDebugBox( bbox, ac->GetEntity()->GetLocalToWorld(), Color::GREEN );
		}
	}
	else
	{
		Uint32 colorMask = 1;

		for ( Uint32 i=0; i<m_animatedComponents.Size(); ++i )
		{
			colorMask = (colorMask + 1) % 7;	
			if ( !colorMask ) colorMask = 1;

			Color color = Color(	colorMask & 1 ? 255 : 0, 
				colorMask & 2 ? 255 : 0,
				colorMask & 4 ? 255 : 0 );

			CAnimatedComponent* ac = m_animatedComponents[i];
			DisplaySkeleton( frame, ac, color );
		}
	}

	if ( m_listener )
	{
		m_listener->OnViewportGenerateFragments( view, frame );
	}
}

Bool CEdAnimBrowserPreview::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if( key == IK_MouseZ )
	{
		Float currentFov = GetCameraFov();

		if( data > 0.0f )
		{
			currentFov -= 2.5f;
		}
		else
		{
			currentFov += 2.5f;
		}

		currentFov = Clamp( currentFov, 1.0f, 180.0f );
		SetCameraFov( currentFov );

		return true;
	}

	return CEdPreviewPanel::OnViewportInput( view, key, action, data );
}

Bool CEdAnimBrowserPreview::IsSkeleton()
{
	return ( m_skeletonPreviewControl != NULL && m_skeletonPreviewControl->IsSkeleton() );
}

Bool CEdAnimBrowserPreview::IsBoneNames()
{
	return ( m_skeletonPreviewControl != NULL && m_skeletonPreviewControl->IsBoneNames() );
}

Bool CEdAnimBrowserPreview::IsBoneAxis()
{
	return ( m_skeletonPreviewControl != NULL && m_skeletonPreviewControl->IsBoneAxis() );
}

void CEdAnimBrowserPreview::TakeScreenshotsFromAnim( CPlayedSkeletalAnimation *playedAnimation )
{
	ASSERT( playedAnimation != NULL, TXT( "Can't take a screenshot with a NULL animation" ) );

	Float totalTime = playedAnimation->GetDuration();
	CWorldTickInfo tickInfo ( GetWorld(), 0.0000001 );
	String animName = playedAnimation->GetName().AsString();
	String folderName = TXT( "screenshots\\" );

	Uint32 frameNumber = 0;
	for( Float time = 0.0f; time <= totalTime; time += 0.0333, frameNumber++ )
	{
		String fileName = folderName + animName;
		playedAnimation->SetTime( time );
		GetWorld()->Tick( tickInfo );

		CRenderFrameInfo frameInfo( GetViewport() );
		frameInfo.m_clearColor = this->GetClearColor();
		CRenderFrame* frame = GRender->CreateFrame( NULL, frameInfo );
		if ( !frame )
		{
			HALT( "Failed to create frame" );
		}

		GetWorld()->RenderWorld( frame );
		frame->Release();

		// Take screenshot
		fileName += String::Printf( TXT( "%04d.bmp" ), frameNumber );
		TakeScreenshot( fileName );
	}

	String avsContent = TXT( "ImageSource(\"" );
	avsContent += animName;
	avsContent += TXT( "%04d.bmp\", 0, " );
	avsContent += String::Printf( TXT("%d"), frameNumber - 1 ) ;
	avsContent += TXT( ", 30)" );
	String avsName = folderName + animName;
	avsName += TXT( ".avs" );
	String pureAvsName = animName;
	pureAvsName += TXT( ".avs" );

	GFileManager->SaveStringToFile( avsName, avsContent );	
	ShellExecute( NULL, NULL, TXT( "2avi.bat" ), pureAvsName.AsChar(), folderName.AsChar(), SW_HIDE);

}

void CEdAnimBrowserPreview::SetAnimatedComponent( CAnimatedComponent* animatedComponent )
{
	const Bool resetCamera = m_animatedComponents.Empty() || m_animatedComponents[ 0 ] == nullptr;

	m_animatedComponents.Clear();
	m_animatedComponents.PushBack( animatedComponent );

	if ( resetCamera )
	{
		ResetCameraPosition();
	}
}

void CEdAnimBrowserPreview::SetAnimatedComponents( TDynArray< CAnimatedComponent* >& animatedComponents )
{
	const Bool resetCamera = m_animatedComponents.Empty() || m_animatedComponents[ 0 ] == nullptr;

	m_animatedComponents.Clear();
	m_animatedComponents = animatedComponents;

	if ( resetCamera )
	{
		ResetCameraPosition();
	}
}

void CEdAnimBrowserPreview::ResetCameraPosition()
{
	if ( m_animatedComponents.Size() > 0 )
	{
		CAnimatedComponent* ac = m_animatedComponents[ 0 ];
		if ( ac )
		{
			// if the position of component/entity is far away from the world center, move it back to the default position as in animationPreview
			// see: CEdAnimationPreview::ShouldBackToDefaultPosition
			if ( ac->GetWorldPositionRef().SquareMag3() < 100000.f )
			{
				Box box;
				box.Clear();

				if ( CMimicComponent* m = Cast< CMimicComponent >( ac ) )
				{
					const Int32 num = m->GetBonesNum();
					for ( Int32 i=0; i<num; ++i )
					{
						box.AddPoint( m->GetBoneMatrixWorldSpace( i ).GetTranslation() );
					}

					if ( !box.IsEmpty() )
					{
						SetCameraPosition( box.CalcCenter() + Vector( 0.4f, 2.5f, 0.9f ) * ( box.CalcSize().Mag3() * 0.4f ) );
						SetCameraRotation( EulerAngles( 0.f, -5.f, 175.f ) );
					}
				}
				else
				{
					box = ac->GetEntity()->CalcBoundingBox();

					if ( !box.IsEmpty() )
					{
						SetCameraPosition( box.CalcCenter() + Vector( 1.f, 2.f, 1.f ) * box.CalcSize().Mag3() * 0.4f );
						SetCameraRotation( EulerAngles( 0.f, -20.f, 160.f ) );
					}			
				}
			}
			else
			{
				// wrap entity position
				ac->GetEntity()->SetPosition( Vector::ZERO_3D_POINT );
				ac->GetEntity()->SetRotation( EulerAngles::ZEROS );
				if ( CHardAttachment* att = ac->GetEntity()->GetTransformParent() )
				{
					att->GetParent()->SetPosition( Vector::ZERO_3D_POINT );
					att->GetParent()->SetRotation( EulerAngles::ZEROS );
				}

				SetCameraPosition( Vector( 3.0f, 3.0f, 3.0f ) );

				Vector entPosVector = Vector::ZERO_3D_POINT - m_cameraPosition;
				entPosVector.Normalize3();

				Matrix rotMtx;
				rotMtx.BuildFromDirectionVector( entPosVector );
				SetCameraRotation( rotMtx.ToEulerAngles() );
			}
		}
	}
}

CEdEntityClone* CEdAnimBrowserPreview::FindClone( Uint32 id ) const
{
	const Uint32 size = m_clones.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CEdEntityClone* clone = m_clones[ i ];
		if ( clone->GetId() == id )
		{
			return clone;
		}
	}

	return NULL;
}

void CEdAnimBrowserPreview::DestroyClones()
{
	m_clones.ClearPtr();
}

void CEdAnimBrowserPreview::RecreateClones()
{
	// TODO
}

void CEdAnimBrowserPreview::UpdateClones( Float dt )
{
	for ( Int32 i=m_clones.SizeInt()-1; i>=0; --i )
	{
		CEdEntityClone* clone = m_clones[ i ];

		if ( !clone->Update( dt ) )
		{
			m_clones.RemoveAt( i );
			delete clone;
		}
	}
}

Bool CEdAnimBrowserPreview::CreateClone( Uint32 id, const CEntity* entity )
{
	if ( HasClone( id ) )
	{
		ASSERT( !HasClone( id ) );
		SyncCloneTo( id, entity );
		return false;
	}
	
	CEdEntityClone* clone = new CEdEntityClone( id );
	if ( clone->Init( entity ) )
	{
		m_clones.PushBack( clone );
		return true;
	}
	else
	{
		delete clone;
		return false;
	}
}

Bool CEdAnimBrowserPreview::CreateTemporaryClone( Uint32 id, const CEntity* entity, Float lifeTime )
{
	Bool ret = CreateClone( id, entity );

	if ( ret )
	{
		FindClone( id )->SetLifeTime( lifeTime );
	}

	return ret;
}

Bool CEdAnimBrowserPreview::DestroyClone( Uint32 id )
{
	CEdEntityClone* clone = FindClone( id );
	if ( clone )
	{
		m_clones.Remove( clone );
		delete clone;
		return true;
	}
	return false;
}

Bool CEdAnimBrowserPreview::HasClone( Uint32 id ) const
{
	return FindClone( id ) != NULL;
}

Bool CEdAnimBrowserPreview::SyncCloneTo( Uint32 id, const CEntity* entity )
{
	CEdEntityClone* clone = FindClone( id );
	if ( clone )
	{
		return clone->CopyPoseFrom( entity );
	}

	return false;
}

Bool CEdAnimBrowserPreview::SyncCloneTo( Uint32 id, const CPlayedSkeletalAnimation* anim, Bool compressedPose )
{
	CEdEntityClone* clone = FindClone( id );
	if ( clone )
	{
		return clone->CopyPoseFrom( anim, compressedPose );
	}

	return false;
}

void CEdAnimBrowserPreview::SetOffsetForClone( Uint32 id, Float offset )
{
	CEdEntityClone* clone = FindClone( id );
	if ( clone )
	{
		clone->SetOffset( offset );
	}
}

void CEdAnimBrowserPreview::SetCloneColor( Uint32 id, CEdEntityClone::ECloneColor color )
{
	CEdEntityClone* clone = FindClone( id );
	if ( clone )
	{
		clone->SetColor( color );
	}
}

void CEdAnimBrowserPreview::SetCloneVisibility( Uint32 id, Bool flag )
{
	CEdEntityClone* clone = FindClone( id );
	if ( clone )
	{
		clone->SetVisibility( flag );
	}
}

void CEdAnimBrowserPreview::ShowGhosts( Uint32 number, PreviewGhostContainer::EGhostType type )
{
	m_ghosts.InitGhosts( number, type );
}

void CEdAnimBrowserPreview::HideGhosts()
{
	m_ghosts.DestroyGhosts();
}

Bool CEdAnimBrowserPreview::HasGhosts()
{
	return m_ghosts.HasGhosts();
}

//////////////////////////////////////////////////////////////////////////


CEdEntityClone::CEdEntityClone( Uint32 id )
	: m_id( id )
	, m_entity( NULL )
	, m_temporary( false )
	, m_lifeTimer( 0.f )
	, m_offset( 0.f )
	, m_firstUpdate( true )
{

}

CEdEntityClone::~CEdEntityClone()
{
	if ( m_entity )
	{
		// Destroy entity
		m_entity->Destroy();
	}
}

Bool CEdEntityClone::Init( const CEntity* entity )
{
	// Remember it
	m_original = entity;

	// Create entity
	CEntityTemplate* templ = entity->GetEntityTemplate();
	if ( !templ )
	{
		return false;
	}

	if ( !entity->GetLayer() )
	{
		ASSERT( entity->GetLayer() );
		return false;
	}

	CLayer* layer = entity->GetLayer()->GetWorld()->GetDynamicLayer();
	if ( !layer )
	{
		return false;
	}

	EntitySpawnInfo info;
	info.m_template = templ;
	info.m_name = String::Printf( TXT("Clone_%d"), m_id );
	m_entity = layer->CreateEntitySync( info );
	if ( !m_entity )
	{
		return false;
	}

	for ( ComponentIterator< CAnimatedComponent > it( m_entity ); it; ++it )
	{
		(*it)->Freeze();
	}

	return true;
}

Bool CEdEntityClone::Update( Float dt )
{
	if ( m_temporary )
	{
		m_lifeTimer -= dt;

		if ( m_lifeTimer < 0.f )
		{
			return false;
		}
	}

	if ( m_firstUpdate && m_entity )
	{
		CActor* actor = Cast< CActor >( m_entity );
		if( actor && actor->GetInventoryComponent() )
		{
			actor->InitInventory();
			actor->GetInventoryComponent()->SpawnMountedItems();
		}

		actor->ForceUpdateTransformNodeAndCommitChanges();
		actor->ForceUpdateBoundsNode();

		m_firstUpdate = false;
	}

	return true;
}

Uint32 CEdEntityClone::GetId() const
{
	return m_id;
}

Bool CEdEntityClone::CopyPoseFrom( const CEntity* entity )
{
	if ( !m_entity )
	{
		ASSERT( m_entity );
		return false;
	}

	TDynArray< CAnimatedComponent* > compsA;
	TDynArray< CAnimatedComponent* > compsB;
	
	for ( ComponentIterator< CAnimatedComponent > it( entity ); it; ++it )
	{
		compsA.PushBack( *it );
	}

	for ( ComponentIterator< CAnimatedComponent > it( m_entity ); it; ++it )
	{
		compsB.PushBack( *it );
	}

	if ( compsA.Size() != compsB.Size() )
	{
		return false;
	}

	const Uint32 size = compsA.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAnimatedComponent* cA = compsA[ i ];
		CAnimatedComponent* cB = compsB[ i ];

		if ( !cA->GetBehaviorGraphSampleContext() )
		{
			continue;
		}

		const SBehaviorGraphOutput& poseA = cA->GetBehaviorGraphSampleContext()->GetSampledPose();

		cB->ForceBehaviorPose( poseA );
	}

	return true;
}

Bool CEdEntityClone::CopyPoseFrom( const CPlayedSkeletalAnimation* anim, Bool compressedPose )
{
	if ( !anim->GetAnimationEntry() || !anim->GetAnimationEntry()->GetAnimation() )
	{
		return false;
	}

	if ( !m_entity )
	{
		ASSERT( m_entity );
		return false;
	}

	CAnimatedComponent* root = m_entity->GetRootAnimatedComponent();
	if ( root )
	{
		const CSkeleton* skeleton = root->GetSkeleton();
		if ( skeleton )
		{
			CPoseProvider* poseAlloc = skeleton->GetPoseProvider();
			if ( poseAlloc )
			{
				CPoseHandle pose = poseAlloc->AcquirePose();
				if ( pose )
				{
					const CSkeletalAnimation* skAnim = anim->GetAnimationEntry()->GetAnimation();

					skAnim->SampleCompressedPose( pose->m_numBones, pose->m_outputPose, pose->m_numFloatTracks, pose->m_floatTracks, skeleton );

					if ( root->UseExtractedTrajectory() )
					{
						pose->ExtractTrajectory( root );
					}

					root->ForceBehaviorPose( *pose );

					return true;
				}
			}
		}
	}

	return false;
}

void CEdEntityClone::SetOffset( Float offset )
{
	Float diff = offset - m_offset;

	// Change offset
	if ( m_entity )
	{
		m_entity->SetPosition( m_entity->GetPosition() + Vector( diff, 0.f, 0.f ) );
	}

	m_offset = offset;
}

void CEdEntityClone::SetLifeTime( Float time )
{
	m_temporary = true;
	m_lifeTimer = time;
}

void CEdEntityClone::SetVisibility( Bool flag )
{
	if ( m_entity )
	{
		m_entity->SetHideInGame( !flag );
	}
}

void CEdEntityClone::SetColor( ECloneColor color )
{
	if ( m_entity->HasMaterialReplacement() )
	{
		m_entity->DisableMaterialReplacement();
	}

	switch ( color )
	{
	case CC_Original:
		{
			
		}
		break;


	case CC_Default:
		{
			CMaterialGraph* material = GRender->GetFallbackShader();
			if ( material )
			{
				m_entity->SetMaterialReplacement( material );
			}
			else
			{
				SetColor( CC_Original );
			}
		}
		break;


	case CC_Red:
		{
			CMaterialGraph* material = resMaterialRedShader.LoadAndGet< CMaterialGraph >();
			if ( material )
			{
				m_entity->SetMaterialReplacement( material );
			}
			else
			{
				SetColor( CC_Default );
			}
		}
		break;


	case CC_Green:
		{
			CMaterialGraph* material = resMaterialGreenShader.LoadAndGet< CMaterialGraph >();
			if ( material )
			{
				m_entity->SetMaterialReplacement( material );
			}
			else
			{
				SetColor( CC_Default );
			}
		}
		break;
	}
}
