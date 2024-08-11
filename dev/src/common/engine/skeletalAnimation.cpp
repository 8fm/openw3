/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "skeletalAnimation.h"
#include "havokAnimationUtils.h"
#include "animationCache2.h"
#include "animationBufferUncompressed.h"
#include "animationBuffer.h"
#include "../core/feedback.h"
#include "motionExtraction.h"
#include "animationManager.h"
#include "skeletalAnimationEntry.h"
#include "skeletalAnimationSet.h"
#include "behaviorIncludes.h"
#include "skeleton.h"
#include "baseEngine.h"
#include "animationLogger.h"

IMPLEMENT_RTTI_ENUM( ESkeletalAnimationType );
IMPLEMENT_ENGINE_CLASS( CSkeletalAnimation );
IMPLEMENT_ENGINE_CLASS( ISkeletalAnimationImporter );
IMPLEMENT_ENGINE_CLASS( ISkeletalAnimationExporter );

//---------------------------------------------------------------------------

AnimImporterParams::AnimImporterParams()
	: m_type( SAT_Normal )
	, m_additiveType( AT_Local )
	, m_dontCompress( false )
	, m_compression( nullptr )
	, m_tPoseTransform( nullptr )
	, m_tPoseTronsformNum( 0 )
	, m_skeleton( 0 )
	, m_maxPartFrames( -1 )
	, m_extractMotion( true )
	, m_extractTrajectory( true )
	, m_resetRoot( false )
	, m_preferBetterQuality( false )
	, m_animationSet( nullptr )
	, m_existingAnimationEntry( nullptr )
	, m_existingAnimation( nullptr )
	, m_authorName( String::EMPTY )
	, m_filePath( String::EMPTY )
	, m_baseResourcePath( String::EMPTY )
	, m_errorCode( IImporter::ImportOptions::EEC_Success )
{}

ISkeletalAnimationImporter::ISkeletalAnimationImporter()
{
}

ISkeletalAnimationImporter* ISkeletalAnimationImporter::FindImporter( const String& fileFormat )
{
	// Request importer classes
	TDynArray< CClass* > importerClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< ISkeletalAnimationImporter >(), importerClasses );

	// Linear search :P
	for ( Uint32 i=0; i<importerClasses.Size(); i++ )
	{
		ISkeletalAnimationImporter* importer = importerClasses[i]->GetDefaultObject< ISkeletalAnimationImporter >();
		if ( NULL != importer )
		{
			const TDynArray< CFileFormat >& formats = importer->m_formats;

			for ( Uint32 j=0; j<formats.Size(); ++j )
			{
				if ( formats[j].GetExtension().EqualsNC( fileFormat ) )
				{
					return importer;
				}
			}
		}
	}

	// Format not supported
	return NULL;
}

void ISkeletalAnimationImporter::EnumImportFormats( TDynArray< CFileFormat >& formats )
{
	// Request importer classes
	TDynArray< CClass* > importerClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< ISkeletalAnimationImporter >(), importerClasses );

	// Gather import formats
	for ( Uint32 i=0; i<importerClasses.Size(); i++ )
	{
		ISkeletalAnimationImporter* importer = importerClasses[i]->GetDefaultObject< ISkeletalAnimationImporter >();
		if ( NULL != importer )
		{
			formats.PushBack( importer->m_formats );
		}
	}
}

//---------------------------------------------------------------------------

ISkeletalAnimationExporter::ISkeletalAnimationExporter()
{
}

ISkeletalAnimationExporter* ISkeletalAnimationExporter::FindExporter( const String& fileFormat )
{
	// Request importer classes
	TDynArray< CClass* > exporterClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< ISkeletalAnimationExporter >(), exporterClasses );

	// Linear search :P
	for ( Uint32 i=0; i<exporterClasses.Size(); i++ )
	{
		ISkeletalAnimationExporter* exporter = exporterClasses[i]->GetDefaultObject< ISkeletalAnimationExporter >();
		if ( NULL != exporter )
		{
			const TDynArray< CFileFormat >& formats = exporter->m_formats;

			for ( Uint32 j=0; j<formats.Size(); ++j )
			{
				if ( formats[j].GetExtension().EqualsNC( fileFormat ) )
				{
					return exporter;
				}
			}
		}
	}

	// Format not supported
	return NULL;
}

