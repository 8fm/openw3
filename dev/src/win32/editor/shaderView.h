/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CShaderView : public wxDialog
{
protected:
	IMaterial*			m_material;			// Material

public:
	CShaderView (wxWindow* parent, IMaterial* material);

};