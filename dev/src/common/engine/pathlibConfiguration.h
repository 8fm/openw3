/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibWorldLayersMapping.h"

class CSimpleBufferWriter;
class CSimpleBufferReader;
class CDiskFile;

namespace PathLib
{
class CAreaDescription;
class CTerrainInfo;
class CWorldLayersMapping;

class CPathLibConfiguration : public Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_PathLib );

	friend class CPathLibConfigurationFactory;

protected:
	TDynArray< CAreaDescription* >								m_areas;
	CWorldLayersMapping											m_worldLayers;
	Uint32														m_terrainInfoHash;

	void														WriteToBuffer( CSimpleBufferWriter& writer );
	Bool														ReadFromBuffer( CSimpleBufferReader& reader );

	static void													GetFileName( String& fileName );
	static CDiskFile*											GetFile( CPathLibWorld& pathlib, Bool createIfMissing );

public:
	static const Uint16 RES_VERSION								= 4;

	CPathLibConfiguration();
	~CPathLibConfiguration();

	// saving interface
	Bool														AddArea( CAreaDescription* area );
	void														Reserve( Uint32 areasCount )					{ m_areas.Reserve( areasCount ); }
	void														SetTerrainInfo( const CTerrainInfo& terrainInfo );
	void														SetWorldLayersInfo( const CWorldLayersMapping& worldLayers );
	void														Clear();

	// loading interface
	Bool														ValidateTerrainInfo( const CTerrainInfo& terrainInfo );
	const TDynArray< CAreaDescription* >&						GetAreas() const								{ return m_areas; }
	CWorldLayersMapping&&										GetWorldLayers()								{ return Move( m_worldLayers ); }
	void														PostLoad()										{ Clear(); }

	static const Char*											GetFileExtension()								{ return TXT("navconfig"); }

	Bool														Save( CPathLibWorld& pathlib );
	static CPathLibConfiguration*								Load( CPathLibWorld& pathlib );
};



};			// namespace PathLib
