//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CarSimEditControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CarSimNumberEdit::CarSimNumberEdit()
:	m_dblValue( 0.0 )
,	m_bAllowNegativeValue( false )
,	m_uiNumberOfDecimal( 2 )
,	m_dblMinValue( 0.0 )
,	m_dblMaxValue( 1.0 )
{}

void CarSimNumberEdit::Init( bool in_bAllowNegativeValue, UINT in_iNumberofDecimal, const double& in_rMinValue, const double& in_rMaxValue )
{
	m_bAllowNegativeValue = in_bAllowNegativeValue;
	m_uiNumberOfDecimal = in_iNumberofDecimal;
	m_dblMinValue = in_rMinValue;
	m_dblMaxValue = in_rMaxValue;
}

double CarSimNumberEdit::GetValue()
{
	ValidateAndUpdate();

	return m_dblValue;
}
	
void CarSimNumberEdit::SetValue( const double& in_rValue )
{
	m_dblValue = in_rValue;

	Clamp();

	UpdateText();
}

BEGIN_MESSAGE_MAP(CarSimNumberEdit, CEdit)
    ON_CONTROL_REFLECT_EX(EN_KILLFOCUS, OnEnKillfocus)
    ON_WM_CHAR()
END_MESSAGE_MAP()

BOOL CarSimNumberEdit::OnEnKillfocus()
{
	ValidateAndUpdate();
	
	return FALSE;
}
	
void CarSimNumberEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if ( nChar == VK_RETURN )
    {
        if( GetParent() )
            GetParent()->SetFocus();

        SetSel(0, -1);
    }
    else if ( IsValidChar(nChar) )
	{
		// Let it pass.
		__super::OnChar(nChar, nRepCnt, nFlags);
	}
}

void CarSimNumberEdit::UpdateText()
{
	CString csFormatString;
	csFormatString.Format( L"%%.%df", m_uiNumberOfDecimal );

	CString csText;
	csText.Format( csFormatString, m_dblValue );

	SetWindowText( csText );
}

bool CarSimNumberEdit::ValidateAndUpdate()
{
	double oldValue = m_dblValue;

	CString csText;
	GetWindowText( csText );

	m_dblValue = _tstof( csText );

	Clamp();

	UpdateText();

	return oldValue != m_dblValue;
}

bool CarSimNumberEdit::IsValidChar( UINT in_nChar )
{
	return isdigit(in_nChar) || ( m_bAllowNegativeValue? in_nChar == _T('-') : false ) || in_nChar == _T('\b') || in_nChar == _T('\t') || in_nChar == _T('.');
}

void CarSimNumberEdit::Clamp()
{
	// Clamp the value
	m_dblValue = min( m_dblValue, m_dblMaxValue );
	m_dblValue = max( m_dblValue, m_dblMinValue );
}