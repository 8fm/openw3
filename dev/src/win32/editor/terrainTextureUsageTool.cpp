
#include "build.h"
#include "terrainTextureUsageTool.h"

#include "../../common/engine/clipMap.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/engine/material.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/textureArray.h"
#include "resourceIterator.h"

namespace
{
	void ExtractTexIndices( TControlMapType cmTexel, Int32& hTexIdx, Int32& vTexIdx )
	{
		hTexIdx = ( cmTexel & 31 ) - 1;
		vTexIdx = ( ( cmTexel >> 5 ) & 31 ) - 1;
		ASSERT ( hTexIdx >= -1 && hTexIdx < 32 && vTexIdx >= -1 && vTexIdx < 32 );
	}

	void SetTexIndices( TControlMapType& cmTexel, Int32 hTexIdx, Int32 vTexIdx )
	{
		ASSERT ( hTexIdx >= 0 && hTexIdx < 32 && vTexIdx >= 0 && vTexIdx < 32 );
		cmTexel &= ~1023; // clear previous values
		cmTexel |= hTexIdx + 1;
		cmTexel |= ( vTexIdx + 1 ) << 5;
	}

	String GetUsageString( Uint32 number, Uint32 total )
	{
		if ( number == 0 )
		{
			return TXT("not used");
		}

		Float percentage = 100.0f * number / total;

		if ( percentage < 0.01f )
		{
			return TXT("< 0.01%");
		}

		return String::Printf( TXT("%.2f%%"), percentage );
	}
}

CEdTerrainTextureUsageTool::CEdTerrainTextureUsageTool( wxWindow* parent, CClipMap* terrain )
	: m_parent( parent )
	, m_terrain( terrain )
{
}

CEdTerrainTextureUsageTool::~CEdTerrainTextureUsageTool()
{
}