void ISkeletalAnimationExporter::EnumExportFormats( TDynArray< CFileFormat >& formats )
{
	// Request exporter classes
	TDynArray< CClass* > exporterClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< ISkeletalAnimationExporter >(), exporterClasses );

	// Gather import formats
	for ( Uint32 i=0; i<exporterClasses.Size(); i++ )
	{
		ISkeletalAnimationExporter* exporter = exporterClasses[i]->GetDefaultObject< ISkeletalAnimationExporter >();
		if ( NULL != exporter )
		{
			formats.PushBack( exporter->m_formats );
		}
	}
}

//---------------------------------------------------------------------------

CSkeletalAnimation* CSkeletalAnimation::AnimationList = NULL;

CSkeletalAnimation::CSkeletalAnimation()
	: m_entry( nullptr )
#ifndef NO_EDITOR
	, m_animType( SAT_Normal )
	, m_additiveType( AT_Local )
#endif
	, m_id( INVALID_ID )
	, m_streamingJob( nullptr )
	, m_lastTouchTime( 0 )
	, m_nextAnimation( nullptr )
	, m_prevAnimation( nullptr )
	, m_motionExtraction( nullptr )
	, m_compressedPose( -1 )
	, m_streamingType( SAST_Standard )
	//, m_poseType( SAPT_Invalid )
	//, m_poseDeformationLevel( PDL_Medium )
	, m_animBuffer( nullptr )
#ifndef NO_EDITOR
	, m_uncompressedMotionExtraction( nullptr )
	, m_sourceDataCreatedFromAnimBuffer( false )
	, m_skeleton( nullptr )
	, m_useOwnBitwiseCompressionParams( false )
#endif
{
}

CSkeletalAnimation::~CSkeletalAnimation()
{

	if ( HasStreamingPending() )
	{
		CancelStreaming();
	}

	if ( CanBeStreamed() )
	{
		UnlinkFromAnimationList();
	}

	if ( NULL != m_animBuffer )
	{
		delete m_animBuffer;
		m_animBuffer = NULL;
	}

	if ( NULL != m_motionExtraction )
	{
		delete m_motionExtraction;
		m_motionExtraction = NULL;
	}

#ifndef NO_EDITOR
	if ( NULL != m_uncompressedMotionExtraction )
	{
		delete m_uncompressedMotionExtraction;
		m_uncompressedMotionExtraction = NULL;
	}
#endif

	ASSERT( !m_streamingJob );
	ASSERT( !m_nextAnimation );
	ASSERT( !m_prevAnimation );
}

#ifndef NO_EDITOR
Bool CSkeletalAnimation::CheckNewerImportFileExists() const
{
	// Check if there exists a file with never version
	CDateTime importFileTimeStamp( GFileManager->GetFileTime( m_importFile ) );
	if ( importFileTimeStamp.IsValid() && importFileTimeStamp > m_importFileTimeStamp )
	{
		return true;
	}

	// Not found
	return false;
}

void CSkeletalAnimation::SetImportPath( const String& importPath )
{
	m_importFile = importPath;
	m_importFileTimeStamp = GFileManager->GetFileTime( importPath );
}

void CSkeletalAnimation::SetAuthorName( const String& n )
{
	m_authorName = n;
}

void CSkeletalAnimation::SetBaseResourcePath( const String& n )
{
	m_baseResourcePath = n;
}

void CSkeletalAnimation::StoreAnythingUsefulFromImporterParams( const AnimImporterParams& params )
{
	m_animType = params.m_type;
	m_additiveType = params.m_additiveType;
	m_addAnimationToExtractedFile = params.m_addAnimationToExtractedFile;
#ifndef NO_EDITOR
	if (params.m_skeleton)
	{
		m_skeleton = const_cast<CSkeleton*>( params.m_skeleton.Get() );
	}
#endif //!NO_EDITOR
}
#endif

