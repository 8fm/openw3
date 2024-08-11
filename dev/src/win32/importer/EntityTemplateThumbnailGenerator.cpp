/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"
#include "../../common/gpuApiUtils/gpuApiTypes.h"
#include "../../common/engine/appearanceComponent.h"
#include "../editor/editorExternalResources.h"
#include "ThumbnailGeneratorHelpers.h"
#include "../../common/core/thumbnail.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/componentIterator.h"
#include "../../common/engine/entity.h"

// Simple thumbnail generator for CEntityTemplate
class CEntityTemplateThumbnailGenerator : public IThumbnailGenerator
{
	DECLARE_ENGINE_CLASS( CEntityTemplateThumbnailGenerator, IThumbnailGenerator, 0 );

public:
	// Create thumbnail from resource
	virtual TDynArray<CThumbnail*> Create( CResource* resource, const SThumbnailSettings& settings ) override;

private:
	CThumbnail* GenerateEntityThumbnail( CWorld* fakeWorld, CEntity* entity, const SThumbnailSettings& settings );
};

DEFINE_SIMPLE_RTTI_CLASS(CEntityTemplateThumbnailGenerator,IThumbnailGenerator);
IMPLEMENT_ENGINE_CLASS(CEntityTemplateThumbnailGenerator);

CThumbnail* CEntityTemplateThumbnailGenerator::GenerateEntityThumbnail( CWorld* fakeWorld, CEntity* entity, const SThumbnailSettings& settings )
{
	// No dissolves for all drawable components
	for ( ComponentIterator< CDrawableComponent > it( entity ); it; ++it )
	{
		(*it)->SetNoDisolves( true );
	}

	// Determine camera position
	Box box = entity->CalcBoundingBox();

	ThumbnailGeneratorHelpers::SetUpGroundAndFoliage( fakeWorld, box, settings );

	// Update world to perform delayed tasks
	ThumbnailGeneratorHelpers::UpdateWorld( fakeWorld );

	return ThumbnailGeneratorHelpers::CreateThumbnail( fakeWorld, box, 1.7f, settings );
}

TDynArray<CThumbnail*> CEntityTemplateThumbnailGenerator::Create( CResource* resource, const SThumbnailSettings& settings )
{
	// Only entity templates
	if ( !resource->IsA< CEntityTemplate >() )
	{
		return TDynArray<CThumbnail*>();
	}

	TDynArray<CThumbnail*> res;

	// Create fake world
	CWorld* fakeWorld = ThumbnailGeneratorHelpers::PrepareFakeWorld();
	
	// Create entity
	EntitySpawnInfo einfo;
	einfo.m_template = SafeCast< CEntityTemplate >( resource );
	einfo.m_name = TXT("ThumbnailEntity");
	einfo.m_detachTemplate = false;
	einfo.m_previewOnly = true;

	if ( CEntity* entity = fakeWorld->GetDynamicLayer()->CreateEntitySync( einfo ) )
	{
		fakeWorld->DelayedActions();

		entity->CreateStreamedComponents( SWN_NotifyWorld );
		entity->ForceFinishAsyncResourceLoads();

		// Get all appearances
		TDynArray< const CEntityAppearance* > allAppearances;	
		einfo.m_template->GetAllEnabledAppearances( allAppearances );

		CAppearanceComponent* appearance = CAppearanceComponent::GetAppearanceComponent( entity );

		if ( appearance && allAppearances.Size() > 0 )
		{
			TDynArray< const CEntityAppearance* >::const_iterator appCurr = allAppearances.Begin(),
				appLast = allAppearances.End();
			for ( ; appCurr != appLast; ++appCurr )
			{
				appearance->ApplyAppearance( (*appCurr)->GetName() );

				if ( CThumbnail *thumb = GenerateEntityThumbnail( fakeWorld, entity, settings ) )
				{
					thumb->SetName( (*appCurr)->GetName().AsString() );
					res.PushBack( thumb );
				}
			}
		}
		else
		{
			if ( CThumbnail *thumb = GenerateEntityThumbnail( fakeWorld, entity, settings ) )
			{
				res.PushBack( thumb );
			}
		}
	}

	return res;	
}
