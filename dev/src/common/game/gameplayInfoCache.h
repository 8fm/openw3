/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// The following class holds a lazy initialized runtime info about gameplay entity.
// The first n bits contains information about given functionality.
// The next n bits contains information about whether the information was already evaluated.
// Information is evaluated at the first time it is needed and stored for the future use.
// The proper evaluation code should be placed in Evaluate method or injected through
// CustomEvaluator functor (for r4/r6 specific code).

class CGameplayEntity;

//////////////////////////////////////////////////////////////////////////

enum EGameplayInfoCacheType
{
	GICT_IsInteractive,
	GICT_HasDrawableComponents,
	GICT_Custom0,			// R4: CFocusModeController: IsFocusSoundClue
	GICT_Custom1,			// R4: CombatTargetSelection: IsMonster
	GICT_Custom2,			// R4: W3MonsterClue: testLineOfSight property
	GICT_Custom3,			// R4: CInventoryComponent: HasAssociatedInventory
	GICT_Custom4,			// R4: focus mode visibility was updated
	GICT_Total,
};

BEGIN_ENUM_RTTI( EGameplayInfoCacheType )
	ENUM_OPTION( GICT_IsInteractive );
	ENUM_OPTION( GICT_HasDrawableComponents );
	ENUM_OPTION( GICT_Custom0 );
	ENUM_OPTION( GICT_Custom1 );
	ENUM_OPTION( GICT_Custom2 );
	ENUM_OPTION( GICT_Custom3 );
	ENUM_OPTION( GICT_Custom4 );
END_ENUM_RTTI()

//////////////////////////////////////////////////////////////////////////

struct SGameplayInfoCache
{
	mutable Uint32	m_bits;

public:

	struct DefaultEvaluator
	{
		RED_INLINE Bool operator()( const CGameplayEntity* entity ) const
		{
			return false;
		}
	};

	SGameplayInfoCache()
		: m_bits( 0 )
	{}

	template < typename CustomEvaluator >
	Bool Get( const CGameplayEntity* entity, EGameplayInfoCacheType type, const CustomEvaluator& evaluator = CustomEvaluator() ) const
	{
		const Uint32 valueFlag = 1 << static_cast< Uint32 >( type );
		const Uint32 wasEvaluatedFlag = 1 << ( static_cast< Uint32 > ( GICT_Total ) +  static_cast< Uint32 >( type ) );

		if ( !( m_bits & wasEvaluatedFlag ) )
		{
			m_bits |= wasEvaluatedFlag;
			if ( Evaluate( entity, type, evaluator ) )
			{
				m_bits |= valueFlag;
			}
		}

		return ( m_bits & valueFlag ) != 0;
	}

	RED_INLINE Bool Get( const CGameplayEntity* entity, EGameplayInfoCacheType type ) const
	{
		return Get< DefaultEvaluator >( entity, type );
	}

	void Set( EGameplayInfoCacheType type, Bool flag ) const
	{
		const Uint32 valueFlag = 1 << static_cast< Uint32 >( type );
		const Uint32 wasEvaluatedFlag = 1 << ( static_cast< Uint32 > ( GICT_Total ) +  static_cast< Uint32 >( type ) );

		if ( flag )
		{
			m_bits |= valueFlag;
		}
		else
		{
			m_bits &= ~valueFlag;
		}
		m_bits |= wasEvaluatedFlag;
	}

private:

	Bool IsCustomType( EGameplayInfoCacheType type ) const
	{
		return ( type >= GICT_Custom0 && type < GICT_Total );
	}

	template < typename CustomEvaluator >
	Bool Evaluate( const CGameplayEntity *entity, EGameplayInfoCacheType type, CustomEvaluator& evaluator ) const
	{
		if ( entity == nullptr )
		{
			return false;
		}
		Bool res = false;
		if ( EvaluateInternal( entity, type, res ) )
		{
			return res;
		}
		else if ( IsCustomType( type ) )
		{
			return evaluator( entity );
		}
		else
		{
			RED_ASSERT( false, TXT( "SGameplayFunctionalityRuntimeCache::Evalute: unknown functionality type: %d " ), type );
		}
		return false;
	}

	// We need this in a separate file to resolve problems with CGameplayEntity forward declaration.
	// The method returns true if evaluation occurred and return its result in 'res' param.
	Bool EvaluateInternal(  const CGameplayEntity *entity, EGameplayInfoCacheType type, Bool& res ) const;
};