/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "havokDataBuffer.h"
#include "animBuffer.h"
#include "poseCompression.h"
#include "animMemStats.h"
#include "animMath.h"
#include "animationBuffer.h"
#include "animationBufferBitwiseCompressed.h"
#include "../core/importer.h"
#include "../core/resource.h"
#include "../core/datetime.h"
#include "behaviorIncludes.h"

class IAnimationCompression;
class CUncompressedMotionExtraction;
class CAnimationStreamingJob;
class CSkeletalAnimationSet;
class ISkeletalAnimationExporter;
enum ESkeletalAnimationStreamingType : CEnum::TValueType;
class CSkeleton;

//////////////////////////////////////////////////////////////////////////

enum ESkeletalAnimationType : CEnum::TValueType
{
	SAT_Normal,
	SAT_Additive,
	SAT_MS
};

BEGIN_ENUM_RTTI( ESkeletalAnimationType );
	ENUM_OPTION( SAT_Normal );
	ENUM_OPTION( SAT_Additive );
	ENUM_OPTION( SAT_MS );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

struct AnimImporterParams
{
	String								m_authorName;
	
	// file to import from
	String								m_filePath;
	// base file path to have reference betweern fbx/max file and re file
	String								m_baseResourcePath;

	// existing resource location
	THandle< CSkeletalAnimationSet >	m_animationSet;
	class CSkeletalAnimationSetEntry*	m_existingAnimationEntry;
	class CSkeletalAnimation*			m_existingAnimation;
	
	ESkeletalAnimationType				m_type;

	EAdditiveType						m_additiveType;
	Bool								m_dontCompress;
	IAnimationCompression*				m_compression;

	const AnimQsTransform*				m_tPoseTransform;
	Int32								m_tPoseTronsformNum;

	THandle< CSkeleton >				m_skeleton;

	String								m_addAnimationToExtractedFile;

	Int32								m_maxPartFrames;

	Bool								m_extractMotion;
	Bool								m_extractTrajectory;
	Bool								m_resetRoot;
	Bool								m_preferBetterQuality;
	IImporter::ImportOptions::EErrorCode	m_errorCode;			// Return code for any anomaly while importing f.ex. wrong version etc

	AnimImporterParams();
};

struct AnimExporterParams
{
	String m_filePath; // file to import from
	const CSkeleton* m_skeleton;
	Bool m_exportTrajectory;
	AnimExporterParams()
		: m_skeleton( nullptr )
		, m_exportTrajectory( true )
	{}
};

class IMotionExtraction;

//////////////////////////////////////////////////////////////////////////
/// Since we changed CSkeletalAnimation to not be a resource we need a new importer for animations
class ISkeletalAnimationImporter
{
	DECLARE_RTTI_SIMPLE_CLASS( ISkeletalAnimationImporter );

protected:
	TDynArray< CFileFormat >	m_formats;

public:
	ISkeletalAnimationImporter();
	virtual ~ISkeletalAnimationImporter() {}

	// notice that we use animation import options directly here
	virtual class CSkeletalAnimation*	DoImport( const AnimImporterParams& options ) = 0;
	virtual Bool						PrepareForImport( const String& filePath, AnimImporterParams& options ) = 0;

public:
	static ISkeletalAnimationImporter* FindImporter( const String& fileFormat );
	static void EnumImportFormats( TDynArray< CFileFormat >& formats );
};

BEGIN_ABSTRACT_CLASS_RTTI( ISkeletalAnimationImporter )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
/// Since we changed CSkeletalAnimation to not be a resource we need a new importer for animations
class ISkeletalAnimationExporter
{
	DECLARE_RTTI_SIMPLE_CLASS( ISkeletalAnimationExporter );

protected:
	TDynArray< CFileFormat >	m_formats;

public:
	ISkeletalAnimationExporter();
	virtual ~ISkeletalAnimationExporter(){}
	virtual Bool DoExport( CSkeletalAnimation* animation, const AnimExporterParams& options ) = 0;

public:
	static ISkeletalAnimationExporter* FindExporter( const String& fileFormat );
	static void EnumExportFormats( TDynArray< CFileFormat >& formats );
};

BEGIN_ABSTRACT_CLASS_RTTI( ISkeletalAnimationExporter )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
/// Since skeletal animations are not used as separate files they were changed from CResource to ISerializable
class CSkeletalAnimation : public ISerializable
{
	friend class AnimationIterator;
	friend class CAnimsetCooker;
	friend class AnimsetLoader;
	friend class AnimationCacheCooker;
	friend class CCutsceneStreamer;

	DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( CSkeletalAnimation, MC_Animation );
	
protected:
	CSkeletalAnimationSetEntry*		m_entry;						// Animation set entry related to this animation (can be NULL)
	CName							m_name;							// Animation name

#ifndef NO_EDITOR
	CDateTime						m_importFileTimeStamp;			// Timestamp of the file this resource was imported from
	String							m_importFile;					// Path to the import file
	String							m_authorName;					// Author name
	String							m_baseResourcePath;				// fbx/max file

	ESkeletalAnimationType			m_animType;
	EAdditiveType					m_additiveType;
	String							m_addAnimationToExtractedFile;
#endif

	IAnimationBuffer*				m_animBuffer;					// Animation buffer
	Float							m_framesPerSecond;				// Animation speed (in frames per second)
	Float							m_duration;						// Animation total duration

	IMotionExtraction*				m_motionExtraction;				// Animation motion extraction ( compressed )
	CompressedPoseHandle			m_compressedPose;				// Compressed pose - first frame

#ifndef NO_EDITOR
	Bool							m_sourceDataCreatedFromAnimBuffer;	// marked that source anim data was actually created from anim buffer (not imported)
	LatentSourceAnimDataBuffer		m_sourceAnimData;				// Source anim data that is "compressed" data read from file and used for recompression (what we're lacking here is info if there is source anim data - too late for that)
	THandle< CSkeleton >			m_skeleton;						// Skeleton as reference for some utilities

	CUncompressedMotionExtraction*	m_uncompressedMotionExtraction; // Animation motion extraction ( uncompressed )
	CName							m_compressedPoseName;
#endif

	//ESkeletalAnimationPoseType		m_poseType;						// Animation pose type
	//EPoseDeformationLevel			m_poseDeformationLevel;			// Pose deformation level
	Bool							m_hasBundingBox;				// Has animation bounding box - PTom TODO: do wyrzucenia
	Box								m_boundingBox;					// Animation bounding box

	mutable Uint64					m_lastTouchTime;				// Last time this animation was used
	CAnimationStreamingJob*			m_streamingJob;					// Animation streaming job
	ESkeletalAnimationStreamingType m_streamingType;				// Streaming type

	Uint32							m_id;							// Animation id
	static const Uint32				INVALID_ID = 0xFFFFFFFF;		// Invalid id

#ifndef NO_EDITOR
private: friend class CAnimationBufferBitwiseCompressed;
	Bool										m_useOwnBitwiseCompressionParams;
	SAnimationBufferBitwiseCompressionPreset	m_bitwiseCompressionPreset;		// had to be moved here, as buffers are no longer objects and their data can't be accessed
	SAnimationBufferBitwiseCompressionSettings	m_bitwiseCompressionSettings;	// it's actually copy/proxy. well, it's actually quickly done hack to allow accessing those params
#endif

private:
	CSkeletalAnimation**			m_prevAnimation;				// Previous animation link
	CSkeletalAnimation*				m_nextAnimation;				// Link to next animation

	static CSkeletalAnimation*		AnimationList;					// Animation list ( only streamable animations )

public:
	//! Get related animation set entry
	RED_INLINE CSkeletalAnimationSetEntry* GetEntry() const { return m_entry; }

#ifndef NO_EDITOR
	//! Get animation type
	RED_INLINE ESkeletalAnimationType GetAnimationType() const { return m_animType; }

	//! Get additive type
	RED_INLINE EAdditiveType GetAdditiveType() const { return m_additiveType; }

	//! Get animation name when importing additive anim
	RED_INLINE String const & GetAddAnimationToExtractedFile() const { return m_addAnimationToExtractedFile; }
#endif

	//! Get animation name
	RED_INLINE const CName& GetName() const { return m_name; }

	//Get number of frames per second
	RED_INLINE Float GetFramesPerSecond() const { return m_framesPerSecond; } 

	//! Get animation bounding box
	RED_INLINE const Box& GetBoundingBox() const { return m_boundingBox; }

	//! Has bounding box
	RED_INLINE Bool HasBoundingBox() const { return m_hasBundingBox; }

	//! Has switches
	RED_INLINE Bool HasSwitches() const { return m_animBuffer != nullptr? m_animBuffer->GetPartsNum() > 1 : false; }

	//! Get last time this animation was used
	RED_INLINE Uint64 GetLastTouchTime() const { return m_lastTouchTime; }

	//! Get id
	RED_INLINE Uint32 GetId() const { return m_id; }

	//! Has valid id
	RED_INLINE Bool HasValidId() const { return m_id != INVALID_ID; }

