
#pragma once

#include "animationCacheCooker.h"

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

#include "../../common/core/depot.h"

class AnimationCacheDepotSaver : public AnimationCacheCooker
{
public:
	void CollectAnimationsFromDepot()
	{
		TDynArray< CDiskFile* > files;
		GDepot->Search( TXT(".w2anims"), files );

		for ( Uint32 i=0; i<files.Size(); ++i )
		{
			CDiskFile* file = files[ i ];
			if ( file )
			{
				if ( !file->IsLoaded() )
				{
					ResourceLoadingContext context;
					file->Load( context );
				}

				CSkeletalAnimationSet* animset = Cast< CSkeletalAnimationSet >( file->GetResource() );
				if ( animset )
				{
					const TDynArray< CSkeletalAnimationSetEntry* >& animationEntries = animset->GetAnimations();
					for ( Uint32 j=0; j<animationEntries.Size(); ++j )
					{
						CSkeletalAnimationSetEntry* entry = animationEntries[ j ];
						CSkeletalAnimation* anim = entry ? entry->GetAnimation() : NULL;
						if ( anim )
						{
							//...
						}
					}
				}
			}
		}
	}
};

#endif
