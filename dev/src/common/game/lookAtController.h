/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CActorLookAtDesc;

//////////////////////////////////////////////////////////////////////////

class CLookAtDynamicParam
{
public:
	virtual Vector			GetTarget() const = 0;
	virtual ELookAtLevel	GetLevel() const = 0;
	virtual Float			GetSpeed() const = 0;
	virtual Float			GetSpeedOverride() const = 0;
	virtual Bool			IsAutoLimitDeact() const = 0;
	virtual Float			GetRange() const = 0;
	virtual Bool			IsInstant() const = 0;
	virtual Int32			GetID() const = 0;
	virtual Float			GetHeadRotationRatio() const = 0;
	virtual Float			GetEyesLookAtConvergenceWeight() const = 0;
	virtual Bool			IsEyesLookAtAdditive() const = 0;
	virtual Float			GetEyesLookAtDampScale() const = 0;
	virtual Float			GetTimeFromStart() const = 0;

protected:

	~CLookAtDynamicParam(){}
};

//////////////////////////////////////////////////////////////////////////

class CLookAtContextParam
{
public:
	CLookAtContextParam() : m_actorLevel( LL_Body ), m_speed( 0.f ), m_reset( false ), m_headRotationRatio(1.f) {}
	
	Float			m_headRotationRatio;
	ELookAtLevel	m_actorLevel;
	Float			m_speed;
	Bool			m_reset;
};

//////////////////////////////////////////////////////////////////////////

enum ELookAtSupplierPriority
{
	LPP_Debug,
	LPP_Dialog,
	LPP_Script,
	LPP_Reaction,
};

//////////////////////////////////////////////////////////////////////////
// Targets
//////////////////////////////////////////////////////////////////////////

enum ELookAtTargetType
{
	LTT_Static,
	LTT_Dynamic,
	LTT_Bone,
};

class ILookAtTarget
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );

public:
	virtual Vector	GetTarget() const = 0;
	virtual Bool	IsValid() const = 0;
	virtual void	GetTargetName( String& name ) const = 0;
	virtual ELookAtTargetType GetType() const = 0;

public:
	virtual ~ILookAtTarget() {};
};


class CLookAtTargetDynamic : public ILookAtTarget
{
private:
	THandle< CNode > m_target;

public:
	CLookAtTargetDynamic( const CNode* target );

	virtual Vector	GetTarget() const;
	virtual Bool	IsValid() const;
	virtual void	GetTargetName( String& name ) const;
	virtual ELookAtTargetType GetType() const;
};

class CLookAtTargetStatic : public ILookAtTarget
{
private:
	Vector m_target;

public:
	CLookAtTargetStatic( const Vector& target );

	virtual Vector	GetTarget() const;
	virtual Bool	IsValid() const;
	virtual void	GetTargetName( String& name ) const;
	virtual ELookAtTargetType GetType() const;
};

class CLookAtTargetBone : public ILookAtTarget
{
private:
	THandle< CAnimatedComponent >	m_targetOwner;
	Int32							m_boneIndex;
	
public:
	CLookAtTargetBone( const CAnimatedComponent* boneOwner, Int32 boneIndex );

	virtual Vector	GetTarget() const;
	virtual Bool	IsValid() const;
	virtual void	GetTargetName( String& name ) const;
	virtual ELookAtTargetType GetType() const;
};


//////////////////////////////////////////////////////////////////////////
// Look at creation info
//////////////////////////////////////////////////////////////////////////

