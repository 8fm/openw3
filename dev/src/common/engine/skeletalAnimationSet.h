/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "slotAnimationShiftingInterval.h"
#include "animationBufferBitwiseCompressed.h"
#include "skeletalAnimation.h"
#include "extAnimEventsFile.h"

class CSkeletalAnimationSetEntry;
class IFeedbackSystem;

struct SCompressedPoseInfo
{
	CName	m_name;
	CName	m_animation;
	Float	m_time;

	DECLARE_RTTI_STRUCT( SCompressedPoseInfo );
};

BEGIN_CLASS_RTTI( SCompressedPoseInfo );
	PROPERTY( m_name );
	PROPERTY( m_animation );
	PROPERTY( m_time );
END_CLASS_RTTI();

struct ImportAnimationOptions
{
	CSkeletalAnimationSetEntry*		m_entry;
	String							m_animationFile;
	Bool							m_overrideAnimType; // or use existing if there is one
	ESkeletalAnimationType			m_type;
	EAdditiveType					m_addType;
	CName							m_animationName;
	IFeedbackSystem*				m_feedback;
	Int32							m_maxPartFrames;
	String							m_addAnimationToExtractedFile;
	Bool							m_extractMotion;
	Bool							m_extractTrajectory;
	Bool							m_resetRoot;
	Bool							m_preferBetterQuality;

	ImportAnimationOptions();
};

// Set of skeletal animations
class CSkeletalAnimationSet : public CExtAnimEventsFile // remove after resave!			  
{
	friend class CAnimsetCooker;

	DECLARE_ENGINE_RESOURCE_CLASS( CSkeletalAnimationSet, CExtAnimEventsFile, "w2anims", "Set of animations" );	

protected:
	TDynArray< CSkeletalAnimationSetEntry* >	m_animations;			//!< Animations
	THandle< CSkeleton >						m_skeleton;				//!< All animation should based on this skeleton

	TDynArray< ICompressedPose* >				m_compressedPoses;		//!< Compressed poses for animations
#ifndef NO_EDITOR
	TDynArray< SCompressedPoseInfo >			m_compressedPoseInfos;	//!< Editor only
#endif

#ifdef USE_EXT_ANIM_EVENTS
	typedef TDynArray< THandle< CExtAnimEventsFile > > TExtFiles;
	TExtFiles									m_extAnimEvents;		//!< External animation events files
#endif

#ifndef NO_EDITOR
	Bool										m_forceUncompressedImport;
	Bool										m_overrideBitwiseCompressionSettingsOnImport;
	SAnimationBufferBitwiseCompressionPreset	m_bitwiseCompressionPreset;
	SAnimationBufferBitwiseCompressionSettings	m_bitwiseCompressionSettings;
	TDynArray< CName >							m_duplicatedAnimations;
#endif
	SAnimationBufferStreamingOption				m_streamingOption;
	Uint32										m_nonStreamableBones;

public:
	// Is this set self contained
	RED_INLINE Bool IsSelfContained() const { return true; }

	// Get number of animations in animation set
	RED_INLINE Uint32 GetNumAnimations() const { return m_animations.Size(); }

	// Get animations
	RED_INLINE const TDynArray< CSkeletalAnimationSetEntry* >& GetAnimations() const { return m_animations; }
	RED_INLINE	   TDynArray< CSkeletalAnimationSetEntry* >& GetAnimations()	   { return m_animations; }

	// Get skeleton
	RED_INLINE const CSkeleton* GetSkeleton() const { return m_skeleton.Get(); }

public:
	CSkeletalAnimationSet();
	virtual ~CSkeletalAnimationSet();

	// Resource was loaded
	virtual void OnPostLoad();
	
	virtual void OnPropertyPostChange( IProperty* prop );

	virtual void OnSerialize( IFile& file );
	
	virtual void CleanupSourceData();

	// Get animations
	Uint32 GetAnimations( TDynArray< CSkeletalAnimationSetEntry* >& animations ) const;

	// Find animation by name (slow)
	CSkeletalAnimationSetEntry* FindAnimation( const CName& name );

	// Find animation by name (slow)
	const CSkeletalAnimationSetEntry* FindAnimation( const CName& name ) const;

	// Find animation by name (slow)
	CSkeletalAnimationSetEntry* FindAnimation( CSkeletalAnimation *anim );

public:
	Uint32 GetNumCompressedPoses() const;

	const ICompressedPose* GetCompressedPose( CompressedPoseHandle handle ) const;

public:
	//! Get information about streaming
	void GetStreamingOption( SAnimationBufferStreamingOption & outStreamingOption, Uint32 & outNonStreamableBones ) const;

#ifndef NO_EDITOR
public:
	//! Add new animation to set
	Bool AddAnimation( CSkeletalAnimation *anim );

	//! Remove animation from set
	Bool RemoveAnimation( CSkeletalAnimation *anim );

	//! Remove animation from set
	Bool RemoveAnimation( CSkeletalAnimationSetEntry *anim );

	//! Swap animation in the set
	Bool ChangeAnimation( CSkeletalAnimation *animOld, CSkeletalAnimation *animNew );

	//! Remove all empty animation from the animation set
	Bool RemoveEmptyAnimations();

	//! Return list of detected duplicates removed during loading this animation
	const TDynArray< CName >& GetDuplicatedAnimations() const { return m_duplicatedAnimations; }

public:
	const ICompressedPose* FindCompressedPoseByName( const CName& name ) const;

	const CName& FindCompressedPoseName( CompressedPoseHandle handle ) const;
	CompressedPoseHandle FindCompressedPoseHandle( const CName& name ) const;
	const SCompressedPoseInfo* FindCompressedPoseInfo( const CName& name ) const;

