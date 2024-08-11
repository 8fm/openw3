/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "string.h"

/// Description of file format
class CFileFormat
{
protected:
	String		m_extension;
	String		m_description;

public:
	CFileFormat();
	CFileFormat( const String& ext, const String& desc );

	RED_INLINE const String& GetExtension() const { return m_extension; }
	RED_INLINE const String& GetDescription() const { return m_description; }
};