struct SLookAtInfo
{
public:
	ELookAtSupplierPriority		GetPriority() const		{ return m_priority; }
	ELookAtTargetType			GetTargetType() const	{ return m_targetType; }

protected:
	SLookAtInfo( ELookAtSupplierPriority prio, ELookAtTargetType targetType ) 
		: m_priority ( prio )
		, m_targetType( targetType ) 
		, m_headRotationRatio(1.f)
	{
		m_delay = GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDelay > 0.f ?
			GEngine->GetRandomNumberGenerator().Get< Float >( GGame->GetGameplayConfig().m_lookAtConfig.m_lookAtDelay ) : 0.f;
	}

private:
	ELookAtSupplierPriority		m_priority;
	ELookAtTargetType			m_targetType;

protected:
	Float						m_delay;
public:
	Float						m_headRotationRatio;
	String						m_desc;

public:
	virtual Bool				IsValid() const = 0;
public:
	virtual ELookAtLevel		GetLevel() const { return LL_Body; }
	virtual Float				GetSpeed() const { return 0.f; }
	virtual Float				GetSpeedOverride() const { return 0.f; }
	virtual Bool				IsInstant() const { return false; }
	virtual Float				GetDuration() const { return 0.f; }
	virtual Bool				IsAutoLimitDeact() const { return true; }
	virtual Float				GetRange() const { return 120.f; }
	virtual Float				GetEyesLookAtConvergenceWeight() const { return 0.f; }
	virtual Bool				IsEyesLookAtAdditive() const { return true; }
	virtual Float				GetEyesLookAtDampScale() const { return 1.f; }
	virtual Float				GetTimeFromStart() const { return 0.f; }

public:
	const String&				GetDesc() const { return m_desc; }
	Float						GetHeadRotationRatio() const{ return m_headRotationRatio; }
	Float						GetDelay() const { return m_delay; }
	void						SetDelay( Float d ) { m_delay = d; }
};

struct SLookAtStaticInfo : public SLookAtInfo
{
public:
	virtual Bool IsValid() const;

	Vector m_target;

protected:
	SLookAtStaticInfo( ELookAtSupplierPriority prio );
};

struct SLookAtDynamicInfo : public SLookAtInfo
{
public:
	virtual Bool IsValid() const;

	const CNode* m_target;

protected:
	SLookAtDynamicInfo( ELookAtSupplierPriority prio );
};

struct SLookAtBoneInfo : public SLookAtInfo
{
public:
	virtual Bool IsValid() const;

	const CAnimatedComponent*	m_targetOwner;
	Int32						m_boneIndex;

protected:
	SLookAtBoneInfo( ELookAtSupplierPriority prio );
};


//////////////////////////////////////////////////////////////////////////
// Supplier
//////////////////////////////////////////////////////////////////////////

class CLookAtSupplier : public CLookAtDynamicParam
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Gameplay );

public:
	CLookAtSupplier( const SLookAtInfo& lookAtInfo, Int32 id );
	virtual ~CLookAtSupplier();

	ELookAtSupplierPriority GetPriority() const;

	virtual Vector			GetTarget() const;
	virtual ELookAtLevel	GetLevel() const;
	virtual Float			GetSpeed() const;
	virtual Float			GetSpeedOverride() const;
	virtual Bool			IsAutoLimitDeact() const;
	virtual Float			GetRange() const;
	virtual Bool			IsInstant() const;
	virtual Int32			GetID() const;
	virtual Float			GetHeadRotationRatio() const;
	virtual Float			GetEyesLookAtConvergenceWeight() const;
	virtual Bool			IsEyesLookAtAdditive() const;
	virtual Float			GetEyesLookAtDampScale() const;
	virtual Float			GetTimeFromStart() const;

	void					GetFriendlyName( String& name ) const;
	const String&			GetDesc() const;
	Float					GetDelay() const;

	Bool					IsEqual( const SLookAtInfo& lookAtInfo  ) const;

	Bool Wait( Float dt );
	Bool Update( Float dt );

private:
	void CreateTarget( const SLookAtInfo& lookAtInfo );
	
protected:
	ILookAtTarget*			m_target;
	ELookAtSupplierPriority	m_priority;
	Float					m_timer;
	Float					m_duration;
	ELookAtLevel			m_level;
	Float					m_speed;
	Float					m_speedOverride;
	Bool					m_instantStart;
	Bool					m_autoLimitDeact;
	Float					m_range;
	String					m_desc;
	Float					m_delay;
	Int32					m_id;
	Float					m_headRotationRatio;
	Float					m_eyesLookAtConvergenceWeight;
	Bool					m_eyesLookAtIsAdditive;
	Float					m_eyesLookAtDampScale;
	Float					m_timeFromStart;
};