CSkeletalAnimation::FactoryInfo::FactoryInfo()
	: m_duration(0.0f)
	, m_framesPerSecond(30.0f)
	, m_animationEntry( NULL )
	, m_animationSet( NULL )
{
}

void CSkeletalAnimation::Bind( CSkeletalAnimationSetEntry* animationSetEntry, CSkeletalAnimationSet* /*animationSet*/ )
{
	ASSERT( NULL != animationSetEntry );
	ASSERT( NULL == m_entry || m_entry == animationSetEntry );
	m_entry = animationSetEntry;
}

void CSkeletalAnimation::OnSerialize( IFile &file )
{
#ifndef NO_EDITOR
	if ( file.IsCooker() && file.IsWriter() )
	{
		// make sure animation is compressed, so we won't cook uncompressed animation
		if ( CAnimationBufferUncompressed * uncompressed = Cast< CAnimationBufferUncompressed >( m_animBuffer ) )
		{
			SetBitwiseCompressionPreset( ABBCP_VeryHighQuality, true );
			Recompress();
		}

		// remove source data after cooking
		m_sourceAnimData.Clear();
	}
#endif

	ISerializable::OnSerialize( file );

	if ( file.IsReader() && (NULL == m_animBuffer))
	{
		WARN_ENGINE (TXT("Reading animation with NULL animation buffer - was the animation converted to new format ? '%ls'"), m_name.AsChar() );
	}

#ifndef NO_EDITOR
	// store/read source anim data (TODO: in cooked resource this buffer is empty)
	m_sourceAnimData.Serialize(file, true, false);
#endif
}

void CSkeletalAnimation::OnPostLoad()
{
	ISerializable::OnPostLoad();

#ifndef NO_EDITOR
	// copy from bitwise compressed buffer to know what's current settings
	if ( CAnimationBufferBitwiseCompressed* bitwiseCompressedBuffer = Cast< CAnimationBufferBitwiseCompressed >( m_animBuffer ) )
	{
		m_bitwiseCompressionPreset = bitwiseCompressedBuffer->m_compressionPreset;
		m_bitwiseCompressionSettings = bitwiseCompressedBuffer->m_compressionSettings;
	}
#endif // NO_EDITOR

	if ( CanBeStreamed() )
	{
		ASSERT( !m_animBuffer );

		LinkToAnimationList();

		ASSERT( GAnimationManager );
		GAnimationManager->OnAnimationCreated( this );
	}
}

void CSkeletalAnimation::OnPropertyPostChange( IProperty* prop )
{
	ISerializable::OnPropertyPostChange( prop );

#ifndef NO_EDITOR
	Bool copyToAnimBufferAndRecompress = false;
	if ( prop->GetName() == TXT("bitwiseCompressionPreset") )
	{
		m_bitwiseCompressionSettings.UsePreset( m_bitwiseCompressionPreset );
		m_useOwnBitwiseCompressionParams = true;
		copyToAnimBufferAndRecompress = true;
	}
	if ( prop->GetName() == TXT("bitwiseCompressionSettings") )
	{
		m_bitwiseCompressionPreset = ABBCP_Custom;
		m_useOwnBitwiseCompressionParams = true;
		copyToAnimBufferAndRecompress = true;
	}
	if ( prop->GetName() == TXT("useOwnBitwiseCompressionParams" ) )
	{
		if ( ! m_useOwnBitwiseCompressionParams )
		{
			if ( GetEntry() && GetEntry()->GetAnimSet() )
			{
				m_bitwiseCompressionPreset = GetEntry()->GetAnimSet()->GetBitwiseCompressionPreset();
				m_bitwiseCompressionSettings = GetEntry()->GetAnimSet()->GetBitwiseCompressionSettings();
			}
		}
		copyToAnimBufferAndRecompress = true;
	}
	if ( copyToAnimBufferAndRecompress && m_animBuffer )
	{
		// copy values to bitwise compressed anim buffer
		m_animBuffer->SetCompressionParams( m_bitwiseCompressionPreset, m_bitwiseCompressionSettings );

		if ( GFeedback->AskYesNo( TXT("Recompress animation now?") ) )
		{
			Recompress();

			if ( GetEntry() && GetEntry()->GetAnimSet() )
			{
				GetEntry()->GetAnimSet()->LogMemoryStats();
			}
		}
	}
#endif
}

