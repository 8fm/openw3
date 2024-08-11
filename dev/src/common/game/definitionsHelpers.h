/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../redMath/random/noise.h"

class CDefinitionsHelpers
{
public:

	class DefaultDefinitionValidator
	{
	public:

		template < typename T >
		bool operator()( T* ) const
		{
			return true;
		}
	};

	// To use ChooseDefinitions methods::
	// - definition class (T) should implement two methods: 
	//									                    - CName GetName()
	//									                    - Float GetChance()
	// - validator class (V) should implement operator()( const T* ) (see DefaultDefinitionValidator example above)
	// - randSeed == 0 means no static rand seed

	template < typename T, typename V >
	static void ChooseDefinitions( const TDynArray< T > & definitions,  TDynArray< T > & chosenDefinitions, Uint32 quantity, const V& definitionValidator, Uint32 randSeed );
	template < typename T >
	static void ChooseDefinitions( const TDynArray< T > & definitions,  TDynArray< T > & chosenDefinitions, Uint32 quantity )
	{
		ChooseDefinitions( definitions, chosenDefinitions, quantity, DefaultDefinitionValidator(), 0 );
	}

	// "In-place" versions of ChooseDefinitions.
	// The following methods reorganize the initial array, so we don't need to use additional arrays (but also we don't preserve initial order)
	// Returned value is the number of n-first elements that have been chosen.
	template < typename T, typename V >
	static Uint32 ChooseDefinitions( TDynArray< T > & definitions, Uint32 quantity, const V& definitionValidator, Uint32 randSeed );
	template < typename T >
	static Uint32 ChooseDefinitions( TDynArray< T > & definitions,  Uint32 quantity )
	{
		return ChooseDefinitions( definitions, quantity, DefaultDefinitionValidator(), 0 );
	}

	// Copies definition name (GetName) to CName array
	// 'count' param represents number of items to be copied (or all if equal to default -1)
	template < typename T >
	static void CopyNames( const TDynArray< T > & definitions, TDynArray< CName > & names, Int32 count = -1 );

	template < typename T >
	static void ValidateDefinitions( const TDynArray< T > & defItems, TDynArray< T > & validItems );

private:

	template < typename T >
	static void ChooseRequiredDefinitions( TDynArray< T > & requiredDefinitions,  TDynArray< T > & chosenDefinitions, Uint32 quantity, Uint32 randSeed );
	template < typename T >
	static void ChooseAdditionalDefinitions( TDynArray< T > & additionalDefinitions,  TDynArray< T > & chosenDefinitions, Uint32 quantity, Float totalChance, Uint32 randSeed );

	// in-place version of two methods above
	template < typename T >
	static void ChooseRequiredDefinitions( TDynArray< T > & definitions,  Uint32 offset, Uint32 count, Uint32 quantity, Uint32 randSeed );
	template < typename T >
	static void ChooseAdditionalDefinitions( TDynArray< T > & definitions, Uint32 offset, Uint32 count, Uint32 quantity, Float totalChance, Uint32 randSeed );

	// Use SFINAE to resolve element accessors so we don't need re-implement or specialise all the Choose* functions
	// for arrays of pointers and raw structs
	template< typename T, class Enable = void >
	struct WrapAccessors
	{
		static Float GetChance( const T& data ) { return data.GetChance(); }
		static CName GetName( const T& data ) { return data.GetName(); }
	};

	template< typename T >
	struct WrapAccessors< T, typename std::enable_if<std::is_pointer<T>::value >::type >
	{
		static Float GetChance( const T& data )	{ return data->GetChance(); }
		static CName GetName( const T& data )	{ return data->GetName(); }
	};

	template< typename T, typename V, class Enable = void >
	struct WrapValidate
	{
		static bool Validate( const T& data, const V& validator )	{ return validator( &data ); }
	};

	template< typename T, typename V >
	struct WrapValidate< T, V, typename std::enable_if<std::is_pointer<T>::value >::type >
	{
		static bool Validate( const T& data, const V& validator )	{ return validator( data ); }
	};
};

