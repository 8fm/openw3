/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "propertyItemEngineTransform.h"
#include "../../common/core/xmlFile.h"

CName CPropertyItemEngineTransform::propTranslationX( TXT("Position X") );
CName CPropertyItemEngineTransform::propTranslationY( TXT("Position Y") );
CName CPropertyItemEngineTransform::propTranslationZ( TXT("Position Z") );
CName CPropertyItemEngineTransform::propRotationPitch( TXT("Pitch") );
CName CPropertyItemEngineTransform::propRotationRoll( TXT("Roll") );
CName CPropertyItemEngineTransform::propRotationYaw( TXT("Yaw") );
CName CPropertyItemEngineTransform::propScaleX( TXT("Scale X") );
CName CPropertyItemEngineTransform::propScaleY( TXT("Scale Y") );
CName CPropertyItemEngineTransform::propScaleZ( TXT("Scale Z") );

/// Group for engine transform
class CPropertyEngineTransformGroupItem : public CBaseGroupItem
{
public:
	TDynArray< CProperty* >		m_properties;	
	String						m_caption;

public:
	CPropertyEngineTransformGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, const TDynArray< CProperty* >& props, const String& caption )
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


CPropertyItemEngineTransform::CPropertyItemEngineTransform( CEdPropertiesPage* page, CBasePropItem* parent )
	: CPropertyItem( page, parent )
{
	m_isExpandable = true;
}

CPropertyItemEngineTransform::~CPropertyItemEngineTransform()
{
	m_dynamicProperties.ClearPtr();
}

CProperty* CPropertyItemEngineTransform::CreateFloatProperty( CName propName, const Char* info )
{
	// Bit filed bits uses boolean type for displaying properties
	IRTTIType* floatType = SRTTI::GetInstance().FindFundamentalType( ::GetTypeName< Float >() );
	CProperty* prop = new CProperty( floatType, NULL, 0, propName, info, PF_Editable );
	m_dynamicProperties.PushBack( prop );
	return prop;
}

void CPropertyItemEngineTransform::CreateControls()
{
	AddButton( m_page->GetStyle().m_iconClear, wxCommandEventHandler( CPropertyItemEngineTransform::OnResetToIdentity ) );
	CPropertyItem::CreateControls();
}

void CPropertyItemEngineTransform::Expand()
{
	// Translation	
	{
		TDynArray< CProperty* > props;
		props.PushBack( CreateFloatProperty( propTranslationX, TXT("Translation in the X direction") ) );
		props.PushBack( CreateFloatProperty( propTranslationY, TXT("Translation in the Y direction") ) );
		props.PushBack( CreateFloatProperty( propTranslationZ, TXT("Translation in the Z direction") ) );
		new CPropertyEngineTransformGroupItem( m_page, this, props, TXT("Position") );
	}

	// Rotation
	{
		TDynArray< CProperty* > props;
		props.PushBack( CreateFloatProperty( propRotationPitch, TXT("Rotation pitch") ) );
		props.PushBack( CreateFloatProperty( propRotationRoll, TXT("Rotation roll") ) );
		props.PushBack( CreateFloatProperty( propRotationYaw, TXT("Rotation yaw") ) );
		new CPropertyEngineTransformGroupItem( m_page, this, props, TXT("Rotation") );
	}

	// Scale
	{
		TDynArray< CProperty* > props;
		props.PushBack( CreateFloatProperty( propScaleX, TXT("Scale in the X direction") ) );
		props.PushBack( CreateFloatProperty( propScaleY, TXT("Scale in the Y direction") ) );
		props.PushBack( CreateFloatProperty( propScaleZ, TXT("Scale in the Z direction") ) );
		new CPropertyEngineTransformGroupItem( m_page, this, props, TXT("Scale") );
	}

	// Redraw
	CPropertyItem::Expand();
}

void CPropertyItemEngineTransform::Collapse()
{
	// Redraw
	CPropertyItem::Collapse();

	// Delete all shit
	m_dynamicProperties.ClearPtr();
}

Bool CPropertyItemEngineTransform::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*=0*/ )
{
	// Load data
	EngineTransform data;
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

		// Pitch
		if ( childItem->GetProperty()->GetName() == propRotationPitch )
		{
			* ( Float* ) buffer = data.GetRotation().Pitch;
			return true;
		}

		// Yaw
		if ( childItem->GetProperty()->GetName() == propRotationYaw )
		{
			* ( Float* ) buffer = data.GetRotation().Yaw;
			return true;
		}

		// Roll
		if ( childItem->GetProperty()->GetName() == propRotationRoll )
		{
			* ( Float* ) buffer = data.GetRotation().Roll;
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

Bool CPropertyItemEngineTransform::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*=0*/ )
{
	// Load data
	EngineTransform data;
	if ( Read( &data, objectIndex ) )
	{
		// Extract
		Vector pos = data.GetPosition();
		EulerAngles rot = data.GetRotation();
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

		// Pitch
		if ( childItem->GetProperty()->GetName() == propRotationPitch )
		{
			rot.Pitch = *( const Float* ) buffer;
		}

		// Yaw
		if ( childItem->GetProperty()->GetName() == propRotationYaw )
		{
			rot.Yaw = *( const Float* ) buffer;
		}

		// Roll
		if ( childItem->GetProperty()->GetName() == propRotationRoll )
		{
			rot.Roll = *( const Float* ) buffer;
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

Bool CPropertyItemEngineTransform::SerializeXML( IXMLFile& file )
{
	EngineTransform data;
	const Uint32 numObjects = GetNumObjects();

	if ( file.IsReader() )
	{
		String value;
		Vector position, scale;
		EulerAngles rotation;

		for ( Uint32 i=0; i<numObjects; ++i )
		{
			if ( !file.BeginNode( String::Printf( TXT("transform%d"), i ) ) )
			{
				return false;
			}

			file.Attribute( TXT("position"), value );
			if ( !FromString( value, position ) )
			{
				return false;
			}
			file.Attribute( TXT("rotation"), value );
			if ( !FromString( value, rotation ) )
			{
				return false;
			}
			file.Attribute( TXT("scale"), value );
			if ( !FromString( value, scale ) )
			{
				return false;
			}

			file.EndNode();

			data.SetPosition( position );
			data.SetRotation( rotation );
			data.SetScale( scale );
			Write( &data, i );
		}
		// Refresh
		GrabPropertyValue();
	}
	else
	{
		for ( Uint32 i=0; i<numObjects; ++i )
		{
			Read( &data, i );
			file.BeginNode( String::Printf( TXT("transform%d"), i ) );
			file.Attribute( TXT("position"), ToString( data.GetPosition() ) );
			file.Attribute( TXT("rotation"), ToString( data.GetRotation() ) );
			file.Attribute( TXT("scale"), ToString( data.GetScale() ) );
			file.EndNode();
		}
	}

	return true;
}

void CPropertyItemEngineTransform::OnResetToIdentity( wxCommandEvent& event )
{
	// Write the shit
	const Uint32 numObjects = GetNumObjects();
	for ( Uint32 i=0; i<numObjects; i++ )
	{
		EngineTransform data;
		data.Identity();
		Write( &data, i );
	}

	// Refresh
	Collapse();
	Expand();
}
