/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialParameter.h"
#include "materialGraph.h"
#include "renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CMaterialParameter );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialParameter::OnPropertyPostChange( IProperty *property )
{
	// Name changed
	if ( property->GetName() == CNAME( parameterName ) )
	{
		// Invalidate block layout because caption will change
		InvalidateLayout();

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
		// Update parameters in the base shader
		GetGraph()->UpdateParametersList();
#endif // NO_RUNTIME_MATERIAL_COMPILATION
	}

	TBaseClass::OnPropertyPostChange( property );
}

Color CMaterialParameter::GetTitleColor() const
{
	// Use special color when its named parameter
	if ( m_parameterName )
	{
		return Color( 64, 128, 255 );
	}
	else
	{
		// Get default color
		return TBaseClass::GetTitleColor();
	}
}

String CMaterialParameter::FormatParamCaption( const String& defaultCaption ) const
{
	if ( m_parameterName )
	{
		// Use parameter name as caption
		return String::Printf( TXT( "%s (%s)" ), defaultCaption.AsChar(), m_parameterName.AsString().AsChar() );
	}
	else
	{
		// Get default caption for this class
		return defaultCaption;
	}
}

void CMaterialParameter::SetParameterName(const CName paramName )
{
	m_parameterName = paramName;
}

#endif
