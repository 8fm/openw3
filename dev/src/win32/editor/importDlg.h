/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Return options from import/create dialog
enum EImportCreateReturn
{
	ECR_Cancel,
	ECR_OK,
	ECR_OKAll,
	ECR_Skip,
};

/// Import dialog
class CEdImportDlg : public wxDialog
{
	DECLARE_EVENT_TABLE();

protected:
	String		m_fileName;
	Bool		m_createOnly;
	String		m_resourceClassName;
	TDynArray< String >* m_favClasses;
	Bool		m_favFlipped;

public:
	CEdImportDlg( 
		wxWindow* parent, const CClass* resourceClass, const CDirectory* fileDirectory, const String& fileName, CObject* configObject,
		TDynArray< String >* favClasses = nullptr
		);
	~CEdImportDlg();

	EImportCreateReturn DoModal();
	RED_INLINE const String& GetFileName() const { return m_fileName; }

protected:
	void OnOK( wxCommandEvent& event );
	void OnOKAll( wxCommandEvent& event );
	void OnSkip( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnAddToFavsCheck( wxCommandEvent& event );
	void OnUpdateUI( wxUpdateUIEvent& event );

	void ReadValuesBack();
};

/// Material auto assignment config
struct MeshMaterialAutoAssignmentConfig
{
	String		m_materialName;
	Bool		m_setDiffuseParam;
	String		m_diffuseParameterName;
	Bool		m_setNormalParam;
	String		m_normalParameterName;
	Bool		m_setSpecularParam;
	String		m_specularParameterName;
	Bool		m_setMaskParam;
	String		m_maskParameterName;

	MeshMaterialAutoAssignmentConfig()
		: m_setDiffuseParam( false )
		, m_setNormalParam( false )
		, m_setSpecularParam( false )
		, m_setMaskParam( false )
	{};
};

/// Import dialog for meshes - material assignment
class CEdImportMeshDlg : public wxDialog
{
	DECLARE_EVENT_TABLE();

protected:
	CMeshTypeResource*					m_mesh;
	String								m_recentMaterial;
	TDynArray< String >					m_recentMaterialsList;
	MeshMaterialAutoAssignmentConfig	m_autoAssignConfig;

public:
	//! Get the config
	const MeshMaterialAutoAssignmentConfig& GetConfig() const { return m_autoAssignConfig; }

public:
	CEdImportMeshDlg( wxWindow* parent, CMeshTypeResource* mesh );
	~CEdImportMeshDlg();

	void UpdateMaterialList();
	EImportCreateReturn DoModal();

protected:
	void OnOK( wxCommandEvent& event );
	void OnOKAll( wxCommandEvent& event );
	void OnSkip( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnMaterialChanged( wxCommandEvent& event );
	void OnText( wxCommandEvent& event );

	void SaveConfig();
	void UpdateMaterialTextures();
};