void CSkeletalAnimation::SetBitwiseCompressionPreset( SAnimationBufferBitwiseCompressionPreset preset, Bool asOwn )
{
#ifndef NO_EDITOR
	m_useOwnBitwiseCompressionParams = asOwn;
	m_bitwiseCompressionPreset = preset;
	m_bitwiseCompressionSettings.UsePreset( m_bitwiseCompressionPreset );
	if ( m_animBuffer )
	{
		m_animBuffer->SetCompressionParams( m_bitwiseCompressionPreset, m_bitwiseCompressionSettings );
	}
#endif
}

void CSkeletalAnimation::SetBitwiseCompressionPresetAndSettings( SAnimationBufferBitwiseCompressionPreset preset, SAnimationBufferBitwiseCompressionSettings settings, Bool asOwn )
{
#ifndef NO_EDITOR
	m_useOwnBitwiseCompressionParams = asOwn;
	m_bitwiseCompressionPreset = preset;
	m_bitwiseCompressionSettings = settings;
	if ( m_animBuffer )
	{
		m_animBuffer->SetCompressionParams( m_bitwiseCompressionPreset, m_bitwiseCompressionSettings );
	}
#endif
}

SAnimationBufferBitwiseCompressionPreset CSkeletalAnimation::GetBitwiseCompressionPreset() const
{
#ifndef NO_EDITOR
	return m_bitwiseCompressionPreset;
#else
	return ABBCP_Custom;
#endif
}

SAnimationBufferBitwiseCompressionSettings CSkeletalAnimation::GetBitwiseCompressionSettings() const
{
#ifndef NO_EDITOR
	return m_bitwiseCompressionSettings;
#else
	return SAnimationBufferBitwiseCompressionSettings();
#endif
}

void CSkeletalAnimation::LinkToAnimationList()
{
	ASSERT( !m_nextAnimation );
	ASSERT( !m_prevAnimation );

	m_prevAnimation = &AnimationList;
	m_nextAnimation = AnimationList;

	if ( AnimationList )
	{
		AnimationList->m_prevAnimation = &m_nextAnimation;
	}	

	AnimationList = this;
}

void CSkeletalAnimation::UnlinkFromAnimationList()
{
	if ( m_nextAnimation )
	{
		m_nextAnimation->m_prevAnimation = m_prevAnimation;
	}

	if ( m_prevAnimation )
	{
		*m_prevAnimation = m_nextAnimation; 
	}

	m_nextAnimation = NULL;
	m_prevAnimation = NULL;
}

Uint64 CSkeletalAnimation::GetAnimBufferCRC( Uint64 seed ) const
{
	if( m_animBuffer == nullptr )
		return 0;
	return m_animBuffer->GetDataCRC( seed );
}

void CSkeletalAnimation::Touch() const
{
	m_lastTouchTime = GEngine->GetCurrentEngineTick();

	if ( CanBeStreamed() && !IsLoaded() && !HasStreamingPending() )
	{
		GAnimationManager->MarkToLoad( this );
	}
}

void CSkeletalAnimation::MarkToUnload()
{
	if ( CanBeStreamed() )
	{
		GAnimationManager->MarkToUnload( this );
	}
}

void CSkeletalAnimation::SetName( const CName& name )
{
	m_name = name;
}

Float CSkeletalAnimation::GetDuration() const
{
	return m_duration;
}

Bool CSkeletalAnimation::HasCompressedMotionExtraction() const
{
	if ( m_motionExtraction )
	{
		return m_motionExtraction->IsCompressed();
	}
	else
	{
		return true;
	}
}

Bool CSkeletalAnimation::HasExtractedMotion() const
{
	return m_motionExtraction != NULL;
}

void CSkeletalAnimation::GetMovementAtTime( Float time, Matrix &movement ) const
{
	RedQsTransform motionOut = GetMovementAtTime( time );
	RedMatrix4x4 conversionMatrix;
	conversionMatrix = motionOut.ConvertToMatrix();
	movement = reinterpret_cast< const Matrix& >( conversionMatrix );
}