Bool CEdTerrainTextureUsageTool::Execute()
{
	if ( IMaterial* material = m_terrain->GetMaterial() )
	{
		if ( !material->ReadParameter( RED_NAME( diffuse ), m_textureArray ) )
		{
			return false;
		}

		if ( !material->ReadParameter( RED_NAME( normal ), m_textureArrayNormals ) )
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	CEdRemappingDialog::Mappings mappings = GatherUsageData();

	Bool sthChanged = false;

	CEdRemappingDialog dlg( m_parent, TXT("Terrain materials re-mapper") );
	dlg.SetupSpecialActionButton( TXT("Remove unused"), [this, &sthChanged]( CEdRemappingDialog* dlg ) 
	{	
		if ( RemoveUnusedMaterials() )
		{
			sthChanged = true;
		}
		dlg->ResetMappings( GatherUsageData() );
	});

	if ( dlg.Execute( mappings ) )
	{
		// will return 'true' only sth has changed, otherwise 'ok' acts like 'cancel' and returns 'false'
		if ( RemapMaterials( mappings ) )
		{
			sthChanged = true;
		}
	}

	return sthChanged;
}


CEdRemappingDialog::Mappings CEdTerrainTextureUsageTool::GatherUsageData()
{
	Red::MemoryZero( m_histogram, sizeof( m_histogram ) );

	Uint32 numTiles = m_terrain->GetNumTilesPerEdge();
	Uint32 totalTexels = 0;

	GFeedback->BeginTask( TXT("Gathering texture usage data"), false );

	Int32 tileIdx = 0;
	for ( Uint32 tileY = 0; tileY < numTiles; ++tileY )
	{
		for ( Uint32 tileX = 0; tileX < numTiles; ++tileX )
		{
			CTerrainTile* tile = m_terrain->GetTile( tileX, tileY );
			const TControlMapType* cm = tile->GetLevelSyncCM( 0 );

			for ( Uint32 idx = 0; idx < tile->GetResolution()*tile->GetResolution(); ++idx )
			{
				Int32 hTexIdx, vTexIdx;
				ExtractTexIndices( cm[idx], hTexIdx, vTexIdx );

				if ( hTexIdx >= 0 )
				{
					++m_histogram[ hTexIdx ];
				}

				if ( vTexIdx >= 0 )
				{
					++m_histogram[ vTexIdx ];
				}

				++totalTexels;
			}

			GFeedback->UpdateTaskProgress( ++tileIdx, numTiles*numTiles );
		}
	}

	GFeedback->EndTask();

	TDynArray< CName > textureNames;
	m_textureArray->GetTextureNames( textureNames );

	m_sortedTexureInfo.ClearFast();
	for ( Uint32 i = 0; i < Min( textureNames.Size(), NUM_OF_MAT_SLOTS ); ++i )
	{
		m_sortedTexureInfo.PushBack( TextureInfo( i, textureNames[i], m_histogram[i] ) );
	}

	// sort textures by usage

	Sort( m_sortedTexureInfo.Begin(), m_sortedTexureInfo.End(), 
		[]( const TextureInfo& a, const TextureInfo& b ){
			return a.usage > b.usage;
		});

	TDynArray< String > possibilities;
	TDynArray< String > tooltips;
	for ( const TextureInfo& info : m_sortedTexureInfo )
	{
		String path  = GetPathWithLastDirOnly( info.name.AsString() );
		String usage = GetUsageString( info.usage, totalTexels*2/*histogram accumulates for both horz & vert*/ );
		possibilities.PushBack( String::Printf( TXT("%2i - %s [%s]"), info.originalIndex, path.AsChar(), usage.AsChar() ) );
		tooltips.PushBack( String::Printf( TXT("Texels covered (horz + vert): %i"), info.usage ) );
	}

	// create mappings for dialog

	CEdRemappingDialog::Mappings mappings;
	for ( Uint32 i = 0; i < possibilities.Size(); ++i )
	{
		mappings.PushBack( CEdRemappingDialog::MappingEntry( possibilities[i], possibilities, -1, tooltips[i] ) );
	}

	return mappings;
}

template < typename Func >
void CEdTerrainTextureUsageTool::ProcessTerrainControlMap( Func func )
{
	Uint32 numTiles = m_terrain->GetNumTilesPerEdge();

	// gather all tile files

	TDynArray< CDiskFile* > tileFiles;
	for ( Uint32 tileY = 0; tileY < numTiles; ++tileY )
	{
		for ( Uint32 tileX = 0; tileX < numTiles; ++tileX )
		{
			tileFiles.PushBack( m_terrain->GetTile( tileX, tileY )->GetFile() );
		}
	}

	// do the remapping tile-by tile (CResourceIterator will do a proper check-outs and saves for us)


	Uint32 tilesProcessed = 0;
	for ( CResourceIterator< CTerrainTile > tile( tileFiles, TXT("Processing terrain control map") ); tile; ++tile )
	{
		TControlMapType* cm = tile->GetLevelWriteSyncCM( 0 );

		Bool sthChangedOnTile = false;
		for ( Uint32 idx = 0; idx < tile->GetResolution()*tile->GetResolution(); ++idx )
		{
			Int32 hTexIdx, vTexIdx;
			ExtractTexIndices( cm[idx], hTexIdx, vTexIdx );

			if ( func( hTexIdx, vTexIdx ) )
			{
				SetTexIndices( cm[idx], hTexIdx, vTexIdx );
				sthChangedOnTile = true;
			}
		}

		if ( sthChangedOnTile )
		{
			tile->SetDirty( true );
			tile->RebuildMipmaps();
		}

		++tilesProcessed;
	}
}

Bool CEdTerrainTextureUsageTool::RemapMaterials( const CEdRemappingDialog::Mappings& mappings )
{
	Uint32 texelsChanged = 0;

	for ( Uint32 i = 0; i < mappings.Size(); ++i )
	{
		if ( mappings[i].m_selectedIdx != -1 )
		{
			Uint32 origIdx = m_sortedTexureInfo[ i ].originalIndex;
			Uint32 newIdx  = m_sortedTexureInfo[ mappings[i].m_selectedIdx ].originalIndex;

			ProcessTerrainControlMap( 
				[ origIdx, newIdx, &texelsChanged ]( Int32& hTexIdx, Int32& vTexIdx ) -> Bool
				{
					if ( hTexIdx == origIdx || vTexIdx == origIdx )
					{
						if ( hTexIdx == origIdx )
						{
							hTexIdx = newIdx;
							++texelsChanged;
						}

						if ( vTexIdx == origIdx )
						{
							vTexIdx = newIdx;
							++texelsChanged;
						}

						return true;
					}

					return false;
				} );
		}
	}

	return texelsChanged != 0;
}

Bool CEdTerrainTextureUsageTool::RemoveUnusedMaterials()
{
	Int32 idxShift[ NUM_OF_MAT_SLOTS ];
	Red::MemoryZero( idxShift, sizeof( idxShift ) );

	Uint32 texturesRemoved = 0;
	for ( Int32 i = Min( m_textureArray->GetTextureCount(), NUM_OF_MAT_SLOTS )-1; i>=0; --i )
	{
		if ( m_histogram[i] == 0 )
		{
			if ( !m_textureArray->MarkModified() || !m_textureArrayNormals->MarkModified() )
			{
				return false;
			}

			m_textureArray->RemoveTextureAt( i );
			m_textureArrayNormals->RemoveTextureAt( i );
			++texturesRemoved;

			for ( Int32 j=i+1; j<NUM_OF_MAT_SLOTS; ++j )
			{
				--idxShift[j];
			}
		}
	}

	if ( texturesRemoved == 0 )
	{
		GFeedback->ShowMsg( TXT("Remove unused materials"), TXT("No materials removed") );
	}
	else
	{
		m_textureArray->Save();

		// fix indices
		ProcessTerrainControlMap( 
			[ &idxShift ]( Int32& hTexIdx, Int32& vTexIdx ) -> Bool
			{
				if ( hTexIdx >= 0 && vTexIdx >= 0 )
				{
					if ( idxShift[ hTexIdx ] != 0 || idxShift[ vTexIdx ] != 0 )
					{
						hTexIdx += idxShift[ hTexIdx ];
						vTexIdx += idxShift[ vTexIdx ];
						return true;
					}
				}

				return false;
			} );

		GFeedback->ShowMsg( TXT("Remove unused materials"), String::Printf( TXT("Removed %i materials"), texturesRemoved ).AsChar() );
	}

	return texturesRemoved != 0;
}