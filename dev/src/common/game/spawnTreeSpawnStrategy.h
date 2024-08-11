#pragma once

#include "encounterTypes.h"
#include "encounterSpawnPoints.h"
#include "spawnTree.h"

struct SCompiledSpawnStrategyInitializer
{
	DECLARE_RTTI_STRUCT( SCompiledSpawnStrategyInitializer );

	ISpawnTreeSpawnStrategy*	m_initializer;
	CSpawnTreeInstance*			m_instance;

	SCompiledSpawnStrategyInitializer( ISpawnTreeSpawnStrategy* initializer, CSpawnTreeInstance* instance )
		: m_initializer( initializer )
		, m_instance( instance ) {}

	SCompiledSpawnStrategyInitializer()
		: m_initializer( nullptr )
		, m_instance( nullptr ) {}
};

BEGIN_CLASS_RTTI( SCompiledSpawnStrategyInitializer );
END_CLASS_RTTI();

class ISpawnTreeSpawnStrategy : public ISpawnTreeInitializer
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISpawnTreeSpawnStrategy, ISpawnTreeInitializer );

protected:
	Bool			m_enablePooling;
	Float			m_overflowPoolRange;

public:
	enum ESpawnPointTestResult
	{
		VR_Reject,
		VR_Accept_NoVisibilityTest,
		VR_Accept_VisibilityTest,
	};

					ISpawnTreeSpawnStrategy();

	Bool			IsTickable() const override { return m_enablePooling; }
	Bool			IsConflicting( const ISpawnTreeInitializer* initializer ) const override;

	void			Tick( CEncounterCreaturePool::SCreatureList& creatures, CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context ) const override;

	virtual ESpawnPointTestResult	CheckSpawnPoint( SSpawnTreeUpdateSpawnContext& context, const Vector3& point ) const = 0;

protected:
	virtual Bool	CheckPool( CActor* actor, const Vector3& referencePos, const Matrix& referenceLtW, Bool isOverflow ) const;

	friend class CEncounterGlobalSettings;
	virtual void	LoadFromConfig();
	virtual void	SaveToConfig();
};

BEGIN_ABSTRACT_CLASS_RTTI( ISpawnTreeSpawnStrategy )
	PARENT_CLASS( ISpawnTreeInitializer )
	PROPERTY_EDIT( m_enablePooling, TXT("Is pooling enabled") )
	PROPERTY_EDIT( m_overflowPoolRange, TXT("Range at which entities will be despawned once global spawn limit has been exceeded") )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CSimpleSpawnStrategy : public ISpawnTreeSpawnStrategy
{
	DECLARE_ENGINE_CLASS( CSimpleSpawnStrategy, ISpawnTreeSpawnStrategy, 0 );

private:
	Float	m_minSpawnRange;
	Float	m_visibilityTestRange;
	Float	m_maxSpawnRange;
	Float	m_maxSqrSpawnRange;
	Float	m_minSqrSpawnRange;
	Float	m_sqrVisibilityTestRange;

	Bool	m_canPoolOnSight;
	Float	m_minPoolRange;
	Float	m_forcePoolRange;

public:
	CSimpleSpawnStrategy();

	String	GetEditorFriendlyName() const override;

	ESpawnPointTestResult	CheckSpawnPoint( SSpawnTreeUpdateSpawnContext& context, const Vector3& point ) const override;

	void	OnPropertyPostChange( IProperty* property ) override;
	void	OnPostLoad() override;

protected:
	Bool	CheckPool( CActor* actor, const Vector3& referencePos, const Matrix& referenceLtW, Bool isOverflow ) const override;
	
	void	LoadFromConfig()	override;
	void	SaveToConfig()		override;
};

