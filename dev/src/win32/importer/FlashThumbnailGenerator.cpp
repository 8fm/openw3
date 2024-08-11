/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "ThumbnailGeneratorHelpers.h"
#include "../../common/core/thumbnail.h"
#include "../../common/engine/swfResource.h"
#include "../../common/engine/swfTexture.h"

class CFlashThumbnailGenerator : public IThumbnailGenerator
{
	DECLARE_ENGINE_CLASS( CFlashThumbnailGenerator, IThumbnailGenerator, 0 );

public:
	CFlashThumbnailGenerator() {}

	// Create thumbnail from resource
	virtual TDynArray<CThumbnail*> Create( CResource* resource, const SThumbnailSettings& settings ) override;
};

DEFINE_SIMPLE_RTTI_CLASS(CFlashThumbnailGenerator, IThumbnailGenerator);
IMPLEMENT_ENGINE_CLASS(CFlashThumbnailGenerator);

TDynArray<CThumbnail*> CFlashThumbnailGenerator::Create( CResource* resource, const SThumbnailSettings& settings )
{
	TDynArray< CThumbnail* > res;

	CSwfResource* swfResource = Cast< CSwfResource >( resource );
	if ( ! swfResource )
	{
		return res;
	}

#if 0
	CSwfTexture* swfTexture = swfResource->GetThumbnailTextureSource();
	if ( ! swfTexture )
	{
		return res;
	}

	if ( swfTexture->GetMips().Empty() )
	{
		return res;
	}

	TDynArray< CClass* > classes;
	SRTTI::GetInstance().EnumClasses( ClassID< IThumbnailGenerator >(), classes );
	if ( !classes.Size() )
	{
		return res;
	}

	for ( Uint32 i=0; i<classes.Size(); i++ )
	{
		// Generate thumbnail using generator
		IThumbnailGenerator* generator = classes[i]->GetDefaultObject< IThumbnailGenerator >();
		if ( generator == this )
		{
			continue;
		}

		res = generator->Create( swfTexture, settings );

		// Valid thumbnail generated
		if ( res.Size() > 0 )
		{		
			break;
		}
	}
#endif

	return res;
}