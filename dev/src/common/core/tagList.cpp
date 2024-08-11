/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "tagList.h"

TagList TagList::EMPTY;

IMPLEMENT_SIMPLE_RTTI_TYPE( TagList )

TagList::TagList()
{
}

TagList::TagList( const TagList& other )
{
	SetTags( other );
}

TagList::TagList( TagList&& other )
	: m_tags( Move( other.m_tags ) )
{
}

TagList::TagList( const TTagList& tags )
	: m_tags( tags )
{
}

TagList::TagList( const CName& tag )
{
	AddTag( tag );
}

TagList::~TagList()
{
}

const TagList::TTagList& TagList::GetTags() const
{
	return m_tags;
}

void TagList::Clear()
{
	m_tags.Clear();
}

void TagList::SetTags( const TTagList& tags )
{
	m_tags = tags;
}

void TagList::SetTags( const TagList& other )
{
	m_tags = other.m_tags;
}

void TagList::AddTags( const TagList& tags )
{
	m_tags.PushBackUnique( tags.m_tags );
}

void TagList::AddTag( const CName& tag )
{
	m_tags.PushBackUnique( tag );
}

void TagList::Reserve( Uint32 size )
{
	m_tags.Reserve( size );
}

void TagList::SubtractTags( const TagList& tags )
{
	if ( !m_tags.Empty() && !tags.Empty() )
	{
		// Remove all occurrences
		for ( Uint32 i=0; i<tags.m_tags.Size(); i++ )
		{
			m_tags.Remove( tags.m_tags[i] );
		}
	}
}

void TagList::SubtractTag( const CName& tag )
{
	m_tags.Remove( tag );
}

TagList& TagList::operator=( const TagList& other )
{
	if( this != &other )
	{
		m_tags = other.m_tags;
	}

	return *this;
}

TagList& TagList::operator=( TagList&& other )
{
	if( this != &other )
	{
		m_tags = Move( other.m_tags );
	}
	
	return *this;
}

Bool TagList::operator==( const TagList& other ) const
{
	return other.Size() == this->Size() &&  MatchAll( *this, other );
}

Bool TagList::operator!=( const TagList& other ) const
{
	return other.Size() != this->Size() || !MatchAll( *this, other );
}

void TagList::Serialize( IFile& ar )
{
	ar << m_tags;
}

Bool TagList::FromString( const String& tagString )
{
	// Special case - no tags 
	if ( tagString.EqualsNC( TXT("[No tags]") ) )
    {
        Clear();
		return true;
    }

	// Gather tag list
	TTagList tagList;	
	if ( !tagString.Empty() )
	{
		String tagString2 = tagString;

		// Extract shit
		if ( tagString2.BeginsWith(TXT("[")) && tagString2.EndsWith(TXT("]")) )
		{
			tagString2 = tagString2.MidString( 1, tagString2.GetLength()-2 );
		}

		// Cut the crap
		tagString2.Trim();

		// Slice into tags
		TDynArray< String > tagList2;
		tagString2.Slice( tagList2, TXT(";") );

		// Trim and convert to CNames
		for ( Uint32 i=0; i<tagList2.Size(); i++ )
		{
			String tagName = tagList2[i];
			tagName.Trim();

			CName tagListCName( tagName.AsChar() );
			if ( tagListCName )
			{
				tagList.PushBack( tagListCName );
			}
		}
	}

	// Create tag list
	SetTags( tagList );
	return true;
}

String TagList::ToString( bool withSpaces /*= true*/ ) const
{
	// Special case - no tags in list
	if ( Empty() )
	{
		return TXT("[No tags]");
	}

	// Assemble single readable string
	String outputString = withSpaces ? TXT("[ ") : TXT("[");

	for ( Uint32 i=0; i < m_tags.Size(); i++ )
	{
		// If it's not first item append separator
		if ( i > 0 )
		{
			outputString += TXT("; ");
		}

		// Append tag name
		outputString += m_tags[i].AsString();		
	}
	outputString += withSpaces ? TXT(" ]") : TXT("]");

	// Return formated string
	return outputString;
}


Bool TagList::MatchAny( const TagList& tagList, const TagList& referenceList )
{
	// Nothing to match, assume matched
	if ( tagList.Empty() )
	{
		return true;
	}

	// Empty target list, not matched
	if ( referenceList.Empty() )
	{
		return false;
	}

	// Try to match any tag from reference list in the given tag list
	for ( Uint32 i=0; i<tagList.m_tags.Size(); i++ )
	{
		CName tag = tagList.m_tags[i];
		if ( referenceList.HasTag( tag ) )
		{
			return true;
		}
	}

	// Not matched
	return false;
}

Bool TagList::MatchAll( const TagList& tagList, const TagList& referenceList )
{
	// Nothing to match, assume matched
	if ( tagList.Empty() )
	{
		return true;
	}

	// Empty target list, not matched
	if ( referenceList.Empty() )
	{
		return false;
	}

	// Try to match all tag from reference list in the given tag list
	for ( Uint32 i=0; i<tagList.m_tags.Size(); i++ )
	{
		CName tag = tagList.m_tags[i];
		if ( !referenceList.HasTag( tag ) )
		{
			// This tag was missing
			return false;
		}
	}

	// All tags were found, matched
	return true;
}

Bool TagList::Match( const CName& reference, const TagList& tagList )
{
	// Empty target list, not matched
	if ( tagList.Empty() )
	{
		return false;
	}

	// Try to match any tag from reference
	return tagList.HasTag( reference );
}

CName TagList::GetTag( Uint32 index ) const
{
	Uint32 size = Size();
	if( size > 0 && size > index )
	{
		return m_tags[index];
	}

	return CName::NONE;
}
