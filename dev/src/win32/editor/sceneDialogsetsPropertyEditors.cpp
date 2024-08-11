/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sceneDialogsetsPropertyEditors.h"
#include "editorExternalResources.h"
#include "sceneDialogsetEditor.h"

#include "../../common/core/depot.h"
#include "../../common/game/actionPoint.h"

CEdDialogsetCameraShotNamePropertyEditor::CEdDialogsetCameraShotNamePropertyEditor( CPropertyItem* propertyItem )
	: ISelectionEditor( propertyItem )
{

}

CEdDialogsetCameraShotNamePropertyEditor::~CEdDialogsetCameraShotNamePropertyEditor()
{

}

void CEdDialogsetCameraShotNamePropertyEditor::FillChoices()
{
	m_ctrlChoice->AppendString( TXT( "Default" ) );
	m_ctrlChoice->AppendString( TXT( "Close Up" ) );
	m_ctrlChoice->AppendString( TXT( "Medium Close Up" ) );
	m_ctrlChoice->AppendString( TXT( "Extreme Close up" ) );
}

CEdDialogsetCameraShotAnimationPropertyEditor::CEdDialogsetCameraShotAnimationPropertyEditor( CPropertyItem* propertyItem )
	: ISelectionEditor( propertyItem )
{

}

CEdDialogsetCameraShotAnimationPropertyEditor::~CEdDialogsetCameraShotAnimationPropertyEditor()
{

}

void CEdDialogsetCameraShotAnimationPropertyEditor::FillChoices()
{
	const CEdSceneDialogsetPropertyPage* dialogsetPropertyPage 
		= static_cast< const CEdSceneDialogsetPropertyPage* >( m_propertyItem->GetPage() );
	
	if ( dialogsetPropertyPage == NULL || dialogsetPropertyPage->GetDialogset() == NULL )
	{
		return;
	}

	Uint32 cameraNumber = dialogsetPropertyPage->GetCameraNumber();

	TDynArray< CName > cameraShotNames;

	if ( cameraNumber > 0 )
	{
		//dialogsetPropertyPage->GetDialogset()->GetCameraShotsNames( cameraNumber, cameraShotNames );
	}
	else
	{
		//dialogsetPropertyPage->GetDialogset()->GetAllCameraShotsNames( cameraShotNames );
	}
	

	m_ctrlChoice->AppendString( CName::NONE.AsString().AsChar() );
	for ( TDynArray< CName >::const_iterator shotNameIter = cameraShotNames.Begin();
		shotNameIter != cameraShotNames.End(); ++shotNameIter )
	{
		m_ctrlChoice->AppendString( shotNameIter->AsString().AsChar() );
	}

}


CEdDialogsetCameraNumberPropertyEditor::CEdDialogsetCameraNumberPropertyEditor( CPropertyItem* propertyItem )
	: INumberRangePropertyEditor( propertyItem )
{

}

CEdDialogsetCameraNumberPropertyEditor::~CEdDialogsetCameraNumberPropertyEditor()
{

}

void CEdDialogsetCameraNumberPropertyEditor::ApplyRange()
{
	const CEdSceneDialogsetPropertyPage* dialogsetPropertyPage 
		= static_cast< const CEdSceneDialogsetPropertyPage* >( m_propertyItem->GetPage() );

	if ( dialogsetPropertyPage == NULL || dialogsetPropertyPage->GetDialogset() == NULL )
	{
		return;
	}

	Uint32 maxRange = dialogsetPropertyPage->GetDialogset()->GetCameraTrajectories().Size();
	if ( maxRange >= 1 )
	{
		m_ctrlSpin->SetRange( 1, maxRange );
	}
	else
	{
		m_ctrlSpin->SetRange( 0, 0 );
	}

}

void CEdDialogsetCameraNumberPropertyEditor::OnSpinChanged( wxCommandEvent& event )
{
	INumberRangePropertyEditor::OnSpinChanged( event );
	
	CEdSceneDialogsetPropertyPage* dialogsetPropertyPage 
		= static_cast< CEdSceneDialogsetPropertyPage* >( m_propertyItem->GetPage() );

	if ( dialogsetPropertyPage == NULL || dialogsetPropertyPage->GetDialogset() == NULL )
	{
		return;
	}

	SScenePersonalCameraDescription* personalCamera = dialogsetPropertyPage->GetPersonalCameraDescription();
	SSceneMasterCameraDescription* masterCamera = dialogsetPropertyPage->GetMasterCameraDescription();
	if ( personalCamera != NULL )
	{
		personalCamera->m_cameraShots.Clear();
		//dialogsetPropertyPage->GetDialogset()->CreateCameraShots( personalCamera->m_cameraNumber, personalCamera->m_cameraShots );
	}
	else if ( masterCamera != NULL )
	{
		masterCamera->m_cameraShots.Clear();
		//dialogsetPropertyPage->GetDialogset()->CreateCameraShots( masterCamera->m_cameraNumber, masterCamera->m_cameraShots );
	}
	
}


CEdDialogsetCharacterNumberPropertyEditor::CEdDialogsetCharacterNumberPropertyEditor( CPropertyItem* propertyItem )
	: INumberRangePropertyEditor( propertyItem )
{

}

CEdDialogsetCharacterNumberPropertyEditor::~CEdDialogsetCharacterNumberPropertyEditor()
{

}

void CEdDialogsetCharacterNumberPropertyEditor::ApplyRange()
{
	const CEdSceneDialogsetPropertyPage* dialogsetPropertyPage 
		= static_cast< const CEdSceneDialogsetPropertyPage* >( m_propertyItem->GetPage() );

	if ( dialogsetPropertyPage == NULL || dialogsetPropertyPage->GetDialogset() == NULL )
	{
		return;
	}
	
	Uint32 maxRange = dialogsetPropertyPage->GetDialogset()->GetNumberOfSlots();
	if ( maxRange >= 1 )
	{
		m_ctrlSpin->SetRange( 0, maxRange );
	}
	else
	{
		m_ctrlSpin->SetRange( 0, 0 );
	}
}

void CEdActionCategoryPropEditor::FillChoices()
{
	const C2dArray& categoriesArray = SActionPointResourcesManager::GetInstance().Reload2dArray();
	// Fill temporary array with voice tags
	for ( Uint32 i=0; i<categoriesArray.GetNumberOfRows(); i++ )
	{
		String catTag = categoriesArray.GetValue( 0, i );
		m_ctrlChoice->AppendString( catTag.AsChar() );
	}
}