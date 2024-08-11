/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "object.h"
#include "fileFormat.h"

/// Base resource importer interface
class IImporter : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IImporter, CObject );

public:
	/// Import options
	class ImportOptions
	{
	public:

		enum EErrorCode
		{
			EEC_Success,		// imported correctly
			EEC_FileToReimport,	// you exported old file. please export with latest tech. can import
			EEC_BadVersion,		// bad version. we don't support this one. do not import
			EEC_FileNotExist,	// file dont exist
			EEC_Max
		};

		struct ImportParams {};

		CResource*		m_existingResource;		// Existing resource that should be reused
		CObject*		m_parentObject;			// NULL for normal resources, sometimes used for embedded resources
		String			m_sourceFilePath;		// Path to file we should import the resource from
		ImportParams*	m_params;				// Import params
		EErrorCode		m_errorCode;			// Return code for any anomaly while importing f.ex. wrong version etc

	public:
		RED_INLINE ImportOptions()
			: m_existingResource( NULL )
			, m_parentObject( NULL )
			, m_params( NULL )
			, m_errorCode( EEC_Success )
		{};
	};

protected:
	CClass*							m_resourceClass;		// Resource class this importer supports
	TDynArray< CFileFormat >		m_formats;				// Supported file formats

public:
	// Import resource
	virtual CResource* DoImport( const ImportOptions& options )=0;

	// Prepare resource import for import custom resource
	virtual Bool PrepareForImport( const String& /*filePath*/, ImportOptions& options ){ return true; }

	// Check support
	Bool SupportsResource( CClass* resourceClass ) const;
	Bool SupportsFormat( const String& fileFormat ) const;

	CClass* GetSupportedResourceClass() const { return m_resourceClass; }

public:
	static IImporter* FindImporter( CClass* resourceClass, const String& fileFormat );
	static void EnumImportFormats( CClass* resourceClass, TDynArray< CFileFormat >& formats );
	static void EnumImportClasses( TDynArray< CClass* >& importClasses );
};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IImporter, CObject );
