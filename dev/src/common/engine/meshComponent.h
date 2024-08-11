/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "meshTypeComponent.h"
#include "../redThreads/redThreadsAtomic.h"
#include "../redThreads/redThreadsThread.h"
#include "../core/softHandleProcessor.h"

class CMeshSkinningAttachment;
struct VectorI;
class CMeshTypeResource;

#ifdef USE_UMBRA
class CUmbraScene;
namespace Umbra{ class Scene; }
#endif

class CMeshComponent;

/// Component rendering mesh
class CMeshComponent : public CMeshTypeComponent
{
	DECLARE_ENGINE_CLASS( CMeshComponent, CMeshTypeComponent, 0 )

public:
	// Allow a mesh to be rendered past its auto-hide distance by this much. This allows for fading the mesh out when it
	// reaches the auto-hide distance.
	static const Float AUTOHIDE_MARGIN;

public:
	CMeshComponent();
	virtual ~CMeshComponent();

	// Get attachment group for this component - it determines the order
	RED_FORCE_INLINE virtual EAttachmentGroup GetAttachGroup() const { return ATTACH_GROUP_A2; }

	// Setup everything like in other mesh component
	void SetAsCloneOf( const CMeshComponent* otherMeshComponent );

	//! Try to use given resource passed from editor during spawning (ex. mesh for StaticMesh template)
	virtual Bool TryUsingResource( CResource* resource );

	virtual CMeshTypeResource* GetMeshTypeResource() const override;

	virtual Uint32 GetOcclusionId() const override;

	// Override the internal mesh
	virtual void SetResource( CResource* resource ) override;
	virtual void GetResource( TDynArray< const CResource* >& resources ) const override;

	// Try to get the mesh (asynchronous load). Returns null if its not loaded (yet)
	CMesh* TryGetMesh() const;

	// Loads and returns the mesh synchronously.
	CMesh* GetMeshNow() const;

	// Get the resource path of the mesh. Does not load anything
	virtual const String GetMeshResourcePath() const;

	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	// Property has changed
	virtual void OnPropertyPostChange( IProperty* property );

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world
	virtual void OnDetached( CWorld* world );

#ifdef USE_UMBRA
	virtual Bool ShouldBeCookedAsOcclusionData() const;

	virtual Bool OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds );
#endif

	// Get sprite icon
	virtual CBitmapTexture* GetSpriteIcon() const;

	// Get sprite rendering color
	virtual Color CalcSpriteColor() const;

	// Get sprite rendering size
	virtual Float CalcSpriteSize() const;

#ifndef NO_DATA_VALIDATION
	// Check data
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif // NO_DATA_VALIDATION

#ifndef NO_RESOURCE_USAGE_INFO
	//! Resource usage reporting
	virtual void CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const override;
#endif

	// Overriding this to handle the type change for mesh
	virtual Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );

	// Added or removed because of appearance change
	virtual void OnAppearanceChanged( Bool added );

	//! Returns true if this component is mergable into world merged geometry
	virtual Bool IsMergableIntoWorldGeometry() const;

private:
	THandle< CMesh >	m_mesh;									//!< Mesh to draw
};

BEGIN_CLASS_RTTI( CMeshComponent );
	PARENT_CLASS( CMeshTypeComponent );
	PROPERTY_EDIT( m_mesh, TXT("Mesh") );
END_CLASS_RTTI();