void CSkeletalAnimation::GetMovementBetweenTime( Float startTime, Float endTime, Int32 loops, Matrix &movement ) const
{
	RedQsTransform motionOut = GetMovementBetweenTime( startTime, endTime, loops );
	RedMatrix4x4 conversionMatrix;
	conversionMatrix = motionOut.ConvertToMatrix();
	movement = reinterpret_cast< const Matrix& >( conversionMatrix );
}

AnimQsTransform CSkeletalAnimation::GetMovementAtTime( Float time ) const
{
	AnimQsTransform motionOut( AnimQsTransform::IDENTITY );
	if ( m_motionExtraction )
	{
		m_motionExtraction->GetMovementAtTime( time, motionOut );
	}
	else
	{
		ASSERT( 0 );
	}
	return motionOut;
}

AnimQsTransform CSkeletalAnimation::GetMovementBetweenTime( Float startTime, Float endTime, Int32 loops ) const
{
	AnimQsTransform motionOut( AnimQsTransform::IDENTITY );
	if ( m_motionExtraction )
	{
		m_motionExtraction->GetMovementBetweenTime( startTime, endTime, loops, motionOut );
	}
	else
	{
		ASSERT( 0 );
	}

	return motionOut;
}

Uint32 CSkeletalAnimation::GetStaticSize() const
{
	Uint32 me = sizeof( CSkeletalAnimation );
	return me;
}

Uint32 CSkeletalAnimation::GetDataSize() const
{
	Uint32 mem = sizeof(CSkeletalAnimation);

	if ( NULL != m_motionExtraction)
	{
		mem +=  m_motionExtraction->GetDataSize();
	}

	if ( HasCompressedPose() )
	{
		const ICompressedPose* pose = GetCompressedPose( m_compressedPose );
		if ( pose )
		{
			mem += pose->GetSize();
		}
	}

	if ( NULL != m_animBuffer )
	{
		Uint32 nonStreamable;
		Uint32 streamableLoaded;
		Uint32 streamableTotal;
		Uint32 sourceDataSize;
		m_animBuffer->GetMemorySize(nonStreamable, streamableLoaded, streamableTotal, sourceDataSize);
		mem += nonStreamable + streamableTotal;
	}

	return mem;
}

Uint32 CSkeletalAnimation::GetSizeOfAnimBuffer() const
{
	return 0;
}

void CSkeletalAnimation::GetMemStats( AnimMemStats& stats ) const
{
	if (m_animBuffer)
	{
		m_animBuffer->GetMemorySize( stats.m_animBufferNonStreamable, stats.m_animBufferStreamableLoaded, stats.m_animBufferStreamableWhole, stats.m_animSource );
	}
	else
	{
		stats.m_animBufferNonStreamable = 0;
		stats.m_animBufferStreamableLoaded = 0;
		stats.m_animBufferStreamableWhole = 0;
		stats.m_animSource = 0;
	}
	stats.m_animCachedBuffer = 0;
	stats.m_motionExtraction = m_motionExtraction ? m_motionExtraction->GetDataSize() : 0;
	stats.m_compressedPose = 0;
	stats.m_compressedPoseData = 0;

	if ( HasCompressedPose() )
	{
		const ICompressedPose* pose = GetCompressedPose( m_compressedPose );
		if ( pose )
		{
			stats.m_compressedPose = pose->GetSize();
			stats.m_compressedPoseData = pose->GetBufferSize();
		}
	}
}

Bool CSkeletalAnimation::IsCompressed() const
{
	if (NULL != m_animBuffer)
	{
		return m_animBuffer->IsCompressed();
	}

	return false;
}