//////////////////////////////////////////////////////////////////////////
// Controller
//////////////////////////////////////////////////////////////////////////

enum ELookAtMode
{
	LM_Dialog			= FLAG( 0 ),
	LM_Cutscene			= FLAG( 1 ),
	LM_MiniGame			= FLAG( 2 ),
	LM_GameplayLock		= FLAG( 3 ),
};

BEGIN_ENUM_RTTI( ELookAtMode );
	ENUM_OPTION( LM_Dialog );
	ENUM_OPTION( LM_Cutscene );
	ENUM_OPTION( LM_MiniGame );
	ENUM_OPTION( LM_GameplayLock );
END_ENUM_RTTI();

class CLookAtStaticParam;



class CLookAtController
{
private:
	struct SDialogLookatFilterData
	{
		SDialogLookatFilterData( CName	_key, ELookAtLevel	_level ) : key( _key ), level( _level ), isActive( false )
		{}
		CName			key;
		ELookAtLevel	level;
		Bool			isActive;
	};

	Int32							m_bestSupplier;
	TDynArray< CLookAtSupplier*	>	m_waitingSuppliers;
	TDynArray< CLookAtSupplier*	>	m_suppliers;
	ELookAtLevel					m_level;
	Int32							m_nextId;
	const CLookAtStaticParam*		m_staticParam;
	Float							m_deactSpeed;
	Uint8							m_modeFlags;

	TDynArray<SDialogLookatFilterData>	m_filterData;

	Float							m_headRotationRatio_current;
	Float							m_headRotationRatio_velo;
	const static Float				m_headRotationRatio_damp;

public:
	CLookAtController();
	~CLookAtController();

	void SetMode( ELookAtMode mode );
	void ResetMode( ELookAtMode mode );
	Bool HasFlag( ELookAtMode mode ) const;

	void FindAndCacheStaticLookAtParam( const CEntityTemplate* templ );
	
	Bool Update( Float dt );

	Bool AddLookAt( const SLookAtInfo& lookAtInfo );
	void SetNoLookAts();
	void SetNoDialogsLookAts( Float speed );
	void SetNoScriptLookAts();
	void RemoveAllNonDialogsLookAts();

	void GetLookAtParams( const CLookAtDynamicParam*& dynamicParam, const CLookAtStaticParam*& staticParam, CLookAtContextParam& contextParam ) const;

	void SetLevel( ELookAtLevel level );
	ELookAtLevel GetLevel() const;

	void SetLookatFilterData( ELookAtLevel level, CName key );
	void RemoveLookatFilterData( CName key );
	void ActivateLookatFilter( CName key, Bool value );

	Bool	HasLookAt() const;
	Vector	GetTarget() const;
	Vector	GetBodyPartWeights() const;
	Vector	GetCompressedData() const;
	Vector	GetEyesCompressedData() const;

	void GenerateDebugFragments( CRenderFrame* frame, const Matrix& localToWorld ) const;
	
	void GetDesc( CActorLookAtDesc& desc ) const;

private:
	CLookAtSupplier* GetBestSupplier() const;
	void FindBestSupplier();

	Bool CanAddSupplier( const SLookAtInfo& lookAtInfo ) const;
	void AddSupplier( CLookAtSupplier* provider );

	CLookAtSupplier* CreateSupplier( const SLookAtInfo& lookAtInfo );

	void DestroySupplier( CLookAtSupplier* s );
	void DestroySuppliers();
	void DestroySuppliersWithPrio( ELookAtSupplierPriority prio );
	void DestroySuppliersWithoutPrio( ELookAtSupplierPriority prio );
	void DestroyEqualsSuppliers( CLookAtSupplier* s );

