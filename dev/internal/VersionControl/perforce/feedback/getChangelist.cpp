/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "getChangelist.h"

namespace VersionControl
{
	namespace Feedback
	{
		GetChangelist::GetChangelist()
		{

		}

		GetChangelist::~GetChangelist()
		{

		}

		void GetChangelist::OutputStat( StrDict* varList )
		{
			StrPtr* value = varList->GetVar( "change" );

			if( value )
			{
				m_changelistNumber = *value;
			}
		}
	}
}
