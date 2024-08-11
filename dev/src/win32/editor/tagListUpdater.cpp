#include "build.h"
#include "tagListUpdater.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layerGroup.h"

STagNode::STagNode(String name, Uint32 count /*= 0*/, const CTagListProvider *provider /*= NULL*/)
: m_name(name)
, m_count(count)
, m_provider(provider)
{ }

void CTagListProvider::GetTags( STagNode &root, String& filter )
{
	STagNode &node = root.AppendChild(GetTagGroupName(), 0, this);
	DoGetTags(node, filter);
}

void CHistoryTagListProvider::SaveSession()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	
	String sTags;
	TSet<String>::iterator	currTag = m_historyTags.Begin(),
							lastTag = m_historyTags.End();
	for( ; currTag != lastTag; ++currTag )
		sTags += *currTag + TXT(";");
	config.Write( TXT("History"), sTags );
}

void CHistoryTagListProvider::LoadSession()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	String sTags;
	if ( ! config.Read( TXT("History"), &sTags ) )
		return;

	TDynArray<String> tags;
	String(sTags).Slice(tags, TXT(";"));

	TDynArray<String>::iterator	currTag = tags.Begin(),
								lastTag = tags.End();
	for( ; currTag != lastTag; ++currTag )
	{
		(*currTag).Trim();
		if ( ! currTag->Empty() )
			m_historyTags.Insert( *currTag );
	}
}

Int32 CHistoryTagListProvider::DoGetTags( STagNode &node, String& filter )
{
	Int32 addedTags = 0;
	for ( TSet< String >::iterator it = m_historyTags.Begin(); it != m_historyTags.End(); ++it )
	{
		if ( filter.Empty() || it->BeginsWith( filter ) )
		{
			++addedTags;
			node.AppendChild(it->AsChar(), 1, this);
		}
	}

	return addedTags;
}

Int32 CWorldTagListProvider::DoGetTags( STagNode &node, String& filter )
{
	Int32 addedTags = 0;
	CWorld* world = GGame->GetActiveWorld();
	if ( world && world->GetWorldLayers() )
	{
		// Get list of layers from active world
		TDynArray< CLayerInfo* > layers;
		world->GetWorldLayers()->GetLayers( layers, false );

		// Create layer node for each loaded layer
		for ( Uint32 k = 0; k < layers.Size(); k++ )
		{
			CLayerInfo* info = layers[k];
			if ( info->IsLoaded() )
			{
				CLayer* trueLayer = info->GetLayer();

				// Collect tags and count them
				THashMap< CName, Uint32 > names;
				
				TDynArray<CEntity*> entities;
				trueLayer->GetEntities( entities );
				for ( Uint32 i = 0; i < entities.Size(); ++i )
				{
					CEntity *entity = entities[i];
					
					// Collect tags
					const TDynArray< CName >& localNames = entity->GetTags().GetTags();
					for ( Uint32 j=0; j<localNames.Size(); j++ )
					{
						Uint32 count = 0;
						CName name = localNames[ j ];
						if ( filter.Empty() || name.AsString().BeginsWith( filter ) )
						{
							names.Find( name, count );
							names[ name ] = count + 1;
						}
					}
				}

				// Add layer item only if there are some tags inside it
				if ( names.Size() )
				{
					String caption = String::Printf( TXT("Layer '%ls'"), info->GetShortName().AsChar() );
					STagNode &layerNode = node.AppendChild(caption, 0, this);

					// Fill list
					for ( THashMap< CName, Uint32 >::const_iterator it = names.Begin(); it != names.End(); ++it )
					{
						++addedTags;
						layerNode.AppendChild(it->m_first.AsString(), it->m_second, this);
					}
				}
			}
		}
	}

	return addedTags;
}