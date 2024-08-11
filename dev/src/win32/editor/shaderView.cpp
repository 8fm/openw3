
#include "build.h"
#include "shaderView.h"


CShaderView::CShaderView (wxWindow* parent, IMaterial* material)
{
	m_material = material;
	wxXmlResource::Get()->LoadDialog( this, parent, TXT("ShaderViewDialog") );
	Layout();


	
}



