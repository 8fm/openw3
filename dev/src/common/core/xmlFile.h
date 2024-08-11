/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "file.h"
#include "string.h"

#define RAPIDXML_NO_EXCEPTIONS
#define RAPIDXML_STATIC_POOL_SIZE	32 * 1024
#include "rapidxml.hpp"

typedef rapidxml::xml_node<Char> XMLNode;
typedef rapidxml::xml_attribute<Char> XMLAttr;

/// XML reader/writer interface
class IXMLFile : public IFile, public Red::System::NonCopyable
{
protected:
	typedef rapidxml::xml_document<Char> XMLDoc;

	String m_data;
	XMLDoc* m_doc;
	TDynArray<XMLNode*> m_nodeStack;
	XMLNode* m_topNodeStack;
	XMLNode* m_currentChild;
	const XMLAttr* m_lastUsedAttr;
	Char* m_xmlData;

public:
	// XML attribute
	struct NodeAttribute
	{
		String		m_name;		// Attribute name
		String		m_value;	// Attribute value

		RED_INLINE NodeAttribute()
		{};

		RED_INLINE NodeAttribute( const String& name, const String& value )
			: m_name( name )
			, m_value( value )
		{};
	};

	typedef TDynArray< NodeAttribute > AttrArray;

public:
	IXMLFile();
	virtual ~IXMLFile();

	// XML interface
public:
	//! Find or create named node in current node
	//! Return true if succeeded
	//! All subsequent calls will apply to it
	virtual Bool BeginNode( const String& name ) = 0;

	//! Set or get attrib value
	virtual Bool Attribute( const String& name, String& value ) = 0;

	//! Set or get value
	virtual Bool Value( String& value ) = 0;
	
	//! End processing current node
	virtual void EndNode( Bool moveToNext = true ) = 0;

    //! Get numbers of children
    virtual Uint32 GetChildCount() = 0;

public:
	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size );

	// Get position in file stream
	virtual Uint64 GetOffset() const;

	// Get size of the file stream
	virtual Uint64 GetSize() const;

	// Seek to file position
	virtual void Seek( Int64 offset );
};
