/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/engine/destructionSystemComponent.h"
#include "../../common/engine/clothComponent.h"
#include "../../common/engine/furComponent.h"
#include "../../common/engine/destructionComponent.h"

class CMeshTypePreviewComponent;
class CDestructionPreviewComponent;

enum EMeshTypePreviewType
{
	MTPT_Unknown,
	MTPT_Mesh,
	MTPT_Destruction,
	MTPT_Cloth,
	MTPT_Fur,
	MTPT_PhysxDestruction
};

BEGIN_ENUM_RTTI( EMeshTypePreviewType );
ENUM_OPTION( MTPT_Unknown );
ENUM_OPTION( MTPT_Mesh );
ENUM_OPTION( MTPT_Destruction );
ENUM_OPTION( MTPT_Cloth );
ENUM_OPTION( MTPT_Fur );
END_ENUM_RTTI();

enum EMeshTypePreviewPropertyChangeAction
{
	MTPPCA_Refresh,
	MTPPCA_Reload,
};

/// Component to render preview for any mesh type resource. Each mesh type has a separate subclass with specific functionality.
class CMeshTypePreviewComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CMeshTypePreviewComponent, CComponent, 0 );

	EMeshTypePreviewType	m_meshType;

protected:
	CMeshTypeResource*		m_meshTypeResource;
	CWorld*					m_previewWorld;

	virtual void OnSetMeshTypeResource() {}
	virtual void OnUnsetMeshTypeResource() {}


public:
	/// Create new CMeshTypePreviewComponent based on mesh type, attach to entity.
	static CMeshTypePreviewComponent* Create( CEntity* entity, CMeshTypeResource* meshTypeResource );

	CMeshTypePreviewComponent();
	CMeshTypePreviewComponent( EMeshTypePreviewType type );
	virtual ~CMeshTypePreviewComponent();

	EMeshTypePreviewType	GetMeshType() { return m_meshType; }
	CWorld*					GetPreviewWorld() { return m_previewWorld; }

	void SetMeshTypeResource( CWorld* previewWorld, CMeshTypeResource* meshTypeResource );
	void Reload( CMeshTypeResource* meshTypeResource );
	virtual void Refresh() {}

	CMeshTypeResource* GetMeshTypeResource() { return m_meshTypeResource; }

	virtual CMesh* GetMesh() { return NULL; }
	virtual const CCollisionMesh* GetCollisionMesh() { return NULL; }
	virtual CDrawableComponent* GetDrawableComponent() { return NULL; }
	
	virtual void ShowBoundingBox( Bool show ) {}
	virtual void ShowCollision( Bool show ) {}
	virtual void UpdateBounds() {}
	virtual void OverrideViewLOD( Int32 lodOverride ) {}

	virtual void SetActiveCollisionShape( Int32 shapeIndex ) {}

	virtual EMeshTypePreviewPropertyChangeAction OnResourcePropertyModified( CName propertyName ) { return MTPPCA_Reload; }
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y ) { return false; }
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data ) { return false; }
	virtual Bool OnViewportMouseMove( const CMousePacket& packet ) { return false; }
};


BEGIN_CLASS_RTTI( CMeshTypePreviewComponent );
PARENT_CLASS( CComponent );
END_CLASS_RTTI();



class CMeshTypePreviewMeshComponent : public CMeshTypePreviewComponent
{
	DECLARE_ENGINE_CLASS( CMeshTypePreviewMeshComponent, CMeshTypePreviewComponent, 0 );

	CMeshPreviewComponent* m_meshComponent;

protected:
	virtual void OnSetMeshTypeResource();
	virtual void OnUnsetMeshTypeResource();

public:
	CMeshTypePreviewMeshComponent( EMeshTypePreviewType mtp = MTPT_Mesh );
	virtual ~CMeshTypePreviewMeshComponent();

public:
	virtual CMesh* GetMesh();
	virtual const CCollisionMesh* GetCollisionMesh();
	virtual CDrawableComponent* GetDrawableComponent();	

	virtual void ShowBoundingBox( Bool show );
	virtual void ShowCollision( Bool show );
	virtual void UpdateBounds();
	virtual void SetActiveCollisionShape( Int32 shapeIndex );

	virtual void OverrideViewLOD( Int32 lodOverride );
};