	//! Do we have valid animation buffer
	RED_INLINE Bool HasAnimBuffer() const { return NULL != m_animBuffer; }

#ifdef USE_HAVOK_ANIMATION
	//! Get the animation buffer (read only)
	RED_INLINE const AnimBuffer* GetAnimBuffer() const { return m_animBuffer; }
#endif

	//! Get the animation buffer (read only)
	RED_INLINE const IAnimationBuffer* GetAnimBuffer() const { return m_animBuffer; }

#ifndef NO_EDITOR
	//! Get source anim data (to load and then to export etc)
	LatentSourceAnimDataBuffer* GetSourceAnimData(Bool createIfDoesntExist = false);
	RED_INLINE void DiscardSourceAnimData( ) { m_sourceAnimData.Clear( ); }
#endif

	//! Get animation pose type
	//RED_INLINE ESkeletalAnimationPoseType GetPoseType() const { return m_poseType; }

#ifndef NO_EDITOR
	//! Get skeleton associated with this animation (that can be used by some utility classes)
	RED_INLINE const CSkeleton* GetSkeleton() const { return m_skeleton.Get(); }
	RED_INLINE CSkeleton* GetSkeleton() { return m_skeleton.Get(); }

	//! Set skeleton associated with this animation (that can be used by some utility classes)
	void SetSkeleton(CSkeleton* skeleton);

	//! Get compressed pose handler
	RED_INLINE CompressedPoseHandle GetCompressedPoseHandle() const { return m_compressedPose; }

	//! Set coompressd pose name
	RED_INLINE void SetCompressedPoseName( const CName& name ) { m_compressedPoseName = name; }

	//! Get coompressd pose name
	RED_INLINE const CName& GetCompressedPoseName() const { return m_compressedPoseName; }

	// Get uncompress motion
	RED_INLINE const CUncompressedMotionExtraction* GetUncompressedMotion() const { return m_uncompressedMotionExtraction; }

	// Get import file name
	RED_INLINE const String& GetImportFile() const { return m_importFile; }

#endif

public:
	virtual ~CSkeletalAnimation();
	
protected:
	CSkeletalAnimation();

	//! Serialize object
	virtual void OnSerialize( IFile &file );

	//! Post load event
	virtual void OnPostLoad();

	virtual void OnPropertyPostChange( IProperty* prop );

public:
	// Get streaming type
	RED_INLINE ESkeletalAnimationStreamingType GetStreamingType() const { return m_streamingType; }

#ifndef NO_EDITOR
	RED_INLINE void SetStreamingType( ESkeletalAnimationStreamingType type ) { m_streamingType = type; }
#endif

	//! Are we streaming right now ?
	RED_INLINE Bool HasStreamingPending() const { return m_streamingJob != NULL; }

	// Can animation be streamed
	Bool CanBeStreamed() const;

	// Start streaming
	Bool StartStreaming();

	// Update streaming, returns true if it has finished
	Bool UpdateStreaming();

	// Cancel in-flight streaming
	void CancelStreaming();

	// Animation was streamed
	void OnStreamed();

	void RecreateAnimBufferFromSourceData( const IAnimationBuffer::SourceData& animData );

public:
	//! Touch animation
	virtual void Touch() const;

	//! Mark to unload
	void MarkToUnload();

public:
	//! Set animation name
	void SetName( const CName& name );

public:
	//! Get duration of the animation 
	Float GetDuration() const;

	//! Does this animation have compressed pose ?
	Bool HasCompressedPose() const; 

	// Has motion extraction compressed
	Bool HasCompressedMotionExtraction() const;

	//! Is compressed
	Bool IsCompressed() const;

	//! Recompress animation buffer using source anim data
	void Recompress();

	//! Make sure source anim data exists (fill it with data from compressed anim, if needed)
	void MakeSureSourceAnimDataExists();

#ifndef NO_EDITOR
	//! Check if has own bitwise compression params
	Bool HasOwnBitwiseCompressionParams() const { return m_useOwnBitwiseCompressionParams; }

	void RestoreAnimation();
#endif

	//! Set bitwise compressed preset (don't recompress automatically)
	void SetBitwiseCompressionPreset( SAnimationBufferBitwiseCompressionPreset preset, Bool asOwn );

	//! Set bitwise compressed preset and settings (don't recompress automatically)
	void SetBitwiseCompressionPresetAndSettings( SAnimationBufferBitwiseCompressionPreset preset, SAnimationBufferBitwiseCompressionSettings settings, Bool asOwn );

