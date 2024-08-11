#pragma once
#include "multiCurve.h"
#include "entity.h"
#include "../core/object.h"

class IGameSaver;
class IGameLoader;

enum EPropertyCurveMode
{
	PCM_Forward,
	PCM_Backward
};

BEGIN_ENUM_RTTI( EPropertyCurveMode );
	ENUM_OPTION( PCM_Forward );
	ENUM_OPTION( PCM_Backward );
END_ENUM_RTTI();

RED_DECLARE_NAME( PropAnims );
RED_DECLARE_NAME( NumPropAnims );
RED_DECLARE_NAME( PropAnim );
RED_DECLARE_NAME( PropAnim_PropertyName );
RED_DECLARE_NAME( PropAnim_AnimationName );
RED_DECLARE_NAME( PropAnim_LengthScale );
RED_DECLARE_NAME( PropAnim_Timer );
RED_DECLARE_NAME( PropAnim_Count );
RED_DECLARE_NAME( PropAnim_Counter );
RED_DECLARE_NAME( PropAnim_Mode );
RED_DECLARE_NAME( propertyAnimationSet );
RED_DECLARE_NAME( animations );

//! Animation of CGameplayEntity (or any of its components) property
struct SPropertyAnimation
{
	DECLARE_RTTI_STRUCT( SPropertyAnimation );

	SPropertyAnimation();
	RED_INLINE Bool HasAssignedProperty() const { return m_property != NULL; }

	CName						m_propertyName;		// Linked CGameplayEntity's property name; if this is property of its component, then this is prefixed with component name
	CName						m_animationName;	// Animation name; by default 'None'; this allows to have multiple animations per property
	CObject*					m_propertyParent;	// Property parent
	CProperty*					m_property;			// Linked property; possibly NULL (if link got broken due to property being changed / removed)
	SMultiCurve					m_curve;			// Edited curve
	Bool						m_playOnStartup;	// Indicates to play this animation on game startup
	CName						m_effectToPlay;		// Optional effect to play when animation is played
};

BEGIN_CLASS_RTTI( SPropertyAnimation );
	PROPERTY_CUSTOM_EDIT( m_propertyName, TXT("Full property name (including optional component name)"), TXT("AnimatedPropertyName") );
	PROPERTY_EDIT( m_animationName, TXT("Optional animation name") );
	PROPERTY_CUSTOM_EDIT( m_curve, TXT("Animation curve"), TXT("MultiCurveEditor") );
	PROPERTY_EDIT( m_playOnStartup, TXT("Play this animation on game startup? (Infinite loop mode only)") );
	PROPERTY_EDIT( m_effectToPlay, TXT("Optional effect to play when animation is played") );
END_CLASS_RTTI();

//! Listener for quest block condition
class IAnimationSetListener
{
public:
	virtual void OnStateChanged( CPropertyAnimationSet* animationSet ) { }
};

//! Instance of the property animation
struct SPropertyAnimationInstance
{
	SPropertyAnimationInstance();

	SPropertyAnimation*			m_animation;	// Animation
	Float						m_timer;		// Current animation time
	Float						m_lengthScale;	// Animation time length scale (2.0f means animation will be played 2 times slower)
	Uint32						m_counter;		// Current loop index
	Uint32						m_count;		// Number of times the animation shall be played
	EPropertyCurveMode			m_mode;			// Animation instance mode
	Bool						m_paused;		// Paused flag
};

//! Set of CGameplayEntity's (or its components') property animations
class CPropertyAnimationSet : public CObject, public IEntityListener
{
	friend class CGameplayEntity;
	friend class CPropertyAnimationSetManager;
	friend class CAnimatedPropertyCapture;

protected:
	TDynArray< SPropertyAnimation >				m_animations;
	TDynArray< SPropertyAnimationInstance >		m_animationInstances;
	TDynArray< IAnimationSetListener* >			m_listeners;
	TDynArray< CName >							m_dirtyPropertiesForSaving;	
	Bool										m_isRegisteredToWorld;

public:
	DECLARE_ENGINE_CLASS( CPropertyAnimationSet, CObject, 0 );

	CPropertyAnimationSet();

	//! Updates all animation instances
	void Update( Float dt );

