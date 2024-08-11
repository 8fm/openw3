
#pragma once

#include "remappingDialog.h"

class CEdTerrainTextureUsageTool
{
public:
	CEdTerrainTextureUsageTool( wxWindow* parent, CClipMap* terrain );

	~CEdTerrainTextureUsageTool();

	Bool Execute();

private:

	static const Uint32 NUM_OF_MAT_SLOTS = 32;

	CEdRemappingDialog::Mappings GatherUsageData();
	template < typename Func > void ProcessTerrainControlMap( Func func );
	Bool RemapMaterials( const CEdRemappingDialog::Mappings& mappings );
	Bool RemoveUnusedMaterials();

	wxWindow* m_parent;
	CClipMap* m_terrain;
	THandle< CTextureArray > m_textureArray;
	THandle< CTextureArray > m_textureArrayNormals;

	Uint32 m_histogram[NUM_OF_MAT_SLOTS];

	struct TextureInfo
	{
		TextureInfo( Uint32 index, CName name, Uint32 usage ) 
			: originalIndex( index ), name( name ), usage( usage ) {}

		Uint32 originalIndex;
		CName  name;
		Uint32 usage;
	};

	TDynArray< TextureInfo > m_sortedTexureInfo;

};
