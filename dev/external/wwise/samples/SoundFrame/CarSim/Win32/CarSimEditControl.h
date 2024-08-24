//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "afxwin.h"

class CarSimNumberEdit : public CEdit
{
public:
	CarSimNumberEdit();

	void Init( bool in_bAllowNegativeValue, UINT in_iNumberofDecimal, const double& in_rMinValue, const double& in_rMaxValue );

	double GetValue();
	void SetValue( const double& in_rValue );

protected:

	DECLARE_MESSAGE_MAP()

	afx_msg BOOL OnEnKillfocus();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);

private:
	void UpdateText();
	bool ValidateAndUpdate();

	bool IsValidChar( UINT in_nChar );

	void Clamp();

	bool m_bAllowNegativeValue;
	UINT m_uiNumberOfDecimal;
	double m_dblMinValue;
	double m_dblMaxValue;

	double m_dblValue;
};