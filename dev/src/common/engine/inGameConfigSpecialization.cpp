#include "build.h"
#include "inGameConfigSpecialization.h"
#include "baseEngine.h"

namespace InGameConfig
{
	/***************************************************************/
	/******************** Helper Functions *************************/
	/***************************************************************/
	namespace ListingHelper
	{
		void GetAllOptions(const CName& funcName, TDynArray< SConfigPresetOption >& output)
		{
			TDynArray<Listing::SEngineConfigPresetDesc> optionsDesc;
			Listing::GListingFunctionRegister::GetInstance().Call( funcName, optionsDesc );

			for( Uint32 i=0; i<optionsDesc.Size(); ++i )
			{
				output.PushBack( SConfigPresetOption( i, optionsDesc[i].optionDisplayName ) );
			}
		}

		void GetEntries(const CName& funcName, Int32 optionIdx, TDynArray< SConfigPresetEntry >& output)
		{
			TDynArray<Listing::SEngineConfigPresetDesc> optionsDesc;
			Listing::GListingFunctionRegister::GetInstance().Call( funcName, optionsDesc );

			if( 0 <= optionIdx && optionIdx < (Int32)optionsDesc.Size() )
			{
				Listing::SEngineConfigPresetDesc& optDesc = optionsDesc[optionIdx];
				for( Listing::SEngineConfigPresetEntryDesc& entry : optDesc.entriesDesc )
				{
					output.PushBack( SConfigPresetEntry( optionIdx, entry.groupId, entry.varId, entry.value ) );
				}
			}
		}

		void GetAllEntries(const CName& funcName, TDynArray< SConfigPresetEntry >& output)
		{
			TDynArray<Listing::SEngineConfigPresetDesc> optionsDesc;
			Listing::GListingFunctionRegister::GetInstance().Call( funcName, optionsDesc );

			for( Uint32 optionIdx = 0; optionIdx < optionsDesc.Size(); ++optionIdx )
			{
				if( optionsDesc.Size() > 0 && optionIdx < (Int32)optionsDesc.Size() )
				{
					Listing::SEngineConfigPresetDesc& optDesc = optionsDesc[optionIdx];
					for( Listing::SEngineConfigPresetEntryDesc& entry : optDesc.entriesDesc )
					{
						output.PushBack( SConfigPresetEntry( optionIdx, entry.groupId, entry.varId, entry.value ) );
					}
				}
			}
		}

		void GetAllOptionAndAllEntries(const CName& funcName, TDynArray< SConfigPresetOption >& outOptions, TDynArray< SConfigPresetEntry >& outEntries)
		{
			TDynArray<Listing::SEngineConfigPresetDesc> optionsDesc;
			Listing::GListingFunctionRegister::GetInstance().Call( funcName, optionsDesc );

			for( Uint32 optionIdx = 0; optionIdx < optionsDesc.Size(); ++optionIdx )
			{
				outOptions.PushBack( SConfigPresetOption( optionIdx, optionsDesc[optionIdx].optionDisplayName ) );

				if( optionsDesc.Size() > 0 && optionIdx < (Int32)optionsDesc.Size() )
				{
					Listing::SEngineConfigPresetDesc& optDesc = optionsDesc[optionIdx];
					for( Listing::SEngineConfigPresetEntryDesc& entry : optDesc.entriesDesc )
					{
						outEntries.PushBack( SConfigPresetEntry( optionIdx, entry.groupId, entry.varId, entry.value ) );
					}
				}
			}
		}

	}

	namespace CompareHelper
	{
		Bool IsCharNum(const Char chr)
		{
			if ( (chr >= TXT('0') && chr <= TXT('9')) )
			{
				return true;
			}

			return  false;
		}

		Bool IsNumber(const String& str)
		{
			Uint32 length = str.GetLength();

			Uint32 startIdx = 0;
			if( length > 0 && str[0] == '-' )		// First character can be minus
				startIdx++;

			for( Uint32 i=startIdx; i<length; ++i )
			{
				// Is not - is num or dot with previous character as num (so you can't type '.0')
				if( IsCharNum( str[i] ) == false && !(str[i] == '.' && i>0 && IsCharNum( str[i-1] )) )
					return false;
			}

			return true;
		}

