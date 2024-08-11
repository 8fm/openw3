#include "build.h"
#include "behTreeWorkInitializer.h"

#include "behTreeInstance.h"
#include "behTreeWorkData.h"

void CBehTreeMetanodeWorkInitializer::RunOnSpawn( CBehTreeSpawnContext& context, CBehTreeInstance* owner ) const
{
	CAIStorageItem* item = owner->GetItem( CBehTreeWorkData::GetStorageName() );
	CBehTreeWorkData* data = item ? item->GetPtr< CBehTreeWorkData >() : nullptr;
	if ( item )
	{
		// Its even more temporary and hacky as it was at E3, but still we have no other solution at hand
		data->SpawnToWork( owner->GetLocalTime() + 1.f );
	}
}