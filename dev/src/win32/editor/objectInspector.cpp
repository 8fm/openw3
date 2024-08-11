/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "objectInspector.h"
#include "..\..\common\engine\component.h"
#include "..\..\common\engine\mesh.h"
#include "..\..\common\engine\umbraScene.h"
#include "..\..\common\engine\umbraStructures.h"
#include "..\..\common\engine\meshComponent.h"
#include "..\..\common\engine\decalComponent.h"
#include "..\..\common\engine\stripeComponent.h"
#include "..\..\common\engine\dimmerComponent.h"
#include "..\..\common\engine\lightComponent.h"
#include "..\..\common\engine\bitmapTexture.h"

BEGIN_EVENT_TABLE( CEdObjectInspector, wxFrame )
END_EVENT_TABLE()

#define UPDATE_FLAG_CHECKBOX_EVENT(cbox,flg)        \
	if ( GetInspectedObject() != nullptr )		    \
	{                                               \
		if ( cbox->GetValue() )					    \
		{										    \
			GetInspectedObject()->SetFlag( flg );   \
		}										    \
		else                                        \
		{                                           \
			GetInspectedObject()->ClearFlag( flg ); \
		}											\
	}
	
CEdObjectInspector::CEdObjectInspector()
{
	// Load dialog resource
	wxXmlResource::Get()->LoadFrame( this, NULL, wxT("ObjectInspector") );

	// Get controls from XRC
	m_refresh = XRCCTRL( *this, "m_refresh", wxBitmapButton );
    m_className = XRCCTRL( *this, "m_className", wxTextCtrl );
    m_parentInfo = XRCCTRL( *this, "m_parentInfo", wxTextCtrl );
    m_inspectParent = XRCCTRL( *this, "m_inspectParent", wxBitmapButton );
    m_indexLabel = XRCCTRL( *this, "m_indexLabel", wxStaticText );
    m_newInspector = XRCCTRL( *this, "m_newInspector", wxBitmapButton );
    m_notebook = XRCCTRL( *this, "m_notebook", wxNotebook );
    m_propertiesNotebookPanel = XRCCTRL( *this, "m_propertiesNotebookPanel", wxPanel );
    m_propertiesContainer = XRCCTRL( *this, "m_propertiesContainer", wxPanel );
    m_flagsNotebookPanel = XRCCTRL( *this, "m_flagsNotebookPanel", wxPanel );
    m_childrenNotebookPanel = XRCCTRL( *this, "m_childrenNotebookPanel", wxPanel );
    m_nodeNotebookPanel = XRCCTRL( *this, "m_nodeNotebookPanel", wxPanel );
	m_meshComponentPanel = XRCCTRL( *this, "m_meshComponentPanel", wxPanel );
    m_finalizedFlag = XRCCTRL( *this, "m_finalizedFlag", wxCheckBox );
    m_rootFlag = XRCCTRL( *this, "m_rootFlag", wxCheckBox );
    m_inlinedFlag = XRCCTRL( *this, "m_inlinedFlag", wxCheckBox );
    m_scriptedFlag = XRCCTRL( *this, "m_scriptedFlag", wxCheckBox );
    m_discardedFlag = XRCCTRL( *this, "m_discardedFlag", wxCheckBox );
    m_transientFlag = XRCCTRL( *this, "m_transientFlag", wxCheckBox );
    m_referencedFlag = XRCCTRL( *this, "m_referencedFlag", wxCheckBox );
    m_highlightedFlag = XRCCTRL( *this, "m_highlightedFlag", wxCheckBox );
    m_defaultFlag = XRCCTRL( *this, "m_defaultFlag", wxCheckBox );
    m_scriptCreatedFlag = XRCCTRL( *this, "m_scriptCreatedFlag", wxCheckBox );
    m_hasHandleFlag = XRCCTRL( *this, "m_hasHandleFlag", wxCheckBox );
    m_unusedFlag = XRCCTRL( *this, "m_unusedFlag", wxCheckBox );
    m_wasCookedFlag = XRCCTRL( *this, "m_wasCookedFlag", wxCheckBox );
    m_userFlag = XRCCTRL( *this, "m_userFlag", wxCheckBox );
    m_destroyedFlag = XRCCTRL( *this, "m_destroyedFlag", wxCheckBox );
    m_selectedFlag = XRCCTRL( *this, "m_selectedFlag", wxCheckBox );
    m_attachedFlag = XRCCTRL( *this, "m_attachedFlag", wxCheckBox );
    m_attachingFlag = XRCCTRL( *this, "m_attachingFlag", wxCheckBox );
    m_detatchingFlag = XRCCTRL( *this, "m_detatchingFlag", wxCheckBox );
    m_scheduledUpdateTransformFlag = XRCCTRL( *this, "m_scheduledUpdateTransformFlag", wxCheckBox );
    m_includedFromTemplateFlag = XRCCTRL( *this, "m_includedFromTemplateFlag", wxCheckBox );
    m_postAttachSpawnCalledFlag = XRCCTRL( *this, "m_postAttachSpawnCalledFlag", wxCheckBox );
    m_hideInGameFlag = XRCCTRL( *this, "m_hideInGameFlag", wxCheckBox );
    m_wasAttachedInGameFlag = XRCCTRL( *this, "m_wasAttachedInGameFlag", wxCheckBox );
    m_wasInstancedFromTemplateFlag = XRCCTRL( *this, "m_wasInstancedFromTemplateFlag", wxCheckBox );
    m_suspendRenderingFlag = XRCCTRL( *this, "m_suspendRenderingFlag", wxCheckBox );
    m_shouldSaveFlag = XRCCTRL( *this, "m_shouldSaveFlag", wxCheckBox );
    m_childrenList = XRCCTRL( *this, "m_childrenList", wxListBox );
    m_nodeName = XRCCTRL( *this, "m_nodeName", wxTextCtrl );
    m_nodeGUID = XRCCTRL( *this, "m_nodeGUID", wxTextCtrl );
    m_parentAttachmentList = XRCCTRL( *this, "m_parentAttachmentList", wxListBox );
    m_childAttachmentList = XRCCTRL( *this, "m_childAttachmentList", wxListBox );
    m_attachmentNotebookPanel = XRCCTRL( *this, "m_attachmentNotebookPanel", wxPanel );
    m_attachmentParentNode = XRCCTRL( *this, "m_attachmentParentNode", wxTextCtrl );
    m_inspectAttachmentParent = XRCCTRL( *this, "m_inspectAttachmentParent", wxBitmapButton );
    m_attachmentChildNode = XRCCTRL( *this, "m_attachmentChildNode", wxTextCtrl );
    m_inspectAttachmentChild = XRCCTRL( *this, "m_inspectAttachmentChild", wxBitmapButton );
    m_brokenAttachment = XRCCTRL( *this, "m_brokenAttachment", wxCheckBox );
	m_meshChunksGrid = XRCCTRL( *this, "m_chunksGrid", wxGrid );

	// Connect events to XRC controls
    m_refresh->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnRefreshClicked ), NULL, this );
    m_className->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdObjectInspector::OnClassNameUpdated ), NULL, this );
    m_parentInfo->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdObjectInspector::OnParentInfoUpdated ), NULL, this );
    m_inspectParent->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnInspectParentClicked ), NULL, this );
    m_newInspector->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnNewInspectorClicked ), NULL, this );
    m_finalizedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnFinalizedFlagClicked ), NULL, this );
    m_rootFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnRootFlagClicked ), NULL, this );
    m_inlinedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnInlinedFlagClicked ), NULL, this );
    m_scriptedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnScriptedFlagClicked ), NULL, this );
    m_discardedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnDiscardedFlagClicked ), NULL, this );
    m_transientFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnTransientFlagClicked ), NULL, this );
    m_referencedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnReferencedFlagClicked ), NULL, this );
    m_highlightedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnHighlightedFlagClicked ), NULL, this );
    m_defaultFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnDefaultFlagClicked ), NULL, this );
    m_scriptCreatedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnScriptCreatedFlagClicked ), NULL, this );
    m_hasHandleFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnHasHandleFlagClicked ), NULL, this );
    m_unusedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnUnusedFlagClicked ), NULL, this );
    m_wasCookedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnWasCookedFlagClicked ), NULL, this );
    m_userFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnUserFlagClicked ), NULL, this );
    m_destroyedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnDestroyedFlagClicked ), NULL, this );
    m_selectedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnSelectedFlagClicked ), NULL, this );
    m_attachedFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnAttachedFlagClicked ), NULL, this );
    m_attachingFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnAttachingFlagClicked ), NULL, this );
    m_detatchingFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnDetatchingFlagClicked ), NULL, this );
    m_scheduledUpdateTransformFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnScheduledUpdateTransformFlagClicked ), NULL, this );
    m_includedFromTemplateFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnIncludedFromTemplateFlagClicked ), NULL, this );
    m_postAttachSpawnCalledFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnPostAttachSpawnCalledFlagClicked ), NULL, this );
    m_hideInGameFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnHideInGameFlagClicked ), NULL, this );
    m_wasAttachedInGameFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnWasAttachedInGameFlagClicked ), NULL, this );
    m_wasInstancedFromTemplateFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnWasInstancedFromTemplateFlagClicked ), NULL, this );
    m_suspendRenderingFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnSuspendRenderingFlagClicked ), NULL, this );
    m_shouldSaveFlag->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnShouldSaveFlagClicked ), NULL, this );
    m_childrenList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CEdObjectInspector::OnChildrenListDoubleClicked ), NULL, this );
    m_nodeName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdObjectInspector::OnNodeNameUpdated ), NULL, this );
    m_nodeGUID->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdObjectInspector::OnNodeGUIDUpdated ), NULL, this );
    m_parentAttachmentList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CEdObjectInspector::OnParentAttachmentListDoubleClicked ), NULL, this );
    m_childAttachmentList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CEdObjectInspector::OnChildAttachmentListDoubleClicked ), NULL, this );
	m_inspectAttachmentParent->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnInspectAttachmentParentClicked ), NULL, this );
	m_inspectAttachmentChild->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnInspectAttachmentChildClicked ), NULL, this );
	m_brokenAttachment->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdObjectInspector::OnBrokenAttachmentClicked ), NULL, this );

	// Create properties browser
	PropertiesPageSettings propertiesPageSettings;
	propertiesPageSettings.m_autoExpandGroups = true;
	m_propertiesPage = new CEdPropertiesPage( m_propertiesContainer, propertiesPageSettings, nullptr );
	m_propertiesContainer->GetSizer()->Add( m_propertiesPage, 1, wxEXPAND );
}

