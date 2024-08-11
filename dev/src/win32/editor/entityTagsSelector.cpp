#include "build.h"
#include "entityTagsSelector.h"

#include "sortNames.h"
#include "../../common/engine/worldIterators.h"


void CEntityTagsSelector::FillChoices()
{
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}
	
	THashSet< CName > tagSet;
	WorldAttachedEntitiesIterator it( world );
	// collect all tags using hashset (for worst case scenario)
	while ( it )
	{
		CEntity* entity = *it;
		if ( entity->IsA( m_entityClass ) )
		{
			const auto& tagList = entity->GetTags().GetTags();
			for ( auto it = tagList.Begin(), end = tagList.End(); it != end; ++it )
			{
				tagSet.Insert( *it );
			}
		}
		++it;
	}

	// now prepare tags for iteration and sort - storing them in new structure
	TDynArray< CName > tagList( tagSet.Size() );
	Uint32 i = 0;
	for ( auto it = tagSet.Begin(), end = tagSet.End(); it != end; ++it )
	{
		tagList[ i++ ] = *it;
	}

	::SortNames( tagList );

	for ( Uint32 i = 0, n = tagList.Size(); i < n; ++i )
	{
		m_ctrlChoice->AppendString( tagList[ i ].AsString().AsChar() );
	}

}
Bool CEntityTagsSelector::IsTextEditable() const
{
	return true;
}