#ifndef NO_EDITOR
void CSkeletalAnimation::RestoreAnimation()
{
	if ( GetEntry() && GetEntry()->GetAnimSet() )
	{
		String filepath = GetEntry()->GetAnimSet() ? GetEntry()->GetAnimSet()->GetDepotPath() : String::EMPTY;

		Bool valid = m_sourceAnimData.Load() && m_sourceAnimData.GetSize();
		IAnimationBuffer::SourceData animationData;
		if( valid )
		{
			m_sourceAnimData.ReadAnimDataTo( animationData, this );
			valid = animationData.IsValid( this );
		}
		else
		{
			RED_LOG( RED_LOG_CHANNEL( AnimationRestore ), TXT( "Source animation data is empty or couldn't be loaded, file: %ls, animation: %ls" ), filepath.AsChar(), GetName().AsString().AsChar() );
		}

		if ( !valid )
		{
			animationData.BuildFrom(this);
			m_sourceDataCreatedFromAnimBuffer = true;
			m_sourceAnimData.LoadAnimDataFrom( animationData );
		}

		// recreate animBuffer from source to check if everything went okay
		if ( !m_sourceAnimData.ReadAnimDataTo( animationData, this ) )
		{
			RED_LOG( RED_LOG_CHANNEL( AnimationRestore ), TXT( "Couldn't read source animation data after recreating. Animation will be corrupted, file: %ls, animation: %ls" ), 
				filepath.AsChar(), GetName().AsString().AsChar() );
		}
		RecreateAnimBufferFromSourceData( animationData );

		// cleanup
		animationData.CleanUp();
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( AnimationRestore ), TXT( "Couldn't find entry or animset while restoring, animation: %ls" ), GetName().AsString().AsChar() );
	}
}
#endif

void CSkeletalAnimation::Recompress()
{
#ifndef NO_EDITOR
	if (GetEntry() && GetEntry()->GetAnimSet())
	{
		String filePath = GetEntry()->GetAnimSet() ? GetEntry()->GetAnimSet()->GetDepotPath() : String::EMPTY;

		if (m_sourceAnimData.Load())
		{
			if ( m_sourceAnimData.GetSize() )
			{
				IAnimationBuffer::SourceData animationData;
				if (! m_sourceAnimData.ReadAnimDataTo( animationData, this ) ||
					! animationData.IsValid( this ))
				{
					// build source data from current anim buffer
					animationData.BuildFrom(this);
					m_sourceDataCreatedFromAnimBuffer = true;
					// and store it in source data
					m_sourceAnimData.LoadAnimDataFrom( animationData );
					// read it again (in case importing changes something)
					m_sourceAnimData.ReadAnimDataTo( animationData, this );
				}
				else
				{
					RED_LOG( RED_LOG_CHANNEL( AnimationRecompress ), TXT( "Couldn't read source animation data while recompressing, file: %ls, animation: %ls" ), 
						filePath.AsChar(), GetName().AsString().AsChar() );
				}
				RecreateAnimBufferFromSourceData( animationData );
				// cleanup
				animationData.CleanUp();
			}
			else
			{
				RED_LOG( RED_LOG_CHANNEL( AnimationRecompress ), TXT( "Source animation data is empty (recompressing), file: %ls, animation: %ls" ), 
					filePath.AsChar(), GetName().AsString().AsChar() );
			}
		}
		else
		{
			RED_LOG( RED_LOG_CHANNEL( AnimationRecompress ), TXT( "Couldn't load source animation data is empty while recompressing, file: %ls, animation: %ls" ), 
				filePath.AsChar(), GetName().AsString().AsChar() );
		}
	}
	else
	{
		RED_LOG( RED_LOG_CHANNEL( AnimationRecompress ), TXT( "Couldn't find entry or animset while recompressing, animation: %ls" ), GetName().AsString().AsChar() );
	}
#endif
}

void CSkeletalAnimation::RecreateAnimBufferFromSourceData( const IAnimationBuffer::SourceData& animationData )
{
	// create/recompress
	IAnimationBuffer* oldBuffer = m_animBuffer;
	m_animBuffer = IAnimationBuffer::CreateFromSourceData(GetEntry()->GetAnimSet(), this, animationData, oldBuffer, false );
	{	// get frames per second from bitwise compressed
		const IAnimationBuffer * animBuffer = m_animBuffer;
		while (animBuffer)
		{
			const IAnimationBuffer * partBuffer = animBuffer->GetPart(0);
			if (animBuffer == partBuffer)
			{
				break;
			}
			animBuffer = partBuffer;
		}
		if ( const CAnimationBufferBitwiseCompressed * bitwiseCompressed = Cast< CAnimationBufferBitwiseCompressed >( animBuffer ) )
		{
			m_framesPerSecond = 1.0f / bitwiseCompressed->GetDeltaFrameTime();
		}
	}
}