void CEdObjectInspector::OnRefreshClicked( wxCommandEvent& event )
{
	UpdateDataFromObject();
}

void CEdObjectInspector::OnClassNameUpdated( wxCommandEvent& event )
{
}

void CEdObjectInspector::OnParentInfoUpdated( wxCommandEvent& event )
{
}

void CEdObjectInspector::OnInspectParentClicked( wxCommandEvent& event )
{
	if ( GetInspectedObject() != nullptr && GetInspectedObject()->GetParent() )
	{
		CreateInspector( GetInspectedObject()->GetParent() );
	}
}

void CEdObjectInspector::OnNewInspectorClicked( wxCommandEvent& event )
{
	CEdObjectInspector* inspector = CreateInspector( GetInspectedObject() );

	// Use the same page in the new inspector
	if ( inspector != nullptr )
	{
		inspector->m_notebook->SetSelection( m_notebook->GetSelection() );
	}
}

void CEdObjectInspector::OnFinalizedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_finalizedFlag, OF_Finalized );
}

void CEdObjectInspector::OnRootFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_rootFlag, OF_Root );
}

void CEdObjectInspector::OnInlinedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_inlinedFlag, OF_Inlined );
}

void CEdObjectInspector::OnScriptedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_scriptedFlag, OF_Scripted );
}

