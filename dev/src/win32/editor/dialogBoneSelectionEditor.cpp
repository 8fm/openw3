
#include "build.h"
#include "dialogBoneSelectionEditor.h"
#include "propertiesPageComponent.h"
#include "../../common/game/storySceneIncludes.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/core/feedback.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/skeleton.h"

//////////////////////////////////////////////////////////////////////////

CGatheredResource resSceneBodyParts( TXT("gameplay\\globals\\scene_body_parts.csv"), RGF_Startup );

//////////////////////////////////////////////////////////////////////////

CDialogBodyPartSelection_GroupName::CDialogBodyPartSelection_GroupName( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
{
	ASSERT( propertyItem->GetPropertyType()->GetName() == CNAME( String ) );

	m_iconSelect = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_PICK") );
	m_iconSave = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_NEW") );
	m_iconClear = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_CLEAR") );
}

void CDialogBodyPartSelection_GroupName::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_propertyItem->AddButton( m_iconClear, wxCommandEventHandler( CDialogBodyPartSelection_GroupName::OnButtonClear ), this );
	m_propertyItem->AddButton( m_iconSave, wxCommandEventHandler( CDialogBodyPartSelection_GroupName::OnButtonSave ), this );
	m_propertyItem->AddButton( m_iconSelect, wxCommandEventHandler( CDialogBodyPartSelection_GroupName::OnButtonSelect ), this );
}

void CDialogBodyPartSelection_GroupName::CloseControls()
{
}

Bool CDialogBodyPartSelection_GroupName::GrabValue( String& displayData )
{
	m_propertyItem->Read( &displayData );
	m_value = displayData;
	return false;
}

Bool CDialogBodyPartSelection_GroupName::SaveValue()
{
	m_propertyItem->Write( &m_value );
	return true;
}

void CDialogBodyPartSelection_GroupName::OnButtonSave( wxCommandEvent &event )
{
	SavePreset( m_value );
}

void CDialogBodyPartSelection_GroupName::OnButtonClear( wxCommandEvent &event )
{
	if ( YesNo( TXT("Do you want to remove preset '%s'?"), m_value.AsChar() ) )
	{
		if ( RemovePreset( m_value ) )
		{
			SelectPreset( String::EMPTY );
		}
		else
		{
			GFeedback->ShowWarn( TXT("Can not remove preset '%s'."), m_value.AsChar() );
		}
	}
}

void CDialogBodyPartSelection_GroupName::OnButtonSelect( wxCommandEvent &event )
{
	String currentValue;
	m_propertyItem->Read( &currentValue );

	const wxString newWord = wxT("_<new>_");
	const wxString cancelWord = wxT("_<cancel>_");

	wxPoint pos = wxGetMousePosition();
	CEdPoppedUp< wxListBox > ed( NULL, pos, wxDefaultSize, wxLB_SINGLE | wxLB_SORT );
	ed.AppendString( newWord );
	ed.AppendString( cancelWord );

	CollectRecords( ed );

	ed.SetStringSelection( currentValue.AsChar() );

	if ( ed.ShowModal() )
	{
		wxString selectedStr = ed.GetStringSelection();

		if ( selectedStr == newWord )
		{
			if ( YesNo( TXT("Do you want to add new preset with data form 'bones' property?") ) )
			{
				String presetName = m_value;
				if ( InputBox( NULL, TXT("Preset name"), TXT("Write preset name"), presetName ) )
				{
					if ( !SavePreset( presetName ) )
					{
						//...
					}
					else
					{
						SelectPreset( presetName );
					}
				}
			}
		}
		else if ( selectedStr == cancelWord )
		{
			// Nothing
		}
		else
		{
			SelectPreset( selectedStr.wc_str() );
		}
	}
}

IDialogBodyPartOwner* CDialogBodyPartSelection_GroupName::GetOwner()
{
	IDialogBodyPartOwner* owner = dynamic_cast< IDialogBodyPartOwner* >( m_propertyItem->GetRootObject( 0 ).As< CStorySceneEvent >() );
	return owner;
}

