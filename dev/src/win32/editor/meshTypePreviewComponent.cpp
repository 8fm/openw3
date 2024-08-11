/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "meshTypePreviewComponent.h"
#include "meshPreviewComponent.h"
#include "destructionPreviewComponent.h"
#include "../../common/engine/apexClothResource.h"
#include "../../common/engine/apexDestructionResource.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/furMeshResource.h"
#include "../../common/engine/physicsDestructionResource.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/destructionComponent.h"



IMPLEMENT_RTTI_ENUM( EMeshTypePreviewType );
IMPLEMENT_ENGINE_CLASS( CMeshTypePreviewComponent );


CMeshTypePreviewComponent* CMeshTypePreviewComponent::Create( CEntity* entity, CMeshTypeResource* meshTypeResource )
{
	CClass* componentClass = NULL;

	if ( meshTypeResource->IsA< CPhysicsDestructionResource >() )
	{
		componentClass = ClassID< CMeshTypePreviewPhysicsDestructionComponent >();
	}
	else if ( meshTypeResource->IsA< CMesh >() )
	{
		componentClass = ClassID< CMeshTypePreviewMeshComponent >();
	}
	else if ( meshTypeResource->IsA< CFurMeshResource >() )
	{
		componentClass = ClassID< CMeshTypePreviewFurComponent >();
	}
#ifdef USE_APEX
	else if ( meshTypeResource->IsA< CApexClothResource >() )
	{
		componentClass = ClassID< CMeshTypePreviewClothComponent >();
	}
	else if ( meshTypeResource->IsA< CApexDestructionResource >() )
	{
		componentClass = ClassID< CMeshTypePreviewDestructionComponent >();
	}
#endif

	CMeshTypePreviewComponent* meshTypePreview = NULL;
	
	if ( componentClass )
	{
		meshTypePreview = SafeCast< CMeshTypePreviewComponent >( entity->CreateComponent( componentClass, SComponentSpawnInfo() ) );
		meshTypePreview->SetMeshTypeResource( entity->GetLayer()->GetWorld(), meshTypeResource );
	}

	return meshTypePreview;
}


CMeshTypePreviewComponent::CMeshTypePreviewComponent()
	: m_meshType( MTPT_Unknown )
	, m_meshTypeResource( NULL )
	, m_previewWorld( NULL )
{
}

CMeshTypePreviewComponent::CMeshTypePreviewComponent( EMeshTypePreviewType type )
	: m_meshType( type )
	, m_meshTypeResource( NULL )
	, m_previewWorld( NULL )
{
}

CMeshTypePreviewComponent::~CMeshTypePreviewComponent()
{
}


void CMeshTypePreviewComponent::SetMeshTypeResource( CWorld* previewWorld, CMeshTypeResource* meshTypeResource )
{
	m_previewWorld = previewWorld;
	m_meshTypeResource = meshTypeResource;

	OnSetMeshTypeResource();
}


void CMeshTypePreviewComponent::Reload( CMeshTypeResource* meshTypeResource )
{
	OnUnsetMeshTypeResource();

	m_meshTypeResource = meshTypeResource;

	OnSetMeshTypeResource();
}


IMPLEMENT_ENGINE_CLASS( CMeshTypePreviewMeshComponent );

CMeshTypePreviewMeshComponent::CMeshTypePreviewMeshComponent( EMeshTypePreviewType mtp )
	: TBaseClass( mtp )
	, m_meshComponent( NULL )
{
}

CMeshTypePreviewMeshComponent::~CMeshTypePreviewMeshComponent()
{
}

void CMeshTypePreviewMeshComponent::OnSetMeshTypeResource()
{
	m_previewWorld->DelayedActions();

	if ( m_meshComponent = Cast< CMeshPreviewComponent >( GetEntity()->CreateComponent( ClassID< CMeshPreviewComponent >(), SComponentSpawnInfo() ) ) )
	{
		m_meshComponent->SetResource( m_meshTypeResource );
		m_meshComponent->SetCastingShadows( true );
	}
}