	//! Get bitwise compressed preset
	SAnimationBufferBitwiseCompressionPreset GetBitwiseCompressionPreset() const;

	//! Get bitwise compressed settings
	SAnimationBufferBitwiseCompressionSettings GetBitwiseCompressionSettings() const;
#ifndef NO_EDITOR
	// Check if there is a newer version of the file to import
	Bool CheckNewerImportFileExists() const;
	// Set import path
	void SetImportPath( const String& importPath );
	// Set suthor name
	void SetAuthorName( const String& authorName );
	// Set base resource path
	void SetBaseResourcePath( const String& resPath );

	void StoreAnythingUsefulFromImporterParams( const AnimImporterParams& params );
#endif
public:
	//! Does this animation have extracted motion ?
	virtual Bool HasExtractedMotion() const; 

	//! Get movement parameters at given time
	void GetMovementAtTime( Float time, Matrix &movement ) const;

	//! Get movement delta between times in animation
	void GetMovementBetweenTime( Float startTime, Float endTime, Int32 loops, Matrix &movement ) const;

	//! Get movement parameters at given time
	virtual AnimQsTransform GetMovementAtTime( Float time ) const;

	//! Get movement delta between times in animation
	virtual AnimQsTransform GetMovementBetweenTime( Float startTime, Float endTime, Int32 loops ) const;

public:
	//! Get static size
	Uint32 GetStaticSize() const;

	//! Get size of animation
	Uint32 GetDataSize() const;

	//! Get size of animation buffer
	Uint32 GetSizeOfAnimBuffer() const;

	//! Get memory stats
	void GetMemStats( AnimMemStats& stats ) const;

public:
	//! Check if this animation is loaded
	Bool IsLoaded( ) const { Uint32 bonesLoaded; Uint32 bonesAlwaysLoaded; return GetAnimationBufferDataAvailable( 0 /* don't request bones, just if loaded */, bonesLoaded, bonesAlwaysLoaded ) != ABDA_None; }

	//! Check if this animation is fully loaded
	Bool IsFullyLoaded() const { Uint32 bonesLoaded; Uint32 bonesAlwaysLoaded; return GetAnimationBufferDataAvailable( (Uint32)-1, bonesLoaded, bonesAlwaysLoaded ) == ABDA_All; }

	//! Preload animation
	virtual void Preload() const;

	//! Force sync load of  animation
	virtual void SyncLoad() const;

	//! What data do we have available in animation buffer (for sampling)
	virtual EAnimationBufferDataAvailable GetAnimationBufferDataAvailable( Uint32 bonesRequested, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded ) const;
	
	//! Set streaming option for animation (for its animation buffer)
	void SetStreamingOption( SAnimationBufferStreamingOption streamingOption, Uint32 nonStreamableBones );

	//! Usage counting - it should always come in pairs
#ifdef DEBUG_ANIMATION_USAGE
	RED_INLINE void AddUsage() const { if ( m_animBuffer ) { m_animBuffer->AddUsage( GetName() ); } }
	RED_INLINE void ReleaseUsage() const { if ( m_animBuffer ) { m_animBuffer->ReleaseUsage(); } }
#else
	RED_INLINE void AddUsage() const { if ( m_animBuffer ) { m_animBuffer->AddUsage(); } }
	RED_INLINE void ReleaseUsage() const { if ( m_animBuffer ) { m_animBuffer->ReleaseUsage(); } }
#endif

	//! Get number of tracks
	virtual Uint32 GetTracksNum() const;
	
	//! Get number of bones
	virtual Uint32 GetBonesNum() const;

	//! Sample animation
	virtual Bool Sample( Float time, Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) const;

	//! Sample animation
	virtual Bool Sample( Float time, TDynArray< AnimQsTransform >& bonesOut, TDynArray< AnimFloat >& tracksOut ) const;

	//! Sample animation (fallback)
	virtual Bool SampleFallback( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) const;

	//! Sample compressed frame
	Bool SampleCompressedPose( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, Float* tracksOut, const CSkeleton* skeleton ) const;

	//! Sample compressed frame - without touch
	Bool SampleCompressedPoseWithoutTouch( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, Float* tracksOut, const CSkeleton* skeleton ) const;

public:
	//! Bind animation to animation set entry and animation set (legacy code support)
	virtual void Bind( CSkeletalAnimationSetEntry* animationSetEntry, CSkeletalAnimationSet* animationSet );

public:
	//! 
	struct FactoryInfo
	{
		struct Trajectory
		{
			TDynArray< Vector >		m_data;
			Int32					m_degree;
			String					m_name;
		};

		// name
		String							m_name;