	Bool AddCompressedPose( const CName& poseName, const CName& animation, Float time, IFeedbackSystem* sys = NULL );
	Bool RemoveCompressedPose( const CName& poseName, IFeedbackSystem* sys = NULL );
	void RemoveCompressedPosesWithAnimation( const CName& aniamtion );

	Bool RecreateCompressedPose( const CName& poseName, IFeedbackSystem* sys = NULL );
	Bool RecreateAllCompressedPoses( IFeedbackSystem* sys  = NULL );

	struct SCompressedPosesData
	{
		typedef TPair< CName, CName > TAnimAndPose;

		TDynArray< SCompressedPoseInfo >	m_infos;
		TDynArray< TAnimAndPose >			m_poses;

		Bool IsEmpty() const
		{
			return m_infos.Empty() && m_poses.Empty();
		}
	};

	void ExportCompressedPoses( SCompressedPosesData& data ) const;
	void ImportCompressedPoses( const SCompressedPosesData& data, IFeedbackSystem* sys = NULL );

#endif

#ifndef NO_EDITOR
	Bool GetForceUncompressedImport() const { return m_forceUncompressedImport; }
	Bool ShouldOverrideBitwiseCompressionSettingsOnImport() const { return m_overrideBitwiseCompressionSettingsOnImport; }
	SAnimationBufferBitwiseCompressionPreset GetBitwiseCompressionPreset() const { return m_bitwiseCompressionPreset; }
	SAnimationBufferBitwiseCompressionSettings GetBitwiseCompressionSettings() const { return m_bitwiseCompressionSettings; }
	void SetBitwiseCompressionPreset(SAnimationBufferBitwiseCompressionPreset preset);
#endif

public:
	void LogMemoryStats();

public:
	// Get additional resource info, displayed in editor
	virtual void GetAdditionalInfo( TDynArray< String >& info ) const;

#ifndef NO_RESOURCE_IMPORT

public:
	//! Import new animation from file
	Bool ImportAnimation( const ImportAnimationOptions& options );

	//! Reimport animation
	Bool ReimportAnimation( const ImportAnimationOptions& options );

	//! Rename animation entry
	Bool RenameAnimation( CSkeletalAnimationSetEntry* entry, const CName& name );

	// Adds events file
	void AddEventsFile( CExtAnimEventsFile* eventsFile );

	// Deletes events file
	void DeleteEventsFile( CExtAnimEventsFile* eventsFile );

	// Called before save
	void OnAnimsetPreSaved();

	// Check for corrupted animations
	Bool CheckAllAnimations( TDynArray< CName >& outCorruptedAnimations, IFeedbackSystem* f = nullptr ) const;
#endif

public:

#ifdef USE_EXT_ANIM_EVENTS

	// Gets external animation events files
	RED_INLINE TDynArray< THandle< CExtAnimEventsFile > >& GetEventsFiles() { return m_extAnimEvents; }

	// Gets all events of given type
	template < class EventType >
	void GetExternalEventsOfType( const CName& animName, TDynArray< EventType* >& list ) const;

	// Gets all events of given type
	template < class EventType, class Collector >
	void GetExternalEventsOfType( const CName& animName, Collector& collector ) const;

	// Generic function to find events
	void GetExternalEventsByTime( const CSkeletalAnimationSetEntry* animation, Float prevTime, Float currTime, Int32 numLoops, Float alpha, TDynArray< CAnimationEventFired >* events, SBehaviorGraphOutput* output, const CName& tag ) const;

	Uint32 GetNumExternalEventsOfType( const CName& animName, const CClass* c ) const;

#endif

private:
	//! Copies events from one animation to another
	void CopyEventsBetweenAnimations( CSkeletalAnimationSetEntry* dest, CSkeletalAnimationSetEntry* src );

	//! When cooking - moves external events to proper animations, so finding event is faster
	void ClearExternalEvents( TDynArray< THandle< CExtAnimEventsFile > >& extAnimEvents );
};

BEGIN_CLASS_RTTI( CSkeletalAnimationSet );
	PARENT_CLASS( CExtAnimEventsFile );
	PROPERTY_INLINED( m_animations, TXT("Animations in set") );
#ifdef USE_EXT_ANIM_EVENTS
	PROPERTY_EDIT( m_extAnimEvents, TXT( "External events files" ) );
#endif
	PROPERTY_EDIT( m_skeleton, TXT("Animations skeleton") );
	PROPERTY( m_compressedPoses );
#ifndef NO_EDITOR
	PROPERTY_NOT_COOKED( m_compressedPoseInfos );
#endif
	PROPERTY_EDIT_NAME( m_streamingOption, TXT("Streaming option"), TXT("Mark if this animset should be streamable - requires resaving") );
	PROPERTY_EDIT_NAME( m_nonStreamableBones, TXT("Number of non-streamable bones"), TXT("Used for partial streaming (0 - take from skeleton)") );
#ifndef NO_EDITOR
	PROPERTY_EDIT_NOSERIALIZE( m_forceUncompressedImport, TXT("Force uncompressed import for debug only") );
	PROPERTY_EDIT_NOSERIALIZE( m_overrideBitwiseCompressionSettingsOnImport, TXT("Override bitwise compression settings") );
	PROPERTY_EDIT_NOT_COOKED( m_bitwiseCompressionPreset, TXT("Preset for bitwise compression settings") );
	PROPERTY_EDIT_NOT_COOKED( m_bitwiseCompressionSettings, TXT("Bitwise compression settings") );
#endif
END_CLASS_RTTI();

///////////////////////////////////////////////////////////

#include "skeletalAnimationSet.inl"
