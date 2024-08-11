/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Forward declarations
class CFXTrackGroup;
class CEntityTemplate;
class CFXTrackItem;
class CEntityTemple;
class CEntity;
class CCutsceneTemplate;

/// Parameter controlled by FX
struct SFXParameter
{
public:
	CName			m_parameterName;		//!< Name of the parameter
	IRTTIType*		m_parameterType;		//!< Type of the parameter

public:
	SFXParameter( const CName &paramName, IRTTIType *paramType )
		: m_parameterName( paramName )
		, m_parameterType( paramType )
	{ }

	SFXParameter( const SFXParameter &other )
		: m_parameterName( other.m_parameterName )
		, m_parameterType( other.m_parameterType ) 
	{}

	//!
	const SFXParameter& operator=( const SFXParameter &other )
	{
		m_parameterName = other.m_parameterName;
		m_parameterType = other.m_parameterType;
		return *this;
	}

	//! Compare
	Bool operator==( const SFXParameter &other ) const
	{
		return (m_parameterName == other.m_parameterName) && ( m_parameterType == other.m_parameterType );
	}
};

/// Group of parameters for FX
class CFXParameters
{
private:
	TDynArray< SFXParameter >		m_effectParameters;

public:
	//! Add parameter definition
	template< typename T >
	RED_INLINE void AddParameter( const CName &parameterName )
	{
		IRTTIType* parameterType = SRTTI::GetInstance().FindType( GetTypeName<T>() );
		new ( m_effectParameters ) SFXParameter( parameterName, parameterType );
	}

	//! Is the list of parameters empty ?
	RED_INLINE Bool Empty() const { return m_effectParameters.Empty(); }

	//! Get size of the parameter list
	RED_INLINE Uint32 Size() const { return m_effectParameters.Size(); }

	//! Get n-th parameter name
	RED_INLINE CName GetParameterName( Uint32 i ) const { return m_effectParameters[i].m_parameterName; }

	//! Get n-th parameter type
	RED_INLINE IRTTIType* GetParameterType( Uint32 i ) const { return m_effectParameters[i].m_parameterType; }
};

/// Entity effect data
class CFXDefinition : public CObject
{
	DECLARE_ENGINE_CLASS( CFXDefinition, CObject, 0 );

protected:
	CName								m_name;				//!< Name of the effect
	CName								m_animationName;	//!< Name of the related animation ( for animation driven effects )
	TDynArray< CFXTrackGroup* >			m_trackGroups;		//!< Track groups in the effect
	Float								m_length;			//!< Length of the effect
	Float								m_loopStart;		//!< Effect loop start
	Float								m_loopEnd;			//!< Effect loop end
	Bool								m_isLooped;			//!< Effect is looping until stopped
	Bool								m_randomStart;		//!< Start time is drawed
	Bool								m_stayInMemory;		//!< Want key effect data to stay in memory as long as whole effect stays (eg. particle systems)
	Float								m_showDistance;		//!< Distance at which the effect will fail to play

public:
	//! Get name of the effect
	RED_INLINE const CName& GetName() const { return m_name; }

	//! Get related animation
	RED_INLINE const CName& GetAnimationName() const { return m_animationName; }

	//! Get the list of effect track groups
	RED_INLINE TDynArray< CFXTrackGroup* > &GetEffectTrackGroup() { return m_trackGroups; }

	//! Is this effect bound to an animation ?
	RED_INLINE Bool IsBoundToAnimation() const { return m_animationName; }

	//! Get effect length
	RED_INLINE Float GetLength() const { return m_length; }

	//! Get start time
	RED_INLINE Float GetStartTime() const { return 0.0f; }

	//! Get end time
	RED_INLINE Float GetEndTime() const { return m_length; }

	//! Get loop start time
	RED_INLINE Float GetLoopStart() const { return m_loopStart; }

	//! Get loop end time
	RED_INLINE Float GetLoopEnd() const { return m_loopEnd; }

	//! Is this effect looped
	RED_INLINE Bool IsLooped() const { return m_isLooped; }

	//! Is start time is drawed
	RED_INLINE Bool IsRandomStart() const { return m_randomStart; }

	//! Should related data stay in memory?
	RED_INLINE Bool IsStayInMemory() const { return m_stayInMemory; }

	RED_INLINE Float GetShowDistance() const { return m_showDistance; }

	RED_INLINE Float GetShowDistanceSqr() const { return m_showDistance * m_showDistance; }

protected:
	CFXDefinition() 
		: m_length( 1.0f )
		, m_loopStart( 0.0f )
		, m_loopEnd( 1.0f )
		, m_randomStart( false )
		, m_stayInMemory( false )
		, m_showDistance( 12.0f )
	{
	}

public:
	CFXDefinition( CEntityTemplate* entityTemplate, CName effectName );
	
	CFXDefinition( CCutsceneTemplate* cutsceneTemplate, CName effectName );

	//! Object was loaded
	virtual void OnPostLoad();

	//! Set effect length
	void SetLength( Float length );

	//! Set effect loop start
	void SetLoopStart( Float time );

	//! Set effect loop end
	void SetLoopEnd( Float time );

	//! Set name of the effect
	void SetName( const CName& name );

public:
	//! Append track group to the effect
	CFXTrackGroup* AddTrackGroup( const String &trackGroupName );

	//! Remove track group from the effect
	Bool RemoveTrackGroup( CFXTrackGroup* effectTrackGroup );

public:
	//! Bind effect to entity animation
	Bool BindToAnimation( const CName& animationName );

	//! Unbind effect from entity animation
	void UnbindFromAnimation();

public:
	//! Prefetch track item resources
	void PrefetchResources(TDynArray< TSoftHandle< CResource > >& requiredResources) const;

	//! Collect track items to start 
	void CollectTracksToStart( Float prevTime, Float curTime, TDynArray< const CFXTrackItem* >& trackItems ) const;
};

BEGIN_CLASS_RTTI( CFXDefinition );
	PARENT_CLASS( CObject );
	PROPERTY( m_trackGroups );
	PROPERTY_EDIT( m_length, TXT("Length") );
	PROPERTY_EDIT( m_loopStart, TXT("Loop start") );
	PROPERTY_EDIT( m_loopEnd, TXT("Loop end") );
	PROPERTY( m_name );
	PROPERTY( m_animationName );
	PROPERTY_EDIT( m_showDistance, TXT("Distance at which effect will be disabled") );
	PROPERTY_EDIT( m_stayInMemory, TXT("Once effect is loaded, keep related data in memory") );
	PROPERTY_EDIT( m_isLooped, TXT("Effect is looping until stopped") );
	PROPERTY_EDIT( m_randomStart, TXT( "Random offset is added to effect start time" ) );
END_CLASS_RTTI();
