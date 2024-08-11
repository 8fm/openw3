/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/gpuApiUtils/gpuApiMemory.h"
#include "../../common/gpuApiUtils/gpuApiTypes.h"
#include "../../common/engine/foliageEditionController.h"
#include "ThumbnailGeneratorHelpers.h"
#include "../../common/core/thumbnail.h"
#include "../../common/engine/baseTree.h"

extern CGatheredResource resDefaultMesh;

// Simple thumbnail generator for CSRTBaseTree
class CSpeedTreeThumbnailGenerator : public IThumbnailGenerator
{
	DECLARE_ENGINE_CLASS( CSpeedTreeThumbnailGenerator, IThumbnailGenerator, 0 );

public:
	CSpeedTreeThumbnailGenerator() {}

	// Create thumbnail from resource
	virtual TDynArray<CThumbnail*> Create( CResource* resource, const SThumbnailSettings& settings ) override;
};

DEFINE_SIMPLE_RTTI_CLASS(CSpeedTreeThumbnailGenerator,IThumbnailGenerator);
IMPLEMENT_ENGINE_CLASS(CSpeedTreeThumbnailGenerator);

TDynArray<CThumbnail*> CSpeedTreeThumbnailGenerator::Create( CResource* resource, const SThumbnailSettings& settings )
{
	TDynArray< CThumbnail* > res;

#ifndef NO_EDITOR
	// Only speed tree resources
	if ( !resource->IsA< CSRTBaseTree >() )
	{
		return TDynArray<CThumbnail*>();
	}

	// Grab resource by proper type
	CSRTBaseTree* baseTree =  Cast< CSRTBaseTree >( resource );

	// Create fake world
	CWorld* fakeWorld = ThumbnailGeneratorHelpers::PrepareFakeWorld();

	// Create an array of one instance :)
	FoliageInstanceContainer instanceInfos;
	new ( instanceInfos ) SFoliageInstance();
	SFoliageInstance& iInfo = instanceInfos.Back();

	// Initialize parameters
	iInfo.SetPosition( Vector::ZEROS );
	iInfo.SetQuaternion( EulerAngles( 0.f, 0.f, 0.f ).ToQuat() );
	iInfo.SetScale( 1.0f );

	//const Vector2 position = Vector2( 0.f, 0.f );
	//const Vector2 cellDimension = Vector2( 64.0f, 64.0f );
	//const Box box = Box( position , position + cellDimension );

	fakeWorld->GetFoliageEditionController().AddPreviewInstances( baseTree, instanceInfos, baseTree->GetBBox() );

	// Update world to perform delayed tasks
	ThumbnailGeneratorHelpers::UpdateWorld( fakeWorld );

	// Determine camera position
	Box box = baseTree->GetBBox();

	if ( CThumbnail* thumb = ThumbnailGeneratorHelpers::CreateThumbnail( fakeWorld, box, 1.0f, settings ) )
	{
		res.PushBack( thumb );
	}

	// clean the world
	fakeWorld->GetFoliageEditionController().RemovePreviewInstances( baseTree, baseTree->GetBBox() );
	ThumbnailGeneratorHelpers::UpdateWorld( fakeWorld );
#endif

	return res;
}
