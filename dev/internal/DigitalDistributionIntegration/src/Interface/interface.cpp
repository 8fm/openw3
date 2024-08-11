/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "interface.h"

#include "log.h"

Interface::Interface()
{

}

Interface::~Interface()
{

}

Bool Interface::SetStat( const AnsiChar*, Int32 )
{
	return false;
}

Bool Interface::SetStat( const AnsiChar*, Float )
{
	return false;
}

Bool Interface::SetAverageStat( const AnsiChar*, Float, Double )
{
	return false;
}

Bool Interface::IncrementStat( const AnsiChar*, Int32 )
{
	return false;
}

Bool Interface::IncrementStat( const AnsiChar*, Float )
{
	return false;
}

Bool Interface::GetStat( const AnsiChar*, Int32& )
{
	return false;
}

Bool Interface::GetStat( const AnsiChar*, Float& )
{
	return false;
}

Uint32 Interface::GetNumberOfCloudStoragePaths() const
{
	return 0;
}

Bool Interface::GetCloudStoragePath( Uint32, AnsiChar*, size_t ) const
{
	return false;
}

bool Interface::SetDebugLogListener( DebugLogFunc callback )
{
	return DDI_LOG_SETCALLBACK( callback );
}