void CEdObjectInspector::OnDiscardedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_discardedFlag, OF_Discarded );
}

void CEdObjectInspector::OnTransientFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_transientFlag, OF_Transient );
}

void CEdObjectInspector::OnReferencedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_referencedFlag, OF_Referenced );
}

void CEdObjectInspector::OnHighlightedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_highlightedFlag, OF_Highlighted );
}

void CEdObjectInspector::OnDefaultFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_defaultFlag, OF_DefaultObject );
}

void CEdObjectInspector::OnScriptCreatedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_scriptedFlag, OF_Scripted );
}

void CEdObjectInspector::OnHasHandleFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_hasHandleFlag, OF_HasHandle );
}

void CEdObjectInspector::OnUnusedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_unusedFlag, OF_Unused );
}

void CEdObjectInspector::OnWasCookedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_wasCookedFlag, OF_WasCooked );
}

void CEdObjectInspector::OnUserFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_userFlag, OF_UserFlag );
}

void CEdObjectInspector::OnDestroyedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_destroyedFlag, NF_Destroyed );
}

void CEdObjectInspector::OnSelectedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_selectedFlag, NF_Selected );
}

void CEdObjectInspector::OnAttachedFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_attachedFlag, NF_Attached );
}

void CEdObjectInspector::OnAttachingFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_attachingFlag, NF_Attaching );
}

