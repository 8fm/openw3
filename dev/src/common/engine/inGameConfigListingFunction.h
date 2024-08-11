#pragma once
#include <functional>

namespace InGameConfig
{
	namespace Listing
	{
		/******************** Listing function ********************/
		struct SEngineConfigPresetEntryDesc
		{
			SEngineConfigPresetEntryDesc() { /* Intentionally Empty */ }
			SEngineConfigPresetEntryDesc( CName groupId, CName varId, String value )
				: groupId( groupId )
				, varId( varId )
				, value( value )
			{
				/* Intentionally Empty */
			}

			CName groupId;
			CName varId;
			String value;
		};

		struct SEngineConfigPresetDesc
		{
			String optionDisplayName;
			TDynArray<SEngineConfigPresetEntryDesc> entriesDesc;
		};

		typedef std::function<void(TDynArray<SEngineConfigPresetDesc>&)> ListingFunction;

		class CListingFunctionRegister
		{
		public:
			void RegisterListingFunction( CName name, ListingFunction func )
			{
				RED_FATAL_ASSERT( m_listingFunctions.KeyExist( name ) == false, "InGameConfig: listing function already exist: %ls", UNICODE_TO_ANSI( name.AsChar() ) );

				m_listingFunctions[name] = func;
			}

			void Call( CName name, TDynArray<SEngineConfigPresetDesc>& output )
			{
				if( m_listingFunctions.KeyExist( name ) == true )
				{
					m_listingFunctions[name]( output );
				}
				else
				{
					ERR_ENGINE( TXT("InGameConfig: Can't find listing function with name: %ls"), name.AsChar() );
				}
			}

		private:
			THashMap<CName, ListingFunction> m_listingFunctions;

		};

		typedef TSingleton< CListingFunctionRegister > GListingFunctionRegister;
	}
}
