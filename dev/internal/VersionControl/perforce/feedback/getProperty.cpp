/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "getProperty.h"

namespace VersionControl
{
	namespace Feedback
	{
		GetProperty::GetProperty( const char* key )
		:	m_key( key )
		{

		}

		GetProperty::~GetProperty()
		{

		}

		void GetProperty::OutputStat( StrDict* varList )
		{
			StrPtr* value = varList->GetVar( m_key );

			if( value )
			{
				m_value.Set( value );
			}
		}
	}
}
