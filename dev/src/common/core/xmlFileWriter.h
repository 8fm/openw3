/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "xmlWriter.h"

/// Simple XML writer
class CXMLFileWriter : public CXMLWriter
{

protected:

    IFile& m_file;		// Output file

public:

	CXMLFileWriter( IFile& file );

public: // CXMLWriter interface

    // Saves xml content
    virtual void Flush();

};