		Bool Equals( const String& valueA, const String& valueB )
		{
			Float valueAFlt;
			Float valueBFlt;
			if( IsNumber(valueA) == true &&
				IsNumber(valueB) == true &&
				FromString( valueA, valueAFlt ) == true &&
				FromString( valueB, valueBFlt ) == true )
			{
				return Red::Math::MAbs( valueAFlt - valueBFlt ) <= NumericLimits< Float >::Epsilon();
			}
			else
			{
				return valueA == valueB;
			}
		}
	}

	/************************************************************/
	/******************** Generic Group *************************/
	/************************************************************/

	Bool CConfigGroupEngineConfig::IsVisible() const
	{
		if( m_visibilityCondition != nullptr )
		{
			return m_visibilityCondition->Evaluate();
		}

		return true;
	}

	const THashSet<CName>& CConfigGroupEngineConfig::GetTags() const
	{
		return m_tags;
	}

	Bool CConfigGroupEngineConfig::HasTag(CName tag) const
	{
		return m_tags.Exist( tag );
	}

	const CName CConfigGroupEngineConfig::GetConfigId() const
	{
		return m_name;
	}

	const Int32 CConfigGroupEngineConfig::GetActivePreset() const
	{
		Int32 result;		// Start with last preset and iterate to -1 (which is always custom)

		for( result = m_presets.Size()-1; result > -1; --result )
		{
			Bool doMatchPreset = true;

			for( auto& entry : m_presetEntries )
			{
				if( entry.presetId == result )
				{
					String currentValue;
					Bool found = false;

					// Look for variables with options
					for( auto var : m_vars )
					{
						if( var->GetConfigId() == entry.varId )
						{
							currentValue = var->GetValue().GetAsString();
							found = true;
							break;
						}
					}

					if( found == false )
					{
						SConfig::GetInstance().GetValue( entry.groupId.AsAnsiChar(), entry.varId.AsAnsiChar(), currentValue );
					}

					if( CompareHelper::Equals( currentValue, entry.value ) == false )
					{
						doMatchPreset = false;
					}
				}
			}

			if( doMatchPreset == true )
			{
				break;
			}
		}

		return result;
	}

	void CConfigGroupEngineConfig::ApplyPreset(const Int32 presetId, const EConfigVarSetType setType)
	{
		Int32 properPresetId = Red::Math::NumericalUtils::Clamp<Int32>( presetId, 0, m_presets.Size()-1 );

		for( auto& entry : m_presetEntries )
		{
			if( entry.presetId == properPresetId )
			{
				// Look for variables with options
				Bool found = false;
				for( auto var : m_vars )
				{
					if( var->GetConfigId() == entry.varId )
					{
						var->SetValue( CConfigVarValue( entry.value ), setType );
						found = true;
						break;
					}
				}

				if( found == false )
				{
					SConfig::GetInstance().SetValue( entry.groupId.AsAnsiChar(), entry.varId.AsAnsiChar(), entry.value );
				}
			}
		}

		if( setType == eConfigVarAccessType_UserAction )
		{
			DoNeedToRefreshEngine();
		}
	}

	void CConfigGroupEngineConfig::ListPresets(TDynArray< SConfigPresetOption >& output) const
	{
		output.PushBack( m_presets );
	}

	void CConfigGroupEngineConfig::ListConfigVars(TDynArray< IConfigVar* >& output)
	{
		output.PushBack( m_vars );
	}

	const String CConfigGroupEngineConfig::GetDisplayName() const
	{
		return m_displayName;
	}

	void CConfigGroupEngineConfig::Discard()
	{
		m_vars.ClearPtr();
		if( m_visibilityCondition != nullptr )
		{
			m_visibilityCondition->Destroy();
		}
	}