const CSkeleton* CDialogBodyPartSelection_GroupName::GetSkeleton()
{
	IDialogBodyPartOwner* owner = GetOwner();
	CEdComponentProperties* cprop = m_propertyItem->GetPage()->QueryComponentProperties();

	if ( owner && cprop && cprop->GetComponent() )
	{
		return owner->GetBodyPartSkeleton( cprop->GetComponent() );
	}

	return nullptr;
}

void CDialogBodyPartSelection_GroupName::SelectPreset( const String& p )
{
	m_value = p;
	m_propertyItem->Write( &m_value );

	IDialogBodyPartOwner* owner = GetOwner();
	const CSkeleton* skeleton = GetSkeleton();

	TDynArray<SBehaviorGraphBoneInfo> newBones;
	if ( skeleton && owner && CollectRecords( skeleton, newBones ) )
	{
		TDynArray<SBehaviorGraphBoneInfo>* bones = owner->GetBodyPartBones();
		if ( bones )
		{
			*bones = newBones;
		}

		owner->OnBodyPartsChanged();
	}

	//m_propertyItem->GetPage()->RefreshValues();
	RunLaterOnce( [ this ](){ m_propertyItem->GetPage()->RefreshValues(); } );
}

Bool CDialogBodyPartSelection_GroupName::CollectRecords( const CSkeleton* skeleton, TDynArray<SBehaviorGraphBoneInfo>& bonesArr ) const
{
	const C2dArray* arr = resSceneBodyParts.LoadAndGet< C2dArray >();
	if ( arr && !m_value.Empty() )
	{
		const Uint32 num = arr->GetNumberOfRows();
		for ( Uint32 i=0; i<num; ++i )
		{
			const String& presetName = arr->GetValueRef( 0, i );
			if ( presetName == m_value )
			{
				const String& bonesStr = arr->GetValueRef( 1, i );

				TDynArray< String > items;
				const String separator = TXT(":");
				items = bonesStr.Split( separator );

				for ( Uint32 itemIt=0; itemIt<items.Size(); ++itemIt )
				{
					String& item = items[ itemIt ];
					if ( !item.Empty() )
					{
						TDynArray< String > itemData;
						const String separatorData = TXT("-");
						itemData = item.Split( separatorData );

						SCENE_ASSERT( itemData.Size() == 2 );

						if ( itemData.Size() == 2 )
						{
							SBehaviorGraphBoneInfo binfo;
							binfo.m_boneName = itemData[ 0 ];
							if ( !FromString( itemData[ 1 ], binfo.m_weight ) )
							{
								continue;
							}
							binfo.m_num = skeleton->FindBoneByName( binfo.m_boneName.AsChar() );
							bonesArr.PushBack( binfo );
						}
					}
				}

				return true;
			}
		}
	}

	return false;
}

void CDialogBodyPartSelection_GroupName::CollectRecords( CEdPoppedUp< wxListBox >& ed ) const
{
	const C2dArray* arr = resSceneBodyParts.LoadAndGet< C2dArray >();
	if ( arr )
	{
		const Uint32 num = arr->GetNumberOfRows();
		for ( Uint32 i=0; i<num; ++i )
		{
			const String& str = arr->GetValueRef( 0, i );
			ed.AppendString( str.AsChar() );
		}
	}
}

Bool CDialogBodyPartSelection_GroupName::RemovePreset( const String& presetName )
{
	C2dArray* arr = resSceneBodyParts.LoadAndGet< C2dArray >();
	if ( arr )
	{
		const Uint32 num = arr->GetNumberOfRows();
		for ( Uint32 i=0; i<num; ++i )
		{
			const String& str = arr->GetValueRef( 0, i );
			if ( str == presetName )
			{
				arr->DeleteRow( i );

				return true;
			}
		}
	}

	return false;
}

