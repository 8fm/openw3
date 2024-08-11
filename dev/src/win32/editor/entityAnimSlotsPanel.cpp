/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityEditor.h"
#include "checkListDlg.h"

#include "../../common/core/depot.h"
#include "../../common/engine/animSlotsParam.h"
#include "../../common/engine/skeletalAnimation.h"

class wxAnimSlotsListClientData : public wxClientData
{
public:
	CAnimationSlots*	m_animSlots;
	Bool				m_included;

public:
	wxAnimSlotsListClientData( CAnimationSlots* slots, Bool included )
		: m_animSlots( slots )
		, m_included( included )
	{}
};

static Bool FillAnimationSlots( CAnimationSlots* slots, CSkeletalAnimation* animation )
{
	if( !slots || !animation )
	{
		return false;
	}

	slots->m_transforms.Clear();
	TDynArray< AnimQsTransform > bones;
	TDynArray< Float > tracks;
	if ( !animation->Sample( 0.f, bones, tracks ) )
	{
		return false;
	}

	if( bones.Size() < 3 )
	{
		return false;
	}
	// Root transform - root is always 0 and not needed by the way...
	AnimQsTransform rootTransform = bones[ 0 ];
	AnimQsTransform offsetTransform = bones[ 1 ];
	AnimQsTransform pivotTransform = bones[ 2 ];
// 	pivotTransform.setIdentity();
// 	hkQuaternion tempRot;
// 	tempRot.setAxisAngle( hkVector4( 0, 1, 0 ), 180 );
// 	pivotTransform.setRotation( tempRot );
// 
// 	offsetTransform.setIdentity();
// 	offsetTransform.setTranslation( hkVector4( 1, 0, 0 ) );
#ifdef USE_HAVOK_ANIMATION
	const hkQuaternion pivotRotation = pivotTransform.getRotation();
	hkQuaternion pivotInversRotation = pivotRotation;
	pivotInversRotation.setReal( hkReal(-1) * pivotRotation.getReal() );
	const hkVector4 pivotTranslation = pivotTransform.getTranslation();

	hkVector4 translation = offsetTransform.getTranslation();
	translation.sub4( pivotTranslation );
	offsetTransform.setTranslation( translation );
	hkQsTransform rotationMat;
	rotationMat.setIdentity();
	rotationMat.setRotation( pivotInversRotation );
	offsetTransform.setMul( rotationMat, offsetTransform );

	Matrix mat;
	HavokTransformToMatrix_Renormalize( offsetTransform, &mat );
	slots->m_transforms.PushBack( mat );

	hkQsTransform identityTransform;
	identityTransform.setIdentity();

	for( Uint32 i = 3; i < bones.Size(); ++i )
	{
		hkQsTransform transform = bones[i];
		if( transform.isApproximatelyEqual( identityTransform ) )
		{
			//Default number of bones is 25. Of course not every bone is used, so assume that the end is when we reach identity matrix.
			break;
		}

		translation = transform.getTranslation();
		translation.sub4( pivotTranslation );
		transform.setTranslation( translation );
		hkQsTransform rotationMat;
		rotationMat.setIdentity();
		rotationMat.setRotation( pivotInversRotation );
		transform.setMul( rotationMat, transform );

		HavokTransformToMatrix_Renormalize( transform, &mat );
		slots->m_transforms.PushBack( mat );
	}
#else
	const RedQuaternion pivotRotation = pivotTransform.GetRotation();
	RedQuaternion pivotInversRotation = pivotRotation;
	pivotInversRotation.SetReal( -1.0f * pivotRotation.GetReal() );
	const RedVector4 pivotTranslation = pivotTransform.GetTranslation();

	RedVector4 translation = offsetTransform.GetTranslation();
	SetSub( translation, pivotTranslation );
	offsetTransform.SetTranslation( translation );
	RedQsTransform rotationMat;
	rotationMat.SetIdentity();
	rotationMat.SetRotation( pivotInversRotation );
	offsetTransform.SetMul( rotationMat, offsetTransform );

	
	RedMatrix4x4 conversionMatrix = offsetTransform.ConvertToMatrixNormalized();
	Matrix mat = reinterpret_cast< const Matrix& >( conversionMatrix );
	slots->m_transforms.PushBack( mat );

	RedQsTransform identityTransform;
	identityTransform.SetIdentity();

	for( Uint32 i = 3; i < bones.Size(); ++i )
	{
		RedQsTransform transform = bones[i];
		if( transform.IsAlmostEqual( identityTransform ) )
		{
			//Default number of bones is 25. Of course not every bone is used, so assume that the end is when we reach identity matrix.
			break;
		}

		translation = transform.GetTranslation();
		SetSub( translation, pivotTranslation );
		transform.SetTranslation( translation );
		RedQsTransform rotationMat;
		rotationMat.SetIdentity();
		rotationMat.SetRotation( pivotInversRotation );
		transform.SetMul( rotationMat, transform );

		conversionMatrix = transform.ConvertToMatrixNormalized();
		mat = reinterpret_cast< const Matrix& >( conversionMatrix );
		slots->m_transforms.PushBack( mat );
	}
#endif
	return true;
}

