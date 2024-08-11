/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "object.h"
#include "fileFormat.h"

/// Base resource exporter interface
class IExporter : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IExporter, CObject );

public:
	/// Export options
	class ExportOptions
	{
	public:
		CResource*		m_resource;			// Resource that should be exported
		String			m_saveFilePath;		// Path to file we should Export the resource to
		CFileFormat		m_saveFileFormat;	// Save file format
		Uint32			m_lodToUse;			// Defines mesh's lod to be exported

	public:
		RED_INLINE ExportOptions()
			: m_resource( NULL )
			, m_lodToUse( 0 )
		{};
	};

protected:
	CClass*							m_resourceClass;		// Resource class this Exporter supports
	TDynArray< CFileFormat >		m_formats;				// Supported file formats

public:
	// Get resource class
	RED_INLINE CClass* GetSupportedResourceClass() const { return m_resourceClass; }

	// Export resource
	virtual Bool DoExport( const ExportOptions& options )=0;

	// Check support
	Bool SupportsResource( CClass* resourceClass ) const;
	Bool SupportsFormat( const String& fileFormat ) const;

public:
	static IExporter* FindExporter( CClass* resourceClass, const String& fileFormat );
	static void EnumExportFormats( CClass* resourceClass, TDynArray< CFileFormat >& formats );
	static void EnumExportClasses( TDynArray< CClass* >& exportClasses );
};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IExporter, CObject );
