#pragma once

#include "propertyConverter.h"

template < class T >
Bool IAIParameters::GetParameter( CName name, T& outVal ) const
{
#ifdef DEBUG_AI_INITIALIZATION
	ASSERT( m_initialized );
#endif
	auto itFind = m_paramsMap.Find( name );
	if ( itFind != m_paramsMap.End() )
	{
		if ( PropertyConverter::ConvertProperty( this, itFind->m_second, outVal ) )
		{
			if ( PropertyConverter::TTypeSpecifiedCode< T >::PostConversionAcceptance( outVal ) )
			{
				return true;
			}
		}
	}
	for ( auto it = m_subParameters.Begin(), end = m_subParameters.End(); it != end; ++it )
	{
		if ( (*it)->GetParameter( name, outVal ) )
		{
			return true;
		}
	}
	return false;
}
