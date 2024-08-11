/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdObjectInspector : public wxFrame
{
	DECLARE_EVENT_TABLE()

private:
	String								m_titleTag;
	THandle< CObject >					m_inspectedObject;
	TDynArray< THandle< CObject > >		m_children;
	TDynArray< THandle< IAttachment > >	m_parentAttachments;
	TDynArray< THandle< IAttachment > >	m_childAttachments;

protected:
	// From XRC
	wxBitmapButton*						m_refresh;
	wxTextCtrl*							m_className;
	wxTextCtrl*							m_parentInfo;
	wxBitmapButton*						m_inspectParent;
	wxStaticText*						m_indexLabel;
	wxBitmapButton*						m_newInspector;
	wxNotebook*							m_notebook;
	wxPanel*							m_propertiesNotebookPanel;
	wxPanel*							m_propertiesContainer;
	wxPanel*							m_flagsNotebookPanel;
	wxPanel*							m_childrenNotebookPanel;
	wxPanel*							m_nodeNotebookPanel;
	wxPanel*							m_meshComponentPanel;
	wxCheckBox*							m_finalizedFlag;
	wxCheckBox*							m_rootFlag;
	wxCheckBox*							m_inlinedFlag;
	wxCheckBox*							m_scriptedFlag;
	wxCheckBox*							m_discardedFlag;
	wxCheckBox*							m_transientFlag;
	wxCheckBox*							m_referencedFlag;
	wxCheckBox*							m_highlightedFlag;
	wxCheckBox*							m_defaultFlag;
	wxCheckBox*							m_scriptCreatedFlag;
	wxCheckBox*							m_hasHandleFlag;
	wxCheckBox*							m_unusedFlag;
	wxCheckBox*							m_wasCookedFlag;
	wxCheckBox*							m_userFlag;
	wxCheckBox*							m_destroyedFlag;
	wxCheckBox*							m_selectedFlag;
	wxCheckBox*							m_attachedFlag;
	wxCheckBox*							m_attachingFlag;
	wxCheckBox*							m_detatchingFlag;
	wxCheckBox*							m_scheduledUpdateTransformFlag;
	wxCheckBox*							m_includedFromTemplateFlag;
	wxCheckBox*							m_postAttachSpawnCalledFlag;
	wxCheckBox*							m_hideInGameFlag;
	wxCheckBox*							m_wasAttachedInGameFlag;
	wxCheckBox*							m_wasInstancedFromTemplateFlag;
	wxCheckBox*							m_suspendRenderingFlag;
	wxCheckBox*							m_shouldSaveFlag;
	wxListBox*							m_childrenList;
	wxTextCtrl*							m_nodeName;
	wxTextCtrl*							m_nodeGUID;
	wxListBox*							m_parentAttachmentList;
	wxListBox*							m_childAttachmentList;
	wxPanel*							m_attachmentNotebookPanel;
	wxTextCtrl*							m_attachmentParentNode;
	wxBitmapButton*						m_inspectAttachmentParent;
	wxTextCtrl*							m_attachmentChildNode;
	wxBitmapButton*						m_inspectAttachmentChild;
	wxCheckBox*							m_brokenAttachment;
	wxGrid*								m_meshChunksGrid;

	// Handmade
	CEdPropertiesPage*					m_propertiesPage;

	void OnRefreshClicked( wxCommandEvent& event );
	void OnClassNameUpdated( wxCommandEvent& event );
	void OnParentInfoUpdated( wxCommandEvent& event );
	void OnInspectParentClicked( wxCommandEvent& event );
	void OnNewInspectorClicked( wxCommandEvent& event );
	void OnFinalizedFlagClicked( wxCommandEvent& event );
	void OnRootFlagClicked( wxCommandEvent& event );
	void OnInlinedFlagClicked( wxCommandEvent& event );
	void OnScriptedFlagClicked( wxCommandEvent& event );
	void OnDiscardedFlagClicked( wxCommandEvent& event );
	void OnTransientFlagClicked( wxCommandEvent& event );
	void OnReferencedFlagClicked( wxCommandEvent& event );
	void OnHighlightedFlagClicked( wxCommandEvent& event );
	void OnDefaultFlagClicked( wxCommandEvent& event );
	void OnScriptCreatedFlagClicked( wxCommandEvent& event );
	void OnHasHandleFlagClicked( wxCommandEvent& event );
	void OnUnusedFlagClicked( wxCommandEvent& event );
	void OnWasCookedFlagClicked( wxCommandEvent& event );
	void OnUserFlagClicked( wxCommandEvent& event );
	void OnDestroyedFlagClicked( wxCommandEvent& event );
	void OnSelectedFlagClicked( wxCommandEvent& event );
	void OnAttachedFlagClicked( wxCommandEvent& event );
	void OnAttachingFlagClicked( wxCommandEvent& event );
	void OnDetatchingFlagClicked( wxCommandEvent& event );
	void OnScheduledUpdateTransformFlagClicked( wxCommandEvent& event );
	void OnIncludedFromTemplateFlagClicked( wxCommandEvent& event );
	void OnPostAttachSpawnCalledFlagClicked( wxCommandEvent& event );
	void OnHideInGameFlagClicked( wxCommandEvent& event );
	void OnWasAttachedInGameFlagClicked( wxCommandEvent& event );
	void OnWasInstancedFromTemplateFlagClicked( wxCommandEvent& event );
	void OnSuspendRenderingFlagClicked( wxCommandEvent& event );
	void OnShouldSaveFlagClicked( wxCommandEvent& event );
	void OnChildrenListDoubleClicked( wxCommandEvent& event );
	void OnNodeNameUpdated( wxCommandEvent& event );
	void OnNodeGUIDUpdated( wxCommandEvent& event );
	void OnParentAttachmentListDoubleClicked( wxCommandEvent& event );
	void OnChildAttachmentListDoubleClicked( wxCommandEvent& event );
	void OnInspectAttachmentParentClicked( wxCommandEvent& event );
	void OnInspectAttachmentChildClicked( wxCommandEvent& event );
	void OnBrokenAttachmentClicked( wxCommandEvent& event );

	void UpdateDataFromObject();

public:
	CEdObjectInspector();

	RED_INLINE CObject* GetInspectedObject() const { return m_inspectedObject.Get(); }

	static CEdObjectInspector* CreateInspector( THandle< CObject > object, const String& tag = String::EMPTY );

#ifdef USE_UMBRA
private:
	Bool FillComponentInfo( CComponent* component );
	void PrepareUmbraGrid();
	void RefreshUmbraGrid();
#endif // USE_UMBRA
};
