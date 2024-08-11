/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdGenericResourceEditor : public CEdPropertiesFrame
{
protected:
	CResource*		m_resource;

public:
	CEdGenericResourceEditor( wxWindow* parent, CResource* resource );
	~CEdGenericResourceEditor();

	void OnPropertiesChanged( wxCommandEvent& event );

};