/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdUpdateThumbnailDialog : private wxDialog
{
public:
	enum CameraType : Int32 { CT_Auto, CT_LastUsed, CT_FromEditor };

	CEdUpdateThumbnailDialog( wxWindow* parent, Bool enableEditorCamera, Bool enableLastUsedCamera );

	~CEdUpdateThumbnailDialog();

	Bool Execute( CameraType& camType, Int32& flags, Uint32& iconSize );

private:
	wxChoice* m_camera;
	wxCheckBox* m_ground;
	wxCheckBox* m_removeBG;
	wxCheckBox* m_copyEnv;
	wxCheckBox* m_outputIcon;
	wxChoice* m_iconSize;
};
