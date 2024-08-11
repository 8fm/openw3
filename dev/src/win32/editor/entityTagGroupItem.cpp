/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityTagGroupItem.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/xmlReader.h"
#include "../../common/core/depot.h"

//-----

CGatheredResource resEntityTagDefinitions( TXT("gameplay\\globals\\editor_entity_tags.xml"), RGF_NotCooked );

CEntityTagDefinitions::CEntityTagDefinitions()
{
	Red::TScopedPtr< CXMLReader > xml( GDepot->LoadXML( resEntityTagDefinitions.GetPath().ToString() ) );
	if ( xml )
	{
		if ( xml->BeginNode( TXT("EntityTags") ) )
		{
			while ( xml->BeginNode( TXT("Tag") ) )
			{
				String entityClassName = TXT("CEntity");
				xml->Attribute( TXT("class"), entityClassName );

				String name, value;
				xml->Attribute( TXT("name"), name );
				xml->Attribute( TXT("value"), value );

				if ( !name.Empty() && !value.Empty() )
				{
					TagInfo info;
					info.m_class = CName( entityClassName.AsChar() );
					info.m_tag = CName( value.AsChar() );
					info.m_name = CName( name.AsChar() );
					m_tags.PushBack( info );
				}

				xml->EndNode();
			}

			xml->EndNode();
		}
	}
}

CEntityTagDefinitions::~CEntityTagDefinitions()
{
}

void CEntityTagDefinitions::GetTags( const CClass* entityClass, TDynArray< CEntityTagDefinitions::TagInfo >& outTags ) const
{
	for ( const auto& it : m_tags )
	{
		const CClass* filterClass = SRTTI::GetInstance().FindClass( it.m_class );
		if ( filterClass && entityClass->IsA( filterClass ) )
		{
			outTags.PushBack( it );
		}
	}
}

//-----

CEntityTagGroupItem::CEntityTagGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, const CClass* entityClass )
	: CBaseGroupItem( page, parent )
	, m_entityClass( entityClass )
{
	// autoexpand
	m_isExpandable = true;
	Expand();
}

String CEntityTagGroupItem::GetCaption() const
{
	return TXT("Entity tags");
}

void CEntityTagGroupItem::Collapse()
{
	// Pass to base class
	CBaseGroupItem::Collapse();

	// Destroy fake properties 
	for ( auto& it : m_fakeProperties )
		delete it.m_prop;
	m_fakeProperties.Clear();
}

void CEntityTagGroupItem::Expand()
{
	TDynArray< CEntityTagDefinitions::TagInfo > tags;
	SEntityTagDefinitions::GetInstance().GetTags( m_entityClass, tags );

	for ( const auto& tagInfo : tags )
	{
		// Create fake property of matching type and name
		CProperty* fakeProperty = new CProperty( GetTypeObject< Bool >(), NULL, 0, tagInfo.m_name, String::EMPTY, PF_Editable );
		if ( fakeProperty )
		{
			// Remember the fake property
			FakeProp fakeProp;
			fakeProp.m_prop = fakeProperty;
			fakeProp.m_tagName = tagInfo.m_tag;
			m_fakeProperties.PushBack( fakeProp );

			// Create expandable property
			if ( CPropertyItem* item = CreatePropertyItem( m_page, this, fakeProperty ) )
			{
				item->GrabPropertyValue();
			}
		}
	}

	CBasePropItem::Expand();
}

CName CEntityTagGroupItem::GetTagName( const CProperty* prop ) const
{
	for ( Uint32 i=0; i<m_fakeProperties.Size(); ++i )
	{
		if ( m_fakeProperties[i].m_prop == prop )
		{
			return m_fakeProperties[i].m_tagName;
		}
	}

	return CName::NONE;
}

Bool CEntityTagGroupItem::ReadProperty( CProperty *property, void* buffer, Int32 objectIndex /*= 0*/ )
{
	if ( objectIndex == -1 )
	{
		return false;
	}

	// Check for invalid object index
	if ( objectIndex < 0 || (Int32)objectIndex >= GetNumObjects() )
	{
		return false;
	}

	// Get base object - must be an entity
	const auto& entityRoot = GetParentObject( objectIndex );
	if ( !entityRoot.m_object || !entityRoot.m_class->IsA< CEntity >() )
	{
		return false;
	}

	// Find the tag matched with this prop
	const CName tagName = GetTagName( property );
	if ( !tagName )
	{
		return false;
	}

	// Read entity tags
	const TagList& tags = static_cast< CEntity* >( entityRoot.AsObject() )->GetTags();
	const Bool hasTag = tags.HasTag( tagName );

	// Write the result
	*(Bool*) buffer = hasTag;
	return true;
}

Bool CEntityTagGroupItem::WriteProperty( CProperty *property, void* buffer, Int32 objectIndex /*= 0*/ )
{
	if ( objectIndex == -1 )
	{
		return false;
	}

	// Check for invalid object index
	if ( objectIndex < 0 || (Int32)objectIndex >= GetNumObjects() )
	{
		return false;
	}

	// Get base object - must be an entity
	const auto& entityRoot = GetParentObject( objectIndex );
	if ( !entityRoot.m_object || !entityRoot.m_class->IsA< CEntity >() )
	{
		return false;
	}

	// Find the tag matched with this prop
	const CName tagName = GetTagName( property );
	if ( !tagName )
	{
		return false;
	}

	// the the "tags" property because it's the one we are actually modifying, it's a hack but a usefull one
	CEntity* entity = static_cast< CEntity* >( entityRoot.AsObject() );
	CProperty* tagsProp = entity->GetClass()->FindProperty( CName( TXT("tags") ) );
	if ( !tagsProp )
	{
		return false;
	}

	// Modify entity tags
	const Bool hasTag = *( const Bool* ) buffer;
	if ( hasTag )
	{
		TagList tags = entity->GetTags();
		if ( !tags.HasTag( tagName ) )
		{
			m_page->PropertyPreChange( tagsProp, entityRoot );
			tags.AddTag( tagName );
			entity->SetTags( tags );
			m_page->PropertyPostChange( tagsProp, entityRoot );
		}
	}
	else
	{
		TagList tags = entity->GetTags();
		if ( tags.HasTag( tagName ) )
		{
			m_page->PropertyPreChange( tagsProp, entityRoot );
			tags.SubtractTag( tagName );
			entity->SetTags( tags );
			m_page->PropertyPostChange( tagsProp, entityRoot );
		}
	}

	// updated
	return true;
}

Bool CEntityTagGroupItem::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*=0*/ )
{
	ASSERT( objectIndex == 0 || objectIndex == -1 );
	ASSERT( childItem->GetProperty() );

	return ReadProperty( childItem->GetProperty(), buffer, objectIndex );
}

Bool CEntityTagGroupItem::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*=0*/ )
{
	ASSERT( objectIndex == 0 );
	ASSERT( childItem->GetProperty() );

	return WriteProperty( childItem->GetProperty(), buffer, objectIndex );
}