	/***************************************************************/
	/******************** Generic Variable *************************/
	/***************************************************************/

	CConfigGroupEngineConfig::CConfigGroupEngineConfig(CName& name, String& displayName, TDynArray< SConfigPresetOption >& presets, TDynArray< SConfigPresetEntry > presetEntries, TDynArray< IConfigVar* >& vars, THashSet<CName> tags, BoolExpression::IExp* visibilityCondition) : m_name( name )
		, m_displayName( displayName )
		, m_presets( presets )
		, m_presetEntries( presetEntries )
		, m_vars( vars )
		, m_tags( tags )
		, m_visibilityCondition( visibilityCondition )
	{
		/* Intentionally empty */
	}

	CConfigGroupEngineConfig::CConfigGroupEngineConfig()
	{
		/* Intentionally empty */
	}

	void CConfigGroupEngineConfig::DoNeedToRefreshEngine()
	{
		CRefreshEventDispatcher& dispatcher = GRefreshEventDispatcher::GetInstance();

		for( CName& tag : m_tags )
		{
			dispatcher.DispatchIfExists( tag );
		}
	}

	void CConfigGroupEngineConfig::ResetToDefault()
	{
		Config::CConfigSystem& configSys = SConfig::GetInstance();

		for( IConfigVar* var : m_vars )
		{
			var->ResetToDefault();
		}

		DoNeedToRefreshEngine();
	}

	Bool CConfigVarEngineConfig::IsVisible() const
	{
		if( m_visibilityCondition == nullptr )
			return true;

		return m_visibilityCondition->Evaluate();
	}

	const THashSet<CName>& CConfigVarEngineConfig::GetTags() const
	{
		return m_tags;
	}

	Bool CConfigVarEngineConfig::HasTag(CName tag) const
	{
		return m_tags.Exist( tag );
	}

	const CName CConfigVarEngineConfig::GetConfigId() const
	{
		return m_name;
	}

	void CConfigVarEngineConfig::ListOptions(TDynArray< SConfigPresetOption >& output) const
	{
		// Use listing function if available, else use initialized options
		if( m_listingFunction != CName::NONE )
		{
			ListingHelper::GetAllOptions( m_listingFunction, output );
		}
		else
		{
			output.PushBack( m_options );
		}
	}

	void CConfigVarEngineConfig::SetValue(const CConfigVarValue value, const EConfigVarSetType setType)
	{
		if( m_listingFunction != CName::NONE )
		{
			m_activeOption = value.GetAsInt( 0 );
			TDynArray< SConfigPresetEntry > optionEntries;
			ListingHelper::GetEntries( m_listingFunction, m_activeOption, optionEntries );

			for( auto& entry : optionEntries )
			{
				SConfig::GetInstance().SetValue( entry.groupId.AsAnsiChar(), entry.varId.AsAnsiChar(), entry.value );
			}
		}
		else if( m_options.Size() > 0 )
		{
			m_activeOption = value.GetAsInt( 0 );
			for( auto& entry : m_optionEntries )
			{
				if( entry.presetId == m_activeOption )
				{
					SConfig::GetInstance().SetValue( entry.groupId.AsAnsiChar(), entry.varId.AsAnsiChar(), entry.value );
				}
			}
		}
		else
		{
			SConfig::GetInstance().SetValue( m_groupName.AsAnsiChar(), m_name.AsAnsiChar(), value.GetAsString() );
		}

		if( setType == eConfigVarAccessType_UserAction )
		{
			DoNeedToRefreshEngine();
		}
	}