void CEdObjectInspector::OnDetatchingFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_detatchingFlag, NF_Detaching );
}

void CEdObjectInspector::OnScheduledUpdateTransformFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_scheduledUpdateTransformFlag, NF_ScheduledUpdateTransform );
}

void CEdObjectInspector::OnIncludedFromTemplateFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_includedFromTemplateFlag, NF_IncludedFromTemplate );
}

void CEdObjectInspector::OnPostAttachSpawnCalledFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_postAttachSpawnCalledFlag, NF_PostAttachSpawnCalled );
}

void CEdObjectInspector::OnHideInGameFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_hideInGameFlag, NF_HideInGame );
}

void CEdObjectInspector::OnWasAttachedInGameFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_wasAttachedInGameFlag, NF_WasAttachedInGame );
}

void CEdObjectInspector::OnWasInstancedFromTemplateFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_wasInstancedFromTemplateFlag, NF_WasInstancedFromTemplate );
}

void CEdObjectInspector::OnSuspendRenderingFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_suspendRenderingFlag, NF_SuspendRendering );
}

void CEdObjectInspector::OnShouldSaveFlagClicked( wxCommandEvent& event )
{
	UPDATE_FLAG_CHECKBOX_EVENT( m_shouldSaveFlag, NF_ShouldSave );
}

void CEdObjectInspector::OnChildrenListDoubleClicked( wxCommandEvent& event )
{
	CObject* child = m_children[ event.GetSelection() ].Get();
	if ( child != nullptr )
	{
		CreateInspector( child );
	}
}

void CEdObjectInspector::OnNodeNameUpdated( wxCommandEvent& event )
{
}

void CEdObjectInspector::OnNodeGUIDUpdated( wxCommandEvent& event )
{
}

void CEdObjectInspector::OnParentAttachmentListDoubleClicked( wxCommandEvent& event )
{
	IAttachment* attachment = m_parentAttachments[ event.GetSelection() ].Get();
	if ( attachment != nullptr )
	{
		CreateInspector( attachment );
	}
}

void CEdObjectInspector::OnChildAttachmentListDoubleClicked( wxCommandEvent& event )
{
	IAttachment* attachment = m_childAttachments[ event.GetSelection() ].Get();
	if ( attachment != nullptr )
	{
		CreateInspector( attachment );
	}
}

void CEdObjectInspector::OnInspectAttachmentParentClicked( wxCommandEvent& event )
{
	IAttachment* attachment = Cast< IAttachment >( GetInspectedObject() );
	if ( attachment != nullptr )
	{
		CreateInspector( attachment->GetParent() );
	}
}

void CEdObjectInspector::OnInspectAttachmentChildClicked( wxCommandEvent& event )
{
	IAttachment* attachment = Cast< IAttachment >( GetInspectedObject() );
	if ( attachment != nullptr )
	{
		CreateInspector( attachment->GetChild() );
	}
}

void CEdObjectInspector::OnBrokenAttachmentClicked( wxCommandEvent& event )
{
	IAttachment* attachment = Cast< IAttachment >( GetInspectedObject() );
	if ( attachment == nullptr )
	{
		return;
	}

	if ( m_brokenAttachment->GetValue() )
	{
		m_brokenAttachment->SetValue( false ); // we cannot restore a broken attachment
	}
	else
	{
		attachment->Break();
	}
}

