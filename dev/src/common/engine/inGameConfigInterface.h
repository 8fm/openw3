/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace InGameConfig
{
	/*********************** Config Enums ***********************/

	enum EConfigValueType
	{
		eConfigValueType_Int32,
		eConfigValueType_Float,
		eConfigValueType_Bool,
		eConfigValueType_String,
	};

	enum EConfigVarSetType
	{
		eConfigVarAccessType_Loaded,
		eConfigVarAccessType_UserAction,
	};

	/*********************** Config Interface ***********************/

	class CConfigVarValue;
	struct SConfigPresetOption;

	class IConfigVar
	{
	public:
		virtual ~IConfigVar() {}
		virtual const String GetDisplayType() const = 0;
		virtual const String GetDisplayName() const = 0;
		virtual const CConfigVarValue GetValue() const = 0;
		virtual void SetValue( const CConfigVarValue value, const EConfigVarSetType setType ) = 0;
		virtual void ListOptions( TDynArray< SConfigPresetOption >& output ) const = 0;
		virtual const CName GetConfigId() const = 0;
		virtual Bool HasTag( CName tag ) const = 0;
		virtual const THashSet<CName>& GetTags() const = 0;
		virtual Bool IsVisible() const = 0;
		virtual void ResetToDefault() = 0;
	};

	class IConfigGroup
	{
	public:
		virtual ~IConfigGroup() {}
		virtual const String GetDisplayName() const = 0;
		virtual void ListConfigVars( TDynArray< IConfigVar* >& output ) = 0;
		virtual void ListPresets( TDynArray< SConfigPresetOption >& output ) const = 0;
		virtual void ApplyPreset( const Int32 id, const EConfigVarSetType setType ) = 0;
		virtual const Int32 GetActivePreset() const = 0;
		virtual const CName GetConfigId() const = 0;
		virtual Bool HasTag( CName tag ) const = 0;
		virtual const THashSet<CName>& GetTags() const = 0;
		virtual Bool IsVisible() const = 0;
		virtual void ResetToDefault() = 0;
	};

	class IConfigDynamicGroup : public IConfigGroup
	{
	public:
		virtual void Discard() = 0;
	};

	/*********************** Config Var Value ***********************/

	class CConfigVarValue
	{
	public:
		explicit CConfigVarValue( const Int32 value )
			: m_valueStr( ToString( value ) )
		{

		}

		explicit CConfigVarValue( const Float value )
			: m_valueStr( ToString( value ) )
		{

		}

		explicit CConfigVarValue( const Bool value )
			: m_valueStr( ToString( value ) )
		{

		}

		explicit CConfigVarValue( const String value )
			: m_valueStr( value )
		{

		}

		const Int32 GetAsInt() const
		{
			Int32 result = 0;
			Bool isOk = FromString( m_valueStr, result );
			RED_ASSERT( isOk, TXT("Invalid config variable value - can't parse '%s' to Int32"), m_valueStr.AsChar() );
			if( isOk == false )
			{
				RED_LOG( Configuration, TXT("Invalid config variable value - can't parse '%s' to Int32"), m_valueStr.AsChar() );
			}
			return result;
		}

		RED_INLINE const Int32 GetAsInt( const Int32 defaultValue ) const	// Returns default value if can't parse from string
		{
			Int32 result = defaultValue;
			FromString( m_valueStr, result );
			return result;
		}

		const Float GetAsFloat() const
		{
			Float result = 0;
			Bool isOk = FromString( m_valueStr, result );
			RED_ASSERT( isOk, TXT("Invalid config variable value - can't parse '%s' to Float"), m_valueStr.AsChar() );
			if( isOk == false )
			{
				RED_LOG( Configuration, TXT("Invalid config variable value - can't parse '%s' to Float"), m_valueStr.AsChar() );
			}
			return result;
		}

		RED_INLINE const Float GetAsFloat( const Float defaultValue ) const	// Returns default value if can't parse from string
		{
			Float result = defaultValue;
			FromString( m_valueStr, result );
			return result;
		}

		const Bool GetAsBool() const
		{
			Bool result = 0;
			Bool isOk = FromString( m_valueStr, result );
			RED_ASSERT( isOk, TXT("Invalid config variable value - can't parse '%s' to Bool"), m_valueStr.AsChar() );
			if( isOk == false )
			{
				RED_LOG( Configuration, TXT("Invalid config variable value - can't parse '%s' to Bool"), m_valueStr.AsChar() );
			}
			return result;
		}

		RED_INLINE const Bool GetAsBool( const Bool defaultValue ) const	// Returns default value if can't parse from string
		{
			Bool result = defaultValue;
			FromString( m_valueStr, result );
			return result;
		}

		RED_INLINE const String GetAsString() const
		{
			return m_valueStr;
		}

	private:
		const String m_valueStr;
	};

	struct SConfigPresetOption
	{
		SConfigPresetOption() {}
		SConfigPresetOption( const Int32 id, const String displayName )
			: id( id )
			, displayName( displayName )
		{
			/* Intentionally Empty */
		}

		Int32 id;
		String displayName;
	};

	struct SConfigPresetEntry
	{
		SConfigPresetEntry() { /* Intentionally empty */ }
		SConfigPresetEntry( Int32 presetId, CName& groupId, CName& varId, String& value ) : presetId(presetId), groupId( groupId ), varId(varId), value(value) { /* Intentionally empty */ }

		Int32 presetId;
		CName groupId;		// By default it's the same as in config group, but you can override it in config matrix file
		CName varId;
		String value;
	};
}