void CEdEntityEditor::UpdateAnimSlotsList( const CName& animSlotsNameToSelect )
{
	// Clear current lists
	m_listAnimSlots->Freeze();
	m_listAnimSlots->Clear();

	// Gather anim slots
	{
		TDynArray< CAnimSlotsParam* > params;
		m_template->GetAllParameters( params );

		CAnimSlotsParam* thisParam = m_template->FindParameter< CAnimSlotsParam >( false );

		for ( Uint32 i = 0; i < params.Size(); ++i )
		{
			CAnimSlotsParam* currParam = params[ i ];

			const TDynArray< CAnimationSlots* >& slots = currParam->GetAnimationSlots();
			for ( Uint32 j = 0; j < slots.Size(); ++j )
			{
				CAnimationSlots* animSlot = slots[ j ];

				Bool included = false;

				wxString name( animSlot->m_name.AsString().AsChar() );

				if ( thisParam != currParam )
				{
					name += wxT(" (Included)");
					included = true;
				}

				int index = m_listAnimSlots->Append( name, new wxAnimSlotsListClientData( animSlot, included ) );

				if ( animSlot->m_name == animSlotsNameToSelect )
				{
					m_listAnimSlots->SetSelection( index );
				}
			}
		}
	}

	// Finalize list
	if ( m_listAnimSlots->GetCount() )
	{
		m_listAnimSlots->Enable();
	}
	else
	{
		m_listAnimSlots->Append( wxT("(No Animation Slots)") );
		m_listAnimSlots->SetSelection( 0 );
		m_listAnimSlots->Disable();
	}

	// Refresh
	m_listAnimSlots->Thaw();
	m_listAnimSlots->Refresh();

	// Refresh properties
	wxCommandEvent fakeEvent;
	OnAnimSlotsSelectionChanged( fakeEvent );
}

void CEdEntityEditor::OnAnimSlotsModified( wxCommandEvent& event )
{

}

void CEdEntityEditor::OnAnimSlotsSelectionChanged( wxCommandEvent& event )
{
	CAnimationSlots* selectedSlots = NULL;
	Bool included = false;

	// Get selected slot
	Int32 selection = m_listAnimSlots->GetSelection();
	if ( selection != -1 && m_listAnimSlots->HasClientObjectData() )
	{
		wxAnimSlotsListClientData* data = static_cast< wxAnimSlotsListClientData* >( m_listAnimSlots->GetClientObject( selection ) );
		if ( data )
		{
			selectedSlots = data->m_animSlots;
			included = data->m_included;
		}
	}

	// Update properties
	if ( selectedSlots )
	{
		m_animSetsPanelProp->SetObject( selectedSlots );
		m_animSetsPanelProp->Enable( !included );
	}
	else
	{
		m_animSetsPanelProp->Enable( true );
		m_animSetsPanelProp->SetNoObject();
	}
}

