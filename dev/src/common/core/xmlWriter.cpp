/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "xmlWriter.h"

using namespace rapidxml;

CXMLWriter::CXMLWriter()
: IXMLFile()
{
	m_flags |= FF_Writer;

	// initializations
	m_xmlData = NULL;
	m_doc = new xml_document<Char>();
	m_doc->name( TXT( "XMLDoc" ) );
	m_nodeStack.Clear();
	m_currentChild = m_doc;
}

CXMLWriter::~CXMLWriter()
{	
    if ( m_doc )
    {
        m_doc->clear();
        delete m_doc;
        m_doc = NULL;
    }
}

Bool CXMLWriter::BeginNode( const String& name )
{
	if ( !m_currentChild ) 
	{
		return false;
	}

	XMLNode* newNode = new XMLNode( rapidxml::node_element );

	Uint32 nameLen = name.GetLength();
	Char * charName = CopyStr( name.AsChar(), nameLen );
	newNode->name( charName, nameLen );

	m_currentChild->append_node( newNode );
	m_nodeStack.PushBack( m_currentChild );
	m_currentChild = newNode;

	return true;
}

void CXMLWriter::EndNode( Bool )
{
	ASSERT( !m_nodeStack.Empty() );
	m_currentChild = m_nodeStack.PopBack();
}

Bool CXMLWriter::Attribute( const String& name, String& value )
{
	return SetAttribute( name, value );
}

Bool CXMLWriter::SetAttribute( const String& name, const String& value )
{
	XMLAttr* attribute = new XMLAttr();
	
	Uint32 strLen = name.GetLength();
	Char * tmpStr = CopyStr( name.AsChar(), strLen );
	attribute->name( tmpStr, strLen );

	strLen = value.GetLength();
	tmpStr = CopyStr( value.AsChar(), strLen );
	attribute->value( tmpStr, strLen );

	m_currentChild->append_attribute( attribute);

	return true;
}

Bool CXMLWriter::Value( String& value )
{
	return SetValue( value );
}

Bool CXMLWriter::SetValue( const String& value )
{
	Uint32 strLen = value.GetLength();
	Char* valStr = CopyStr( value.AsChar(), strLen );
	m_currentChild->value( valStr, strLen );

	return true;
}

void CXMLWriter::Flush()
{
	if ( !m_doc )
	{
		return;
	}

	String content( TXT( "<?xml version=\"1.0\" encoding=\"UTF-16\"?>\n" ) );

	// 1. appending thousands of string to another string is extremely slow...
	// 2. reserve is memory consuming
	// 3. both above MUST be avoided in Game
	content.Reserve( 4 * 1024 * 1024 );

	for ( XMLNode* nextChild = m_doc->first_node(); 
		nextChild; 
		nextChild = nextChild->next_sibling() )
	{
		FlushNode( 0, nextChild, content);
	}

	m_data = content;
}

Bool CXMLWriter::FlushHeader( Uint32 level, XMLNode* node, String& content ) const
{
    String indentation;
    for ( Uint32 i = 0; i < level; ++i )
    {
        indentation += TXT( "    " );
    }
    content += indentation;
    content += TXT( "<" );
    content += node->name();

    for ( XMLAttr* attrib = node->first_attribute(); attrib; attrib = attrib->next_attribute() )
    {
        content += TXT( " " );
        content += attrib->name();
        content += TXT( "=\"" );
        content += attrib->value();
        content += TXT( "\"" );
    }

    if ( IsNodeInlined( node ) )
    {
        content += TXT( " />\n" );
        return false;
    }
    else
    {
        content += TXT( ">\n" );

        if ( node->value_size() > 0 )
        {
            content += indentation + TXT( "    " ) + node->value() + TXT("\n");
        }

        return true;
    }
}

void CXMLWriter::FlushNode( Uint32 level, XMLNode* node, String& content ) const
{
    if ( FlushHeader( level, node, content ) )
    {
        for ( XMLNode* nextChild = node->first_node(); nextChild; nextChild = nextChild->next_sibling() )
        {
            FlushNode( level + 1, nextChild, content );
        }

        FlushTail( level, node, content );
    }
}

void CXMLWriter::FlushTail( Uint32 level, XMLNode* node, String& content ) const
{
	for ( Uint32 i = 0; i < level; ++i )
	{
		content += TXT( "    " );
	}

	content += TXT( "</" );
	content += node->name();
	content += TXT( ">\n" );
}

Bool CXMLWriter::IsNodeInlined( XMLNode* node ) const
{
    return node && node->value_size() == 0 && !node->first_node();
}

Char* CXMLWriter::CopyStr( const Char* str, Uint32 strLen ) const
{
    Char * newStr = m_doc->allocate_string( NULL, strLen + 1 );
    Red::System::StringCopy( newStr, str, strLen + 1 );

    return newStr;
}