BEGIN_CLASS_RTTI( CSimpleSpawnStrategy )
	PARENT_CLASS( ISpawnTreeSpawnStrategy )
	PROPERTY_EDIT( m_minSpawnRange, TXT("Minimal spawn range") )
	PROPERTY_EDIT( m_visibilityTestRange, TXT("Range in which we do visibility tests" ) )
	PROPERTY_EDIT( m_maxSpawnRange, TXT("Maximal spawn range") )
	PROPERTY_EDIT( m_canPoolOnSight, TXT("Can creature despawn while being visible.") )
	PROPERTY_EDIT( m_minPoolRange, TXT("Minimal range from player for creature to despawn.") )
	PROPERTY_EDIT( m_forcePoolRange, TXT("Range on which visibility limitation is not important") )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SSpawnStrategyRange
{
	Float	m_spawnRange;
	Float	m_poolRange;
	Vector2	m_offset;

	DECLARE_RTTI_SIMPLE_CLASS( SSpawnStrategyRange ); 
};

BEGIN_CLASS_RTTI( SSpawnStrategyRange )
	PROPERTY_EDIT( m_spawnRange, TXT("Spawn range") )
	PROPERTY_EDIT( m_poolRange, TXT("Pool range") )
	PROPERTY_EDIT( m_offset, TXT("Offset of the range in player local space") )
END_CLASS_RTTI();

template<>
RED_INLINE String ToString( const SSpawnStrategyRange& value )
{
	return String::Printf( TXT("%g %g %g %g"), value.m_spawnRange, value.m_poolRange, value.m_offset.X, value.m_offset.Y );
}

template<>
RED_INLINE Bool FromString( const String& text, SSpawnStrategyRange& value )
{
	TDynArray< String > vals = text.Split( TXT(" ") );
	RED_ASSERT( vals.Size() == 4 );
	if ( vals.Size() != 4 )
	{
		return false;
	}
	FromString( vals[0], value.m_spawnRange );
	FromString( vals[1], value.m_poolRange );
	FromString( vals[2], value.m_offset.X );
	FromString( vals[3], value.m_offset.Y );

	return true;
};

class CMultiRangeSpawnStrategy : public ISpawnTreeSpawnStrategy
{
	DECLARE_ENGINE_CLASS( CMultiRangeSpawnStrategy, ISpawnTreeSpawnStrategy, 0 );

private:
	Float							m_primaryMinSpawnRange;
	Float							m_primaryMaxSpawnRange;
	Float							m_primaryMaxSqrSpawnRange;
	Float							m_primaryMinSqrSpawnRange;
	Float							m_visibilityTestRange;
	Float							m_sqrVisibilityTestRange;

	Float							m_primaryMinPoolRange;

	TDynArray<SSpawnStrategyRange>	m_orientedRanges;

	Bool							m_canPoolOnSight;
	Float							m_forcePoolRange;

	Float							m_poolDelay;

public:
	CMultiRangeSpawnStrategy();

	String	GetEditorFriendlyName() const override;

	ESpawnPointTestResult	CheckSpawnPoint( SSpawnTreeUpdateSpawnContext& context, const Vector3& point ) const override;

	void	OnPropertyPostChange( IProperty* property ) override;
	void	OnPostLoad() override;

protected:
	Bool	CheckPool( CActor* actor, const Vector3& referencePos, const Matrix& referenceLtW, Bool isOverflow ) const override; 

	void	LoadFromConfig()	override;
	void	SaveToConfig()		override;
};

BEGIN_CLASS_RTTI( CMultiRangeSpawnStrategy )
	PARENT_CLASS( ISpawnTreeSpawnStrategy )
	PROPERTY_EDIT( m_primaryMinSpawnRange, TXT("Minimal primary spawn range ( calculated from player position )") )
	PROPERTY_EDIT( m_primaryMaxSpawnRange, TXT("Maximal primary spawn range ( calculated from player position )") )
	PROPERTY_EDIT( m_visibilityTestRange, TXT("Range in which we do visibility tests" ) )
	PROPERTY_EDIT( m_primaryMinPoolRange, TXT("Minimal primary pool range ( calculated from player position )") )
	PROPERTY_EDIT( m_orientedRanges, TXT("Oriented spawn ranges based on direction that the player moves in") )
	PROPERTY_EDIT( m_canPoolOnSight, TXT("Can creature despawn while being visible.") )
	PROPERTY_EDIT( m_forcePoolRange, TXT("Range on which visibility limitation is not important") )
	PROPERTY_EDIT( m_poolDelay, TXT("Delay after which entity is pooled when last seen") )
END_CLASS_RTTI();