	const CConfigVarValue CConfigVarEngineConfig::GetValue() const
	{
		String outValue = TXT("");
		if( m_listingFunction != CName::NONE )		// If variable is option variable and uses listing function
		{
			TDynArray< SConfigPresetOption > options;
			TDynArray< SConfigPresetEntry > optionEntries;
			ListingHelper::GetAllOptionAndAllEntries( m_listingFunction, options, optionEntries );

			Int32 activeOption = GetMatchingOptionIdx( options, optionEntries );
			outValue = ToString( activeOption );
		}
		else if( m_options.Size() > 0 )				// If variable is option variable and uses regular options from XML
		{
			Int32 activeOption = GetMatchingOptionIdx( m_options, m_optionEntries );
			outValue = ToString( activeOption );
		}
		else										// If variable is regular variable
		{
			SConfig::GetInstance().GetValue( m_groupName.AsAnsiChar(), m_name.AsAnsiChar(), outValue );
		}

		outValue = outValue;

		return CConfigVarValue( outValue );
	}

	const String CConfigVarEngineConfig::GetDisplayName() const
	{
		return m_displayName;
	}

	const String CConfigVarEngineConfig::GetDisplayType() const
	{
		return m_displayType;
	}

	CConfigVarEngineConfig::~CConfigVarEngineConfig()
	{
		if( m_visibilityCondition != nullptr )
			m_visibilityCondition->Destroy();
	}

	CConfigVarEngineConfig::CConfigVarEngineConfig(SConfigVarCreationDesc& desc) : m_groupName( desc.groupName )
		, m_name( desc.name )
		, m_displayName( desc.displayName )
		, m_displayType( desc.displayType )
		, m_options( desc.options )
		, m_optionEntries( desc.optionEntries )
		, m_activeOption( 0 )
		, m_tags( desc.tags )
		, m_visibilityCondition( desc.visibilityCondition )
		, m_listingFunction( desc.listingFunction )
	{
		/* Intentionally empty */
	}

	CConfigVarEngineConfig::CConfigVarEngineConfig()
	{
		/* Intentionally empty */
	}

	Int32 CConfigVarEngineConfig::GetMatchingOptionIdx(const TDynArray< SConfigPresetOption >& options, const TDynArray< SConfigPresetEntry >& entries) const
	{
		Int32 result;		// Start with last option and iterate to -1 (which is unknown)

		for( result = options.Size()-1; result > -1; --result )
		{
			Bool doMatchPreset = true;

			for( const SConfigPresetEntry& entry : entries )
			{
				if( entry.presetId == result )
				{
					String currentValue;
					Bool found = false;

					if( found == false )
					{
						SConfig::GetInstance().GetValue( entry.groupId.AsAnsiChar(), entry.varId.AsAnsiChar(), currentValue );
					}

					if( CompareHelper::Equals( currentValue, entry.value ) == false )
					{
						doMatchPreset = false;
					}
				}
			}

			if( doMatchPreset == true )
			{
				break;
			}
		}

		return result;
	}

	void CConfigVarEngineConfig::DoNeedToRefreshEngine()
	{
		CRefreshEventDispatcher& dispatcher = GRefreshEventDispatcher::GetInstance();

		for( CName& tag : m_tags )
		{
			dispatcher.DispatchIfExists( tag );
		}
	}

	RED_INLINE void CConfigVarEngineConfig::ResetToDefault( const CName& group, const CName& key )
	{
		Config::CConfigSystem& configSys = SConfig::GetInstance();
		String defaultValue;

		const AnsiChar* groupName = group.AsAnsiChar();
		const AnsiChar* keyName = key.AsAnsiChar();

		if( configSys.GetDefaultValue( groupName, keyName, defaultValue ) )
		{
			configSys.SetValue( groupName, keyName, defaultValue );
		}
		else
		{
			RED_LOG_ERROR( Config, TXT( "No default value specified for in game option %hs.%hs" ), groupName, keyName );
		}
	}

	void CConfigVarEngineConfig::ResetToDefault()
	{
		const Uint32 numOptionEntries = m_optionEntries.Size();
		if( numOptionEntries > 0 )
		{
			for( Uint32 i = 0; i < numOptionEntries; ++i )
			{
				SConfigPresetEntry& entry = m_optionEntries[ i ];

				ResetToDefault( entry.groupId, entry.varId );
			}
		}
		else
		{
			ResetToDefault( m_groupName, m_name );
		}
	}
}
