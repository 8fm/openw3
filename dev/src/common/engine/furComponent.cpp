#include "build.h"
#include "renderCommands.h"
#include "furComponent.h"
#include "furMeshResource.h"
#include "layer.h"
#include "world.h"
#include "renderProxy.h"
#include "tickManager.h"
#include "entity.h"
#include "meshSkinningAttachment.h"

IMPLEMENT_ENGINE_CLASS( CFurComponent );


CFurComponent::CFurComponent()
	: CMeshComponent()
	, m_furMesh( nullptr )
	, m_forceMesh( false )
	, m_shouldUpdateWetness( false )
{
}

CFurComponent::~CFurComponent()
{
}

void CFurComponent::CheckWetnessSupport( const CEntity* ent )
{
	m_shouldUpdateWetness = ent->FindComponent< CWetnessComponent >() != nullptr;
}

void CFurComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BBoxesFur );

	if ( IsUsingFur() )
	{
		PC_SCOPE_PIX( CFurComponent_OnAttached );

		if ( m_renderProxy )
		{
			( new CRenderCommand_AddFurToScene( world->GetRenderSceneEx(), m_renderProxy ) )->Commit();
		}
	}
}

void CFurComponent::OnDetached( CWorld* world )
{
	if ( IsUsingFur() )
	{
		if ( m_renderProxy )
		{
			( new CRenderCommand_RemoveFurFromScene( world->GetRenderSceneEx(), m_renderProxy ) )->Commit();
		}
	}

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BBoxesFur );

	// Pass to base class
	TBaseClass::OnDetached( world );
}

void CFurComponent::OnVisibilityForced()
{
	if ( IsAttached() && GetLayer()->IsAttached() && m_renderProxy && m_renderProxy->GetType() == RPT_Fur )
	{
		CWorld* attachedWorld = GetLayer()->GetWorld();

		// Should add to scene?
		const Bool shouldAdd = CanAttachToRenderScene();

		if ( shouldAdd == false && m_renderProxy != NULL )
		{
			// Remove current proxy
			if ( attachedWorld->GetRenderSceneEx() )
			{
				( new CRenderCommand_RemoveFurFromScene( attachedWorld->GetRenderSceneEx(), m_renderProxy ) )->Commit();
			}
		}
	}

	TBaseClass::OnVisibilityForced();

	if ( IsAttached() && GetLayer()->IsAttached() && IsUsingFur() )
	{
		CWorld* attachedWorld = GetLayer()->GetWorld();

		// Add new proxy
		if ( attachedWorld->GetRenderSceneEx() && m_renderProxy )
		{
			( new CRenderCommand_AddFurToScene( attachedWorld->GetRenderSceneEx(), m_renderProxy ) )->Commit();
		}
	}
}

void CFurComponent::RefreshRenderProxies()
{
	if ( IsAttached() && GetLayer()->IsAttached() && m_renderProxy && m_renderProxy->GetType() == RPT_Fur )
	{
		CWorld* attachedWorld = GetLayer()->GetWorld();
		// Remove current proxy
		if ( attachedWorld->GetRenderSceneEx() )
		{
			( new CRenderCommand_RemoveFurFromScene( attachedWorld->GetRenderSceneEx(), m_renderProxy ) )->Commit();
		}
	}
	
	TBaseClass::RefreshRenderProxies();
	
	if ( IsAttached() && GetLayer()->IsAttached() && IsUsingFur() )
	{
		CWorld* attachedWorld = GetLayer()->GetWorld();

		// Add the new proxy
		if ( attachedWorld->GetRenderSceneEx() && m_renderProxy )
		{
			( new CRenderCommand_AddFurToScene( attachedWorld->GetRenderSceneEx(), m_renderProxy ) )->Commit();
		}
	}
}

void CFurComponent::OnSpawned( const SComponentSpawnInfo& spawnInfo )
{
	TBaseClass::OnSpawned( spawnInfo );

	m_furMesh = const_cast< CFurMeshResource* >( static_cast< const CFurMeshResource* >( spawnInfo.m_customData ) );
}

Uint32 CFurComponent::GetBoneCount()
{
	if ( IsUsingFur() )
	{
		return m_furMesh->GetBoneCount();
	}
	return 0;
}

void CFurComponent::SetResource( CResource* resource )
{
	RED_ASSERT( resource && resource->IsA< CFurMeshResource >(), TXT("Cannot set '%ls' to '%ls' component."), resource->GetFile()->GetFileName().AsChar(), m_name.AsChar() );
	if ( CFurMeshResource* furMeshResource = Cast<CFurMeshResource>(resource) )
	{
		m_furMesh = furMeshResource;

		// Inform property pages about the change
		EDITOR_DISPATCH_EVENT( CNAME( RefreshPropertiesPage ), NULL );
	}
}

void CFurComponent::GetResource( TDynArray< const CResource* >& resources ) const
{
	resources.PushBack( GetMeshTypeResource() );
}

CMeshTypeResource* CFurComponent::GetMeshTypeResource() const
{ 
	if ( !IsUsingFur() )
	{
		return TBaseClass::GetMeshTypeResource();
	}
	return m_furMesh.Get();
}