// The following algorithm chooses "quantity" number of definitions
// based on their chances (treated as weights). Definitions with chance equal
// to -1 (100%) are considered as "required" and have higher priority.
// The final number of returned definitions must be equal to "quantity" value.
// If the number of "required" definitions is greater than "quantity"
// then algorithm randomly chooses "quantity" of them.
// Optional argument is a reference to "validator" function object
// which takes pointer to a definition and returns true if definition is valid.
template < typename T, typename V >
void CDefinitionsHelpers::ChooseDefinitions( const TDynArray< T > & definitions, TDynArray< T > & chosenDefinitions, Uint32 quantity, const V& definitionValidator, Uint32 randSeed )
{
	chosenDefinitions.ClearFast();
	if ( quantity == 0 )
	{
		return;
	}

	TDynArray< T > requiredDefinitions;
	TDynArray< T > additionalDefinitions;

	if( definitions.Size() > 0 )
	{
		auto halfArraySize = definitions.Size() >> 1;
		requiredDefinitions.Reserve( halfArraySize );
		additionalDefinitions.Reserve( halfArraySize );
	}

	// validating definitions (with player level, etc... )
	Float totalChance = 0.0f;
	for ( auto it = definitions.Begin(); it != definitions.End(); ++it )
	{
		if ( WrapValidate< T, V >::Validate( *it, definitionValidator ) )
		{
			Float chance = WrapAccessors< T >::GetChance( *it );
			if ( chance < 0.0f )
			{
				requiredDefinitions.PushBack( *it );
			}
			else
			{
				additionalDefinitions.PushBack( *it );
				totalChance += chance;
			}
		}
	}

	if ( requiredDefinitions.Size() + additionalDefinitions.Size() <= quantity )
	{
		chosenDefinitions.PushBack( requiredDefinitions );
		chosenDefinitions.PushBack( additionalDefinitions );
	}
	else
	{
		// If there's more valid definitions than we need we're choosing
		// only a number of them regarding their chances.
		if ( requiredDefinitions.Size() > 0 )
		{
			ChooseRequiredDefinitions( requiredDefinitions, chosenDefinitions, quantity, randSeed );
		}
		
		if ( additionalDefinitions.Size() > 0 )
		{
			ChooseAdditionalDefinitions( additionalDefinitions, chosenDefinitions, quantity, totalChance, randSeed );
		}
	}
}

// "In-place" ChooseDefinitions (see description above)
template < typename T, typename V >
Uint32 CDefinitionsHelpers::ChooseDefinitions( TDynArray< T > & definitions, Uint32 quantity, const V& definitionValidator, Uint32 randSeed )
{
	if ( quantity == 0 )
	{
		return 0;
	}

	Uint32 requiredDefinitionsCount = 0;
	Uint32 additionalDefinitionsCount = 0;
	Uint32 invalidDefinitionsCount = 0;

	// validating definitions (with player level, etc... )
	Float totalChance = 0.0f;
	Uint32 size = definitions.Size();
	Uint32 index = 0;
	while ( index < size - invalidDefinitionsCount )
	{
		if ( WrapValidate< T, V >::Validate( definitions[ index ], definitionValidator ) )
		{
			Float chance = WrapAccessors< T >::GetChance( definitions[ index ] );
			if ( chance < 0.0f )
			{
				// if this is a "required" definition put it before any "additional" one
				if ( index != requiredDefinitionsCount )
				{
					definitions[ index ] = definitions[ requiredDefinitionsCount ];
					definitions[ requiredDefinitionsCount ] = definitions[ index ];
				}
				requiredDefinitionsCount++;
			}
			else
			{
				additionalDefinitionsCount++;
				totalChance += chance;
			}
			index++;
		}
		else
		{
			invalidDefinitionsCount++;
			definitions[ index ] = definitions[ size - invalidDefinitionsCount ];
			definitions[ size - invalidDefinitionsCount ] = definitions[ index ];
		}
	}

	ASSERT( requiredDefinitionsCount + additionalDefinitionsCount + invalidDefinitionsCount == definitions.Size() );

	if ( quantity < requiredDefinitionsCount)
	{
		ChooseRequiredDefinitions( definitions, 0, requiredDefinitionsCount, quantity, randSeed );
	}
	else if ( quantity < requiredDefinitionsCount + additionalDefinitionsCount )
	{
		// If there's more valid definitions than we need we're choosing
		// only a number of them regarding their chances.
		ChooseAdditionalDefinitions( definitions, requiredDefinitionsCount, additionalDefinitionsCount, quantity - requiredDefinitionsCount, totalChance, randSeed );
	}
	else
	{
		return ( requiredDefinitionsCount + additionalDefinitionsCount );
	}
	return quantity;
}

