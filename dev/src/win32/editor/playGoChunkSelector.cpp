
#include "build.h"
#include "playGoChunkSelector.h"

#include "editorExternalResources.h"
#include "../../common/core/2darray.h"
#include "../../common/core/depot.h"

CEdPlayGoChunkSelector::CEdPlayGoChunkSelector( CPropertyItem* item )
	: CEdMappedSelectionEditor( item )
	{}


void CEdPlayGoChunkSelector::FillMap( TDynArray< TPair< String, String > >& map )
{
	if ( THandle< C2dArray > list = LoadResource< C2dArray >( PLAYGO_CHUNKS_CSV ) )
	{
		for ( Uint32 i = 0; i < list->GetNumberOfRows(); ++i )
		{
			String chunkName = list->GetValue( TXT("ChunkName"), i );
			String displayedName = list->GetValue( TXT("DisplayedName"), i );
			map.PushBack( MakePair( chunkName, displayedName ) );
		}
	}
}