BEGIN_CLASS_RTTI( CMeshTypePreviewMeshComponent );
PARENT_CLASS( CMeshTypePreviewComponent );
END_CLASS_RTTI();

class CMeshTypePreviewFurComponent : public CMeshTypePreviewComponent
{
	DECLARE_ENGINE_CLASS( CMeshTypePreviewFurComponent, CMeshTypePreviewComponent, 0 );

	CFurComponent* m_furComponent;

protected:
	virtual void OnSetMeshTypeResource();
	virtual void OnUnsetMeshTypeResource();

public:
	CMeshTypePreviewFurComponent();
	virtual ~CMeshTypePreviewFurComponent();

	virtual CDrawableComponent*	GetDrawableComponent() { return m_furComponent; }
	virtual void				OnDestroyed();
	Bool						IsUsingFur();
	virtual void				Refresh() override;
	virtual EMeshTypePreviewPropertyChangeAction OnResourcePropertyModified( CName propertyName );
};

BEGIN_CLASS_RTTI( CMeshTypePreviewFurComponent );
PARENT_CLASS( CMeshTypePreviewComponent );
END_CLASS_RTTI();

class CMeshTypePreviewPhysicsDestructionComponent : public CMeshTypePreviewComponent
{
	DECLARE_ENGINE_CLASS( CMeshTypePreviewPhysicsDestructionComponent, CMeshTypePreviewComponent, 0 );

	CDestructionPreviewComponent* m_destructionComponent;

protected:
	virtual void OnSetMeshTypeResource();
	virtual void OnUnsetMeshTypeResource();

public:
	CMeshTypePreviewPhysicsDestructionComponent();
	virtual ~CMeshTypePreviewPhysicsDestructionComponent();

public:
	virtual CDrawableComponent* GetDrawableComponent();

	CDestructionComponent* GetDestructionComponent();

	virtual const CCollisionMesh* GetCollisionMesh();

	virtual void ShowBoundingBox( Bool show );
	virtual void ShowCollision( Bool show );
	virtual void UpdateBounds();
	virtual void SetActiveCollisionShape( Int32 shapeIndex );

	virtual void OverrideViewLOD( Int32 lodOverride );

	virtual void OnDestroyed();
};

BEGIN_CLASS_RTTI( CMeshTypePreviewPhysicsDestructionComponent );
PARENT_CLASS( CMeshTypePreviewComponent );
END_CLASS_RTTI();


class CMeshTypePreviewDestructionComponent : public CMeshTypePreviewComponent
{
	DECLARE_ENGINE_CLASS( CMeshTypePreviewDestructionComponent, CMeshTypePreviewComponent, 0 );

	CDestructionSystemComponent* m_destructionComponent;

protected:
	virtual void OnSetMeshTypeResource();
	virtual void OnUnsetMeshTypeResource();

public:
	CMeshTypePreviewDestructionComponent();
	virtual ~CMeshTypePreviewDestructionComponent();

public:
	virtual CDrawableComponent* GetDrawableComponent() { return m_destructionComponent; }

	CDestructionSystemComponent* GetDestructionSystemComponent() { return m_destructionComponent; }

	virtual void OnDestroyed();
};

BEGIN_CLASS_RTTI( CMeshTypePreviewDestructionComponent );
PARENT_CLASS( CMeshTypePreviewComponent );
END_CLASS_RTTI();



class CMeshTypePreviewClothComponent : public CMeshTypePreviewComponent
{
	DECLARE_ENGINE_CLASS( CMeshTypePreviewClothComponent, CMeshTypePreviewComponent, 0 );

	CClothComponent* m_clothComponent;

	Uint32	m_selectedVertex;
	Vector	m_hitPosition;

protected:
	virtual void OnSetMeshTypeResource();
	virtual void OnUnsetMeshTypeResource();

public:
	CMeshTypePreviewClothComponent();
	virtual ~CMeshTypePreviewClothComponent();

public:
	virtual CDrawableComponent* GetDrawableComponent() { return m_clothComponent; }

	virtual void OnDestroyed();

	virtual void OverrideViewLOD( Int32 lodOverride );

};

BEGIN_CLASS_RTTI( CMeshTypePreviewClothComponent );
PARENT_CLASS( CMeshTypePreviewComponent );
END_CLASS_RTTI();
