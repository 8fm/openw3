/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC( 4512 )
// adapter for converting IFile to hkStreamWriter
class CIFileHavokAdapter : public hkStreamWriter 
{
	IFile&				m_file;
	Uint32				m_offset;	
public:
	CIFileHavokAdapter( IFile &file )
		: m_file( file )
		, m_offset( static_cast< Uint32 >( file.GetOffset() ) )
	{
	}

	hkBool isOk() const
	{
		return true;
	}

	int write(const void *buf, int nbytes)
	{
		m_file.Serialize( const_cast<void*>( buf ), nbytes );
		m_offset += nbytes;
		return nbytes;
	}

	void flush()
	{
		// ..
	}

	hkBool seekTellSupported() const
	{
		return true;
	}

	hkResult seek( int offset, hkStreamWriter::SeekWhence whence )
	{
		if ( whence == hkStreamWriter::STREAM_SET )
		{
			m_file.Seek( offset );
			m_offset = offset;
			return HK_SUCCESS;
		}
		else if ( whence == hkStreamWriter::STREAM_CUR )
		{
			m_file.Seek( offset + m_file.GetOffset() );
			m_offset += offset;
			return HK_SUCCESS;
		} 
		else if ( whence == hkStreamWriter::STREAM_END )
		{
			return HK_FAILURE;
		}

		return HK_FAILURE;
	}

	int tell() const
	{
		return m_offset;
	}
};

// havok stream writer writing to TDynArray
class CDynArrayHavokStreamWriter : public hkStreamWriter
{
	TDynArray< Uint8 >		&m_array;
	Uint32					m_startOffset;
	Uint32					m_offset;

public:
	CDynArrayHavokStreamWriter( TDynArray< Uint8 > &arr )
		: m_array( arr )
		, m_offset( arr.Size() )
		, m_startOffset( arr.Size() )
	{
	}

	hkBool isOk() const
	{
		return true;
	}

	int write( const void *buf, int nbytes )
	{
		Int32 growSize = nbytes - (m_array.Size() - m_offset);
		if ( growSize > 0 )
		{
			m_array.CBaseArray::GrowBuffer( MC_DynArray, growSize, sizeof( Uint8 ) );
		}

		Red::System::MemoryCopy( m_array.TypedData() + m_offset, buf, nbytes);

		m_offset += nbytes;

		return nbytes;
	}

	void flush()
	{
		// ..
	}

	hkBool seekTellSupported() const
	{
		return true;
	}

	hkResult seek( int offset, hkStreamWriter::SeekWhence whence )
	{
		if ( whence == hkStreamWriter::STREAM_SET )
		{
			m_offset = offset + m_startOffset;
			return HK_SUCCESS;
		}
		else if ( whence == hkStreamWriter::STREAM_CUR )
		{
			m_offset += offset;
			return HK_SUCCESS;
		} 
		else if ( whence == hkStreamWriter::STREAM_END )
		{
			m_offset = m_array.Size() - offset;
			return HK_SUCCESS;
		}

		return HK_FAILURE;
	}

	int tell() const
	{
		return m_offset - m_startOffset;
	}
};

#if 0

template< class _HavokObject >
TMemSize GetHavokObjectMemSize( const _HavokObject* obj )
{
	// todo: implement explicit specializations
	return 0;
}

template< class _HavokObject >
class CHavokResource
{
public:
	CHavokResource();
	CHavokResource( const hkClass *havokClass );

public:
	void Clear();

	void CopyContainer( const _HavokObject *containerToCopy, Bool writeMetaInfo = true );
	
	_HavokObject* GetRootObject();
	const _HavokObject* GetRootObject() const;
	void SetHavokClass(const hkClass *klass) { m_havokClass = klass; }

	TMemSize GetInternalMemSize() const { return ( m_rawData.GetData() ? m_rawData.GetSize() : 0 ) + ( m_rootObject ? GetHavokObjectMemSize<_HavokObject> ( m_rootObject ) : 0 ); }

protected:	
	CDataBuffer				m_rawData;
	_HavokObject			*m_rootObject;	
	const hkClass			*m_havokClass;

	template< class _HavokObject2 >
	friend IFile& operator<<( IFile& file, CHavokResource< _HavokObject2 > &resource );
};


///////////////////////////////////////////////////////////////////////////////////
// CHavokResource implementation


//////////////////////////////////////////////////////////////////////

template< class _HavokObject >
CHavokResource< _HavokObject >::CHavokResource()
	: m_havokClass( NULL )
{
}

