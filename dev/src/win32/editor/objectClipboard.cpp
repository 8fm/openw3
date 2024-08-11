/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencySaver.h"

#define MAGIC_NUMBER 0xC1393049

CEdObjectClipboard GObjectClipboard;

class CEdObjectClipboardDataObject : public wxDataObject
{
	wxDataFormat				m_format;
	void*						m_buffer;
	size_t						m_size;
	StringAnsi					m_desc;
public:
	CEdObjectClipboardDataObject( const wxDataFormat& format )
		: m_format( format )
		, m_buffer( NULL )
		, m_size( 0 )
		, m_desc( "" )
	{}

	virtual CEdObjectClipboardDataObject::~CEdObjectClipboardDataObject()
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Default, m_buffer );
	}

	virtual void GetAllFormats( wxDataFormat *formats, Direction dir /* = Get */ ) const
	{
		switch ( dir )
		{
		case Get:
			formats[0] = m_format;
			formats[1] = wxDF_TEXT;
			break;
		case Set:
			formats[0] = m_format;
			break;
		}
	}

	virtual bool GetDataHere( const wxDataFormat& format, void *buf ) const
	{
		if ( format == m_format )
		{
			Red::System::MemoryCopy( buf, m_buffer, m_size );
		}
		else if ( format == wxDF_TEXT )
		{
			Red::System::MemoryCopy( buf, m_desc.AsChar(), m_desc.GetLength() + 1 );
		}
		else
		{
			return false;
		}
		return true;
	}

	virtual size_t GetDataSize( const wxDataFormat& format ) const
	{
		if ( format == m_format )
		{
			return m_size;
		}
		else if ( format == wxDF_TEXT )
		{
			return m_desc.GetLength() + 1;
		}
		else
		{
			return 0;
		}
	}

	virtual size_t GetFormatCount( Direction dir ) const
	{
		return dir == Get ? 2 : 1;
	}

	virtual wxDataFormat GetPreferredFormat( Direction dir ) const
	{
		return m_format;
	}

	virtual bool SetData( const wxDataFormat& format, size_t len, const void* buf )
	{
		if ( format != m_format || len < 8 ) return false;

		CMemoryFileReaderExternalBuffer reader( buf, len );
		Uint32 magic, objects;
		reader << magic;
		reader << objects;

		if ( magic != MAGIC_NUMBER ) return false;

		char tmp[1024];
		Red::System::SNPrintF( tmp, ARRAY_COUNT( tmp ), "List of %d objects (%d bytes long)", objects, len - 8);
		m_desc = StringAnsi( tmp );

		m_size = len;
		m_buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Default, len );
		Red::System::MemoryCopy( m_buffer, buf, len );

		return true;
	}

	Bool HasObjects() const
	{
		if ( !m_buffer || m_size < 8 ) return false;

		CMemoryFileReaderExternalBuffer reader( m_buffer, m_size );
		Uint32 magic, objects;
		reader << magic;
		reader << objects;

		return magic == MAGIC_NUMBER && objects > 0;
	}
};

CEdObjectClipboard::CEdObjectClipboard()
	: m_format( wxT("REDEngineEditorObjectList") )
{}

Bool CEdObjectClipboard::DoCopy( const TDynArray<CObject*>& objs )
{
	static Uint32 magic = MAGIC_NUMBER;
	Uint32 count = objs.Size();
	if ( !count ) return true;

	TDynArray< Uint8 > buffer;
	buffer.Reserve( 1024 );

	CMemoryFileWriter writer( buffer );
	writer << magic;
	writer << count;
	CDependencySaver saver( writer, NULL );
	DependencySavingContext svctx( objs );
	svctx.m_saveReferenced = svctx.m_saveTransient = true;
	svctx.m_useFeedback = true;
	svctx.m_taskName = TXT("Copying to clipboard");
	if ( !saver.SaveObjects( svctx ) )
	{
		return false;
	}

	if ( wxTheClipboard->Open() )
	{
		CEdObjectClipboardDataObject* data = new CEdObjectClipboardDataObject( m_format );
		data->SetData( m_format, writer.GetSize(), writer.GetBuffer() );
		wxTheClipboard->SetData( data );
		wxTheClipboard->Close();
	} 
	else
	{
		return false;
	}

	return true;
}

Bool CEdObjectClipboard::Copy( CObject* obj )
{
	TDynArray<CObject*> objs;
	objs.PushBack( obj );
	return Copy( objs );
}

Bool CEdObjectClipboard::Paste( TDynArray<CObject*>& objs, CObject* parent )
{
	void* clipbuffer;
	Uint32 clipsize, storedmagic, count;
	TDynArray< Uint8 > buffer;

	if ( wxTheClipboard->Open() )
	{
		if ( !wxTheClipboard->IsSupported( m_format ) ) return false;
		CEdObjectClipboardDataObject data( m_format );
		wxTheClipboard->GetData( data );
		clipbuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, data.GetDataSize( m_format ) );
		clipsize = data.GetDataSize( m_format );
		if ( !data.GetDataHere( m_format, clipbuffer ) )
		{
			wxTheClipboard->Close();
			RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, clipbuffer );
			return false;
		}
		wxTheClipboard->Close();
	}
	else
	{
		return false;
	}

	CMemoryFileReaderExternalBuffer reader( clipbuffer, clipsize );

	reader << storedmagic;
	ASSERT( storedmagic == MAGIC_NUMBER );
	reader << count;
	if ( !count )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, clipbuffer );
		return true;
	}

	CDependencyLoader loader( reader, NULL );
	DependencyLoadingContext loadingContext;
	loadingContext.m_parent = parent;
	if ( !loader.LoadObjects( loadingContext ) )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, clipbuffer );
		return false;
	}
	loader.PostLoad();

	objs.PushBack( loadingContext.m_loadedRootObjects );

	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, clipbuffer );

	return true;
}

Bool CEdObjectClipboard::Paste( CObject*& obj, CObject* parent )
{
	TDynArray<CObject*> objs;
	if ( Paste( objs, parent ) )
	{
		obj = objs[0];
		return true;
	}
	else
	{
		obj = NULL;
		return false;
	}
}

Bool CEdObjectClipboard::HasObjects() const
{
	if ( wxTheClipboard->Open() )
	{
		if ( !wxTheClipboard->IsSupported( m_format ) ) return false;
		CEdObjectClipboardDataObject data( m_format );
		wxTheClipboard->GetData( data );
		if ( !data.HasObjects() ) {
			wxTheClipboard->Close();
			return false;
		}
		wxTheClipboard->Close();
		return true;
	}

	return false;
}
