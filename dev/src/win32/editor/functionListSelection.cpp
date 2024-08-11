/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "functionListSelection.h"
#include "itemSelectionDialogs/itemSelectorDialogBase.h"

namespace 
{
	class CEdFunctionSelectorDialog : public CEdItemSelectorDialog< CName >
	{
	public:
		CEdFunctionSelectorDialog( wxWindow* parent, TDynArray< CName > functions, CName selectedFunction )
			: CEdItemSelectorDialog( parent, TXT( "/Frames/FunctionSelectorDialog" ), TXT( "Function selector" ) )
			, m_selectedFunction( selectedFunction )
		{
			m_functions = functions;
		}

		~CEdFunctionSelectorDialog() {}

	private:
		CName m_selectedFunction;
		TDynArray< CName > m_functions;

		enum EFunctionIcon
		{
			FunctionIcon_Normal = 0,
			FunctionIcon_Max
		};

	protected:
		virtual void Populate() override
		{
			wxImageList* images = new wxImageList( 16, 16, true, FunctionIcon_Max );
			images->Add( SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_BRICK" ) ) ); // 0
			SetImageList( images );

			for ( auto& functionName : m_functions )
			{
				Bool isSelected = functionName == m_selectedFunction;
				AddItem( functionName.AsString(), &functionName, true, FunctionIcon_Normal, isSelected );
			}
		}
	};
}

CFunctionListSelection::CFunctionListSelection( CPropertyItem* item, FunctionType type )
	: ICustomPropertyEditor( item )
	, m_type( type )
{
	m_propertyItem->Read( &m_functionName );
}

void CFunctionListSelection::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_propertyItem->AddButton( m_propertyItem->GetPage()->GetStyle().m_iconClear, wxCommandEventHandler( CFunctionListSelection::OnClearFunction ), this );
	m_propertyItem->AddButton( m_propertyItem->GetPage()->GetStyle().m_iconNew, wxCommandEventHandler( CFunctionListSelection::OnUseSelected ), this );
}

Bool CFunctionListSelection::SaveValue()
{	
	// Write value
	m_propertyItem->Write( &m_functionName );
	return true;
}

void CFunctionListSelection::OnClearFunction( wxCommandEvent &event )
{
	m_functionName = CName::NONE;
	m_propertyItem->SavePropertyValue();
}

void CFunctionListSelection::OnUseSelected( wxCommandEvent &event )
{
	TDynArray< CFunction* > functions;
	SRTTI::GetInstance().EnumFunctions( functions );
	if ( !functions.Size() )
	{
		return;
	}

	TDynArray< CName > functionNames;
	for ( auto* function : functions )
	{
		if ( function && doesFunctionTypeMatch( function ) )
		{
			functionNames.PushBack( function->GetName() );
		}
	}

	Sort( functionNames.Begin(), functionNames.End() );

	CEdFunctionSelectorDialog functionPicker( nullptr, functionNames, m_functionName );
	if ( CName* selectedFunctionName = functionPicker.Execute() )
	{		
		m_functionName = *selectedFunctionName;
		m_propertyItem->SavePropertyValue();
	}
}

bool CFunctionListSelection::doesFunctionTypeMatch( CFunction* function ) const
{
	if ( !function )
	{
		return false;
	}

	switch ( m_type )
	{
	case FT_SCENE:	return function->IsScene();
	case FT_QUEST:	return function->IsQuest();
	case FT_REWARD:	return function->IsReward();
	default:		return false;
	}
}