template< class _HavokObject >
CHavokResource< _HavokObject >::CHavokResource( const hkClass *havokClass )
	: m_havokClass( havokClass )
{
}

template< class _HavokObject >
void CHavokResource< _HavokObject >::Clear()
{
	m_rawData.Clear();
	m_rootObject = NULL;
}

template< class _HavokObject >
void CHavokResource< _HavokObject >::CopyContainer( const _HavokObject *containerToCopy, Bool writeMetaInfo )
{
	hkBinaryPackfileWriter writer;
	hkPackfileWriter::Options options;
	options.m_writeMetaInfo = writeMetaInfo;


	// Serialize source data
	TDynArray< Uint8 > data;
	CDynArrayHavokStreamWriter arrayStreamWriter( data );
	writer.setContents( containerToCopy, *m_havokClass );
	writer.save( &arrayStreamWriter, options );

	// Copy to aligned internal data buffer
	m_rawData = data;

	// deserialize into local buffer
	hkBinaryPackfileReader reader;
	reader.loadEntireFileInplace( m_rawData.GetData(), m_rawData.GetSize() );

	void* dataRead = reader.getContentsWithRegistry( m_havokClass->getName(), 
													 hkBuiltinTypeRegistry::getInstance().getLoadedObjectRegistry() );

	reader.getAllocatedData()->disableDestructors();

	ASSERT( dataRead );

	m_rootObject = static_cast<_HavokObject*>( dataRead );
}

template< class _HavokObject >
IFile& operator<<( IFile& file, CHavokResource< _HavokObject > &res )
{
	if ( file.IsGarbageCollector() )
	{
		return file;
	}

	if ( file.IsReader() )
	{
		// Load data buffer
		file << res.m_rawData;

		void* dataRead = NULL;
		const char *className = "";

		if ( !res.m_rawData.IsEmpty() )
		{
			// Deserialize in place
			hkBinaryPackfileReader reader;
			reader.loadEntireFileInplace( res.m_rawData.GetData(), res.m_rawData.GetSize() );
	
			if (reader.isVersionUpToDate())
			{
				className = reader.getContentsClassName();
				dataRead = reader.getContentsWithRegistry( className, hkBuiltinTypeRegistry::getInstance().getLoadedObjectRegistry() ); 
				reader.getAllocatedData()->disableDestructors();
			}

			if ( !dataRead )
			{
				WARN_ENGINE( TXT("Couldn't load havok data from file '%ls'! (expected class: '%ls')"), file.GetFileNameForDebug(), ANSI_TO_UNICODE(className[0] == 0 ? res.m_havokClass->getName() : className) );
			}
		}

		res.m_rootObject = static_cast<_HavokObject*>( dataRead );
		if (className[0])
			res.m_havokClass = hkBuiltinTypeRegistry::getInstance().getClassNameRegistry()->getClassByName(className);
	}
	else if ( file.IsWriter() )
	{		
		Uint32 sizeOffset = file.GetOffset();
		Uint32 dataSize = 0;
		file << dataSize;

		Uint32 dataStart = file.GetOffset();

		hkBinaryPackfileWriter writer;
		hkPackfileWriter::Options options;

#if _SHIT
		// for now
#ifdef W2_PLATFORM_WIN32
		if ( file.IsByteSwapping() )
#endif
			options.m_layout = hkStructureLayout::Xbox360LayoutRules;
#endif		
		
		if( GIsCooker )
		{
			CBaseCookingEngine* engine = static_cast<CBaseCookingEngine*>( GEngine );
			if( PLATFORM_Xenon == engine->GetCookingPlatform() )
				options.m_layout = hkStructureLayout::Xbox360LayoutRules;
			else if( PLATFORM_PS3 == engine->GetCookingPlatform() )
				options.m_layout = hkStructureLayout::GccPs3LayoutRules;
		}

		CIFileHavokAdapter adapter( file );

		if ( res.m_rootObject )
		{
			writer.setContents( res.m_rootObject, *res.m_havokClass );
			writer.save( &adapter, options );
		}

		Uint32 dataEnd = file.GetOffset();
		dataSize = dataEnd - dataStart;

		file.Seek( sizeOffset );
		file << dataSize;
		file.Seek( dataEnd );
	}

	return file;
}

template< class _HavokObject >
const _HavokObject* CHavokResource< _HavokObject >::GetRootObject() const
{
	return m_rootObject;
}

template< class _HavokObject >
_HavokObject* CHavokResource< _HavokObject >::GetRootObject()
{
	return m_rootObject;
}

#endif
RED_WARNING_POP()
