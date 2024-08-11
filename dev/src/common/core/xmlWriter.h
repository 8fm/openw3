/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "xmlFile.h"
#include "math.h"

/// Simple XML builder
class CXMLWriter : public IXMLFile
{
public:
	CXMLWriter();
	~CXMLWriter();

	
public: // IXMLFile interface

	//! Find or create named node in current node
	//! Return true if succeeded
	//! All subsequent calls will apply to it
	virtual Bool BeginNode( const String& name );

	//! Set desired attrib values
	virtual Bool Attribute( const String& name, String& value );
	
	//! Set desired value
	virtual Bool Value( String& value );

	//! End processing current node
    virtual void EndNode( Bool moveToNext = true );

    //! Get numbers of children
    virtual Uint32 GetChildCount() { return 0; }

public: // CXMLWriter interface

    // Setter for attributes
    Bool SetAttribute( const String& name, const String& value );

    // Setter for values
    Bool SetValue( const String& value );

    // Saves the xml content
    virtual void Flush();

    // Returns the xml content
    const String &GetContent() const { return m_data; }

protected:

    // Flushes node's header
    Bool FlushHeader( Uint32 level, XMLNode* node, String& content ) const;

    // Flushes node's body
    void FlushNode( Uint32 level, XMLNode* node, String& content ) const;

    // Flushes node's tail
    void FlushTail( Uint32 level, XMLNode* node, String& content ) const;

    // True if node is inlined (does not contain any text/value)
    Bool IsNodeInlined( XMLNode* node ) const;

    Char* CopyStr( const Char* str, Uint32 strLen ) const;

public:
	template< class T >
	Bool AttributeT( const String& name, const T& value )
	{
		String str = ToString<T>( value );
		return Attribute( name, str );		
	}
};
