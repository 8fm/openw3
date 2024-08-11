/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphInstance.h"

#include "behaviorBoneMultiSelection.h"
#include "behaviorBoneSelectionDlg.h"
#include "behaviorProperties.h"

CBehaviorBoneMultiSelection::CBehaviorBoneMultiSelection(CPropertyItem* propertyItem, Bool withWeight /* = false  */)
	: ICustomPropertyEditor( propertyItem )
	, m_withWeight(withWeight)
{	
	// Conversion to String because of @ sign
	CName typeName = propertyItem->GetPropertyType()->GetName();
	ASSERT( typeName == GetTypeName< TDynArray< SBehaviorGraphBoneInfo > >() );
	m_icon = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_BONE") );
}

void CBehaviorBoneMultiSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Add editor button
	m_propertyItem->AddButton( m_icon, wxCommandEventHandler( CBehaviorBoneMultiSelection::OnSelectionDialog ), this );
}

void CBehaviorBoneMultiSelection::CloseControls()
{
}

Bool CBehaviorBoneMultiSelection::GrabValue( String& displayData )
{
	//m_propertyItem->Read( &displayData );
	//m_boneChosen = displayData;
	return false;
}

Bool CBehaviorBoneMultiSelection::SaveValue()
{
	return true;
}

void CBehaviorBoneMultiSelection::OnSelectionDialog( wxCommandEvent &event )
{
	IBehaviorGraphBonesPropertyOwner *bonesOwner = dynamic_cast< IBehaviorGraphBonesPropertyOwner* >( m_propertyItem->GetRootObject( 0 ).AsObject() );
	ASSERT(bonesOwner);

	CBehaviorGraphInstance* instance = m_propertyItem->GetPage()->QueryBehaviorEditorProperties()->GetBehaviorGraphInstance();
	ASSERT( instance );

	TDynArray<SBehaviorGraphBoneInfo>* bones = bonesOwner->GetBonesProperty();
	const CSkeleton* skeleton = bonesOwner->GetBonesSkeleton( instance->GetAnimatedComponentUnsafe() );

	//dex++: switched to generalized CSkeleton interface
	if ( bones && skeleton )
	{
		CEdBoneSelectionDialog dlg(m_propertyItem->GetPage(), skeleton, true, m_withWeight);
		//dex--

		for (Uint32 i=0; i<bones->Size(); i++)
		{
			dlg.SelectBones((*bones)[i].m_boneName, false, false);
			if (m_withWeight) dlg.SelectBoneWeight((*bones)[i].m_boneName, (*bones)[i].m_weight);
		}

		dlg.DoModal();

		bones->Clear();
		TDynArray<Int32> newBonesNum;
		dlg.GetSelectedBones(newBonesNum);
		TDynArray<String> newBonesName;
		dlg.GetSelectedBones(newBonesName);

		ASSERT( newBonesNum.Size() == newBonesName.Size() );

		TDynArray<Float> newBonesWeight;
		if (m_withWeight)
		{
			dlg.GetBonesWeight(newBonesWeight);
			ASSERT( newBonesWeight.Size() == newBonesName.Size() );
		}

		for (Uint32 i=0; i<newBonesNum.Size(); i++)
		{
			SBehaviorGraphBoneInfo newBoneInfo;
			newBoneInfo.m_boneName = newBonesName[i];
			newBoneInfo.m_num = newBonesNum[i];
			if (m_withWeight) newBoneInfo.m_weight = newBonesWeight[i];
			bones->PushBack(newBoneInfo);
		}

		m_propertyItem->GetPage()->RefreshValues();
	}
}
