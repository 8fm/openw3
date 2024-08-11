#pragma once
#include "meshTypeComponent.h"
#include "renderSettings.h"
#include "meshComponent.h"

class CFurMeshResource;

class CFurComponent : public CMeshComponent
{
	DECLARE_ENGINE_CLASS( CFurComponent, CMeshComponent, 0 )

protected:
	THandle< CFurMeshResource >		m_furMesh;
	Bool							m_forceMesh;
	Bool							m_shouldUpdateWetness;

public:
	CFurComponent();
	virtual ~CFurComponent();

public:
	virtual CMeshTypeResource* GetMeshTypeResource() const override;

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world ) override;
	virtual void OnParentAttachmentAdded( IAttachment* attachment ) override;
	virtual void OnItemEntityAttached( const CEntity* parentEntity ) override;

	// Called when component is detached from world
	virtual void OnDetached( CWorld* world ) override;

	// Visibility flag was forced to change immediately
	virtual void OnVisibilityForced();

	virtual void RefreshRenderProxies();

	// Called when component is spawned ( usually called in entity template editor )
	virtual void OnSpawned( const SComponentSpawnInfo& spawnInfo );

	Uint32 GetBoneCount();

	// Sets the fur mesh
	virtual void SetResource( CResource* furMeshResource ) override;
	virtual void GetResource( TDynArray< const CResource* >& resources ) const override;

	// Returns the fur mesh
	RED_INLINE CFurMeshResource* GetFurMesh() const { return m_furMesh.Get(); }

	//Return wind scaler
	Float GetFurWindScaler() const;

	virtual Bool CanAttachToRenderScene() const;

	virtual void OnUpdateSkinning(const ISkeletonDataProvider* provider, IRenderSkinningData* renderSkinningData, const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext ) override;
	virtual void OnUpdateTransformWithoutSkinning( const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext ) override;

	void UpdateFurParams( Float wetness );

#ifndef NO_EDITOR
	void EditorSetFurParams();
#endif

#ifndef NO_EDITOR_FRAGMENTS
	// Generate editor rendering fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
#endif
	
	Bool IsUsingFur() const;

	// Force (or not) the component to use the mesh fallback instead of fur
	// Important: this must only be used for material replacement which assumes that it
	// is the only source for forcing the mesh!
	void SetForceMesh( Bool enable );

	// Returns true if the mesh is forced
	RED_INLINE Bool IsMeshForced() const { return m_forceMesh; }

private:
	void CheckWetnessSupport( const CEntity* ent );
};

BEGIN_CLASS_RTTI( CFurComponent );
	PARENT_CLASS( CMeshComponent );
	PROPERTY_EDIT( m_furMesh, TXT("Fur mesh") );
END_CLASS_RTTI();