void CMeshTypePreviewMeshComponent::OnUnsetMeshTypeResource()
{
	if ( m_meshComponent )
	{
		m_meshComponent->Destroy();
		m_meshComponent = NULL;
	}
}

CMesh* CMeshTypePreviewMeshComponent::GetMesh()
{
	return m_meshComponent->GetMeshNow();
}

const CCollisionMesh* CMeshTypePreviewMeshComponent::GetCollisionMesh()
{
	return m_meshComponent->GetMeshNow()->GetCollisionMesh();
}

CDrawableComponent* CMeshTypePreviewMeshComponent::GetDrawableComponent()
{
	return m_meshComponent;
}


void CMeshTypePreviewMeshComponent::ShowBoundingBox( Bool show )
{
	m_meshComponent->ShowBoundingBox( show );
}

void CMeshTypePreviewMeshComponent::ShowCollision( Bool show )
{
	m_meshComponent->ShowCollision( show );
}

void CMeshTypePreviewMeshComponent::UpdateBounds()
{
	m_meshComponent->ScheduleUpdateTransformNode();
	m_meshComponent->ForceUpdateBoundsNode();
}


void CMeshTypePreviewMeshComponent::SetActiveCollisionShape( Int32 shapeIndex )
{
	m_meshComponent->SetActiveCollisionShape( shapeIndex );
}


void CMeshTypePreviewMeshComponent::OverrideViewLOD( Int32 lodOverride )
{
	m_meshComponent->ForceLODLevel( lodOverride );
}

IMPLEMENT_ENGINE_CLASS( CMeshTypePreviewFurComponent );

CMeshTypePreviewFurComponent::CMeshTypePreviewFurComponent()
	: TBaseClass( MTPT_Fur )
{
}

CMeshTypePreviewFurComponent::~CMeshTypePreviewFurComponent()
{
}

void CMeshTypePreviewFurComponent::OnSetMeshTypeResource()
{
	CFurMeshResource* furRes = SafeCast< CFurMeshResource >( m_meshTypeResource );

	// Add fur component
	SComponentSpawnInfo spawnInfo;
	spawnInfo.m_customData = furRes;
	m_furComponent = SafeCast< CFurComponent >( GetEntity()->CreateComponent( ClassID< CFurComponent >(), spawnInfo ) );
}

void CMeshTypePreviewFurComponent::OnUnsetMeshTypeResource()
{
	if ( m_furComponent )
	{
		m_furComponent->Destroy();
		m_furComponent = NULL;
	}
}

void CMeshTypePreviewFurComponent::OnDestroyed()
{
	TBaseClass::OnDestroyed();
}

Bool CMeshTypePreviewFurComponent::IsUsingFur()
{
	return m_furComponent->IsUsingFur();
}

void CMeshTypePreviewFurComponent::Refresh()
{
#ifndef NO_EDITOR
	if( m_furComponent )
	{
		m_furComponent->EditorSetFurParams();
	}
#endif
}

EMeshTypePreviewPropertyChangeAction CMeshTypePreviewFurComponent::OnResourcePropertyModified( CName propertyName )
{
	if (	propertyName != TXT("splineMultiplier") &&
			propertyName != TXT("autoHideDistance") &&
			// and add textures are reload cause it needs to recreate renderProxyFur
			// ATM there is no othet way to do that
			propertyName != TXT("densityTex") &&
			propertyName != TXT("lengthTex") &&
			propertyName != TXT("rootWidthTex") &&
			propertyName != TXT("tipWidthTex") &&
			propertyName != TXT("stiffnessTex") &&
			propertyName != TXT("rootStiffnessTex") &&
			propertyName != TXT("clumpRoundnessTex") &&
			propertyName != TXT("clumpScaleTex") &&
			propertyName != TXT("clumpNoiseTex") &&
			propertyName != TXT("waveScaleTex") &&
			propertyName != TXT("waveFreqTex") &&
			propertyName != TXT("rootColorTex") &&
			propertyName != TXT("tipColorTex") &&
			propertyName != TXT("strandTex") &&
			propertyName != TXT("specularTex")
		)
	{
		return MTPPCA_Refresh;
	}
	return TBaseClass::OnResourcePropertyModified( propertyName );
}


