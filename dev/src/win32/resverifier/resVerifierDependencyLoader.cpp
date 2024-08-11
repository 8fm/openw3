
#include "build.h"

#define FROM_EXPORT_INDEX( x ) ( ((Int)x) - 1 )
#define FROM_IMPORT_INDEX( x ) ( -((Int)x) - 1 )

CResVerifierDependencyLoader::CResVerifierDependencyLoader( IFile& file )
	: IDependencyLinker( file, 0 )
{
}

CResVerifierDependencyLoader::~CResVerifierDependencyLoader()
{
}

IFile &CResVerifierDependencyLoader::operator<<( CObject *&object )
{
	Int16 index = 0;  
	m_file << index;
	object = MapObject( index );
	return *this;   
}

IFile &CResVerifierDependencyLoader::operator<<( CName& name )
{
	Uint16 index = 0;  
	m_file << index;
	name = MapName( index );
	return *this;   
}

CObject *CResVerifierDependencyLoader::MapObject( Int16 index )
{
	if ( index < 0 )
	{
		Int importIndex = FROM_IMPORT_INDEX( index );
		ASSERT( importIndex >= 0 && importIndex < (Int)m_imports.Size() );
		return m_imports[ importIndex ].m_resource;
	}
	else if ( index > 0 )
	{
		Int exportIndex = FROM_EXPORT_INDEX( index );
		ASSERT( exportIndex >= 0 && exportIndex < (Int)m_exports.Size() );
		return m_exports[ exportIndex ].m_object;
	}

	return NULL;
}

CName CResVerifierDependencyLoader::MapName( Uint16 index )
{
	ASSERT( index < (Uint16)m_names.Size() );
	return m_names[ index ];
}

Bool CResVerifierDependencyLoader::GetImports( TDynArray< String >& importFiles )
{
	// Cleanup
	m_names.Clear();
	m_imports.Clear();

	// Load file header
	m_file << m_header; 

	// Setup archive version
	m_version = m_header.m_version;

	// Check magic number
	if ( m_header.m_magic != LINKER_FILE_MAGIC )
	{
		WARN( TXT("Not valid package (0%X, 0x%X)"), m_header.m_magic, LINKER_FILE_MAGIC );
		return false;
	}

	// Check for not supported version
	if ( m_header.m_version > VER_CURRENT )
	{
		WARN( TXT("Version %i is not supported. Highest supported verison is %i."), m_header.m_version, VER_CURRENT );
		return false;  
	}

	// To old ?
	if ( m_header.m_version < VER_MINIMAL )
	{
		WARN( TXT("Version %i is no longer supported. Minimal supported version is %i."), m_header.m_version, VER_MINIMAL );
		return false;   
	}

	// Load names - only for serialization process
	Seek( m_header.m_namesOffset );
	m_names.PushBack( CName() );
	for ( Uint i=0; i<m_header.m_namesCount; i++ )
	{
		CLinkerName name;
		*this << name;
		m_names.PushBack( CName( name.ToString() ) );
	}

	// Load imports
	Seek( m_header.m_importsOffset );
	for ( Uint i=0; i<m_header.m_importsCount; i++ )
	{
		CLinkerImport import;
		*this << import;
		m_imports.PushBack( import );
	}

	// Fill import list
	for ( Uint i=0; i<m_imports.Size(); i++ )
	{
		CLinkerImport& import = m_imports[i];
		String importPath = import.m_path.ToString();
		importFiles.PushBack( importPath );
	}

	return true;
}