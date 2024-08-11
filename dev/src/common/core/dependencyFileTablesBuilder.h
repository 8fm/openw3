#pragma once

#include "hashmap.h"

/// Helper class that can be used to build the content of the 
class CDependencyFileDataBuilder
{
public:
	CDependencyFileDataBuilder( class CDependencyFileData& outData );
	~CDependencyFileDataBuilder();

	struct ImportInfo
	{
		StringAnsi		m_path;
		CName			m_className;
		Bool			m_isObligatory:1;
		Bool			m_isTemplate:1;
		Bool			m_isSoft:1;

		RED_FORCE_INLINE ImportInfo()
			: m_isObligatory(0)
			, m_isTemplate(0)
			, m_isSoft(0)
		{}
	};

	struct ExportInfo
	{
		CName		m_className;
		Uint16		m_objectFlags;
		Uint32		m_parent;
		Int32		m_template;

		RED_FORCE_INLINE ExportInfo()
			: m_objectFlags(0)
			, m_parent(0)
			, m_template(0)
		{}
	};

	struct BufferInfo
	{
		Uint32		m_dataSizeInMemory;

		RED_FORCE_INLINE BufferInfo()
			: m_dataSizeInMemory( 0 )
		{}
	};

	// add ANSI string, returns string index
	Uint32 MapString( const StringAnsi& string );

	// add mapped name, returns name index
	Uint16 MapName( const CName name );

	// add mapped property, returns property index
	Uint16 MapProperty( const CProperty* prop );

	// add import
	Uint32 MapImport( const ImportInfo& importInfo, const Bool hashPath );

	// add export
	Uint32 MapExport( const ExportInfo& exportInfo );

	// add buffer data
	Uint32 MapBuffer( const BufferInfo& bufferInfo );

	// patch export with data offset and size
	void PatchExport( const Uint32 exportIndex, const Uint32 dataOffset, const Uint32 dataSize, const Uint32 dataCRC );

	// patch buffer with data offset and size
	void PatchBuffer( const Uint32 bufferIndex, const Uint32 dataOffset, const Uint32 dataSizeOnDisk, const Uint32 bufferCRC );

private:
	class CDependencyFileData*		m_data;

	typedef THashMap< StringAnsi, Uint32 >			TStringMap;
	typedef THashMap< CName, Uint16 >				TNameMap;
	typedef THashMap< const CProperty*, Uint16 >	TPropertiesMap;

	TStringMap		m_stringMap;
	TNameMap		m_nameMap;
	TPropertiesMap	m_propertyMap;
};