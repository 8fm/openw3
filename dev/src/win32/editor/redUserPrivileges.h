// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

// =================================================================================================

/*
Represents privileges possessed by Red User.
*/
class CRedUserPrivileges
{
public:
	CRedUserPrivileges();
	~CRedUserPrivileges();

	void RevokeAll();

	Bool m_editDialogs;					// Privilege required to edit dialogs in Scene Editor.
	Bool m_approveVo;					// Privilege required to approve local VO in Scene Editor.
	Bool m_editRedStrings;				// Privilege required to edit Red Strings through the LocalizedStringPropertyEditorReadOnly.
};

CRedUserPrivileges RetrieveRedUserPrivileges();

// =================================================================================================
// implementation
// =================================================================================================

/*
Ctor.
*/
RED_INLINE CRedUserPrivileges::CRedUserPrivileges()
: m_editDialogs( false )
, m_approveVo( false )
, m_editRedStrings( false )
{}

/*
Dtor.
*/
RED_INLINE CRedUserPrivileges::~CRedUserPrivileges()
{}

/*
Revokes all privileges.
*/
RED_INLINE void CRedUserPrivileges::RevokeAll()
{
	m_editDialogs = false;
	m_approveVo = false;
	m_editRedStrings = false;
}