template < typename T >
void CDefinitionsHelpers::ChooseRequiredDefinitions( TDynArray< T > & requiredDefinitions, TDynArray< T > & chosenDefinitions, Uint32 quantity, Uint32 randSeed )
{
	// if there's less definitions than we need, take them all
	if ( requiredDefinitions.Size() <= quantity )
	{
		chosenDefinitions.PushBack( requiredDefinitions );
	}
	else
	{
		Uint32 index = 0;
		Uint32 seedOffset = 0;

		while ( chosenDefinitions.Size() < quantity )
		{
			ASSERT( requiredDefinitions.Size() > 0 );

			if ( randSeed == 0 )
			{
				index = GEngine->GetRandomNumberGenerator().Get< Uint32 >( requiredDefinitions.Size() );
			}
			else
			{
				Red::Math::Random::Generator< Red::Math::Random::Noise > noiseMaker( randSeed + seedOffset );
				index = noiseMaker.Get< Uint32 >( requiredDefinitions.Size() );
			}
			++seedOffset;

			chosenDefinitions.PushBack( requiredDefinitions[ index ] );
			requiredDefinitions.RemoveAt( index );
		}
	}
}

template < typename T >
void CDefinitionsHelpers::ChooseRequiredDefinitions( TDynArray< T > & definitions, Uint32 offset, Uint32 count, Uint32 quantity, Uint32 randSeed )
{
	// if there's less definitions than we need, take them all
	if ( count <= quantity )
	{
		return;
	}
	else
	{
		Uint32 index = offset;
		Uint32 lastChosen = index + quantity;
		Uint32 last = index + count;
		while ( index < lastChosen )
		{
			ASSERT( index < last );

			Uint32 chosenIndex;
			if( randSeed == 0 )
			{
				chosenIndex = GEngine->GetRandomNumberGenerator().Get< Uint32 >( index , last );
			}
			else
			{
				Red::Math::Random::Generator< Red::Math::Random::Noise > noiseMaker( randSeed + index );
				chosenIndex = noiseMaker.Get< Uint32 >( index, last );
			}

			if ( chosenIndex != index )
			{
				Swap( definitions[ index ], definitions[ chosenIndex ] );
			}
			index++;
		}
	}
}

template < typename T >
void CDefinitionsHelpers::ChooseAdditionalDefinitions( TDynArray< T > & additionalDefinitions, TDynArray< T > & chosenDefinitions, Uint32 quantity, Float totalChance, Uint32 randSeed )
{
	ASSERT( totalChance >= 0.0f );
	ASSERT( additionalDefinitions.Size() > 0 );

	Float chance = 0.0f;
	Float chanceSum = 0.0f;
	Uint32 index = 0;
	Uint32 seedOffset = 0;

	while ( chosenDefinitions.Size() < quantity )
	{
		if ( randSeed == 0 )
		{
			chance = GEngine->GetRandomNumberGenerator().Get< Float >( totalChance );
		}
		else
		{
			Red::Math::Random::Generator< Red::Math::Random::Noise > noiseMaker( randSeed + seedOffset );
			chance = noiseMaker.Get< Float >( totalChance );
		}
		++seedOffset;

		index = 0;
		chanceSum = WrapAccessors< T >::GetChance( additionalDefinitions[ index ] );

		while ( index < additionalDefinitions.Size() - 1 && chanceSum < chance )
		{
			index++;
			chanceSum += WrapAccessors< T >::GetChance( additionalDefinitions[ index ] );
		}

		chosenDefinitions.PushBack( additionalDefinitions[ index ] );
		totalChance -= WrapAccessors< T >::GetChance( additionalDefinitions[ index ] );
		additionalDefinitions.RemoveAt( index );
	}
}

