#include "build.h"
#include "appearanceTagListUpdater.h"
#include "tagTreeItemData.h"

CEdAppearanceTagListUpdater::CEdAppearanceTagListUpdater( CEntityTemplate *eTemplate )
{
	m_allowedTags = eTemplate->GetEnabledAppearancesNames();
}

Bool CEdAppearanceTagListUpdater::IsTagAllowed( const String& tag )
{
	return m_allowedTags.Exist( CName(tag) );
}

Int32 CEdAppearanceTagListUpdater::DoGetTags( STagNode &node, String& filter )
{
	Uint32 added = 0;

	for ( TDynArray< CName >::iterator it = m_allowedTags.Begin(); it != m_allowedTags.End(); ++it )
	{
		if ( filter.Empty() || it->AsString().BeginsWith( filter ) )
		{
			++added;
			node.AppendChild(it->AsString(), 1);
		}
	}

	return added;
}