static void RemoveNotebookPage( wxNotebook* notebook, wxWindow* page )
{
	for ( size_t i=0; i < notebook->GetPageCount(); ++i )
	{
		if ( notebook->GetPage( i ) == page )
		{
			notebook->RemovePage( i );
			return;
		}
	}
}

void CEdObjectInspector::UpdateDataFromObject()
{
	// Update data only if we have an object available
	if ( GetInspectedObject() != nullptr )
	{
		// Update visible tabs based on object type
		if ( !GetInspectedObject()->IsA< CNode >() )
		{
			RemoveNotebookPage( m_notebook, m_nodeNotebookPanel );
		}
		if ( !GetInspectedObject()->IsA< IAttachment >() )
		{
			RemoveNotebookPage( m_notebook, m_attachmentNotebookPanel );
		}

#ifdef USE_UMBRA
		if ( GetInspectedObject()->IsA< CComponent >() )
		{
			PrepareUmbraGrid();
			if ( FillComponentInfo( Cast< CComponent >( GetInspectedObject() ) ) )
			{
				RefreshUmbraGrid();
			}
			else
			{
				RemoveNotebookPage( m_notebook, m_meshComponentPanel );
			}
		}
#else
		RemoveNotebookPage( m_notebook, m_meshComponentPanel );
#endif // USE_UMBRA

		// Bind properties page
		m_propertiesPage->SetObject( GetInspectedObject() );

		// Update the title
		if ( m_titleTag.Empty() )
		{
			SetTitle( wxString::Format( TXT("Object Inspector - %s %p"), GetInspectedObject()->GetClass()->GetName().AsChar(), GetInspectedObject() ) );
		}
		else
		{
			SetTitle( wxString::Format( TXT("Object Inspector [%s] - %s %p"), m_titleTag.AsChar(), GetInspectedObject()->GetClass()->GetName().AsChar(), GetInspectedObject() ) );
		}

		// Update the object's name
		m_className->SetValue( GetInspectedObject()->GetClass()->GetName().AsChar() );

		// Update the object's parent
		if ( GetInspectedObject()->GetParent() )
		{
			m_parentInfo->SetValue( wxString::Format( TXT("%s %p"), GetInspectedObject()->GetParent()->GetClass()->GetName().AsChar(), GetInspectedObject()->GetParent() ) );
			m_inspectParent->Enable( true );
		}
		else // no parent
		{
			m_parentInfo->SetValue( wxT("No parent") );
			m_inspectParent->Enable( false );
		}

		// Update the object's index
		m_indexLabel->SetLabelText( wxString::Format( TXT("Index #%i"), GetInspectedObject()->GetObjectIndex() ) );

		// Update object flags
		m_finalizedFlag->SetValue( GetInspectedObject()->HasFlag( OF_Finalized ) );
		m_rootFlag->SetValue( GetInspectedObject()->HasFlag( OF_Root ) );
		m_inlinedFlag->SetValue( GetInspectedObject()->HasFlag( OF_Inlined ) );
		m_scriptedFlag->SetValue( GetInspectedObject()->HasFlag( OF_Scripted ) );
		m_discardedFlag->SetValue( GetInspectedObject()->HasFlag( OF_Discarded ) );
		m_transientFlag->SetValue( GetInspectedObject()->HasFlag( OF_Transient ) );
		m_referencedFlag->SetValue( GetInspectedObject()->HasFlag( OF_Referenced ) );
		m_highlightedFlag->SetValue( GetInspectedObject()->HasFlag( OF_Highlighted ) );
		m_defaultFlag->SetValue( GetInspectedObject()->HasFlag( OF_DefaultObject ) );
		m_scriptCreatedFlag->SetValue( GetInspectedObject()->HasFlag( OF_ScriptCreated ) );
		m_hasHandleFlag->SetValue( GetInspectedObject()->HasFlag( OF_HasHandle ) );
		m_unusedFlag->SetValue( GetInspectedObject()->HasFlag( OF_Unused ) );
		m_wasCookedFlag->SetValue( GetInspectedObject()->HasFlag( OF_WasCooked ) );
		m_userFlag->SetValue( GetInspectedObject()->HasFlag( OF_UserFlag ) );

		// Update object children
		TDynArray< CObject* > children;
		GetInspectedObject()->GetChildren( children );
		m_childrenList->Freeze();
		m_childrenList->Clear();
		m_children.Clear();
		for ( auto it=children.Begin(); it != children.End(); ++it )
		{
			CObject* child = *it;
			m_childrenList->AppendString( wxString::Format( TXT("%s %p"), child->GetClass()->GetName().AsChar(), child ) );
			m_children.PushBack( child );
		}
		m_childrenList->Thaw();

		// Update node flag state
		Bool objectIsANode = GetInspectedObject()->IsA< CNode >();
		m_destroyedFlag->Enable( objectIsANode );
		m_selectedFlag->Enable( objectIsANode );
		m_attachedFlag->Enable( objectIsANode );
		m_attachingFlag->Enable( objectIsANode );
		m_detatchingFlag->Enable( objectIsANode );
		m_scheduledUpdateTransformFlag->Enable( objectIsANode );
		m_includedFromTemplateFlag->Enable( objectIsANode );
		m_postAttachSpawnCalledFlag->Enable( objectIsANode );
		m_hideInGameFlag->Enable( objectIsANode );
		m_wasAttachedInGameFlag->Enable( objectIsANode );
		m_wasInstancedFromTemplateFlag->Enable( objectIsANode );
		m_suspendRenderingFlag->Enable( objectIsANode );
		m_shouldSaveFlag->Enable( objectIsANode );	

		// Update node values
		if ( objectIsANode )
		{
			CNode* node = static_cast< CNode* >( GetInspectedObject() );

			// Update flags
			m_destroyedFlag->SetValue( GetInspectedObject()->HasFlag( NF_Destroyed ) );
			m_selectedFlag->SetValue( GetInspectedObject()->HasFlag( NF_Selected ) );
			m_attachedFlag->SetValue( GetInspectedObject()->HasFlag( NF_Attached ) );
			m_attachingFlag->SetValue( GetInspectedObject()->HasFlag( NF_Attaching ) );
			m_detatchingFlag->SetValue( GetInspectedObject()->HasFlag( NF_Detaching ) );
			m_scheduledUpdateTransformFlag->SetValue( GetInspectedObject()->HasFlag( NF_ScheduledUpdateTransform ) );
			m_includedFromTemplateFlag->SetValue( GetInspectedObject()->HasFlag( NF_IncludedFromTemplate ) );
			m_postAttachSpawnCalledFlag->SetValue( GetInspectedObject()->HasFlag( NF_PostAttachSpawnCalled ) );
			m_hideInGameFlag->SetValue( GetInspectedObject()->HasFlag( NF_HideInGame ) );
			m_wasAttachedInGameFlag->SetValue( GetInspectedObject()->HasFlag( NF_WasAttachedInGame ) );
			m_wasInstancedFromTemplateFlag->SetValue( GetInspectedObject()->HasFlag( NF_WasInstancedFromTemplate ) );
			m_suspendRenderingFlag->SetValue( GetInspectedObject()->HasFlag( NF_SuspendRendering ) );
			m_shouldSaveFlag->SetValue( GetInspectedObject()->HasFlag( NF_ShouldSave ) );

			// Update name and GUID
			m_nodeName->SetValue( node->GetName().AsChar() );
			m_nodeGUID->SetValue( ToString( node->GetGUID() ).AsChar() );

			// Update parent attachments
			const TList< IAttachment* >& parentAttachments = node->GetParentAttachments();
			m_parentAttachmentList->Freeze();
			m_parentAttachmentList->Clear();
			m_parentAttachments.Clear();
			for ( auto it=parentAttachments.Begin(); it != parentAttachments.End(); ++it )
			{
				IAttachment* attachment = *it;
				CObject* target = attachment->GetParent();
				m_parentAttachments.PushBack( attachment );
				if ( attachment != nullptr )
				{
					if ( target != nullptr )
					{
						m_parentAttachmentList->AppendString( wxString::Format( TXT("%s %p to %s %p"), attachment->GetClass()->GetName().AsChar(), attachment, target->GetClass()->GetName().AsChar(), target ) );
					}
					else
					{
						m_parentAttachmentList->AppendString( wxString::Format( TXT("broken %s to %s %p"), attachment->GetClass()->GetName().AsChar() ) );
					}
				}
				else
				{
					m_parentAttachmentList->AppendString( wxT("(null attachment!)") );
				}
			}
			m_parentAttachmentList->Thaw();

			// Update child attachments
			const TList< IAttachment* >& childAttachments = node->GetChildAttachments();
			m_childAttachmentList->Freeze();
			m_childAttachmentList->Clear();
			m_childAttachments.Clear();
			for ( auto it=childAttachments.Begin(); it != childAttachments.End(); ++it )
			{
				IAttachment* attachment = *it;
				CObject* target = attachment->GetChild();
				m_childAttachments.PushBack( attachment );
				if ( attachment != nullptr )
				{
					if ( target != nullptr )
					{
						m_childAttachmentList->AppendString( wxString::Format( TXT("%s %p to %s %p"), attachment->GetClass()->GetName().AsChar(), attachment, target->GetClass()->GetName().AsChar(), target ) );
					}
					else
					{
						m_childAttachmentList->AppendString( wxString::Format( TXT("broken %s to %s %p"), attachment->GetClass()->GetName().AsChar() ) );
					}
				}
				else
				{
					m_childAttachmentList->AppendString( wxT("(null attachment!)") );
				}
			}
			m_childAttachmentList->Thaw();
		}
		else // not a node
		{
			// Reset flags
			m_destroyedFlag->SetValue( false );
			m_selectedFlag->SetValue( false );
			m_attachedFlag->SetValue( false );
			m_attachingFlag->SetValue( false );
			m_detatchingFlag->SetValue( false );
			m_scheduledUpdateTransformFlag->SetValue( false );
			m_includedFromTemplateFlag->SetValue( false );
			m_postAttachSpawnCalledFlag->SetValue( false );
			m_hideInGameFlag->SetValue( false );
			m_wasAttachedInGameFlag->SetValue( false );
			m_wasInstancedFromTemplateFlag->SetValue( false );
			m_suspendRenderingFlag->SetValue( false );
			m_shouldSaveFlag->SetValue( false );
		}

		// Update attachment info
		if ( GetInspectedObject()->IsA< IAttachment >() )
		{
			IAttachment* attachment = static_cast< IAttachment* >( GetInspectedObject() );

			// Attachment parent
			if ( attachment->GetParent() != nullptr )
			{
				m_attachmentParentNode->SetValue( wxString::Format( TXT("%s %p"), attachment->GetParent()->GetClass()->GetName().AsChar(), attachment->GetParent() ) );
				m_inspectAttachmentParent->Enable( true );
			}
			else
			{
				m_attachmentParentNode->SetValue( wxT("(no parent)") );
				m_inspectAttachmentParent->Enable( false );
			}

			// Attachment child
			if ( attachment->GetChild() != nullptr )
			{
				m_attachmentChildNode->SetValue( wxString::Format( TXT("%s %p"), attachment->GetChild()->GetClass()->GetName().AsChar(), attachment->GetChild() ) );
				m_inspectAttachmentChild->Enable( true );
			}
			else
			{
				m_attachmentChildNode->SetValue( wxT("(no child)") );
				m_inspectAttachmentChild->Enable( false );
			}

			// Attachment status
			m_brokenAttachment->SetValue( attachment->IsBroken() );
		}
	}
	else // no object
	{
		m_propertiesPage->SetNoObject();
	}
}

