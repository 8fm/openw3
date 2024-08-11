#include "build.h"
#include "2daTagListUpdater.h"
#include "tagTreeItemData.h"
#include "editorExternalResources.h"

#include "../../common/core/depot.h"

CEd2daTagListUpdater::CEd2daTagListUpdater( const String& filename, const String& columnName )
: m_groupName( columnName )
{
	C2dArray* valuesArray = Cast< C2dArray >( GDepot->LoadResource( filename ) );

	if ( valuesArray != NULL && valuesArray->Empty() == false )
	{
		Uint32 numberOfRows = valuesArray->GetNumberOfRows();

		Uint32 columnIndex = -1;
		valuesArray->FindHeader( columnName, columnIndex );

		if ( columnIndex != -1 )
		{
			for ( Uint32 i = 0; i < numberOfRows; ++i )
			{
				String rowValue = valuesArray->GetValue( columnIndex, i );
				rowValue.Trim();
				m_allowedTags.PushBackUnique( rowValue );
			}
		}
	}
}

Bool CEd2daTagListUpdater::IsTagAllowed( const String& tag )
{
	return m_allowedTags.Exist( tag );
}

Int32 CEd2daTagListUpdater::DoGetTags( STagNode &node, String& filter )
{
	Int32 addedTags = 0;
	filter.MakeLower();

	for ( TDynArray< String >::iterator iter = m_allowedTags.Begin(); iter != m_allowedTags.End(); ++iter )
	{
		if ( filter.Empty() || iter->ToLower().BeginsWith( filter ) )
		{
			++addedTags;
			node.AppendChild(*iter, 1, this);
		}
	}

	return addedTags;
}

CEdConversationTagListUpdater::CEdConversationTagListUpdater()
: CEd2daTagListUpdater( SCENE_SECTION_TAGS_CSV, TXT( "Tag" ) )
{
}
