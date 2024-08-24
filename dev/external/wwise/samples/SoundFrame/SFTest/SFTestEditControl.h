//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "afxwin.h"

// Edit control class that validate the input characters to prevent invalid characters
class CSFTestVolumeEdit : public CEdit
{
public:
	CSFTestVolumeEdit();

	double GetVolume();
	void SetVolume( const double& in_rVolume );

protected:

	DECLARE_MESSAGE_MAP()

	afx_msg BOOL OnEnKillfocus();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);

private:
	void UpdateText();
	bool ValidateAndUpdate();

	bool IsValidChar( UINT in_nChar );

	double m_dblVolume;
};