	RED_FORCE_INLINE Uint32 GetPropertyAnimationsCount() const { return m_animations.Size(); }
	RED_FORCE_INLINE SPropertyAnimation* GetPropertyAnimationByIndex( Uint32 index ) { return &m_animations[ index ]; }
	SPropertyAnimation* GetPropertyAnimation( CName propertyName, CName animationName = CName::NONE );
	//! Gets animation's transform at given time; returns true on success
	Bool GetAnimationLength( CName propertyName, CName animationName, Float& length );
	//! Gets animation's transform at given time; returns true on success
	Bool GetAnimationTransformAt( CName propertyName, CName animationName, Float time, EngineTransform& outTransform );

	SPropertyAnimationInstance* FindAnyAnimationInstance( CName animationName );
	SPropertyAnimationInstance* FindAnimationInstance( CName propertyName, CName animationName );
	RED_FORCE_INLINE Uint32 GetAnimationInstanceCount() const { return m_animationInstances.Size(); }
	RED_FORCE_INLINE SPropertyAnimationInstance* GetAnimationInstanceByIndex( Uint32 index ) { return &m_animationInstances[ index ]; }
	//! Gets currently running animation's time; returns true on success
	Bool GetAnimationInstanceTime( CName propertyName, CName animationName, Float& outTime );

	//! Starts animation on all properties with matching animation name
	Bool Play( CName animationName, Uint32 count = 0, Float lengthScale = 1.0f, EPropertyCurveMode mode = PCM_Forward );
	//! Stops animation on all properties with matching animation name and restores property values to their initial values
	void Stop( CName animationName, Bool restoreInitialValues = true );
	//! Stops all animations and restores property values to their initial values
	void StopAll( Bool restoreInitialValues = true );
	//! Rewinds animation on all properties with matching animation name to given time; changes property values immediately
	void Rewind( CName animationName, Float time );
	//! Pauses animation on all properties with matching animation name
	void Pause( CName animationName );
	//! Unpauses animation on all properties with matching animation name
	void Unpause( CName animationName );

	//! Checks if a certain animation is currently playing or not
	Bool IsPlaying( CName animationName ) const;
	Bool IsPlaying( CName propertyName, CName animationName ) const;

	//! Updates initial transform from the following node
	void UpdateInitialTransformFrom( CNode* node );

	// Redirected from CGameplayEntity

	void OnSaveGameplayState( IGameSaver* saver );
	void OnLoadGameplayState( IGameLoader* loader );
	Bool CheckShouldSave() const;
	void OnAttached( CWorld* world );
	void OnAttachFinished( CWorld* world );
	void OnDetached( CWorld* world );

	// Overridden from CObject

	virtual void OnPropertyPostChange( IProperty* property ) override;

	// Overridden from IEntityListener

	virtual void OnNotifyEntityComponentAdded( CEntity* entity, CComponent* component ) override;
	virtual void OnNotifyEntityComponentRemoved( CEntity* entity, CComponent* component ) override;
	virtual void OnNotifyEntityRenderProxyAdded( CEntity* entity, CComponent* component, IRenderProxy* proxy ) {}
	virtual void OnNotifyEntityRenderProxyRemoved( CEntity* entity, CComponent* component ) {}

	Bool AddListener( IAnimationSetListener* listener );
	Bool RemoveListener( IAnimationSetListener* listener );

protected:
	void OnAllComponentsAttached();
	void BindAllProperties();
	void UnbindAllProperties();
	void BindObjectProperties( CNode* propertyParent );
	void UnbindObjectProperties( CNode* propertyParent );
	void RegisterToWorld( CWorld* world );
	void UnregisterFromWorld( CWorld* world );
	CWorld* GetWorld();
	void UpdatePropertyValue( SPropertyAnimationInstance* animationInstance );
	void NotifyListeners();
	Bool ResolveProperty( CName propName, CNode*& objPtr, CProperty*& propPtr );

	void PlayEffect( SPropertyAnimationInstance* instance );
	void StopEffect( SPropertyAnimationInstance* instance );
};

BEGIN_CLASS_RTTI( CPropertyAnimationSet );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_animations, TXT("Property animations") );
END_CLASS_RTTI();

// Helper class used to capture/restore state of animated properties being part of specific animation
class CAnimatedPropertyCapture
{
private:
	struct PropertyOriginalValue
	{
		SPropertyAnimation* m_animation;
		CVariant m_value;
	};
	TDynArray< PropertyOriginalValue > m_values;
	CPropertyAnimationSet* m_animationSet;

public:
	CAnimatedPropertyCapture();
	void Init( CPropertyAnimationSet* animationSet );
	void CaptureAnimatedProperties( const CName animationName );
	void RestoreAnimatedProperties();
};