IMPLEMENT_ENGINE_CLASS( CMeshTypePreviewPhysicsDestructionComponent );

CMeshTypePreviewPhysicsDestructionComponent::CMeshTypePreviewPhysicsDestructionComponent()
	: TBaseClass( MTPT_PhysxDestruction )
	, m_destructionComponent( NULL )
{
}

CMeshTypePreviewPhysicsDestructionComponent::~CMeshTypePreviewPhysicsDestructionComponent()
{
}

CDrawableComponent* CMeshTypePreviewPhysicsDestructionComponent::GetDrawableComponent()
{
	return m_destructionComponent;
}

CDestructionComponent* CMeshTypePreviewPhysicsDestructionComponent::GetDestructionComponent()
{
	return m_destructionComponent;
}

void CMeshTypePreviewPhysicsDestructionComponent::OnSetMeshTypeResource()
{
	CPhysicsDestructionResource* physRes = SafeCast< CPhysicsDestructionResource >( m_meshTypeResource );

	// Add destruction component
	m_destructionComponent = SafeCast< CDestructionPreviewComponent >( GetEntity()->CreateComponent( ClassID< CDestructionPreviewComponent >(), SComponentSpawnInfo() ) );
	m_destructionComponent->ForceUpdateTransformNodeAndCommitChanges();
	m_destructionComponent->SetPreviewResource( physRes, GetPreviewWorld() );

	m_previewWorld->GetEditorFragmentsFilter().RegisterEditorFragment( m_destructionComponent, SHOW_VisualDebug );

	// Add a plane
	m_destructionComponent->OnUpdateBounds();

	CPhysicsWorld* physicsWorld = nullptr;
	m_previewWorld->GetPhysicsWorld( physicsWorld );
	physicsWorld->AddPlane( m_destructionComponent->GetBoundingBox().Min.Z );
}

void CMeshTypePreviewPhysicsDestructionComponent::OnUnsetMeshTypeResource()
{
	if ( m_destructionComponent )
	{
		m_previewWorld->GetEditorFragmentsFilter().UnregisterEditorFragment( m_destructionComponent, SHOW_VisualDebug );
		m_destructionComponent->Destroy();
		m_destructionComponent = NULL;
	}
}

const CCollisionMesh* CMeshTypePreviewPhysicsDestructionComponent::GetCollisionMesh()
{
	return m_destructionComponent->TryGetMesh()->GetCollisionMesh();
}


void CMeshTypePreviewPhysicsDestructionComponent::ShowBoundingBox( Bool show )
{
	m_destructionComponent->ShowBoundingBox( show );
}

void CMeshTypePreviewPhysicsDestructionComponent::ShowCollision( Bool show )
{
	m_destructionComponent->ShowCollision( show );
}

void CMeshTypePreviewPhysicsDestructionComponent::UpdateBounds()
{
	m_destructionComponent->ScheduleUpdateTransformNode();
	m_destructionComponent->ForceUpdateBoundsNode();
}


void CMeshTypePreviewPhysicsDestructionComponent::SetActiveCollisionShape( Int32 shapeIndex )
{
	m_destructionComponent->SetActiveCollisionShape( shapeIndex );
}


void CMeshTypePreviewPhysicsDestructionComponent::OverrideViewLOD( Int32 lodOverride )
{
	m_destructionComponent->ForceLODLevel( lodOverride );
}

void CMeshTypePreviewPhysicsDestructionComponent::OnDestroyed()
{
	m_previewWorld->GetEditorFragmentsFilter().UnregisterEditorFragment( m_destructionComponent, SHOW_VisualDebug );

	TBaseClass::OnDestroyed();
}

IMPLEMENT_ENGINE_CLASS( CMeshTypePreviewDestructionComponent );

CMeshTypePreviewDestructionComponent::CMeshTypePreviewDestructionComponent()
	: TBaseClass( MTPT_Destruction )
	, m_destructionComponent( NULL )
{
}

