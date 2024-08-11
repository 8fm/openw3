/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "xmlReader.h"

using namespace rapidxml;

//#define PROFILE_XML_READER
#ifdef PROFILE_XML_READER
#include "profiler.h"
#endif

namespace rapidxml 
{
	extern void set_error_file( const String& new_error_file );
};

CXMLReader::CXMLReader()
: IXMLFile()
{
    m_flags |= FF_Reader;
}

CXMLReader::CXMLReader( const String &content, Bool _dontCheckXmlHeader )
: IXMLFile()
{
	m_flags |= FF_Reader;

    m_data = content;

    if ( !_dontCheckXmlHeader & !m_data.BeginsWith( TXT("<?xml version=") ) )
    {
        ERR_CORE( TXT("Xml reader cannot parse the given content. It's not an xml format.") );
        return;
    }

	// initializations
	rapidxml::set_error_file( String::EMPTY );
	m_doc = new xml_document< Char >();
	m_nodeStack.Clear();
	m_topNodeStack = NULL;
	m_doc->parse<0>( const_cast< Char * >( m_data.AsChar() ) );
	m_currentChild = m_doc->first_node();
}

CXMLReader::~CXMLReader()
{
    if ( m_doc )
    {
        m_doc->clear();
        delete m_doc;
        m_doc = NULL;
    }
}

Bool CXMLReader::BeginNode( const String& name, Bool searchBackward )
{
#ifdef PROFILE_XML_READER
	PC_SCOPE( XMLBeginNode );
#endif
    if ( !m_doc )
    {
        return false;
    }

	//LOG_CORE( TXT("Beginning node %s"), name );
	const Char *charName = name.AsChar();
	const Uint32 nameLen = static_cast< Uint32 >( Red::System::StringLength( charName ) );
	XMLNode* currentChild =  m_currentChild;
	if ( !currentChild ) 
	{
		return false;
	}

	if ( !Red::System::StringCompare( currentChild->name(), name.AsChar() ) ) 
	{
		m_lastUsedAttr = NULL;
		m_topNodeStack = currentChild;
		m_nodeStack.PushBack( m_topNodeStack );
		m_currentChild = currentChild->first_node();
		return true;
	}

	XMLNode *newNode = currentChild->next_sibling( charName, nameLen, false );
	if ( !newNode )
	{
		if( searchBackward )
		{
			newNode = currentChild->previous_sibling( charName, nameLen, false );
		}

		if ( !newNode )
		{
			return false;
		}
	}
	m_lastUsedAttr = NULL;
	m_topNodeStack = newNode;
	m_nodeStack.PushBack( m_topNodeStack );
	m_currentChild = newNode->first_node();
	return true;
}

void CXMLReader::EndNode( Bool moveToNext /*= true*/ )
{
#ifdef PROFILE_XML_READER
	PC_SCOPE( XMLEndNode );
#endif

    if ( !m_doc )
    {
        return;
    }

	ASSERT( !m_nodeStack.Empty() );
	m_lastUsedAttr = NULL;
	XMLNode* poppedNode = m_nodeStack.PopBack();
	if( !m_nodeStack.Empty() )
		m_topNodeStack = m_nodeStack[m_nodeStack.Size() - 1];
	else
		m_topNodeStack = NULL;
	m_currentChild = moveToNext ? poppedNode->next_sibling() : poppedNode;
}

Bool CXMLReader::Attribute( const String& name, String& value )
{
#ifdef PROFILE_XML_READER
	PC_SCOPE( XMLAttribute );
#endif

    if ( !m_doc )
    {
        return false;
    }

	ASSERT( !m_nodeStack.Empty() );

	const Char* strName = name.AsChar();
	size_t lenName = (size_t)name.GetLength();
	
	const XMLAttr* xmlAttributeUp;
	const XMLAttr* xmlAttributeDown;

	if( m_lastUsedAttr )
	{
		xmlAttributeUp   = m_lastUsedAttr->next_attribute();;
		xmlAttributeDown = m_lastUsedAttr;
	}
	else
	{
		xmlAttributeUp   = m_topNodeStack->first_attribute();
		xmlAttributeDown = NULL;
	}

    while( xmlAttributeUp || xmlAttributeDown )
    {
		if( xmlAttributeUp )
		{
			if ( (xmlAttributeUp->name_size() == lenName) && Red::System::StringCompare( xmlAttributeUp->name(), strName ) == 0 )
			{
				value = xmlAttributeUp->value();
				m_lastUsedAttr = xmlAttributeUp;
				return true;
			}
			xmlAttributeUp = xmlAttributeUp->next_attribute();
		}
		if( xmlAttributeDown )
		{
			if ( (xmlAttributeDown->name_size() == lenName) && Red::System::StringCompare( xmlAttributeDown->name(), strName ) == 0 )
			{
				value = xmlAttributeDown->value();
				m_lastUsedAttr = xmlAttributeDown;
				return true;
			}
			xmlAttributeDown = xmlAttributeDown->previous_attribute();
		}
    }

	m_lastUsedAttr = NULL; 
	value = String::EMPTY;
	return false;
}