Bool CFurComponent::IsUsingFur() const
{
	CFurMeshResource* furMesh = GetFurMesh();
	return !IsMeshForced() && furMesh && ( Int32(furMesh->m_levelOfDetail.m_priority) < Config::cvHairWorksLevel.Get() );
}

void CFurComponent::SetForceMesh( Bool enable )
{
	if ( enable != IsMeshForced() )
	{
		m_forceMesh = enable;
		RefreshRenderProxies();
	}
}

Float CFurComponent::GetFurWindScaler() const
{
	if ( m_furMesh )
	{
		return m_furMesh->GetWindScaler();
	}
	return 1.f;
}

Bool CFurComponent::CanAttachToRenderScene() const
{
	if ( IsUsingFur() )
	{
		// here we are skipping CMeshTypeComponent::CanAttachToRenderScene() because when we are using fur then we don't care if regular mesh is loaded or not
		// so in this case render proxy fur will be created because we are skipping this part
		return CMeshTypeComponent::CanAttachToRenderScene() && m_furMesh.Get();
	}
	else
	{
		//if we are not using fur call TBaseClass to attach render proxy from mesh
		return TBaseClass::CanAttachToRenderScene();
	}
}

void CFurComponent::OnUpdateSkinning(const ISkeletonDataProvider* provider, IRenderSkinningData* renderSkinningData, const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext )
{
	PC_SCOPE( CFurComponent_OnUpdateSkinning )

	Float averageWetness = 0.f;
	if ( IsUsingFur() && m_shouldUpdateWetness )
	{
		// wetness update for fur before calling tbaseclass
		// [ HACK horzel wetcloth ]++
		Matrix* matrices = static_cast< Matrix* >( renderSkinningData->GetWriteData() );

		const Uint32 numBones = renderSkinningData->GetMatrixCount();
		for (Uint32 i=0; i<numBones; ++i )
		{
			averageWetness += 1.f - matrices[ i ].V[ 3 ].A[ 3 ];
			matrices[ i ].V[ 3 ].A[ 3 ] = 1.f;
		}
		averageWetness /= numBones ? numBones : 1.f;
		// [ HACK horzel wetcloth ]--
	}

	TBaseClass::OnUpdateSkinning( provider, renderSkinningData, boxMS, l2w, skinningContext );

	//update fur params as additionam visual boost. Expose as separate call to use it later from mesh editor where is no skinning
	UpdateFurParams( averageWetness );
}

void CFurComponent::OnUpdateTransformWithoutSkinning( const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext )
{
	TBaseClass::OnUpdateTransformWithoutSkinning( boxMS, l2w, skinningContext );
}

void CFurComponent::UpdateFurParams( Float wetness )
{
	// we have to check it here cuz updatetransform happens earlier than refreshrenderproxy from baseEngine->RefreshEngine call
	// so to be sure update wind when we have proxy
	if( m_renderProxy && m_renderProxy->GetType() == RPT_Fur )
	{
		if ( CEntity* ent = GetEntity() )
		{
			if ( CLayer* lay = ent->GetLayer() )
			{
				if ( CWorld* wld = lay->GetWorld() )
				{
					Vector wind = wld->GetWindAtPointForVisuals( m_localToWorld.GetTranslationRef(), true );
					wind *= GetFurWindScaler();
					( new CRenderCommand_UpdateFurParams( m_renderProxy, wind, wetness ) )->Commit();
				}
			}
		}
	}
}

#ifndef NO_EDITOR
void CFurComponent::EditorSetFurParams()
{
	if ( IsUsingFur() )
	{
		CMeshTypeResource* mesh = GetMeshTypeResource();
		if ( CFurMeshResource* fur = Cast< CFurMeshResource >( mesh ) )
		{
			// update wetness/wind as in gameplay
			Float materialWeight = fur->GetMaterialWeight();
			UpdateFurParams( materialWeight );
			fur->EditorRefresh( m_renderProxy );
		}
	}
}
#endif //NO_EDITOR

#ifndef NO_EDITOR_FRAGMENTS
void CFurComponent::OnGenerateEditorFragments(CRenderFrame* frame, EShowFlags flag)
{
	TBaseClass::OnGenerateEditorFragments(frame,flag);
	// Draw only if visible
	if ( IsUsingFur() )
	{
		if ( GetMeshTypeResource() && IsVisible() && IsAttached() && flag == SHOW_BBoxesFur )
		{
			const Vector& camPos = frame->GetFrameInfo().m_camera.GetPosition();
			Float dist = camPos.DistanceTo( m_localToWorld.GetTranslationRef() );
			frame->AddDebugBox( m_boundingBox, Matrix::IDENTITY, Color::RED, false, true );
			frame->AddDebugText( m_boundingBox.CalcCenter(), ToString( dist ), 0, 0, false, Color::GREEN );
			frame->AddDebugAxis( m_localToWorld.GetTranslationRef(), m_localToWorld, 0.1f, true );
		}
	}
}
#endif

void CFurComponent::OnParentAttachmentAdded(IAttachment* attachment)
{
	TBaseClass::OnParentAttachmentAdded( attachment );

	CheckWetnessSupport( attachment->GetParent()->FindParent< CEntity >() );
}

void CFurComponent::OnItemEntityAttached( const CEntity* par )
{
	TBaseClass::OnItemEntityAttached( par );
	CheckWetnessSupport( par );
}
