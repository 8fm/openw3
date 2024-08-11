/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "classSelectorDialog.h"
#include "..\classHierarchyMapper.h"

enum EClassIcon
{
	ClassIcon_Normal = 0,
	ClassIcon_Scripted,

	ClassIcon_Max
};

CEdClassSelectorDialog::CEdClassSelectorDialog( 
	wxWindow* parent, const CClass* baseClass, const CClass* defaultClass, Bool showRoot, 
	const String& configPath, const String& title
	)
	: CEdItemSelectorDialog( parent, configPath, title )
	, m_baseClass( baseClass )
	, m_showRoot( showRoot )
	, m_defaultClass( defaultClass )
{
	ASSERT( m_baseClass != NULL );
}

CEdClassSelectorDialog::~CEdClassSelectorDialog()
{

}

void CEdClassSelectorDialog::Populate()
{
	wxImageList* images = new wxImageList( 16, 16, true, ClassIcon_Max );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_CLASS" ) ) ); // 0
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_CLASS_SCRIPTED" ) ) ); // 1

	SetImageList( images );

	CClass* rootClass = const_cast< CClass* >( m_baseClass );
	
	CClassHierarchyMapper mapper;
	CClassHierarchyMapper::MapHierarchy( rootClass, mapper );

	// The root item is not included in the hierarchy output
	Bool isSelectable = !rootClass->IsAbstract();	
	if ( !rootClass->IsAbstract() && rootClass->IsObject() )
	{
		isSelectable &= rootClass->GetDefaultObject< CObject >()->IsManualCreationAllowed();
	}

	if ( isSelectable == true )
	{
		m_showRoot = true;
	}

	if ( m_showRoot == true )
	{
		AddItem( rootClass->GetName().AsString(), rootClass, isSelectable, ( ( !rootClass->IsNative() )? ClassIcon_Scripted : ClassIcon_Normal ), IsSelectedByDefault( rootClass ) );
	}
	

	for ( Uint32 i = 0; i < mapper.Size(); ++i )
	{
		const Bool isScripted = !mapper[ i ].m_class->IsNative();
		const Int32 icon = isScripted ? ClassIcon_Scripted : ClassIcon_Normal;
		Bool selected = IsSelectedByDefault( mapper[ i ].m_class );

		CClass* mapperClass = mapper[ i ].m_class;

		isSelectable = !mapperClass->IsAbstract();
		if ( !mapperClass->IsAbstract() && mapperClass->IsObject() )
		{
			isSelectable &= mapperClass->GetDefaultObject< CObject >()->IsManualCreationAllowed();
		}

		if ( m_showRoot == false && mapper[ i ].m_parentClass == rootClass )
		{
			AddItem( mapper[ i ].m_className, mapper[ i ].m_class, isSelectable, icon, selected );
		}
		else
		{
			AddItem( mapper[ i ].m_className, mapper[ i ].m_class, mapper[ i ].m_parentClass->GetName().AsString(), isSelectable, icon, selected );
		}
	}


}

Bool CEdClassSelectorDialog::IsSelectedByDefault( const CClass* c ) const
{
	return m_defaultClass != NULL && c == m_defaultClass;
}
