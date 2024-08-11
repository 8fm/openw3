/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "xmlFile.h"
#include "math.h"

/// XML reader/writer interface
class CXMLReader : public IXMLFile
{
protected:

    CXMLReader();

public:
    CXMLReader( const String &content, Bool _dontCheckXmlHeader = false );
	virtual ~CXMLReader();

	// XML interface
public:
	//! Find child node in current node
	//! Return true if succeeded
	//! All subsequent calls will apply to it
	RED_INLINE virtual Bool BeginNode( const String& name ) { return BeginNode( name, false ); }

	//! Get attrib value
	virtual Bool Attribute( const String& name, String& value );

	//! Check if current node has any attribute
	virtual Bool HasAnyAttribute();

	//! Get value
	virtual Bool Value( String& value );

	//! End processing current node
	virtual void EndNode( Bool moveToNext = true );

   //! Get numbers of children
   virtual Uint32 GetChildCount();

	//! Returns the currently scoped xml node
   const XMLNode* GetCurrentNode() const;

   //! Find child node in current node and search backward if elements order doesn't matter
   Bool BeginNode( const String& name, Bool searchBackward );

public:

    //! Get current node name
    virtual Bool GetNodeName( String& name ) const;

    virtual Bool BeginNextNode();

	Bool HasNotAllowedChildnodes( const TDynArray< String >& allowedNodeNames, TDynArray< String >& notAllowedNamesFound ) const;
	Bool HasNotAllowedAttributes( const TDynArray< String >& allowedAttributesNames, TDynArray< String >& notAllowedAttributesFound ) const;

public:
	template< class T >
	Bool AttributeT( const String& name, T& value )
	{
		String str;
		if ( Attribute( name, str ) )
		{
			return FromString<T>( str, value );
		}
		return false;
	}

	template< class T >
	T AttributeTT( const String& name, T def = T(0) )
	{
		T val = def;
		String str;
		if ( AttributeT( name, str ) )
		{
			FromString<T>( str, val );
		}
		return val;
	}
};

template<>
RED_INLINE Bool CXMLReader::AttributeT<StringAnsi>( const String& name, StringAnsi& value )
{
	String str;
	if ( Attribute( name, str ) )
	{
		value = UNICODE_TO_ANSI(str.AsChar());
		return true;
	}
	return false;
}
