/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "physicalCollisionSelector.h"
#include "2dArrayEditor.h"

#include "../../common/core/depot.h"

CEdPhysicalCollisionTypeSelector::CEdPhysicalCollisionTypeSelector( CPropertyItem* item )
	: CListBoxEditor( item )
#ifdef USE_COLLISION_GROUP_RESOURCE_EDITOR
	, m_collisionGroupResourceEditor( 0 )
#endif
{
	SEvents::GetInstance().RegisterListener( CNAME( CSVFileSaved ), this );
}

CEdPhysicalCollisionTypeSelector::~CEdPhysicalCollisionTypeSelector(void)
{
#ifdef USE_COLLISION_GROUP_RESOURCE_EDITOR
	if( m_collisionGroupResourceEditor )
	{
		m_collisionGroupResourceEditor->Close();
		m_collisionGroupResourceEditor = 0;
	}
#endif
	SEvents::GetInstance().UnregisterListener( this );
}

Bool CEdPhysicalCollisionTypeSelector::GrabValue( String& displayValue )
{
	CPhysicalCollision physicalCollision;
	m_propertyItem->Read( &physicalCollision );

	const TDynArray< CName >& names = physicalCollision.m_collisionTypeNames;

#ifdef USE_COLLISION_GROUP_RESOURCE_EDITOR
	UpdateArrayEditorColours();
#endif

	if( names.Empty() )
	{
		displayValue += TXT( "COLLIDE WITH ALL" );
		return true;
	}

	displayValue.Clear();

	for( Uint32 i = 0; i != names.Size(); ++i )
	{
		displayValue += names[ i ].AsString();
		displayValue += TXT( " " );
	}

	return true;
}

wxArrayString CEdPhysicalCollisionTypeSelector::GetListElements()
{
	wxArrayString elements;

	const TDynArray< CName >& names = GPhysicEngine->GetCollsionTypeNames();

	for( Uint32 i = 0; i != names.Size(); ++i )
	{
		elements.push_back( names[ i ].AsString().AsChar() );
	}

	return elements;

}

void CEdPhysicalCollisionTypeSelector::SelectPropertyElements()
{
	CPhysicalCollision physicalCollision;
	m_propertyItem->Read( &physicalCollision );

	const TDynArray< CName >& names = physicalCollision.m_collisionTypeNames;
	for ( Uint32 j=0; j<names.Size(); j++ )
	{
		Int32 result = m_listBoxCtrl->FindString( names[ j ].AsString().AsChar() );
		if( result >= 0 )
		{
			m_listBoxCtrl->Check( result );
		}
	}
}

void CEdPhysicalCollisionTypeSelector::SelectElement( wxString element )
{
	CPhysicalCollision physicalCollision;
	m_propertyItem->Read( &physicalCollision );

	CName name( element );
	TDynArray< CName >& names = physicalCollision.m_collisionTypeNames;
	for( Uint32 i = 0; i != names.Size(); ++i )
		if( names[ i ] == name )
			return;

	names.PushBack( name );

	RunLaterOnce( [=]{
		for ( Int32 i=0; i<m_propertyItem->GetNumObjects(); ++i )
		{
			m_propertyItem->Write( (void*)&physicalCollision, i );
		}
	} );
}

void CEdPhysicalCollisionTypeSelector::DeselectElement( wxString element )
{
	CPhysicalCollision physicalCollision;
	m_propertyItem->Read( &physicalCollision );

	CName name( element );
	TDynArray< CName >& names = physicalCollision.m_collisionTypeNames;
	for( Uint32 i = 0; i != names.Size(); ++i )
		if( names[ i ] == name )
		{
			names.RemoveAt( i );
			break;
		}

	RunLaterOnce( [=]{
		m_propertyItem->Write( (void*)&physicalCollision );
	} );
}

#ifdef USE_COLLISION_GROUP_RESOURCE_EDITOR

void CEdPhysicalCollisionTypeSelector::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	__super::CreateControls( propertyRect, outSpawnedControls );

	if( m_collisionGroupResourceEditor ) return;

	CResource* res = GDepot->FindResource( GPhysicEngine->GetCollisionMaskFileResourceName() );
 	m_collisionGroupResourceEditor = new CEd2dArrayEditor( 0, SafeCast< C2dArray >( res ) );
	m_collisionGroupResourceEditor->Bind( wxEVT_DESTROY, &CEdPhysicalCollisionTypeSelector::On2dArrayEditorClose, this );
 	m_collisionGroupResourceEditor->Show();
 	m_collisionGroupResourceEditor->Raise();
}

void CEdPhysicalCollisionTypeSelector::On2dArrayEditorClose( wxWindowDestroyEvent& event )
{
	if ( m_collisionGroupResourceEditor == event.GetEventObject() )
	{
		m_collisionGroupResourceEditor = nullptr;
	}
}

void CEdPhysicalCollisionTypeSelector::CloseControls()
{
	if( m_collisionGroupResourceEditor )
	{
		m_collisionGroupResourceEditor->Close();
		m_collisionGroupResourceEditor = nullptr;
	}
	__super::CloseControls();
}

