/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "exporter.h"

IMPLEMENT_ENGINE_CLASS( IExporter );

Bool IExporter::SupportsResource( CClass* resourceClass ) const
{
	return resourceClass->IsBasedOn( m_resourceClass );
}

Bool IExporter::SupportsFormat( const String& fileFormat ) const
{
	// Linear search :P
	for ( Uint32 i=0; i<m_formats.Size(); i++ )
	{
		if ( m_formats[i].GetExtension().EqualsNC( fileFormat ) )
		{
			return true;
		}
	}

	// Not supported
	return false;
}

IExporter* IExporter::FindExporter( CClass* resourceClass, const String& fileFormat )
{
	// Request Exporter classes
	TDynArray< CClass* > ExporterClasses;
	SRTTI::GetInstance().EnumClasses( ClassID<IExporter>(), ExporterClasses );

	// Linear search :P
	for ( Uint32 i=0; i<ExporterClasses.Size(); i++ )
	{
		IExporter* Exporter = ExporterClasses[i]->GetDefaultObject< IExporter >();
		if ( Exporter->SupportsResource( resourceClass ) )
		{
			if ( Exporter->SupportsFormat( fileFormat ))
			{
				return Exporter;
			}
		}
	}

	// Format not supported
	return NULL;
}

void IExporter::EnumExportClasses( TDynArray< CClass* >& ExportClasses )
{
	// Request Exporter classes
	TDynArray< CClass* > ExporterClasses;
	SRTTI::GetInstance().EnumClasses( ClassID<IExporter>(), ExporterClasses );

	// Linear search :P
	for ( Uint32 i=0; i<ExporterClasses.Size(); i++ )
	{
		IExporter* Exporter = ExporterClasses[i]->GetDefaultObject< IExporter >();
		ExportClasses.PushBackUnique( Exporter->m_resourceClass );
	}
}

void IExporter::EnumExportFormats( CClass* resourceClass, TDynArray< CFileFormat >& formats )
{
	// Request Exporter classes
	TDynArray< CClass* > ExporterClasses;
	SRTTI::GetInstance().EnumClasses( ClassID<IExporter>(), ExporterClasses );

	// Linear search :P
	for ( Uint32 i=0; i<ExporterClasses.Size(); i++ )
	{
		IExporter* Exporter = ExporterClasses[i]->GetDefaultObject< IExporter >();
		if ( Exporter->SupportsResource( resourceClass ) )
		{
			formats.PushBack( Exporter->m_formats );
		}		
	}
}
