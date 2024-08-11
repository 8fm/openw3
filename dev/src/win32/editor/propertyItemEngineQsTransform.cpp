/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "propertyItemEngineQsTransform.h"
#include "../../common/core/engineQsTransform.h"

CName CPropertyItemEngineQsTransform::propTranslationX( TXT("Position X") );
CName CPropertyItemEngineQsTransform::propTranslationY( TXT("Position Y") );
CName CPropertyItemEngineQsTransform::propTranslationZ( TXT("Position Z") );
CName CPropertyItemEngineQsTransform::propRotationX( TXT("Rotation X") );
CName CPropertyItemEngineQsTransform::propRotationY( TXT("Rotation Y") );
CName CPropertyItemEngineQsTransform::propRotationZ( TXT("Rotation Z") );
CName CPropertyItemEngineQsTransform::propRotationW( TXT("Rotation W") );
CName CPropertyItemEngineQsTransform::propScaleX( TXT("Scale X") );
CName CPropertyItemEngineQsTransform::propScaleY( TXT("Scale Y") );
CName CPropertyItemEngineQsTransform::propScaleZ( TXT("Scale Z") );

/// Group for engine transform
class CPropertyEngineQsTransformGroupItem : public CBaseGroupItem
{
public:
	TDynArray< CProperty* >		m_properties;	
	String						m_caption;

public:
	CPropertyEngineQsTransformGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, const TDynArray< CProperty* >& props, const String& caption )
		: CBaseGroupItem( page, parent )
		, m_properties( props )
		, m_caption( caption )
	{
		m_isExpandable = true;
		Expand();
	};

	virtual String GetCaption() const
	{
		return m_caption;
	}

	virtual void Expand()
	{
		for ( Uint32 i=0; i<m_properties.Size(); i++ )
		{
			// Create and update property
			if ( CPropertyItem* item = CreatePropertyItem( m_page, this, m_properties[i] ) )
			{
				item->GrabPropertyValue();
			}
		}

		CBasePropItem::Expand();
	}
};


CPropertyItemEngineQsTransform::CPropertyItemEngineQsTransform( CEdPropertiesPage* page, CBasePropItem* parent )
	: CPropertyItem( page, parent )
{
	m_isExpandable = true;
}

CPropertyItemEngineQsTransform::~CPropertyItemEngineQsTransform()
{
	m_dynamicProperties.ClearPtr();
}

CProperty* CPropertyItemEngineQsTransform::CreateFloatProperty( CName propName, const Char* info )
{
	// Bit filed bits uses boolean type for displaying properties
	IRTTIType* floatType = SRTTI::GetInstance().FindFundamentalType( ::GetTypeName< Float >() );
	CProperty* prop = new CProperty( floatType, NULL, 0, propName, info, PF_Editable );
	m_dynamicProperties.PushBack( prop );
	return prop;
}

void CPropertyItemEngineQsTransform::CreateControls()
{
	AddButton( m_page->GetStyle().m_iconClear, wxCommandEventHandler( CPropertyItemEngineQsTransform::OnResetToIdentity ) );
	CPropertyItem::CreateControls();
}

void CPropertyItemEngineQsTransform::Expand()
{
	// Translation	
	{
		TDynArray< CProperty* > props;
		props.PushBack( CreateFloatProperty( propTranslationX, TXT("Translation in the X direction") ) );
		props.PushBack( CreateFloatProperty( propTranslationY, TXT("Translation in the Y direction") ) );
		props.PushBack( CreateFloatProperty( propTranslationZ, TXT("Translation in the Z direction") ) );
		new CPropertyEngineQsTransformGroupItem( m_page, this, props, TXT("Position") );
	}

	// Rotation
	{
		TDynArray< CProperty* > props;
		props.PushBack( CreateFloatProperty( propRotationX, TXT("Rotation X") ) );
		props.PushBack( CreateFloatProperty( propRotationY, TXT("Rotation Y") ) );
		props.PushBack( CreateFloatProperty( propRotationZ, TXT("Rotation Z") ) );
		props.PushBack( CreateFloatProperty( propRotationW, TXT("Rotation W") ) );
		new CPropertyEngineQsTransformGroupItem( m_page, this, props, TXT("Rotation") );
	}

	// Scale
	{
		TDynArray< CProperty* > props;
		props.PushBack( CreateFloatProperty( propScaleX, TXT("Scale in the X direction") ) );
		props.PushBack( CreateFloatProperty( propScaleY, TXT("Scale in the Y direction") ) );
		props.PushBack( CreateFloatProperty( propScaleZ, TXT("Scale in the Z direction") ) );
		new CPropertyEngineQsTransformGroupItem( m_page, this, props, TXT("Scale") );
	}

	// Redraw
	CPropertyItem::Expand();
}