void CSkeletalAnimation::MakeSureSourceAnimDataExists()
{
#ifndef NO_EDITOR
	if (GetEntry() && GetEntry()->GetAnimSet())
	{
		if (m_sourceAnimData.Load())
		{
			if ( m_sourceAnimData.GetSize() )
			{
				IAnimationBuffer::SourceData animationData;
				if (! m_sourceAnimData.ReadAnimDataTo( animationData, this ) ||
					! animationData.IsValid( this ))
				{
					// build source data from current anim buffer
					animationData.BuildFrom(this);
					m_sourceDataCreatedFromAnimBuffer = true;
					// and store it in source data
					m_sourceAnimData.LoadAnimDataFrom( animationData );
					// read it again (in case importing changes something)
					m_sourceAnimData.ReadAnimDataTo( animationData, this );
				}
				// cleanup
				animationData.CleanUp();
			}
		}
	}
#endif
}

#ifndef NO_EDITOR
LatentSourceAnimDataBuffer* CSkeletalAnimation::GetSourceAnimData(Bool createIfDoesntExist)
{
	if (GetEntry() && GetEntry()->GetAnimSet())
	{
		if (m_sourceAnimData.Load())
		{
			if ( createIfDoesntExist )
			{
				if ( m_sourceAnimData.GetSize() )
				{
					// read anim data to make sure that it exists and is in proper format/version - if it doesn't, recreate it
					IAnimationBuffer::SourceData animationData;
					if (! m_sourceAnimData.ReadAnimDataTo( animationData, this ) ||
						! animationData.IsValid( this ))
					{
						// build source data from current anim buffer
						animationData.BuildFrom(this);
						m_sourceDataCreatedFromAnimBuffer = true;
						// and store it in source data
						m_sourceAnimData.LoadAnimDataFrom( animationData );
						// read it again (in case importing changes something)
						m_sourceAnimData.ReadAnimDataTo( animationData, this );
					}
					// cleanup
					animationData.CleanUp();
				}
			}
			return &m_sourceAnimData;
		}
	}
	return nullptr;
}
#endif

void CSkeletalAnimation::Preload() const
{
	if ( m_animBuffer )
	{
		m_animBuffer->Preload();
	}
}

void CSkeletalAnimation::SyncLoad() const
{
	if ( m_animBuffer )
	{
		m_animBuffer->SyncLoad();
	}
}

EAnimationBufferDataAvailable CSkeletalAnimation::GetAnimationBufferDataAvailable( Uint32 bonesRequested, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded ) const 
{ 
	outBonesLoaded = 0;
	return m_animBuffer? m_animBuffer->GetAnimationBufferDataAvailable( bonesRequested, outBonesLoaded, outBonesAlwaysLoaded ) : ABDA_None;
}

void CSkeletalAnimation::SetStreamingOption( SAnimationBufferStreamingOption streamingOption, Uint32 nonStreamableBones )
{
	if ( m_animBuffer )
	{
		// we have to make sure that this data exist here
		// otherwise we will switch streaming option, on recompression we would try to rebuild source data and we would do that assuming new streaming option set!
		MakeSureSourceAnimDataExists();
		m_animBuffer->SetStreamingOption( streamingOption, nonStreamableBones );
		// always recompress
		Recompress();
	}
}

Uint32 CSkeletalAnimation::GetTracksNum() const
{
	ASSERT( HasAnimBuffer() );
	return m_animBuffer->GetTracksNum();
}

Uint32 CSkeletalAnimation::GetBonesNum() const
{
	ASSERT( HasAnimBuffer() );
	if( HasAnimBuffer() ) return m_animBuffer->GetBonesNum();
	else return 0;
}