void CEdEntityEditor::OnAnimSlotsAdded( wxCommandEvent& event )
{
	if ( !m_template->MarkModified() )
	{
		return;
	}

	// Get supported file formats for animations
	CFileFormat format( CSkeletalAnimationSet::GetFileExtension(), TXT( "Set of animations" ) );
	TDynArray< CFileFormat > animationFileFormats;
	animationFileFormats.PushBack( format );

	// Ask for files
	CEdFileDialog fileDialog;
	fileDialog.AddFormats( animationFileFormats );
	fileDialog.SetMultiselection( false );
	fileDialog.SetIniTag( TXT("Select animation set") );
	if ( fileDialog.DoOpen( (HWND) GetHWND(), true ) )
	{
		String localPath;
		GDepot->ConvertToLocalPath( fileDialog.GetFile(), localPath );
		CSkeletalAnimationSet* animSet = LoadResource< CSkeletalAnimationSet >( localPath );
		if( !animSet )
		{
			wxMessageBox( "Unable to open resource file.", "ERROR", wxOK, this );
			return;
		}

		TDynArray< String >	animationNames;
		TDynArray< Bool >	animationStates;

		const TDynArray< CSkeletalAnimationSetEntry* > animations = animSet->GetAnimations();
		for( TDynArray< CSkeletalAnimationSetEntry* >::const_iterator it = animations.Begin(); it != animations.End(); ++it )
		{
			animationNames.PushBack( (*it)->GetName().AsString() );
			animationStates.PushBack( false );
		}

		CEdCheckListDialog* dlg = new CEdCheckListDialog( this, TXT("Choose animations"), animationNames, animationStates );
		dlg->ShowModal();
		delete dlg;

		CAnimSlotsParam* thisParam = m_template->FindParameter< CAnimSlotsParam >( false );
		if ( !thisParam )
		{
			thisParam = new CAnimSlotsParam();
			m_template->AddParameterUnique( thisParam );
		}

		for( Uint32 i = 0; i < animations.Size(); ++i )
		{
			if( animationStates[i] )
			{
				CAnimationSlots* newSlots = thisParam->AddAnimationSlots( animations[i]->GetName() );
				if( !FillAnimationSlots( newSlots, animations[i]->GetAnimation() ) )
				{
					thisParam->RemoveAnimationSlots( animations[i]->GetName() );

					wxString msg;
					msg.Format( "Error while sampling animation %s. No slots for this animation created.", animations[i]->GetName().AsString().AsChar() );
					wxMessageBox( msg, "ERROR", wxOK, this );
				}
			}
		}

		UpdateAnimSlotsList();
	}
}

void CEdEntityEditor::OnAnimSlotsRemoved( wxCommandEvent& event )
{
	Int32 selection = m_listAnimSlots->GetSelection();
	if ( selection != -1 && m_listAnimSlots->HasClientObjectData() )
	{
		wxAnimSlotsListClientData* data = static_cast< wxAnimSlotsListClientData* >( m_listAnimSlots->GetClientObject( selection ) );
		if ( data )
		{
			CAnimSlotsParam* thisParam = m_template->FindParameter< CAnimSlotsParam >( false );
			if ( thisParam && thisParam->HasAnimSlots( data->m_animSlots->m_name ) )
			{
				if ( !m_template->MarkModified() )
				{
					return;
				}

				thisParam->RemoveAnimationSlots( data->m_animSlots->m_name );

				if ( thisParam->IsEmpty() )
				{
					m_template->RemoveParameter( thisParam );
				}

				UpdateAnimSlotsList();
			}
			else
			{
				// This definition is not from current entity template
				wxMessageBox( wxT("Selected animation slots are not from this template. Cannot remove."), wxT("Remove animation slots"), wxOK | wxICON_ERROR );
			}
		}
	}
}