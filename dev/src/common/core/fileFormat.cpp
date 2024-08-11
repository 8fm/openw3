/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "fileFormat.h"

CFileFormat::CFileFormat()
{
}

CFileFormat::CFileFormat( const String& ext, const String& desc )
	: m_extension( ext )
	, m_description( desc )
{
}