template < typename T >
void CDefinitionsHelpers::ChooseAdditionalDefinitions( TDynArray< T > & definitions, Uint32 offset, Uint32 count, Uint32 quantity, Float totalChance, Uint32 randSeed )
{
	// if there's less definitions than we need, take them all
	if ( count <= quantity )
	{
		return;
	}
	Uint32 index = offset;
	Uint32 lastChosen = index + quantity;
	Uint32 last = index + count;
	while ( index < lastChosen )
	{
		ASSERT( totalChance >= 0.0f );
		ASSERT( index < last );

		Float chance = 0;
		if( randSeed == 0 )
		{
			chance = GEngine->GetRandomNumberGenerator().Get< Float >( totalChance );
		}
		else
		{
			Red::Math::Random::Generator< Red::Math::Random::Noise > noiseMaker( randSeed + index );
			chance = noiseMaker.Get< Float >( totalChance );
		}

		Uint32 chosenIndex = index;
		Float chanceSum = WrapAccessors< T >::GetChance( definitions[ chosenIndex ] );
		while ( chosenIndex < last - 1 && chanceSum < chance )
		{
			chosenIndex++;
			chanceSum += WrapAccessors< T >::GetChance( definitions[ chosenIndex ] );
		}
		if ( index != chosenIndex )
		{
			Swap( definitions[ index ], definitions[ chosenIndex ] );
		}
		totalChance -= WrapAccessors< T >::GetChance( definitions[ index ] );
		index++;
	}
}

template < typename T >
void CDefinitionsHelpers::CopyNames( const TDynArray< T > & definitions, TDynArray< CName > & names, Int32 count /* = -1 */ )
{
	Uint32 size = ( count == -1 ) ? definitions.Size() : Min( static_cast< Uint32 >( count ), definitions.Size() );
	for ( Uint32 i = 0; i < size; i++ )
	{
		names.PushBack( WrapAccessors< T >::GetName( definitions[i] ) );
	}
}

// W3 EP2 MOD - MSTEINKE - Removes the recipes and schematics the player has already learned
template < typename T >
void CDefinitionsHelpers::ValidateDefinitions( const TDynArray< T > & defItems, TDynArray< T > & validItems )
{
	CName itemName;
	Bool itemAcquired = false;
	TDynArray< CName > alchemyRecipes;
	TDynArray< CName > craftingSchematics;

	if ( GCommonGame && GCommonGame->GetPlayer() )
	{
		CallFunctionRet( GCommonGame->GetPlayer(), CNAME( GetAlchemyRecipes ), alchemyRecipes );
		CallFunctionRet( GCommonGame->GetPlayer(), CNAME( GetCraftingSchematicsNames ), craftingSchematics );
	}
		
	for ( Uint32 index = 0; index < defItems.Size(); ++index )
	{
		itemName = WrapAccessors< T >::GetName( defItems[ index ] );

		for ( Uint32 recipeIndex = 0; recipeIndex < alchemyRecipes.Size(); ++recipeIndex )
		{
			if ( itemName == alchemyRecipes[ recipeIndex ] )
			{
				itemAcquired = true;
				break;
			}
		}

		if ( itemAcquired )
		{
			itemAcquired = false;
			continue;
		}

		for ( Uint32 schematicIndex = 0; schematicIndex < craftingSchematics.Size(); ++schematicIndex )
		{
			if ( itemName == craftingSchematics[ schematicIndex ] )
			{
				itemAcquired = true;
				break;
			}
		}

		if ( itemAcquired )
		{
			itemAcquired = false;
			continue;
		}

		validItems.PushBack( defItems[ index ] );
	}
}
// END W3 EP2 MOD - MSTEINKE