CEdObjectInspector* CEdObjectInspector::CreateInspector( THandle< CObject > object, const String& tag /* = String::EMPTY */ )
{
	// Ignore the request if we got a null object
	if ( object.Get() == nullptr )
	{
		return nullptr;
	}

	// Create inspector frame and attach object
	CEdObjectInspector* inspector = new CEdObjectInspector();
	inspector->m_titleTag = tag;
	inspector->m_inspectedObject = object;

	// Fit the inspector
	inspector->Fit();

	// Move the inspector near the mouse pointer
	int x, y;
	wxGetMousePosition( &x, &y );
	inspector->Move( x - inspector->GetSize().x/2, y - inspector->GetSize().y/3 );

	// Update data from the object
	inspector->UpdateDataFromObject();

	// Show the inspector
	inspector->Show();
	inspector->SetFocus();

	return inspector;
}

#ifdef USE_UMBRA
Bool CEdObjectInspector::FillComponentInfo( CComponent* component )
{
	if ( !component )
	{
		return false;
	}
	if ( !component->GetWorld() )
	{
		return false;
	}
	if ( !component->GetWorld()->GetUmbraScene() )
	{
		return false;
	}
	const TObjectCache& objectCache = component->GetWorld()->GetUmbraScene()->GetObjectCache();
	const Matrix& localToWorld = component->GetLocalToWorld();

	Vector position = localToWorld.V[3];
	EulerAngles rotation = localToWorld.ToEulerAngles();
	Uint32 transformHash = UmbraHelpers::CalculateTransformHash( localToWorld );

	String sPosition = String::Printf( TXT("[%1.3f, %1.3f, %1.3f, %1.3f]"), position.X, position.Y, position.Z, position.W );
	String sRotation = String::Printf( TXT("[%1.3f, %1.3f, %1.3f]"), rotation.Yaw, rotation.Pitch, rotation.Roll );
	String sPosHash  = String::Printf( TXT("%u"), transformHash );

	m_meshChunksGrid->SetCellValue( 0, 0, sPosition.AsChar() );
	m_meshChunksGrid->SetCellValue( 1, 0, sRotation.AsChar() );
	m_meshChunksGrid->SetCellValue( 3, 0, sPosHash.AsChar() );
	

	Uint32 modelId = 0;
	if ( component->IsA< CDimmerComponent >() )
	{
		modelId = Cast< CDimmerComponent >( component )->GetOcclusionId();
	}
	else if ( component->IsA< CStripeComponent >() )
	{
		modelId = Cast< CStripeComponent >( component )->GetOcclusionId();
	}
	else if ( component->IsA< CLightComponent >() )
	{
		modelId = Cast< CLightComponent >( component )->GetOcclusionId();
	}
	else if ( component->IsA< CMeshComponent >() )
	{
		CMesh* mesh = Cast< CMeshComponent >( component )->GetMeshNow();
		if ( !mesh )
		{
			return false;
		}
		modelId = GetHash( mesh->GetDepotPath() );
	}
	else if ( component->IsA< CDecalComponent >() )
	{
		CBitmapTexture* texture = Cast< CDecalComponent >( component )->GetDiffuseTexture();
		if ( !texture )
		{
			return false;
		}
		modelId = GetHash( texture->GetDepotPath() );
	}
	else
	{
		return false;
	}

	String sModelId = String::Printf( TXT("%u"), modelId );
	m_meshChunksGrid->SetCellValue( 2, 0, sModelId.AsChar() );

	TObjectCacheKeyType objectCacheKey = UmbraHelpers::CompressToKeyType( modelId, transformHash );
	String sObjectCacheKey = String::Printf( TXT("%") RED_PRIWu64, objectCacheKey );
	m_meshChunksGrid->SetCellValue( 4, 0, sObjectCacheKey.AsChar() );

	TObjectIdType objectId;
	String sObjectId( TXT("NOT FOUND") );
	if ( objectCache.Find( objectCacheKey, objectId ) )
	{
		sObjectId = String::Printf( TXT("%u"), objectId );
	}
	m_meshChunksGrid->SetCellValue( 5, 0, sObjectId.AsChar() );

	return true;
}

void CEdObjectInspector::PrepareUmbraGrid()
{
	m_meshChunksGrid->ClearGrid();
	m_meshChunksGrid->CreateGrid( 6, 1 );
	m_meshChunksGrid->SetRowLabelValue( 0, TXT("Position") );
	m_meshChunksGrid->SetRowLabelValue( 1, TXT("Rotation [YPR]") );
	m_meshChunksGrid->SetRowLabelValue( 2, TXT("ModelID") );
	m_meshChunksGrid->SetRowLabelValue( 3, TXT("PositionHash") );
	m_meshChunksGrid->SetRowLabelValue( 4, TXT("ObjectCacheKey") );
	m_meshChunksGrid->SetRowLabelValue( 5, TXT("ObjectID") );
}

void CEdObjectInspector::RefreshUmbraGrid()
{
	m_meshChunksGrid->AutoSize();
	m_meshChunksGrid->ForceRefresh();	
}
#endif // USE_UMBRA