void CEdPhysicalCollisionTypeSelector::UpdateArrayEditorColours()
{
	if( !m_collisionGroupResourceEditor ) return;

	CPhysicalCollision physicalCollision;
	m_propertyItem->Read( &physicalCollision );

	const TDynArray< CName >& names = physicalCollision.m_collisionTypeNames;

	CPhysicsEngine::CollisionMask typeMask = GPhysicEngine->GetCollisionTypeBit( names );
	CPhysicsEngine::CollisionMask groupMask = GPhysicEngine->GetCollisionGroupMask( typeMask );
	for( unsigned char i = 0; i != m_collisionGroupResourceEditor->GetRowsNumber(); ++i )
	{
		m_collisionGroupResourceEditor->SetCellColor( wxSheetCoords( i, 0 ), ( ( ( Uint64 )( 1 << ( i + 1 ) ) ) & groupMask ) ? wxColor( 0, 255, 0 ) : wxColor( 255, 255, 255 ) );
	}

}

#endif

void CEdPhysicalCollisionTypeSelector::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( CSVFileSaved ) )
	{
		if( GPhysicEngine->GetCollisionMaskFileResourceName().ContainsSubstring( GetEventData< String >( data ) ) )
		{
#ifdef USE_COLLISION_GROUP_RESOURCE_EDITOR
			UpdateArrayEditorColours();
#endif
			if( m_listBoxCtrl )
			{
				CEdPropertiesPage* parent = ( CEdPropertiesPage* ) m_listBoxCtrl->GetParent();
				parent->SetFocus();
				parent->SelectItem( parent->GetActiveItem() );
				parent->Refresh( false );
			}
		}
	}
}


CEdPhysicalCollisionGroupSelector::CEdPhysicalCollisionGroupSelector( CPropertyItem* item )
	: CListBoxEditor( item )
{
	SEvents::GetInstance().RegisterListener( CNAME( CSVFileSaved ), this );
}

CEdPhysicalCollisionGroupSelector::~CEdPhysicalCollisionGroupSelector(void)
{
	SEvents::GetInstance().UnregisterListener( this );
}

Bool CEdPhysicalCollisionGroupSelector::GrabValue( String& displayValue )
{
	TDynArray< CName > collisionGroups;
	m_propertyItem->Read( &collisionGroups );

	if( collisionGroups.Empty() )
	{
		displayValue += TXT( "COLLIDE WITH ALL" );
		return true;
	}

	displayValue.Clear();

	for( Uint32 i = 0; i != collisionGroups.Size(); ++i )
	{
		displayValue += collisionGroups[ i ].AsString();
		displayValue += TXT( " " );
	}

	return true;
}

wxArrayString CEdPhysicalCollisionGroupSelector::GetListElements()
{
	wxArrayString elements;

	const TDynArray< CName >& names = GPhysicEngine->GetCollsionTypeNames();

	for( Uint32 i = 0; i != names.Size(); ++i )
	{
		elements.push_back( names[ i ].AsString().AsChar() );
	}

	return elements;

}

void CEdPhysicalCollisionGroupSelector::SelectPropertyElements()
{
	TDynArray< CName > collisionGroups;
	m_propertyItem->Read( &collisionGroups );

	for ( Uint32 j=0; j<collisionGroups.Size(); j++ )
	{
		wxString string( collisionGroups[ j ].AsString().AsChar() );
		Int32 result = m_listBoxCtrl->FindString( string );
		if( result >= 0 )
		{
			m_listBoxCtrl->Check( result );
		}
	}
}

void CEdPhysicalCollisionGroupSelector::SelectElement( wxString element )
{
	TDynArray< CName > collisionGroups;
	m_propertyItem->Read( &collisionGroups );

	CName name( element );
	for( Uint32 i = 0; i != collisionGroups.Size(); ++i )
		if( collisionGroups[ i ] == name )
			return;

	collisionGroups.PushBack( name );

	RunLaterOnce( [=]{
		m_propertyItem->Write( (void*)&collisionGroups );
	} );
}

void CEdPhysicalCollisionGroupSelector::DeselectElement( wxString element )
{
	TDynArray< CName > collisionGroups;
	m_propertyItem->Read( &collisionGroups );

	CName name( element );
	for( Uint32 i = 0; i != collisionGroups.Size(); ++i )
		if( collisionGroups[ i ] == name )
		{
			collisionGroups.RemoveAt( i );
			break;
		}

	RunLaterOnce( [=]{ 
		m_propertyItem->Write( (void*)&collisionGroups );
	} );
}


void CEdPhysicalCollisionGroupSelector::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( CSVFileSaved ) )
	{
		if( GPhysicEngine->GetCollisionMaskFileResourceName().ContainsSubstring( GetEventData< String >( data ) ) )
		{
			if( m_listBoxCtrl )
			{
				CEdPropertiesPage* parent = ( CEdPropertiesPage* ) m_listBoxCtrl->GetParent();
				parent->SetFocus();
				parent->SelectItem( parent->GetActiveItem() );
				parent->Refresh( false );
			}
		}
	}
}