CMeshTypePreviewDestructionComponent::~CMeshTypePreviewDestructionComponent()
{
}

void CMeshTypePreviewDestructionComponent::OnSetMeshTypeResource()
{
#ifdef USE_APEX
	CApexDestructionResource* apexRes = SafeCast< CApexDestructionResource >( m_meshTypeResource );

	// Add destruction component
	m_destructionComponent = SafeCast< CDestructionSystemComponent >( GetEntity()->CreateComponent( ClassID< CDestructionSystemComponent >(), SComponentSpawnInfo() ) );
	m_destructionComponent->ForceUpdateTransformNodeAndCommitChanges();
	m_destructionComponent->SetUsePreview( true );
	m_destructionComponent->SetResource( apexRes );

	m_previewWorld->GetEditorFragmentsFilter().RegisterEditorFragment( m_destructionComponent, SHOW_VisualDebug );

	// Add a plane
	m_destructionComponent->OnUpdateBounds();

	CPhysicsWorld* physicsWorld = nullptr;
	m_previewWorld->GetPhysicsWorld( physicsWorld );
	physicsWorld->AddPlane( m_destructionComponent->GetBoundingBox().Min.Z );
#endif
}

void CMeshTypePreviewDestructionComponent::OnUnsetMeshTypeResource()
{
#ifdef USE_APEX
	if ( m_destructionComponent )
	{
		m_previewWorld->GetEditorFragmentsFilter().UnregisterEditorFragment( m_destructionComponent, SHOW_VisualDebug );
		m_destructionComponent->Destroy();
		m_destructionComponent = NULL;
	}
#endif
}

void CMeshTypePreviewDestructionComponent::OnDestroyed()
{
#ifdef USE_APEX
	m_previewWorld->GetEditorFragmentsFilter().UnregisterEditorFragment( m_destructionComponent, SHOW_VisualDebug );
#endif

	TBaseClass::OnDestroyed();
}




IMPLEMENT_ENGINE_CLASS( CMeshTypePreviewClothComponent );

CMeshTypePreviewClothComponent::CMeshTypePreviewClothComponent()
	: TBaseClass( MTPT_Cloth )
	, m_clothComponent( NULL )
	, m_selectedVertex( 0 )
{
}

CMeshTypePreviewClothComponent::~CMeshTypePreviewClothComponent()
{
}

void CMeshTypePreviewClothComponent::OnSetMeshTypeResource()
{
#ifdef USE_APEX
	CApexClothResource* apexRes = SafeCast< CApexClothResource >( m_meshTypeResource );
	// Add cloth component
	m_clothComponent = SafeCast< CClothComponent >( GetEntity()->CreateComponent( ClassID< CClothComponent >(), SComponentSpawnInfo() ) );
	m_clothComponent->ForceUpdateTransformNodeAndCommitChanges();
	m_clothComponent->SetUsePreview( true );
	m_clothComponent->SetResource( apexRes );

	m_previewWorld->GetEditorFragmentsFilter().RegisterEditorFragment( m_clothComponent, SHOW_VisualDebug );
#endif
}

void CMeshTypePreviewClothComponent::OnUnsetMeshTypeResource()
{
#ifdef USE_APEX
	if ( m_clothComponent )
	{
		m_previewWorld->GetEditorFragmentsFilter().UnregisterEditorFragment( m_clothComponent, SHOW_VisualDebug );
		m_clothComponent->Destroy();
		m_clothComponent = NULL;
	}
#endif
}

void CMeshTypePreviewClothComponent::OnDestroyed()
{
#ifdef USE_APEX
	m_previewWorld->GetEditorFragmentsFilter().UnregisterEditorFragment( m_clothComponent, SHOW_VisualDebug );
#endif

	TBaseClass::OnDestroyed();
}

void CMeshTypePreviewClothComponent::OverrideViewLOD( Int32 lodOverride )
{
	m_clothComponent->ForceLODLevel( lodOverride );
}