Bool CSkeletalAnimation::Sample( Float time, Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) const
{
	Touch();

	if ( IsLoaded() )
	{
		// OK, here's the thing: IAnimationBuffer is always a single object
		// IF there's a multi part animation in it it's hidden by the interface.
		// See the CAnimationBufferMultipart implementation.
		if ( NULL != m_animBuffer )
		{

#ifdef USE_ANIMATION_LOGGER
			if ( GGame->GetGameplayConfig().m_logSampledAnimations )
			{
				SAnimationLogger::GetInstance().Log( this, EAnimationLogType::EALT_SampledAnim );
			}
#endif

			const Float frame = time * m_framesPerSecond;
			return m_animBuffer->Sample( frame, boneNumIn, bonesOut, tracksNumIn, tracksOut );
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}

Bool CSkeletalAnimation::Sample( Float time, TDynArray< AnimQsTransform >& bonesOut, TDynArray< AnimFloat >& tracksOut ) const
{
	Touch();

	if ( IsLoaded() )
	{
		bonesOut.Resize( m_animBuffer->GetBonesNum() );
		tracksOut.Resize( m_animBuffer->GetTracksNum() );

		const Float frame = time * m_framesPerSecond;
		if ( NULL != m_animBuffer )
		{
#ifdef USE_ANIMATION_LOGGER
			if ( GGame->GetGameplayConfig().m_logSampledAnimations )
			{
				SAnimationLogger::GetInstance().Log( this, EAnimationLogType::EALT_SampledAnim );
			}
#endif

			m_animBuffer->Sample( frame, bonesOut.Size(), bonesOut.TypedData(), tracksOut.Size(), tracksOut.TypedData() );
			return true;
		}
		else
		{
			Red::System::MemoryZero( bonesOut.TypedData(), bonesOut.Size()*sizeof(RedQsTransform) );
			Red::System::MemoryZero( tracksOut.TypedData(), tracksOut.Size()*sizeof(Float) );
			return true;
		}
	}
	else
	{
		return true;
	}
}

Bool CSkeletalAnimation::SampleFallback( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) const
{
	// OK, here's the thing: IAnimationBuffer is always a single object
	// IF there's a multi part animation in it it's hidden by the interface.
	// See the CAnimationBufferMultipart implementation.
	if ( NULL != m_animBuffer )
	{
		return m_animBuffer->Sample( 0.0f, boneNumIn, bonesOut, tracksNumIn, tracksOut, true );
	}
	else
	{
		return false;
	}
}

Bool CSkeletalAnimation::SampleCompressedPose( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, Float* tracksOut, const CSkeleton* skeleton ) const
{
	Touch();

	return SampleCompressedPoseWithoutTouch( boneNumIn, bonesOut, tracksNumIn, tracksOut, skeleton );
}

Bool CSkeletalAnimation::SampleCompressedPoseWithoutTouch( Uint32 boneNumIn, AnimQsTransform* bonesOut, Uint32 tracksNumIn, Float* tracksOut, const CSkeleton* skeleton ) const
{
	if ( HasCompressedPose() )
	{
		const ICompressedPose* pose = GetCompressedPose( m_compressedPose );
		if ( pose )
		{
			return pose->DecompressAndSample( boneNumIn, bonesOut, tracksNumIn, tracksOut, skeleton );
		}
		else
		{
			ASSERT( pose );
		}
	}

	return false;
}

Bool CSkeletalAnimation::HasCompressedPose() const
{
#ifndef NO_EDITOR
	return m_compressedPoseName != CName::NONE;
#else
	return m_compressedPose != -1;
#endif
}

const ICompressedPose* CSkeletalAnimation::GetCompressedPose( CompressedPoseHandle handle ) const
{
	const CSkeletalAnimationSet* animset = GetEntry() ? GetEntry()->GetAnimSet() : NULL;

#ifndef NO_EDITOR
	if ( animset )
	{
		return animset->FindCompressedPoseByName( m_compressedPoseName );
	}
#else
	if ( HasCompressedPose() )
	{
		if ( animset )
		{
			return animset->GetCompressedPose( handle );
		}
	}
#endif

	return NULL;
}

#ifndef NO_EDITOR
void CSkeletalAnimation::SetSkeleton(CSkeleton* skeleton) 
{
	m_skeleton = skeleton; 
}
#endif