Bool CXMLReader::HasAnyAttribute()
{
#ifdef PROFILE_XML_READER
	PC_SCOPE( XMLHasAnyAttribute );
#endif

	if ( !m_doc )
	{
		return false;
	}

	ASSERT( !m_nodeStack.Empty() );

	const XMLAttr* xmlAttribute = m_topNodeStack->first_attribute();
	return xmlAttribute != nullptr;
}

Bool CXMLReader::Value( String& value )
{
#ifdef PROFILE_XML_READER
	PC_SCOPE( XMLValue );
#endif

    if ( !m_doc )
    {
        return false;
    }

	ASSERT( !m_nodeStack.Empty() );
    value = m_topNodeStack->value();
    value.Trim();    
	return true;
}

Bool CXMLReader::BeginNextNode()
{
#ifdef PROFILE_XML_READER
	PC_SCOPE( XMLBeginNextNode );
#endif

    if ( !m_doc || !m_currentChild ) 
    {
        return false;
    }

	m_lastUsedAttr = NULL;
	m_topNodeStack = m_currentChild;
    m_nodeStack.PushBack( m_topNodeStack );
    m_currentChild = m_topNodeStack->first_node();
    return true;
}

Bool CXMLReader::GetNodeName( String& name ) const
{
#ifdef PROFILE_XML_READER
	PC_SCOPE( XMLGetNodeName );
#endif

    if ( !m_doc )
    {
        return false;
    }

	if ( m_nodeStack.Empty() )
	{
		return false;
	}
	name = m_topNodeStack->name();
	return true;
}

Uint32 CXMLReader::GetChildCount()
{
#ifdef PROFILE_XML_READER
	PC_SCOPE( XMLGetChildCount );
#endif

    if ( !m_doc )
    {
        return false;
    }

    if ( !m_currentChild ) 
    {
        return 0;
    }

    Uint32 count = 1;
    XMLNode *node = m_currentChild->previous_sibling();
    while ( node )
    {
        count++;
        node = node->previous_sibling();
    }
    
    node = m_currentChild->next_sibling();
    while ( node )
    {
        count++;
        node = node->next_sibling();
    }
    
    return count;
}

const XMLNode* CXMLReader::GetCurrentNode() const
{
#ifdef PROFILE_XML_READER
	PC_SCOPE( XMLGetCurrentNode );
#endif

	if ( !m_doc )
	{
		return NULL;
	}

	if ( m_nodeStack.Empty() )
	{
		return NULL;
	}

	return m_topNodeStack;
}

Bool CXMLReader::HasNotAllowedChildnodes( const TDynArray< String >& allowedNodeNames, TDynArray< String >& notAllowedNamesFound ) const
{
#ifdef PROFILE_XML_READER
	PC_SCOPE( XMLHasNotAllowedChildnodes );
#endif

	Bool retVal = false;
	if ( m_currentChild )
	{
		const XMLNode* childNode = m_currentChild->parent()->first_node();

		while ( childNode )
		{
			String name = childNode->name();
			if ( !allowedNodeNames.Exist( name ) )
			{
				retVal = true;
				notAllowedNamesFound.PushBack( name );
			}
			childNode = childNode->next_sibling();
		}
	}

	return retVal;
}

Bool CXMLReader::HasNotAllowedAttributes( const TDynArray< String >& allowedAttributesNames, TDynArray< String >& notAllowedAttributesFound ) const
{
#ifdef PROFILE_XML_READER
	PC_SCOPE( XMLHasNotAllowedAttributes );
#endif

	ASSERT( !m_nodeStack.Empty() );
	Bool retVal = false;

	const XMLAttr* xmlAttribute = m_topNodeStack->first_attribute();
	while( xmlAttribute )
	{
		const String attrName = xmlAttribute->name();
		if ( !allowedAttributesNames.Exist( attrName ) )
		{
			retVal = true;
			notAllowedAttributesFound.PushBack( attrName );
		}
		xmlAttribute = xmlAttribute->next_attribute();
	}
	return retVal;
}
