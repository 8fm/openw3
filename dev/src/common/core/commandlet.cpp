/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "commandlet.h"

IMPLEMENT_ENGINE_CLASS( ICommandlet );

Bool ICommandlet::CommandletOptions::IsAnOption( const String & arg ) const
{
	if( ( arg.GetLength() >= 2 && arg[0] == '-' &&  isalpha( arg[1] ) ) || ( arg.GetLength() >= 3 && arg[1] == '-' && isalpha( arg[2] ) ) )
	{
		return true;
	}
	return false;
}

Bool ICommandlet::CommandletOptions::HasOption( const String &shortVer, const String &longVer ) const
{
	return HasOption( shortVer ) || HasOption( longVer );
}

Bool ICommandlet::CommandletOptions::HasOption( const String &longVer ) const
{
	return m_options.KeyExist( longVer );
}

TList<String> ICommandlet::CommandletOptions::GetOptionValues( const String &shortVer, const String &longVer ) const
{
	TList<String> values;

	if( HasOption( shortVer ) )
		values.PushBack( GetOptionValues( shortVer ) );
	
	if( HasOption( longVer ) )
		values.PushBack( GetOptionValues( longVer ) );

	RED_FATAL_ASSERT( values.Size() > 0, "There is no option '%ls', or '%ls'", shortVer.AsChar(), longVer.AsChar() );

	return values;
}

TList<String> ICommandlet::CommandletOptions::GetOptionValues( const String &longVer ) const
{
	return m_options[ longVer ];
}

Bool ICommandlet::CommandletOptions::GetSingleOptionValue( const String &longVer, String& outString ) const
{
	const TList<String>* ptr = m_options.FindPtr( longVer );
	if ( !ptr || ptr->Size() != 1 )
		return false;

	outString = ptr->Front();
	return true;
}

Bool ICommandlet::CommandletOptions::GetSingleOptionValue( const String &shortVer, const String &longVer, String& outString ) const
{
	const TList<String>* ptr = m_options.FindPtr( shortVer );
	if ( !ptr || ptr->Size() != 1 )
	{
		ptr = m_options.FindPtr( longVer );
		if ( !ptr || ptr->Size() != 1 )
		{
			return false;
		}
	}

	outString = ptr->Front();
	return true;
}

Bool ICommandlet::CommandletOptions::ParseCommandline( Uint32 startIdx, const TDynArray< String > &args )
{
	for( auto i = startIdx; i < args.Size(); i++ )
	{
		auto arg = args[i];
		if( IsAnOption( arg ) )
		{
			arg.TrimLeft( '-' );
			auto eqIdx = arg.GetIndex( '=' );
			if( eqIdx == -1 )
			{
				if( m_options.KeyExist( arg ) )
				{					
					// Same option repeated again - just continue					
					continue;
				}
				m_options.Insert( arg, TList< String >() );
			}else
			{
				String left;
				String right;
				if ( arg.Split( TXT( "=" ), &left, &right ) )
				{
					TDynArray< String > values;
					right.Slice( values, TXT(",") );

					for ( Uint32 i=0; i<values.Size(); ++i )
					{
						if ( values[i].Empty() )
							continue;

						if ( m_options.KeyExist( left ) )
						{					
							m_options[left].PushBack( values[i] );
						}
						else
						{
							TList< String > addValues;
							addValues.PushBack( values[i] );
							m_options.Insert( left, addValues );
						}
					}					
				}
				else
				{
					return false;
				}
			}
		}else
		{
			m_arguments.PushBack( arg );
		}
	}
	return true;
}