	Bool IsMultiSupplierAllowed( ELookAtSupplierPriority prio ) const;
	Bool HasAnySupplierWithPrio( ELookAtSupplierPriority prio ) const;
	Uint32 GetNumberOfSuppliersWithPrio( ELookAtSupplierPriority prio ) const;

	void CheckLookAtsAvailability();
	Bool IsLookAtTypeAvailable( ELookAtSupplierPriority prio ) const;

	String GetSupplierName() const;
	Float GetDampSpeed() const;
	Float GetFollowSpeed() const;
	Bool IsAutoLimitDeact() const;
	Float GetRange() const;
};

//////////////////////////////////////////////////////////////////////////

class CActorLookAtDesc
{
private:
	Int32							m_index;
	Int32							m_active;
	TDynArray< CLookAtSupplier* >	m_suppliers;
	Uint8							m_flags;
	
public:
	CActorLookAtDesc()
		: m_index( -1 )
		, m_active( -1 )
		, m_flags( 0 )
	{}

	void Setup( const TDynArray< CLookAtSupplier* >& suppliers, Uint32 active, Uint8 flags )
	{
		m_suppliers = suppliers;
		m_active = active;
		m_flags = flags;
	}

	Int32 GetActiveLookAtIndex() const
	{
		return m_active;
	}

	Bool IsActiveLookAt() const
	{
		return m_active == m_index;
	}

	Bool SetIndex( Uint32 i ) 
	{ 
		if ( i < (Int32)m_suppliers.Size() )
		{
			m_index = i;
			return true;
		}

		return false;
	}

	Bool NextIndex()
	{
		if ( m_index + 1 < (Int32)m_suppliers.Size() )
		{
			m_index++;
			return true;
		}

		return false;
	}

	Bool IsValidIndex() const
	{
		return m_index != -1 && m_index < (Int32)m_suppliers.Size();
	}

	String GetName() const
	{
		if ( IsValidIndex() )
		{
			String str;
			GetSupplier()->GetFriendlyName( str );
			return str;
		}
		return String::EMPTY;
	}

	String GetLevel() const
	{
		if ( IsValidIndex() )
		{
			ELookAtLevel level = GetSupplier()->GetLevel();
			switch ( level )
			{
			case LL_Body:
				return TXT("Body");
			case LL_Head:
				return TXT("Head");
			case LL_Eyes:
				return TXT("Eyes");
			case LL_Null:
				return TXT("Null");
			default:
				ASSERT( 0 );
			}
		}

		return TXT("Invalid");
	}

	String GetMode() const
	{
		String mode;

		if ( m_flags & LM_Cutscene )
		{
			mode += TXT(" Cutscene;");
		}
		if ( m_flags & LM_Dialog )
		{
			mode += TXT(" Dialog;");
		}
		if ( m_flags & LM_GameplayLock )
		{
			mode += TXT(" GameplayLock;");
		}
		if ( m_flags & LM_MiniGame )
		{
			mode += TXT(" MimiGame;");
		}

		if ( mode.Empty() )
		{
			mode = TXT("<default>");
		}
		
		return mode;
	}

	String GetSpeed() const
	{
		return IsValidIndex() ? ToString( GetSupplier()->GetSpeed() ) : TXT("Invalid");
	}

	String GetRange() const
	{
		return IsValidIndex() ? ToString( GetSupplier()->GetRange() ) : TXT("Invalid");
	}

	String GetExtraDesc() const
	{
		return IsValidIndex() ? GetSupplier()->GetDesc() : TXT("Invalid");
	}

	String IsAutoLimitDeact() const
	{
		return IsValidIndex() ? ToString( GetSupplier()->IsAutoLimitDeact() ) : TXT("Invalid");
	}

	String IsInstant() const
	{
		return IsValidIndex() ? ToString( GetSupplier()->IsInstant() ) : TXT("Invalid");
	}

private:
	const CLookAtSupplier* GetSupplier() const
	{
		ASSERT( IsValidIndex() );
		return m_suppliers[ m_index ];
	}
};