Bool CDialogBodyPartSelection_GroupName::SavePreset( const String& presetName )
{
	C2dArray* arr = resSceneBodyParts.LoadAndGet< C2dArray >();
	IDialogBodyPartOwner* owner = GetOwner();
	
	if ( owner && arr )
	{
		const Uint32 num = arr->GetNumberOfRows();
		for ( Uint32 i=0; i<num; ++i )
		{
			const String& str = arr->GetValueRef( 0, i );
			if ( str == presetName )
			{
				if ( YesNo( TXT("Do you want to override preset '%s'?"), presetName.AsChar() ) )
				{
					if ( !RemovePreset( presetName ) )
					{
						GFeedback->ShowWarn( TXT("Can not remove preset '%s'."), presetName.AsChar() );
						return false;
					}
				}
				else
				{
					return false;
				}
			}
		}

		TDynArray<SBehaviorGraphBoneInfo>* bones = owner->GetBodyPartBones();
		if ( !bones )
		{
			return false;
		}

		String bonesStr;
		for ( Uint32 i=0; i<bones->Size(); ++i )
		{
			const SBehaviorGraphBoneInfo& b = (*bones)[i];
			bonesStr += String::Printf( TXT("%s-%1.1f:"), b.m_boneName.AsChar(), b.m_weight );
		}

		if ( !arr->MarkModified() )
		{
			return false;
		}

		arr->AddRow();

		VERIFY( arr->SetValue( presetName, 0, num ) );
		VERIFY( arr->SetValue( bonesStr, 1, num ) );

		VERIFY( arr->Save() );

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

CDialogBodyPartSelection_Bones::CDialogBodyPartSelection_Bones(CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
{	
	const CName typeName = propertyItem->GetPropertyType()->GetName();
	ASSERT( typeName == GetTypeName< TDynArray< SBehaviorGraphBoneInfo > >() );
	m_iconBone = SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_BONE") );
}

void CDialogBodyPartSelection_Bones::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_propertyItem->AddButton( m_iconBone, wxCommandEventHandler( CDialogBodyPartSelection_Bones::OnSelectionDialog ), this );
}

void CDialogBodyPartSelection_Bones::CloseControls()
{
}

Bool CDialogBodyPartSelection_Bones::GrabValue( String& displayData )
{
	return false;
}

Bool CDialogBodyPartSelection_Bones::SaveValue()
{
	return true;
}

void CDialogBodyPartSelection_Bones::OnSelectionDialog( wxCommandEvent &event )
{
	CEdComponentProperties* cprop = m_propertyItem->GetPage()->QueryComponentProperties();
	IDialogBodyPartOwner* owner = dynamic_cast< IDialogBodyPartOwner* >( m_propertyItem->GetRootObject( 0 ).As< CStorySceneEvent >() );

	if ( owner && cprop && cprop->GetComponent() )
	{
		TDynArray<SBehaviorGraphBoneInfo>* bones = owner->GetBodyPartBones();
		const CSkeleton* skeleton = owner->GetBodyPartSkeleton( cprop->GetComponent() );

		if ( bones && skeleton )
		{
			CEdBoneSelectionDialog dlg( m_propertyItem->GetPage(), skeleton, true, true );
			
			for ( Uint32 i=0; i<bones->Size(); ++i )
			{
				dlg.SelectBones( (*bones)[i].m_boneName, false, false );
				dlg.SelectBoneWeight( (*bones)[i].m_boneName, (*bones)[i].m_weight );
			}

			dlg.DoModal();

			bones->Clear();
			TDynArray<Int32> newBonesNum;
			dlg.GetSelectedBones(newBonesNum);
			TDynArray<String> newBonesName;
			dlg.GetSelectedBones(newBonesName);

			ASSERT( newBonesNum.Size() == newBonesName.Size() );

			TDynArray<Float> newBonesWeight;
			dlg.GetBonesWeight(newBonesWeight);
			ASSERT( newBonesWeight.Size() == newBonesName.Size() );

			for (Uint32 i=0; i<newBonesNum.Size(); i++)
			{
				SBehaviorGraphBoneInfo newBoneInfo;
				newBoneInfo.m_boneName = newBonesName[i];
				newBoneInfo.m_num = newBonesNum[i];
				newBoneInfo.m_weight = newBonesWeight[i];
				bones->PushBack(newBoneInfo);
			}

			owner->OnBodyPartsChanged();

			m_propertyItem->GetPage()->RefreshValues();
		}
	}
}
