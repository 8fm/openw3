/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"
#include "../../common/gpuApiUtils/gpuApiTypes.h"
#include "../../common/engine/apexClothResource.h"
#include "../../common/engine/apexDestructionResource.h"
#include "../editor/editorExternalResources.h"
#include "ThumbnailGeneratorHelpers.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/thumbnail.h"
#include "../../common/engine/clothComponent.h"
#include "../../common/engine/destructionSystemComponent.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/furMeshResource.h"
#include "../../common/engine/furComponent.h"
#include "../../common/engine/entity.h"

// Simple thumbnail generator for CMesh
class CMeshThumbnailGenerator : public IThumbnailGenerator
{
	DECLARE_ENGINE_CLASS( CMeshThumbnailGenerator, IThumbnailGenerator, 0 );

public:
	CMeshThumbnailGenerator() {}

	// Create thumbnail from resource
	virtual TDynArray<CThumbnail*> Create( CResource* resource, const SThumbnailSettings& settings ) override;

private:
	// Helper method
	template < typename T >
	T* CreateComponent( CEntity* entity )
	{
		return Cast< T >( entity->CreateComponent( ClassID< T >(), SComponentSpawnInfo() ) );
	}
};

DEFINE_SIMPLE_RTTI_CLASS(CMeshThumbnailGenerator,IThumbnailGenerator);
IMPLEMENT_ENGINE_CLASS(CMeshThumbnailGenerator);

TDynArray<CThumbnail*> CMeshThumbnailGenerator::Create( CResource* resource, const SThumbnailSettings& settings )
{
	// Only meshes templates
	if ( !resource->IsA< CMeshTypeResource >() )
	{
		return TDynArray< CThumbnail* >();
	}

	// Create fake world
	CWorld* fakeWorld = ThumbnailGeneratorHelpers::PrepareFakeWorld();

	// Create entity
	EntitySpawnInfo einfo;
	CEntity* entity = fakeWorld->GetDynamicLayer()->CreateEntitySync( einfo );
	ASSERT( entity );

	if ( CMesh* meshRes = Cast< CMesh >( resource ) )
	{
		if ( CMeshComponent* meshComp = CreateComponent< CMeshComponent >( entity ) )
		{
			const CMesh::TLODLevelArray& lods = meshRes->GetMeshLODLevels();
			if ( lods.Size() >= 2 && lods[0].m_chunks.Empty() )
			{ // support for proxies
				meshComp->ForceLODLevel( 1 );
			}

			meshComp->SetResource( meshRes );
			meshComp->SetNoDisolves( true );
			meshComp->SetCastingShadows( true );
		}
	}
	else if ( CApexClothResource* clothRes = Cast< CApexClothResource >( resource ) )
	{
		if ( CClothComponent* clothComp = CreateComponent< CClothComponent >( entity ) )
		{
			clothComp->SetResource( clothRes );
			clothComp->SetNoDisolves( true );
			clothComp->ForceLODLevel( -1 );
			clothComp->SetCastingShadows( true );
		}
	}
	else if ( CApexDestructionResource* destructionRes = Cast< CApexDestructionResource >( resource ) )
	{
#ifdef USE_APEX
		if ( CDestructionSystemComponent* destructionComp = CreateComponent< CDestructionSystemComponent >( entity ) )
		{
			destructionComp->SetResource( destructionRes );
			destructionComp->SetNoDisolves( true );
			destructionComp->SetCastingShadows( true );
		}
#endif
	}
	else if ( CFurMeshResource* furRes = Cast< CFurMeshResource >( resource ) )
	{
#ifdef USE_NVIDIA_FUR
		if ( CFurComponent* furComp = CreateComponent< CFurComponent >( entity ) )
		{
			furComp->SetResource( furRes );
			furComp->SetNoDisolves( true );
			furComp->ForceLODLevel( -1 );
			furComp->SetCastingShadows( true );
		}
#endif
	}
	else
	{
		HALT( "Unsupported resource type" )
		fakeWorld->Shutdown();
		return TDynArray< CThumbnail* >();
	}

	ThumbnailGeneratorHelpers::UpdateWorld( fakeWorld );
	Box box = entity->CalcBoundingBox();
	
	ThumbnailGeneratorHelpers::SetUpGroundAndFoliage( fakeWorld, box, settings );

	// Update world to perform delayed tasks
	ThumbnailGeneratorHelpers::UpdateWorld( fakeWorld );

	TDynArray< CThumbnail* > res;

	if ( CThumbnail* thumb = ThumbnailGeneratorHelpers::CreateThumbnail( fakeWorld, box, 1.7f, settings ) )
	{
		res.PushBack( thumb );
	}

	return res;

}
