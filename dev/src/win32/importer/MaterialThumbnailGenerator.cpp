/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "importerExternalResources.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"
#include "../../common/gpuApiUtils/gpuApiTypes.h"
#include "ThumbnailGeneratorHelpers.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/depot.h"
#include "../../common/core/thumbnail.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/entity.h"

// Simple thumbnail generator for CMaterial
class CMaterialTemplateThumbnailGenerator : public IThumbnailGenerator
{
	DECLARE_ENGINE_CLASS( CMaterialTemplateThumbnailGenerator, IThumbnailGenerator, 0 );

public:
	// Create thumbnail from resource
	virtual TDynArray<CThumbnail*> Create( CResource* resource, const SThumbnailSettings& settings ) override;
};

DEFINE_SIMPLE_RTTI_CLASS(CMaterialTemplateThumbnailGenerator,IThumbnailGenerator);
IMPLEMENT_ENGINE_CLASS(CMaterialTemplateThumbnailGenerator);

TDynArray<CThumbnail*> CMaterialTemplateThumbnailGenerator::Create( CResource* resource, const SThumbnailSettings& settings )
{
	// Is material?
	IMaterial* material = Cast< IMaterial >( resource );
	
	if ( !material )
	{
		return TDynArray<CThumbnail*>();
	}

	// Create fake world
	CWorld* fakeWorld = ThumbnailGeneratorHelpers::PrepareFakeWorld();

	TDynArray< CThumbnail* > res;

	// Create entity
	EntitySpawnInfo einfo;

	if ( CEntity* entity = fakeWorld->GetDynamicLayer()->CreateEntitySync( einfo ) )
	{
		// Create mesh component
		if ( CMeshComponent* component = SafeCast< CMeshComponent >( entity->CreateComponent( ClassID< CMeshComponent >(), SComponentSpawnInfo() ) ) )
		{
			// no fckin dissolves
			component->SetNoDisolves( true );
			component->SetResource( LoadResource< CMesh >( MESH_SPHERE ) );

			// HACK to substitute material on rendered crap
			extern IMaterial* GRenderProxyGlobalMaterial;
			GRenderProxyGlobalMaterial = material;
			component->OnRenderingResourceChanged();
		}

		// Update world to perform delayed tasks
		ThumbnailGeneratorHelpers::UpdateWorld( fakeWorld );

		// Determine camera position
		Box box = entity->CalcBoundingBox();
				
		if ( CThumbnail* thumb = ThumbnailGeneratorHelpers::CreateThumbnail( fakeWorld, box, 1.1f, settings ) )
		{
			res.PushBack( thumb );
		}
	}

	// Disable material substitution
	extern IMaterial* GRenderProxyGlobalMaterial;
	GRenderProxyGlobalMaterial = NULL;

	return res;
}
