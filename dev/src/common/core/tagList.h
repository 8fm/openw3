/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dynarray.h"
#include "names.h"
#include "math.h"

/// List of tags
class TagList
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Tags );
public:
	typedef TDynArray< CName > TTagList;

protected:
	TTagList m_tags;			//!< List of tags

public:
	static TagList EMPTY;

public:
	//! Returns true if tag list is empty
	RED_INLINE Bool Empty() const { return m_tags.Empty(); }

	//! Does this tag group contain a given tag
	RED_INLINE Bool HasTag( const CName& name ) const { return m_tags.Exist( name ); }

	RED_INLINE Uint32 Size() const { return m_tags.Size(); }

public:
	//! Match any tag from reference list in the other tagList
	static Bool MatchAny( const TagList& tagList, const TagList& referenceList );

	//! Match all tags from reference list in the other tagList
	static Bool MatchAll( const TagList& tagList, const TagList& referenceList );

	//! Match tag in the tagList
	static Bool Match( const CName& reference, const TagList& tagList );

public:
	//! Default constructor
	TagList();

	//! Copy
	TagList( const TagList& other );

	//! Move
	TagList( TagList&& other );

	//! Create from plain list of names
	TagList( const TTagList& tags );

	//! Create from single name
	TagList( const CName& tag );

	//! Destructor
	~TagList();

	//! Clear tag list
	void Clear();

	//! Set new tags
	void SetTags( const TTagList& tags );

	//! Set new tags
	void SetTags( const TagList& tags );

	//! Merge tag list with another tag list
	void AddTags( const TagList& tags );

	//! Add one tag to tag list
	void AddTag( const CName& tag );

	//! Reserve space for n tags
	void Reserve( Uint32 size );

	//! Remove tags given in tag list from th
	void SubtractTags( const TagList& tags );

	//! Remove one tag from tag list
	void SubtractTag( const CName& tag );

public:
	//! Get list of tags
	const TTagList& GetTags() const;

	CName GetTag( Uint32 index ) const;

public:
	//! Copy tags
	TagList& operator=( const TagList& other );

	//! Move tags
	TagList& operator=( TagList&& other );

	//! Compare
	Bool operator==( const TagList& other ) const;

	//! Compare
	Bool operator!=( const TagList& other ) const;

public:
	//! Initialize from string
	Bool FromString( const String& tags );

	//! Convert to string
	String ToString( bool withSpaces = true ) const;

	//! Serialize to/from file
	void Serialize( IFile& ar );
	
public:
	//! Serialize
	friend IFile& operator<<( IFile& file, TagList& tagList )
	{
		tagList.Serialize( file );
		return file;
	}
};

// Conversion for tag list type type
template<>
RED_INLINE String ToString( const TagList& value )
{
	return value.ToString();
}

// Convert string to tag list
template<>
RED_INLINE Bool FromString( const String& text, TagList& value )
{
	return value.FromString( text );
};

//////////////////////////////////////////////////////////////////////////
// list of tags type. 
RED_DECLARE_RTTI_NAME( TagList );

class CSimpleRTTITypeTagList : public TSimpleRTTIType< TagList >
{
public:
	virtual void Destruct( void *object ) const
	{
		static_cast< TagList* >(object)->~TagList();
	}

	virtual const CName& GetName() const
	{
		return CNAME( TagList );
	}

	virtual ERTTITypeType GetType() const
	{
		return RT_Simple;
	}

	virtual Bool ToString( const void* data, String& valueString ) const
	{
		valueString = static_cast< const TagList* >(data)->ToString();
		return true;
	}

	virtual Bool FromString( void* data, const String& valueString ) const
	{
		return static_cast< TagList* >(data)->FromString( valueString );
	}
};

template<>
struct TTypeName< TagList >
{													
	static const CName& GetTypeName() { return CNAME( TagList ); }	
};
