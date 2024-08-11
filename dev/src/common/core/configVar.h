/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "stringConversion.h"

namespace Config
{
	/// The way the value is being changed
	enum EConfigVarSetMode : Uint8
	{
		eConfigVarSetMode_Initial,		//!< Value was loaded for the first time from the config file
		eConfigVarSetMode_Reload,		//!< Value was reloaded from the config file
		eConfigVarSetMode_Console,		//!< Value is being changed from the console (by developer)
		eConfigVarSetMode_User,			//!< Value is being changed by the user (from UI, etc)
		eConfigVarSetMode_Remote,		//!< Value is being changed remotely
	};

	/// Type of console variable
	enum EConfigVarType : Uint8
	{
		eConsoleVarType_Bool,			//!< On off
		eConsoleVarType_Int,			//!< Any integer (may be limited by variable)
		eConsoleVarType_Float,			//!< Any real value (may be limited by variable) 
		//eConsoleVarType_Vector,		//!< Any 2D, 3D or 4D vector - not needed right now
		eConsoleVarType_String,			//!< General string
	};

	/// Console variable flags
	enum EConfigVarFlags : Uint8
	{
		eConsoleVarFlag_Save			= FLAG( 0 ),	//!< Value is saved in the user config if modified
		eConsoleVarFlag_Developer		= FLAG( 1 ),	//!< Value is only accessible by developer
		eConsoleVarFlag_ReadOnly		= FLAG( 2 ),	//!< Variable cannot be changed in the console (may be changed using C++ code)
	};

	/// Configuration variable that is read/written to config files and is also changable in runtime
	/// Base abstract class - should not be created dynamically
	class IConfigVar
	{
	public:
		IConfigVar( const AnsiChar* group, const AnsiChar* name, EConfigVarType type, const Uint32 flags = 0 );
		virtual ~IConfigVar();

		// Read current value
		virtual Bool GetText( String& outValue ) const = 0;

		// Get raw value
		virtual Bool GetRaw( void* outValue, const EConfigVarType expectedType ) const = 0;

		// Set current value
		virtual Bool SetText( const String& value, const EConfigVarSetMode mode ) = 0;

		// Set raw value
		virtual Bool SetRaw( const void* value, const EConfigVarType expectedType, const EConfigVarSetMode mode ) = 0;

		// Read the default value
		virtual Bool GetTextDefault( String& outValue ) const = 0;
		virtual Bool GetRawDefault( void* outValue, const EConfigVarType expectedType ) const = 0;

		// Get name of the variable
		RED_FORCE_INLINE const AnsiChar* GetName() const { return m_name; }

		// Get group of the variable
		RED_FORCE_INLINE const AnsiChar* GetGroup() const { return m_group; }

		// Get type description
		RED_FORCE_INLINE EConfigVarType GetType() const { return m_type; }

		// Check flag description
		RED_FORCE_INLINE Bool HasFlag( const EConfigVarFlags flag ) const { return 0 != (m_flags & flag); }

		// Reset config to default value
		virtual Bool Reset( const EConfigVarSetMode mode ) = 0;

	protected:
		const AnsiChar*		m_group;		//!< Configuration group
		const AnsiChar*		m_name;			//!< Configuration variable name
		EConfigVarType		m_type;			//!< Console variable type
		Uint8				m_flags;		//!< Flags

		void Register();
	};

	/// Resolves compile time type to a console variable type
	namespace Helper
	{
		template< typename T >
		struct ResolveConsoleType {};

		template<>
		struct ResolveConsoleType<Bool> { static const EConfigVarType value = eConsoleVarType_Bool; };

		template<>
		struct ResolveConsoleType<Int32> { static const EConfigVarType value = eConsoleVarType_Int; };

		template<>
		struct ResolveConsoleType<Float> { static const EConfigVarType value = eConsoleVarType_Float; };

		template<>
		struct ResolveConsoleType<String> { static const EConfigVarType value = eConsoleVarType_String; };

	} // Helper

	/// Validation helpers
	namespace Validation
	{
		/// No validation/limiter
		class Always
		{
		public:
			template< typename T >
			RED_FORCE_INLINE static Bool Validate( T& ret )
			{
				RED_UNUSED( ret );
				return true;
			}
		};

		/// Integer value limiter, auto clamp
		template< Int32 rangeMin, Int32 rangeMax >
		class IntRange
		{
		public:
			RED_FORCE_INLINE static Bool Validate( Int32& ret )
			{
				ret = Red::Math::NumericalUtils::Clamp< Int32 >( ret, rangeMin, rangeMax );
				return true;
			}
		};