		// animation (and motion extraction) duration
		Float							m_duration;

		// animation FPS
		Float							m_framesPerSecond;

		// animation import data
		IAnimationBuffer::SourceData	m_animationData;

		// motion extraction import data (in non empty motion extraction data is created)
		TDynArray< Vector >				m_motionFrames;

		// trajectories for this animation
		TDynArray< Trajectory >			m_trajectories;

		// if possible use better animation quality
		Bool							m_preferBetterQuality;

		// animation entry
		class CSkeletalAnimationSetEntry*	m_animationEntry;

		// skeletal animation set we are creating the animation in
		class CSkeletalAnimationSet*		m_animationSet;

		FactoryInfo();
	};

#ifndef NO_RESOURCE_IMPORT

public:
	//! Create animation from animation data
	static CSkeletalAnimation* Create( const FactoryInfo& data );

	//! Generate bounding box
	virtual Bool GenerateBoundingBox( const CAnimatedComponent* component );

	//! Create compressed pose
	ICompressedPose* CreateCompressedPose( CObject* parent, const CSkeleton* skeleton, Float time );

	//! Create animation bounding box
	void CreateBBox( const CSkeleton* skeleton );

	//! Create compressed motion extraction
	void CreateCompressedMotionExtraction();

	//! Select motion extraction type - compressed or uncompressed
	void UseCompressMotionExtraction( Bool flag );

#endif

protected:
	Bool SerializeBufferOnLoading() const;

	// Get data for compressed pose
	const ICompressedPose* GetCompressedPose( CompressedPoseHandle handle ) const;

protected:
	void LinkToAnimationList();
	void UnlinkFromAnimationList();

public:
	Uint64 GetAnimBufferCRC( Uint64 seed = 0 ) const;
};

BEGIN_CLASS_RTTI( CSkeletalAnimation );
	PARENT_CLASS( ISerializable );
	PROPERTY_RO( m_name, TXT("Animation name") );

#ifndef NO_EDITOR
	PROPERTY_RO_NOT_COOKED( m_importFileTimeStamp, TXT( "Last Modified" ) );
	PROPERTY_EDIT_NAME_NOT_COOKED( m_importFile, TXT("Import file"), TXT("Import file") );
	PROPERTY_RO_NOT_COOKED( m_authorName, TXT("Author name") );
	PROPERTY_RO_NOT_COOKED( m_baseResourcePath, TXT("Base resource Path") );

	// TODO in future, after everything is imported and this info is stored, change those values to RO
	PROPERTY_EDIT_NAME( m_animType, TXT("Animation type for reimport"), TXT("Animation type (next reimport will use this setting)") );
	PROPERTY_EDIT_NAME( m_additiveType, TXT("Additive type for reimport"), TXT("Additive type (next reimport will use this setting)") );
	PROPERTY_EDIT_NAME_NOT_COOKED( m_addAnimationToExtractedFile, TXT("Additive anim ref name for reimport"), TXT("Animation for additive (next reimport will use this setting)") );
	PROPERTY_EDIT_NOT_COOKED( m_skeleton, TXT("Skeleton as reference for some utilities") );
#endif
	//PROPERTY_EDIT( m_poseType, TXT("Pose type") );
	//PROPERTY_EDIT( m_poseDeformationLevel, TXT("Pose deformation level") );
	PROPERTY_EDIT( m_streamingType, TXT("") );
#ifndef NO_EDITOR
	PROPERTY_EDIT_NOT_COOKED( m_useOwnBitwiseCompressionParams, TXT("Use own bitwise compression preset/settings") )
	PROPERTY_EDIT_NOT_COOKED( m_bitwiseCompressionPreset, TXT("Bitwise compression preset") )
	PROPERTY_EDIT_NOT_COOKED( m_bitwiseCompressionSettings, TXT("Bitwise compression settings") );
#endif
#ifndef NO_EDITOR
	PROPERTY_RO_NOT_COOKED( m_sourceDataCreatedFromAnimBuffer, TXT("") );
#endif
	PROPERTY( m_hasBundingBox );
	PROPERTY( m_boundingBox );
	PROPERTY( m_id );
	PROPERTY( m_motionExtraction );
	PROPERTY( m_compressedPose );
	PROPERTY( m_animBuffer );
	PROPERTY( m_framesPerSecond );
	PROPERTY( m_duration );
#ifndef NO_EDITOR
	PROPERTY_NOT_COOKED( m_uncompressedMotionExtraction );
	PROPERTY_NOT_COOKED( m_compressedPoseName );
#endif
END_CLASS_RTTI();
