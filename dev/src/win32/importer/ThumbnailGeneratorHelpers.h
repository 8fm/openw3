
#pragma once

struct SThumbnailSettings;

namespace ThumbnailGeneratorHelpers
{
	CWorld* PrepareFakeWorld();

	void SetUpGroundAndFoliage( CWorld* fakeWorld, const Box& box, const SThumbnailSettings& settings );

	void UpdateWorld( CWorld* fakeWorld );

	CThumbnail* CreateThumbnail( CWorld* fakeWorld, const Box& box, Float distance, const SThumbnailSettings& settings );
}