		/// Real value limiter, auto clamp
		template< Int32 rangeMin, Int32 rangeMax, Int32 scale=1000 >
		class FloatRange
		{
		public:
			RED_FORCE_INLINE static Bool Validate( Float& ret )
			{
				const Float floatRangeMin = (Float)rangeMin / (Float)scale;
				const Float floatRangeMax = (Float)rangeMax / (Float)scale;
				ret = Red::Math::NumericalUtils::Clamp< Float >( ret, floatRangeMin, floatRangeMax );
				return true;
			}
		};
	} // Validation

	/// Typed console variable with it's own memory
	template< typename T, typename Validator = Validation::Always >
	class TConfigVarBinding : public IConfigVar
	{
	public:
		RED_INLINE TConfigVarBinding( const AnsiChar* group, const AnsiChar* name, T* binding, const T defaultValue, const Uint32 flags = 0 )
			: IConfigVar( group, name, Helper::ResolveConsoleType<T>::value, flags )
			, m_binding( binding )
			, m_defaultValue( defaultValue )
		{
			IConfigVar::Register();
		}

	private:
		// Called to validate the value that is being set, value can be modified or the change dropped
		virtual Bool OnValidate( T& value, const EConfigVarSetMode mode ) const
		{
			RED_UNUSED( mode );
			return Validator::Validate( value );
		};

		// Called after the value was successfully set
		virtual void OnChanged( const EConfigVarSetMode mode ) { RED_UNUSED(mode); }

		virtual Bool GetText( String& outValue ) const override
		{
			T ret;
			if ( !GetRaw( &ret, GetType() ) )
				return false;

			outValue = ::ToString<T>( ret );
			return true;
		}

		virtual Bool GetTextDefault( String& outValue ) const override
		{
			T ret;
			if ( !GetRawDefault( &ret, GetType() ) )
				return false;

			outValue = ::ToString<T>( ret );
			return true;
		}

		virtual Bool SetText( const String& value, const EConfigVarSetMode mode ) override
		{
			// parse
			T ret;
			if ( !::FromString<T>( value, ret ) )
				return false;

			// set value
			return SetRaw( &ret, GetType(), mode );
		}

		virtual Bool GetRaw( void* outValue, const EConfigVarType expectedType ) const override
		{
			if ( expectedType != GetType() )
			{
				WARN_CORE( TXT("Invalid access type for console variable '%ls'"), ANSI_TO_UNICODE( GetName() ) );
				return false;
			}

			if ( !m_binding )
			{
				WARN_CORE( TXT("Invalid binding for console variable '%ls'"), ANSI_TO_UNICODE( GetName() ) );
				return false;
			}

			*(T*)outValue = *m_binding;
			return true;
		}

		virtual Bool SetRaw( const void* value, const EConfigVarType expectedType, const EConfigVarSetMode mode ) override
		{
			if ( expectedType != GetType() )
			{
				WARN_CORE( TXT("Invalid access type for console variable '%ls'"), ANSI_TO_UNICODE( GetName() ) );
				return false;
			}

			if ( !m_binding )
			{
				WARN_CORE( TXT("Invalid binding for console variable '%ls'"), ANSI_TO_UNICODE( GetName() ) );
				return false;
			}

			T temp( *(T*)value );
			if ( !OnValidate( temp, mode ) )
				return false;

			*m_binding = temp;
			OnChanged( mode );

			return true;
		}

		virtual Bool GetRawDefault( void* outValue, const EConfigVarType expectedType ) const override
		{
			if ( expectedType != GetType() )
			{
				WARN_CORE( TXT("Invalid access type for console variable '%ls'"), ANSI_TO_UNICODE( GetName() ) );
				return false;
			}

			if ( !m_binding )
			{
				WARN_CORE( TXT("Invalid binding for console variable '%ls'"), ANSI_TO_UNICODE( GetName() ) );
				return false;
			}

			*(T*)outValue = m_defaultValue;
			return true;
		}

		virtual Bool Reset( const EConfigVarSetMode mode ) override
		{
			return SetRaw( &m_defaultValue, GetType(), mode );
		}

		// the value binding
		T*		m_binding;
		T		m_defaultValue;
	};

	/// In place console variable
	template< typename T, typename Validator = Validation::Always >
	class TConfigVar : public TConfigVarBinding< T, Validator >
	{
	public:
		RED_INLINE TConfigVar( const AnsiChar* group, const AnsiChar* name, const T& defaultValue, const Uint32 flags = 0 )
			: TConfigVarBinding< T, Validator >( group, name, &m_value, defaultValue, flags )
			, m_value( defaultValue )
		{
			IConfigVar::Register();
		}

		// Get value (directly)
		RED_FORCE_INLINE const T& Get() const { return m_value; }

		// Set value (directly), NO CALLBACKS! Callbacks are evil
		RED_FORCE_INLINE void Set( const T& value ) { m_value = value; }

	private:
		// the actual value
		T		m_value;
	};

} // Console