/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "xmlReader.h"

// Simple XML reader
class CXMLFileReader : public CXMLReader
{
public:

	// this constructor will not delete the file
    CXMLFileReader( IFile &file );

	// This constructor will delete the file
	CXMLFileReader( IFile *file );

    virtual ~CXMLFileReader();

private:
	void Parse( IFile &file );
};