void CPropertyItemEngineQsTransform::Collapse()
{
	// Redraw
	CPropertyItem::Collapse();

	// Delete all shit
	m_dynamicProperties.ClearPtr();
}

Bool CPropertyItemEngineQsTransform::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex )
{
	// Load data
	EngineQsTransform data;
	if ( Read( &data, objectIndex ) )
	{
		// X translation
		if ( childItem->GetProperty()->GetName() == propTranslationX )
		{
			* ( Float* ) buffer = data.GetPosition().X;
			return true;
		}

		// Y translation
		if ( childItem->GetProperty()->GetName() == propTranslationY )
		{
			* ( Float* ) buffer = data.GetPosition().Y;
			return true;
		}

		// Z translation
		if ( childItem->GetProperty()->GetName() == propTranslationZ )
		{
			* ( Float* ) buffer = data.GetPosition().Z;
			return true;
		}

		// Quat X
		if ( childItem->GetProperty()->GetName() == propRotationX )
		{
			* ( Float* ) buffer = data.GetRotation().X;
			return true;
		}

		// Quat Y
		if ( childItem->GetProperty()->GetName() == propRotationY )
		{
			* ( Float* ) buffer = data.GetRotation().Y;
			return true;
		}

		// Quat Z
		if ( childItem->GetProperty()->GetName() == propRotationZ )
		{
			* ( Float* ) buffer = data.GetRotation().Z;
			return true;
		}

		// Quat W
		if ( childItem->GetProperty()->GetName() == propRotationW )
		{
			* ( Float* ) buffer = data.GetRotation().W;
			return true;
		}

		// X scale
		if ( childItem->GetProperty()->GetName() == propScaleX )
		{
			* ( Float* ) buffer = data.GetScale().X;
			return true;
		}

		// Y scale
		if ( childItem->GetProperty()->GetName() == propScaleY )
		{
			* ( Float* ) buffer = data.GetScale().Y;
			return true;
		}

		// Z scale
		if ( childItem->GetProperty()->GetName() == propScaleZ )
		{
			* ( Float* ) buffer = data.GetScale().Z;
			return true;
		}
	}

	// Not loaded
	return false;
}

Bool CPropertyItemEngineQsTransform::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex )
{
	// Load data
	EngineQsTransform data;
	if ( Read( &data, objectIndex ) )
	{
		// Extract
		Vector pos = data.GetPosition();
		Vector rot = data.GetRotation();
		Vector scale = data.GetScale();

		// X translation
		if ( childItem->GetProperty()->GetName() == propTranslationX )
		{
			pos.SetX( *( const Float* ) buffer );
		}

		// Y translation
		if ( childItem->GetProperty()->GetName() == propTranslationY )
		{
			pos.SetY( *( const Float* ) buffer );
		}

		// Z translation
		if ( childItem->GetProperty()->GetName() == propTranslationZ )
		{
			pos.SetZ( *( const Float* ) buffer );
		}

		// Quat X
		if ( childItem->GetProperty()->GetName() == propRotationX )
		{
			rot.X = *( const Float* ) buffer;
		}

		// Quat Y
		if ( childItem->GetProperty()->GetName() == propRotationY )
		{
			rot.Y = *( const Float* ) buffer;
		}

		// Quat Z
		if ( childItem->GetProperty()->GetName() == propRotationZ )
		{
			rot.Z = *( const Float* ) buffer;
		}

		// Quat W
		if ( childItem->GetProperty()->GetName() == propRotationW )
		{
			rot.W = *( const Float* ) buffer;
		}

		// X scale
		if ( childItem->GetProperty()->GetName() == propScaleX )
		{
			scale.SetX( *( const Float* ) buffer );
		}

		// Y scale
		if ( childItem->GetProperty()->GetName() == propScaleY )
		{
			scale.SetY( *( const Float* ) buffer );
		}

		// Z scale
		if ( childItem->GetProperty()->GetName() == propScaleZ )
		{
			scale.SetZ( *( const Float* ) buffer );
		}

		// Update transform
		data.Init( pos, rot, scale );

		// Write back
		if ( !Write( &data, objectIndex ) )
		{
			return false;
		}

		// Refresh
		GrabPropertyValue();
		return true;
	}

	// Not stored
	return false;
}

Bool CPropertyItemEngineQsTransform::IsReadOnly() const
{
	return true;
}

Bool CPropertyItemEngineQsTransform::SerializeXML( IXMLFile& file )
{
	return false;
}

void CPropertyItemEngineQsTransform::OnResetToIdentity( wxCommandEvent& event )
{
	// Write the shit
	const Uint32 numObjects = GetNumObjects();
	for ( Uint32 i=0; i<numObjects; i++ )
	{
		EngineQsTransform data;
		data.Identity();
		Write( &data, i );
	}

	// Refresh
	Collapse();
	Expand();
}
