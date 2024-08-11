/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphInstance.h"

#include "behaviorBoneSelection.h"
#include "behaviorBoneSelectionDlg.h"
#include "behaviorProperties.h"
#include "../../common/engine/skeleton.h"

ISkeletonBoneSelection::ISkeletonBoneSelection( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
{	
	ASSERT( propertyItem->GetPropertyType()->GetName() == CNAME( String ) || propertyItem->GetPropertyType()->GetName() == CNAME( CName ) );
	//String temp = propertyItem->GetPropertyType()->GetName().AsString();
	m_icon = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_BONE") );
}

void ISkeletonBoneSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Add editor button
	m_propertyItem->AddButton( m_icon, wxCommandEventHandler( CBehaviorBoneSelection::OnSelectionDialog ), this );
}

void ISkeletonBoneSelection::CloseControls()
{
}

void ISkeletonBoneSelection::ReadStringFromProperty( String& readValue )
{
	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( String ) )
	{
		m_propertyItem->Read( &readValue );
	}
	else if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		CName readValueAsName;
		m_propertyItem->Read( &readValueAsName );
		readValue = readValueAsName.AsString();
	}
}

void ISkeletonBoneSelection::WriteStringToProperty( String& valueToWrite )
{
	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( String ) )
	{
		m_propertyItem->Write( &valueToWrite );
	}
	else if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		CName valueToWriteAsName = CName( valueToWrite );
		m_propertyItem->Write( &valueToWriteAsName );
	}
}

Bool ISkeletonBoneSelection::GrabValue( String& displayData )
{
	ReadStringFromProperty( displayData );
	m_boneChosen = displayData;
	return false;
}

Bool ISkeletonBoneSelection::SaveValue()
{
	WriteStringToProperty( m_boneChosen );
	return true;
}

void ISkeletonBoneSelection::OnSelectionDialog( wxCommandEvent &event )
{
	String currentBoneName;
	ReadStringFromProperty( currentBoneName );

	CSkeleton* skeleton = GetSkeleton();
	//dex++: switched to generalized CSkeleton
	if ( NULL != skeleton )
	{
		TDynArray<String> bones;
		CEdBoneSelectionDialog dlg(m_propertyItem->GetPage(), skeleton, false, false);
		//dex--
		if (!currentBoneName.Empty()) dlg.SelectBones(currentBoneName, false);
		dlg.DoModal();
		dlg.GetSelectedBones(bones);

		if ( bones.Size() > 0 )
		{
			m_boneChosen = bones[0];
			WriteStringToProperty( m_boneChosen );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorBoneSelection::CBehaviorBoneSelection( CPropertyItem* propertyItem )
	: ISkeletonBoneSelection( propertyItem )
{

}

CSkeleton* CBehaviorBoneSelection::GetSkeleton()
{
	IBehaviorGraphBonesPropertyOwner *bonesOwner = dynamic_cast< IBehaviorGraphBonesPropertyOwner* >( m_propertyItem->GetRootObject( 0 ).AsObject() );
	ASSERT(bonesOwner);

	CBehaviorGraphInstance* instance = m_propertyItem->GetPage()->QueryBehaviorEditorProperties()->GetBehaviorGraphInstance();
	ASSERT( instance );

	return bonesOwner->GetBonesSkeleton( instance->GetAnimatedComponentUnsafe() );
}

//////////////////////////////////////////////////////////////////////////

CSkeletonBoneSelection::CSkeletonBoneSelection( CPropertyItem* propertyItem )
	: ISkeletonBoneSelection( propertyItem )
{

}

CSkeleton* CSkeletonBoneSelection::GetSkeleton()
{
	return m_propertyItem->GetRootObject( 0 ).As< CSkeleton >();
}
