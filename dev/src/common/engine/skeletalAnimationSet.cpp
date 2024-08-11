/*
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "skeletalAnimationContainer.h"
#include "../core/feedback.h"
#include "skeleton.h"
#include "../core/set.h"

IMPLEMENT_ENGINE_CLASS( SCompressedPoseInfo );
IMPLEMENT_ENGINE_CLASS( CSkeletalAnimationSet );

ImportAnimationOptions::ImportAnimationOptions() 
	: m_type( SAT_Normal )
	, m_overrideAnimType( true )
	, m_addType( AT_Local )
	, m_animationName( CName::NONE )
	, m_feedback( GFeedback )
	, m_maxPartFrames( -1 )
	, m_extractMotion( true )
	, m_extractTrajectory( true )
	, m_resetRoot( false )
	, m_preferBetterQuality( false )
{}

CSkeletalAnimationSet::CSkeletalAnimationSet()
	: m_skeleton( NULL )
#ifndef NO_EDITOR
	, m_forceUncompressedImport( false )
	, m_overrideBitwiseCompressionSettingsOnImport( true )
	, m_bitwiseCompressionPreset( ABBCP_NormalQuality )
#endif // !NO_EDITOR
	, m_streamingOption( ABSO_NonStreamable )
	, m_nonStreamableBones( 0 )
{
#ifndef NO_EDITOR
	m_bitwiseCompressionSettings.UsePreset( m_bitwiseCompressionPreset );
#endif
}

CSkeletalAnimationSet::~CSkeletalAnimationSet()
{
	if ( CAnimationMap::GetInstance() )
		CAnimationMap::GetInstance()->RemoveAnimationSetEntries( m_animations );

	m_animations.ClearPtr();
}

void CSkeletalAnimationSet::OnPostLoad()
{
	// Pass to base class
	TBaseClass::OnPostLoad();

	CAnimationMap::GetInstance()->AddAnimationSetEntries( m_animations );

	// Make sure we don't have duplicated animation entries in the table
	{
		TDynArray< CSkeletalAnimationSetEntry* > validEntries;
		THashSet< CName > validEntriesNames;
		validEntries.Reserve( m_animations.Size() );
		validEntriesNames.Reserve( m_animations.Size() );
		for ( CSkeletalAnimationSetEntry* entry : m_animations )
		{
			const CName entryName = entry->GetName();
			if ( validEntriesNames.Exist( entryName ) )
			{
				ERR_ENGINE( TXT("Animation set '%ls' contains duplicated entry for animation %ls"), 
					GetDepotPath().AsChar(), entryName.AsChar() );
			#ifndef NO_EDITOR
				m_duplicatedAnimations.PushBack( entryName );
			#endif	
			}
			else
			{
				validEntriesNames.Insert( entryName );
				validEntries.PushBack( entry );
			}
		}

		// extract
		if ( validEntries.Size() != m_animations.Size() )
		{
			m_animations.ClearFast();

			for ( auto it = validEntries.Begin(); it != validEntries.End(); ++it )
			{
				m_animations.PushBack( *it );
			}
		}
	}

	// Bind all animation entries to this animation set 
	// This fixes the pointers to match the old hierarchy (Set -> SetEntry -> SkeletalAnimation)
	// This is a legacy code that should be refactored and removed
	for ( CSkeletalAnimationSetEntry* entry : m_animations )
	{
		entry->Bind( this );
	}
}

#ifndef NO_EDITOR
void CSkeletalAnimationSet::SetBitwiseCompressionPreset(SAnimationBufferBitwiseCompressionPreset preset)
{
	m_bitwiseCompressionPreset = preset;
	m_bitwiseCompressionSettings.UsePreset( m_bitwiseCompressionPreset );
}
#endif

void CSkeletalAnimationSet::OnPropertyPostChange( IProperty* prop )
{
	TBaseClass::OnPropertyPostChange( prop );
#ifndef NO_EDITOR
	Bool changeBitwiseCompressionSettings = false;
	if ( prop->GetName() == TXT("bitwiseCompressionPreset") )
	{
		changeBitwiseCompressionSettings = true;
		m_bitwiseCompressionSettings.UsePreset( m_bitwiseCompressionPreset );
	}
	if ( prop->GetName() == TXT("bitwiseCompressionSettings") )
	{
		changeBitwiseCompressionSettings = true;
		m_bitwiseCompressionPreset = ABBCP_Custom;
	}
	if ( prop->GetName() == TXT("Streaming option") ||
		 prop->GetName() == TXT("Number of non-streamable bones") ||
		 prop->GetName() == TXT("bitwiseCompressionPreset") ||
		 prop->GetName() == TXT("bitwiseCompressionSettings") )
	{
		if ( GFeedback->AskYesNo( TXT("Recompress animation set now?") ) )
		{
			SAnimationBufferStreamingOption streamingOption;
			Uint32 nonStreamableBones;
			GetStreamingOption(streamingOption, nonStreamableBones);
			GFeedback->BeginTask( TXT("Recompressing animations"), true );

			Uint32 index = 0;
			for ( CSkeletalAnimationSetEntry* entry : m_animations )
			{
				if ( entry->GetAnimation() )
				{
					GFeedback->UpdateTaskInfo( TXT("Updating '%ls'..."), entry->GetName().AsChar() );
					GFeedback->UpdateTaskProgress( index, m_animations.Size() );
					// will always recompress
					if ( changeBitwiseCompressionSettings && ! entry->GetAnimation()->HasOwnBitwiseCompressionParams() )
					{
						entry->GetAnimation()->SetBitwiseCompressionPresetAndSettings( m_bitwiseCompressionPreset, m_bitwiseCompressionSettings, false );
					}
					entry->GetAnimation()->SetStreamingOption( streamingOption, nonStreamableBones );
				}
				if ( GFeedback->IsTaskCanceled() )
				{
					break;
				}
				++index;
			}
			GFeedback->EndTask();
			LogMemoryStats();
		}
	}
#endif
}

void CSkeletalAnimationSet::GetStreamingOption( SAnimationBufferStreamingOption & outStreamingOption, Uint32 & outNonStreamableBones ) const
{
	outStreamingOption = m_streamingOption;
	outNonStreamableBones = m_nonStreamableBones != 0? m_nonStreamableBones : (Uint32)-1;
	if ( outNonStreamableBones == (Uint32)-1 )
	{
		if ( m_skeleton )
		{
			outNonStreamableBones = (Uint32) m_skeleton->GetFirstStreamableBone();
		}
	}
	if ( outNonStreamableBones == (Uint32)-1 )
	{
		outNonStreamableBones = 35;
	}
}

void CSkeletalAnimationSet::CleanupSourceData()
{
	TBaseClass::CleanupSourceData();

#ifdef USE_EXT_ANIM_EVENTS
	ClearExternalEvents( m_extAnimEvents );
#endif

}

void CSkeletalAnimationSet::ClearExternalEvents( TDynArray< THandle< CExtAnimEventsFile > >& extAnimEvents )
{
	// Iterate through all animation set entries, find external events associated with given animation and move them to animation set entry
	THashSet< CExtAnimEvent* > usedEvents;
	for( CSkeletalAnimationSetEntry* entry : m_animations )
	{
		Bool hasRequiredSfxTag = false;
		TDynArray< THandle< CExtAnimEventsFile > >::const_iterator fileIter;
		for( fileIter = extAnimEvents.Begin(); fileIter != extAnimEvents.End(); ++fileIter )
		{
			//from all external animation events files we need to merge every event without sfx tag to one animation set like it happened before
			if( CExtAnimEventsFile* file = (*fileIter).Get() )
			{
				if( file->GetRequiredSfxTag() != CName::NONE )
				{
					hasRequiredSfxTag = true;
					continue;
				}

				TDynArray< CExtAnimEvent* > externalEventsForAnimation;

				// Get events for given animation
				file->GetEventsForAnimation( entry->GetName(), externalEventsForAnimation );

				for ( CExtAnimEvent* evt : externalEventsForAnimation )
				{	
					if ( usedEvents.Exist( evt ) )
					{
						ERR_ENGINE( TXT("Event object '%ls' in animation '%ls' already used in different animation. WTF? AnimSet: %ls"),
							evt->GetEventName().AsChar(), entry->GetName().AsChar(), GetDepotPath().AsChar() );
					}
					else
					{
						entry->AddEvent( evt );
						usedEvents.Insert( evt );
					}
				}
			}
		}

		if( hasRequiredSfxTag )
		{
			//now we need to add to animation set those events which have sfx tag to have those placed as groups at end of events array
			for( fileIter = extAnimEvents.Begin(); fileIter != extAnimEvents.End(); ++fileIter )
			{
				if( CExtAnimEventsFile* file = (*fileIter).Get() )
				{

					if( file->GetRequiredSfxTag() == CName::NONE )
					{
						continue;
					}

					TDynArray< CExtAnimEvent* > externalEventsForAnimation;

					// Get events for given animation
					file->GetEventsForAnimation( entry->GetName(), externalEventsForAnimation );

					// filter out events that were used twice
					TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter > eventsToMove;
					eventsToMove.Reserve( externalEventsForAnimation.Size() );
					for ( CExtAnimEvent* evt : externalEventsForAnimation )
					{	
						if ( usedEvents.Exist( evt ) )
						{
							ERR_ENGINE( TXT("Event object '%ls' in animation '%ls' already used in different animation. WTF? AnimSet: %ls"),
								evt->GetEventName().AsChar(), entry->GetName().AsChar(), GetDepotPath().AsChar() );
						}
						else
						{
							eventsToMove.Insert( evt );
							usedEvents.Insert( evt );
						}
					}

					entry->AddEventGroup( file->GetRequiredSfxTag(), eventsToMove );
				}
			}
		}
	}
}

void CSkeletalAnimationSet::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

#ifndef NO_EDITOR
	if ( file.IsGarbageCollector() )
	{
		for ( CSkeletalAnimationSetEntry* entry : m_animations )
		{ 
			if ( CSkeletalAnimation* anim = entry->GetAnimation() )
			{
				THandle< CSkeleton > skel = anim->GetSkeleton();
				file << skel;
			}
		}
	}
#endif

	if( file.IsCooker() && file.IsWriter() ) 
	{
#ifdef USE_EXT_ANIM_EVENTS
		// Iterate through all external event files and clear their data
		TDynArray< THandle< CExtAnimEventsFile > >::const_iterator fileIter;
		for( fileIter = m_extAnimEvents.Begin(); fileIter != m_extAnimEvents.End(); ++fileIter )
		{
			CExtAnimEventsFile* file = (*fileIter).Get();

			// Skip broken entries
			if( file == nullptr )
			{
				continue;
			}

			file->CleanupSourceData();
		}

		m_extAnimEvents.Clear();
	}
#endif
}

Uint32 CSkeletalAnimationSet::GetAnimations( TDynArray< CSkeletalAnimationSetEntry* >& animations ) const
{
	animations = m_animations;
	return animations.Size();
}

CSkeletalAnimationSetEntry* CSkeletalAnimationSet::FindAnimation( const CName& name )
{
	return CAnimationMap::GetInstance()->FindAnimation( name, this );
}

const CSkeletalAnimationSetEntry* CSkeletalAnimationSet::FindAnimation( const CName& name ) const
{
	return CAnimationMap::GetInstance()->FindAnimation( name, this );
}

CSkeletalAnimationSetEntry* CSkeletalAnimationSet::FindAnimation( CSkeletalAnimation *anim )
{
	return CAnimationMap::GetInstance()->FindAnimation( anim, this );
}

void CSkeletalAnimationSet::GetAdditionalInfo( TDynArray< String >& info ) const
{
	info.PushBack( String::Printf( TXT("%i animation(s)"), m_animations.Size() ) );
}

#ifndef NO_EDITOR

Bool CSkeletalAnimationSet::AddAnimation( CSkeletalAnimation *anim )
{
	// Add to normal set
	if ( MarkModified() )
	{
		// Do not add if already exists
		if ( !FindAnimation( anim ) )
		{
			// Create set entry
			CSkeletalAnimationSetEntry* entry = new CSkeletalAnimationSetEntry();
			entry->SetAnimation( anim );
			entry->Bind( this );

			// Add to animation list
			m_animations.PushBack( entry );

			CAnimationMap::GetInstance()->AddAnimationSetEntry( entry );
			return true;
		}
	}

	// Not added
	return false;
}

Bool CSkeletalAnimationSet::ChangeAnimation( CSkeletalAnimation *animOld, CSkeletalAnimation *animNew )
{
	// We cannot directly add animations to self-contained set
	if ( IsSelfContained() )
	{
		GFeedback->ShowWarn( TXT("Cannot directly add animations to self-contained animation set") );
		return false;
	}

	// Add to normal set
	if ( MarkModified() )
	{
		// Old animation should exist
		CSkeletalAnimationSetEntry* oldAnimEntry = FindAnimation( animOld );
		if ( oldAnimEntry  )
		{
			if ( !FindAnimation( animNew ) ) // prevent from duplication
			{
				CAnimationMap::GetInstance()->RemoveAnimationSetEntry( oldAnimEntry );

				// Create new
				CSkeletalAnimationSetEntry *entry = new CSkeletalAnimationSetEntry();
				entry->SetAnimation( animNew );
				entry->Bind( this );

				// Connect events to new animation
				CopyEventsBetweenAnimations( entry, oldAnimEntry );

				// Push in proper place
				const Int32 index = static_cast< Int32 >( m_animations.GetIndex( oldAnimEntry ) );
				m_animations.Insert( index, entry );

				CAnimationMap::GetInstance()->AddAnimationSetEntry( entry );

				// Remove old animation
				RemoveAnimation( animOld );
				return true;
			}
		}
	}

	// Not found
	return false;
}

Bool CSkeletalAnimationSet::RemoveAnimation( CSkeletalAnimation *anim )
{
	if ( MarkModified() )
	{
		if ( CSkeletalAnimationSetEntry* entry = FindAnimation( anim ) )
		{
			CAnimationMap::GetInstance()->RemoveAnimationSetEntry( entry );
			delete entry;
			return m_animations.Remove( entry );
		}
	}

	// Not found
	return false;
}

Bool CSkeletalAnimationSet::RemoveAnimation( CSkeletalAnimationSetEntry *anim )
{
	if ( anim )
	{
		CAnimationMap::GetInstance()->RemoveAnimationSetEntry( anim );
		delete anim;
		return m_animations.Remove( anim );
	}
	return false;
}

Bool CSkeletalAnimationSet::RemoveEmptyAnimations()
{
	Bool ret = false;
	Bool tryModify = false;
	
 	// Remove empty slots
	for ( Int32 i=(Int32)m_animations.Size()-1; i>=0; i-- )
	{
		CSkeletalAnimationSetEntry* entry = m_animations[ i ];

		if ( !entry->GetAnimation() )
		{
			if ( !tryModify )
			{
				if ( !MarkModified() )
				{
					return false;
				}

				tryModify = true;
			}

			CAnimationMap::GetInstance()->RemoveAnimationSetEntry( entry );
			m_animations.Erase( m_animations.Begin() + i );
			delete entry;
			ret = true;
		}
	}

	// Done
	return ret;
}

#endif

#ifndef NO_RESOURCE_IMPORT

namespace
{
	Bool IsAnimationFrameCorrupted( Float time, TDynArray< AnimQsTransform >& bones, TDynArray< Float >& tracks, const CSkeletalAnimation* anim )
	{
		const Uint32 numBones = bones.Size();
		const Uint32 numTracks = tracks.Size();

		const Bool ret = anim->Sample( time, bones, tracks );

		// Check ret value
		if ( !ret )
		{
			return true;
		}

		// Bones
		for ( Uint32 i=0; i<numBones; ++i )
		{
			const AnimQsTransform& trans = bones[ i ];

			const Bool isOk = trans.Translation.IsOk() && trans.Rotation.Quat.IsOk() && trans.Scale.IsOk();
			if ( !isOk )
			{
				return true;
			}
		}

		// Tracks
		for ( Uint32 i=0; i<numTracks; ++i )
		{
			const Float value = tracks[ i ];

			if ( !Red::Math::NumericalUtils::IsFinite( value ) )
			{
				return true;
			}
		}

		return false;
	}

	Bool IsAnimationCorrupted( const CSkeletalAnimation* anim )
	{
		const Uint32 numBones = anim->GetBonesNum();
		const Uint32 numTracks = anim->GetTracksNum();
		const Float duration = anim->GetDuration();
		const Float timeStep = 1.f/60.f;

		TDynArray< AnimQsTransform > bones;
		TDynArray< Float > tracks;

		bones.Resize( numBones );
		tracks.Resize( numTracks );

		for ( Float time=0.f; time<duration; time+=timeStep )
		{
			if ( IsAnimationFrameCorrupted( time, bones, tracks, anim ) )
			{
				return true;
			}
		}

		if ( IsAnimationFrameCorrupted( duration, bones, tracks, anim ) )
		{
			return true;
		}

		return false;
	}
}

Bool CSkeletalAnimationSet::CheckAllAnimations( TDynArray< CName >& outCorruptedAnimations, IFeedbackSystem* f ) const
{
	if ( f )
	{
		f->BeginTask( TXT("Checking animations..."), true );
	}

	const Uint32 numAnims = m_animations.Size();
	for ( Uint32 i=0; i<numAnims; ++i )
	{
		if ( f )
		{
			f->UpdateTaskProgress( i, numAnims );
		}

		const CSkeletalAnimationSetEntry* entry = m_animations[ i ];
		if ( entry )
		{
			if ( CSkeletalAnimation* anim = entry->GetAnimation() )
			{
				if ( !anim->IsFullyLoaded() )
				{
					anim->SyncLoad();
				}

				if ( IsAnimationCorrupted( anim ) )
				{
					outCorruptedAnimations.PushBack( anim->GetName() );
				}
			}
		}
	}

	if ( f )
	{
		f->EndTask();
	}

	return outCorruptedAnimations.Size() == 0;
}

Bool CSkeletalAnimationSet::ImportAnimation( const ImportAnimationOptions& importOptions )
{
	const String& animationFile = importOptions.m_animationFile;
	CName animationName = importOptions.m_animationName.Empty() ? CName( CFilePath( animationFile ).GetFileName() ) : importOptions.m_animationName;
	IFeedbackSystem* feedback = importOptions.m_feedback;

	// We cannot directly add animations to non self-contained set
	if ( !IsSelfContained() )
	{
		feedback->ShowWarn( TXT("Cannot directly add animations to external animation set") );
		return false;
	}

	// Import new animation
	MarkModified();

	{
		// Find animation importer
		CFilePath filePath( animationFile );
		ISkeletalAnimationImporter* animImporter = ISkeletalAnimationImporter::FindImporter( filePath.GetExtension() );
		if ( !animImporter )
		{
			feedback->ShowError( TXT("No animation importer to import '%ls' found"), animationFile.AsChar() );
			return false;
		}

		// Get existing entry
		CSkeletalAnimationSetEntry* existingEntry = FindAnimation( animationName );

		// Params
		AnimImporterParams params;
		params.m_animationSet = this;
		params.m_filePath = animationFile;
		params.m_existingAnimationEntry = existingEntry;
		params.m_existingAnimation = existingEntry ? existingEntry->GetAnimation() : NULL;
		params.m_type = importOptions.m_type;
		params.m_additiveType = importOptions.m_addType;
		params.m_addAnimationToExtractedFile = importOptions.m_addAnimationToExtractedFile;
		params.m_maxPartFrames = importOptions.m_maxPartFrames;
		params.m_extractMotion = importOptions.m_extractMotion;
		params.m_extractTrajectory = importOptions.m_extractTrajectory;
		params.m_resetRoot = importOptions.m_resetRoot;
		params.m_preferBetterQuality = importOptions.m_preferBetterQuality;

		if ( ! importOptions.m_overrideAnimType && existingEntry && existingEntry->GetAnimation() )
		{
			params.m_type = existingEntry->GetAnimation()->GetAnimationType();
			params.m_additiveType = existingEntry->GetAnimation()->GetAdditiveType();
			params.m_addAnimationToExtractedFile = existingEntry->GetAnimation()->GetAddAnimationToExtractedFile();
		}

		if ( m_skeleton )
		{
			params.m_tPoseTransform = m_skeleton->GetReferencePoseLS();
			params.m_tPoseTronsformNum = m_skeleton->GetBonesNum();

			params.m_skeleton = m_skeleton;
		}

		// Import animation
		CSkeletalAnimation* animation = animImporter->DoImport( params );
		if ( !animation )
		{
			GFeedback->ShowError( TXT("No animation imported from '%ls' found"), animationFile.AsChar() );
			return false;
		}

		if ( IsAnimationCorrupted( animation ) )
		{
			GFeedback->ShowError( TXT("Animation is corrupted. Please DEBUG") );
			return false;
		}

		// Update animation name
		animation->SetName( animationName );

		// Create new animation set entry (if we were not reimporting)
		if ( !existingEntry )
		{
			existingEntry = new CSkeletalAnimationSetEntry;
			existingEntry->SetAnimation( animation );
			existingEntry->Bind( this );
			CAnimationMap::GetInstance()->AddAnimationSetEntry( existingEntry );

			m_animations.PushBack( existingEntry );
		}

		// Imported
		ASSERT( existingEntry->GetAnimation() == animation );
		ASSERT( existingEntry->GetAnimation() != nullptr );
		ASSERT( animation->GetEntry() == existingEntry );
		return true;
	}

	// Not imported
	return false;
}

Bool CSkeletalAnimationSet::ReimportAnimation( const ImportAnimationOptions& options )
{
#if !defined(NO_EDITOR) && !defined(NO_RESOURCE_IMPORT)
	CSkeletalAnimationSetEntry* entry = options.m_entry;
	ESkeletalAnimationType animType = options.m_type;
	EAdditiveType addType = options.m_addType;
	String addAnimationToExtractedFile = options.m_addAnimationToExtractedFile;
	IFeedbackSystem* feedback = options.m_feedback;

	// No animation, no import source
	if ( !entry || !entry->GetAnimation() )
	{
		feedback->ShowError( TXT("No source to import animation in '%ls'"), GetFriendlyName().AsChar() );
		return false;
	}

	// External animation
	if ( entry->GetAnimation()->GetEntry() != entry )
	{
		feedback->ShowError( TXT("Animation '%ls' is external. Unable to reimport from animset"), entry->GetAnimation()->GetName().AsString().AsChar() );
		return false;
	}

	// No source
	if ( entry->GetAnimation()->GetImportFile().Empty() )
	{
		feedback->ShowError( TXT("Animation '%ls' has no import file."), entry->GetAnimation()->GetName().AsString().AsChar() );
		return false;
	}

	// Invalid import file
	if ( GFileManager->GetFileSize( entry->GetAnimation()->GetImportFile() ) == 0 )
	{
		feedback->ShowError( TXT("Animation '%ls' has missing import file '%ls'."), entry->GetAnimation()->GetName().AsString().AsChar(), entry->GetAnimation()->GetImportFile().AsChar() );
		return false;
	}

	if ( ! options.m_overrideAnimType ) 
	{
		animType = entry->GetAnimation()->GetAnimationType();
		addType = entry->GetAnimation()->GetAdditiveType();
		addAnimationToExtractedFile = entry->GetAnimation()->GetAddAnimationToExtractedFile();
	}

	// Find animation importer
	String animationFile = entry->GetAnimation()->GetImportFile();

	ImportAnimationOptions importOptions;
	importOptions.m_type = animType;
	importOptions.m_addType = addType;
	importOptions.m_animationName = entry->GetAnimation()->GetName();
	importOptions.m_addAnimationToExtractedFile = addAnimationToExtractedFile;
	importOptions.m_feedback = feedback;
	importOptions.m_animationFile = animationFile;

	return ImportAnimation( importOptions );
#else
	RED_HALT( "Cannot re-import animations" );
	return false;
#endif
}

Bool CSkeletalAnimationSet::RenameAnimation( CSkeletalAnimationSetEntry* entry, const CName& name )
{
	// No animation, no import source
	if ( !entry || !entry->GetAnimation() )
	{
		GFeedback->ShowError( TXT("No source to rename animation in '%ls'"), GetFriendlyName().AsChar() );
		return false;
	}

	// External animation
	if ( entry->GetAnimation()->GetEntry() != entry )
	{
		GFeedback->ShowError( TXT("Animation '%ls' is external. Unable to rename from animset"), entry->GetAnimation()->GetName().AsString().AsChar() );
		return false;
	}

	// Prevent from duplication
	CSkeletalAnimationSetEntry* existingEntry = FindAnimation( name );
	if ( existingEntry && existingEntry != entry )
	{
		GFeedback->ShowError( TXT("Animation '%ls' already exists"), entry->GetAnimation()->GetName().AsString().AsChar() );
		return false;
	}

	if ( MarkModified() )
	{
		CName prevName = entry->GetAnimation()->GetName();
		Bool extChanged = false;
		for ( auto iExt = m_extAnimEvents.Begin(); iExt != m_extAnimEvents.End(); ++ iExt )
		{
			if ( CExtAnimEventsFile* ext = (*iExt).Get() )
			{
			if ( ext->RenameAnimationInAllEvents( prevName, name ) )
			{
				if ( ext->MarkModified() )
				{
					ext->Save();
					extChanged = true;
				}
			}
			}
		}
		CAnimationMap::GetInstance()->RemoveAnimationSetEntry( entry );
		entry->GetAnimation()->SetName( name );
		CAnimationMap::GetInstance()->AddAnimationSetEntry( entry );
		if ( extChanged )
		{
			Save();
		}
		return true;
	}
	
	return false;
}

#endif

#ifndef NO_RESOURCE_IMPORT

void CSkeletalAnimationSet::AddEventsFile( CExtAnimEventsFile* eventsFile )
{ 
	RED_UNUSED( eventsFile );
#ifdef USE_EXT_ANIM_EVENTS
	m_extAnimEvents.PushBackUnique( eventsFile );

	MarkModified();
#endif
}

void CSkeletalAnimationSet::DeleteEventsFile( CExtAnimEventsFile* eventsFile )
{ 
	RED_UNUSED( eventsFile );
#ifdef USE_EXT_ANIM_EVENTS
	m_extAnimEvents.RemoveFast( eventsFile );

	MarkModified();
#endif
}

void CSkeletalAnimationSet::OnAnimsetPreSaved()
{
	const Uint32 size = m_animations.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CSkeletalAnimationSetEntry* entry = m_animations[ i ];
		if ( entry )
		{
			// make sure we have valid binding to animation set and animation
			entry->Bind( this );

			// process animation
			CSkeletalAnimation* anim = entry->GetAnimation();
			if ( anim )
			{
				if ( !anim->HasCompressedPose() || !anim->HasBoundingBox() || !anim->HasCompressedMotionExtraction() )
				{
					if ( !MarkModified() )
					{
						return;
					}

					if ( m_skeleton )
					{
						// 1. Bounding box
						if ( !anim->HasBoundingBox() )
						{
							anim->CreateBBox( m_skeleton.Get() );
						}
					}

					// 2. Motion extraction
					if ( !anim->HasCompressedMotionExtraction() )
					{
						anim->CreateCompressedMotionExtraction();
					}
				}
			}
		}
	}
}

#endif

void CSkeletalAnimationSet::CopyEventsBetweenAnimations( CSkeletalAnimationSetEntry* dest, CSkeletalAnimationSetEntry* src )
{
	HALT( "Not implemented! Contact mcinek" );
}

#ifdef USE_EXT_ANIM_EVENTS

void CSkeletalAnimationSet::GetExternalEventsByTime( const CSkeletalAnimationSetEntry* animation, Float prevTime, Float currTime, Int32 numLoops, Float alpha, TDynArray< CAnimationEventFired >* events, SBehaviorGraphOutput* output, const CName& tag ) const
{
	// Get events from external files
	for( TDynArray< THandle< CExtAnimEventsFile > >::const_iterator fileIter = m_extAnimEvents.Begin();
		fileIter != m_extAnimEvents.End(); ++fileIter )
	{
		const CExtAnimEventsFile* file = (*fileIter).Get();

		// Skip broken entries
		if( file == NULL )
		{
			continue;
		}

		file->GetEventsByTime( animation, prevTime, currTime, numLoops, alpha, events, output, tag );
	}
}

#endif

Uint32 CSkeletalAnimationSet::GetNumCompressedPoses() const
{
	return m_compressedPoses.Size();
}

const ICompressedPose* CSkeletalAnimationSet::GetCompressedPose( CompressedPoseHandle handle ) const
{
	return m_compressedPoses[ handle ];
}

#ifndef NO_EDITOR

const ICompressedPose* CSkeletalAnimationSet::FindCompressedPoseByName( const CName& name ) const
{
	ASSERT( m_compressedPoses.Size() == m_compressedPoseInfos.Size() );

	const Uint32 size = m_compressedPoses.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const ICompressedPose* pose = m_compressedPoses[ i ];
		const CName& poseName = m_compressedPoseInfos[ i ].m_name;

		if ( poseName == name )
		{
			return pose;
		}
	}

	return NULL;
}

const SCompressedPoseInfo* CSkeletalAnimationSet::FindCompressedPoseInfo( const CName& name ) const
{
	ASSERT( m_compressedPoses.Size() == m_compressedPoseInfos.Size() );

	const Uint32 size = m_compressedPoseInfos.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CName& poseName = m_compressedPoseInfos[ i ].m_name;
		if ( poseName == name )
		{
			return &(m_compressedPoseInfos[ i ]);
		}
	}

	return NULL;
}

const CName& CSkeletalAnimationSet::FindCompressedPoseName( CompressedPoseHandle handle ) const
{
	ASSERT( m_compressedPoses.Size() == m_compressedPoseInfos.Size() );
	return m_compressedPoseInfos[ handle ].m_name;
}

Int32 CSkeletalAnimationSet::FindCompressedPoseHandle( const CName& name ) const
{
	ASSERT( m_compressedPoses.Size() == m_compressedPoseInfos.Size() );

	const Uint32 size = m_compressedPoseInfos.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CName& poseName = m_compressedPoseInfos[ i ].m_name;
		if ( poseName == name )
		{
			return i;
		}
	}

	return -1;
}

Bool CSkeletalAnimationSet::AddCompressedPose( const CName& poseName, const CName& animation, Float time, IFeedbackSystem* sys )
{
	if ( !m_skeleton )
	{
		return false;
	}

	if ( FindCompressedPoseHandle( poseName ) != -1 )
	{
		return false;
	}

	CSkeletalAnimationSetEntry* animEntry = FindAnimation( animation );
	if ( animEntry && animEntry->GetAnimation() )
	{
		CSkeletalAnimation* anim = animEntry->GetAnimation();

		if ( time > anim->GetDuration() )
		{
			return false;
		}

		ICompressedPose* pose = anim->CreateCompressedPose( this, m_skeleton.Get(), time );
		if ( pose )
		{
			CompressedPoseHandle handle = static_cast< CompressedPoseHandle >( m_compressedPoses.Grow( 1 ) );
			CompressedPoseHandle handle2 = static_cast< CompressedPoseHandle >( m_compressedPoseInfos.Grow( 1 ) );

			ASSERT( handle == handle2 );

			m_compressedPoses[ handle ] = pose;
			
			SCompressedPoseInfo& info = m_compressedPoseInfos[ handle ];
			info.m_name = poseName;
			info.m_animation = animation;
			info.m_time = time;

			return true;
		}
	}

	return false;
}

Bool CSkeletalAnimationSet::RemoveCompressedPose( const CName& poseName, IFeedbackSystem* sys )
{
	CompressedPoseHandle handle = FindCompressedPoseHandle( poseName );
	if ( handle != -1 )
	{
		Uint32 place = (Uint32)handle;
		m_compressedPoses.RemoveAt( place );
		m_compressedPoseInfos.RemoveAt( place );
		return true;
	}

	return false;
}

void CSkeletalAnimationSet::RemoveCompressedPosesWithAnimation( const CName& aniamtion )
{
	for ( Int32 i=m_compressedPoses.SizeInt()-1; i>=0; --i )
	{
		SCompressedPoseInfo& info = m_compressedPoseInfos[ i ];

		if ( info.m_animation == aniamtion )
		{
			m_compressedPoses.RemoveAt( i );
			m_compressedPoseInfos.RemoveAt( i );
		}
	}
}

Bool CSkeletalAnimationSet::RecreateCompressedPose( const CName& poseName, IFeedbackSystem* sys )
{
	if ( !m_skeleton )
	{
		if ( sys )
		{
			sys->ShowError( TXT("Animset doesn't have skeleton") );
		}
		return false;
	}

	const SCompressedPoseInfo* info = FindCompressedPoseInfo( poseName );
	if ( info )
	{
		const CName& animation = info->m_animation;

		CSkeletalAnimationSetEntry* animEntry = FindAnimation( animation );
		if ( animEntry && animEntry->GetAnimation() )
		{
			CSkeletalAnimation* anim = animEntry->GetAnimation();

			Float time = info->m_time;

			if ( time > anim->GetDuration() )
			{
				if ( sys )
				{
					sys->ShowError( TXT("Selected time is greater then anim duration") );
				}
				return false;
			}

			ICompressedPose* pose = anim->CreateCompressedPose( this, m_skeleton.Get(), time );
			if ( pose )
			{
				CompressedPoseHandle handle = FindCompressedPoseHandle( poseName );
				ASSERT( handle != -1 );

				m_compressedPoses[ handle ] = pose;

				return true;
			}
			else if ( sys )
			{
				sys->ShowError( TXT("Couldn't create compressed pose. This is bug for programmers."), poseName.AsString().AsChar() );
			}
		}
		else
		{
			RemoveCompressedPosesWithAnimation( animation );
		}
	}
	else if ( sys )
	{
		sys->ShowError( TXT("Couldn't find compressed pose with name '%ls'"), poseName.AsString().AsChar() );
	}

	return false;
}

Bool CSkeletalAnimationSet::RecreateAllCompressedPoses( IFeedbackSystem* sys )
{
	if ( !m_skeleton )
	{
		if ( sys )
		{
			sys->ShowError( TXT("Animset doesn't have skeleton") );
		}
		return false;
	}

	Bool ret = true;

	ASSERT( m_compressedPoses.Size() == m_compressedPoseInfos.Size() );

	const Int32 size = m_compressedPoseInfos.SizeInt();
	for ( Int32 i=size-1; i>=0; --i )
	{
		const SCompressedPoseInfo& info = m_compressedPoseInfos[ i ];

		const CName& poseName = info.m_name;
		const CName& animation = info.m_animation;

		CSkeletalAnimationSetEntry* animEntry = FindAnimation( animation );
		if ( animEntry && animEntry->GetAnimation() )
		{
			CSkeletalAnimation* anim = animEntry->GetAnimation();

			Float time = info.m_time;

			if ( time > anim->GetDuration() )
			{
				if ( sys )
				{
					sys->ShowError( TXT("Pose '%ls' - Selected time is greater then anim duration"), poseName.AsString().AsChar() );
				}

				ret &= false;
			}

			ICompressedPose* pose = anim->CreateCompressedPose( this, m_skeleton.Get(), time );
			if ( pose )
			{
				CompressedPoseHandle handle = FindCompressedPoseHandle( poseName );
				ASSERT( handle != -1 );

				m_compressedPoses[ handle ] = pose;
			}
			else if ( sys )
			{
				sys->ShowError( TXT("Couldn't create compressed pose '%ls'. This is bug for programmers."), poseName.AsString().AsChar() );

				ret &= false;
			}
		}
		else if ( sys )
		{
			RemoveCompressedPosesWithAnimation( animation );

			sys->ShowError( TXT("Pose '%ls' - Couldn't find animation '%ls'"), poseName.AsString().AsChar(), animation.AsString().AsChar() );
		}
	}

	return ret;
}

void CSkeletalAnimationSet::ExportCompressedPoses( SCompressedPosesData& data ) const
{
	data.m_infos = m_compressedPoseInfos;
	
	const Uint32 size = m_animations.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_animations[ i ] && m_animations[ i ]->GetAnimation() )
		{
			const CSkeletalAnimation* anim = m_animations[ i ]->GetAnimation();

			const CName& animName = anim->GetName();
			const CName& poseName = anim->GetCompressedPoseName();

			data.m_poses.PushBack( SCompressedPosesData::TAnimAndPose( animName, poseName ) );
		}
	}
}

void CSkeletalAnimationSet::ImportCompressedPoses( const SCompressedPosesData& data, IFeedbackSystem* sys )
{
	if( !CanModify() )
	{
		ASSERT( CanModify() );
		return;
	}

	// Clear old data
	m_compressedPoseInfos.Clear();
	m_compressedPoses.Clear();

	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		if ( m_animations[ i ] && m_animations[ i ]->GetAnimation() )
		{
			m_animations[ i ]->GetAnimation()->SetCompressedPoseName( CName::NONE );
		}
	}

	// Recreate poses
	m_compressedPoseInfos = data.m_infos;
	m_compressedPoses.Resize( data.m_infos.Size() );

	for ( Uint32 i=0; i<m_compressedPoses.Size(); ++i )
	{
		m_compressedPoses[ i ] = NULL;
	}

	RecreateAllCompressedPoses( NULL );

	TSet< CName > removedPoses;

	// Remove empty slots
	for ( Int32 i=m_compressedPoses.SizeInt()-1; i>=0; --i )
	{
		if ( m_compressedPoses[ i ] == NULL )
		{
			const SCompressedPoseInfo& info = m_compressedPoseInfos[ i ];

			const CName& poseName = info.m_name;
			const CName& animation = info.m_animation;

			if ( sys )
			{
				sys->ShowError( TXT("Couldn't create compressed pose '%ls' for animation '%ls', animset '%ls'"), poseName.AsString().AsChar(), animation.AsString().AsChar(), GetDepotPath().AsChar() );
			}

			removedPoses.Insert( poseName );

			m_compressedPoses.RemoveAt( i );
			m_compressedPoseInfos.RemoveAt( i );
		}
	}

	// Fill animations data
	const Uint32 size = data.m_poses.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CName& animName = data.m_poses[ i ].m_first;
		const CName& poseName = data.m_poses[ i ].m_second;

		if ( removedPoses.Find( poseName ) != removedPoses.End() && sys )
		{
			sys->ShowError( TXT("Couldn't find animation animation '%ls' for compressed pose '%ls', animset '%ls'"), animName.AsString().AsChar(), poseName.AsString().AsChar(), GetDepotPath().AsChar() );
			continue;
		}

		CSkeletalAnimationSetEntry* entry = FindAnimation( animName );
		if ( entry && entry->GetAnimation() )
		{
			CSkeletalAnimation* anim = entry->GetAnimation();

			ASSERT( anim->GetCompressedPoseName() == CName::NONE );

			anim->SetCompressedPoseName( poseName );
		}
		else if ( sys )
		{
			sys->ShowError( TXT("Couldn't find animation animation '%ls' for compressed pose '%ls', animset '%ls'"), animName.AsString().AsChar(), poseName.AsString().AsChar(), GetDepotPath().AsChar() );
		}
	}
}

#endif

Uint32 CSkeletalAnimationSet::GetNumExternalEventsOfType( const CName& animName, const CClass* c ) const
{
	Uint32 num = 0;

	for ( TDynArray< THandle< CExtAnimEventsFile > >::const_iterator fileIter = m_extAnimEvents.Begin(); fileIter != m_extAnimEvents.End(); ++fileIter )
	{
		if ( const CExtAnimEventsFile* file = (*fileIter).Get()  )
		{
			num += file->GetNumEventsOfType( animName, c );
		}
	}

	return num;
}

void CSkeletalAnimationSet::LogMemoryStats()
{
	RED_LOG( AnimationCompression, TXT("") );
	RED_LOG( AnimationCompression, TXT("Animset \"%s\" stats:"), GetDepotPath().AsChar() );

	AnimMemStats stats;
	stats.m_animBufferNonStreamable = 0;
	stats.m_animBufferStreamableWhole = 0;
	stats.m_animSource = 0;
	for ( Uint32 i=0; i<m_animations.Size(); i++ )
	{
		if ( m_animations[i] )
		{
			CSkeletalAnimationSetEntry* entry = m_animations[i];

			if ( entry->GetAnimation() )
			{
				AnimMemStats animStats;
				entry->GetAnimation()->GetMemStats( animStats );
				stats.m_animBufferNonStreamable += animStats.m_animBufferNonStreamable;
				stats.m_animBufferStreamableWhole += animStats.m_animBufferStreamableWhole;
				stats.m_animSource += animStats.m_animSource;
			}
		}
	}

	Uint32 totalSize = stats.m_animBufferNonStreamable + stats.m_animBufferStreamableWhole;
	RED_LOG( AnimationCompression, TXT("  Source size                       : %10i bytes"), stats.m_animSource );
	RED_LOG( AnimationCompression, TXT("  Compressed                        : %10i bytes"), totalSize );
	RED_LOG( AnimationCompression, TXT("  Compression ratio                 : %10.4f %%"), 100.0f * ( stats.m_animSource != 0? (Float)(totalSize) / (Float)stats.m_animSource : 0.0f ) );
	RED_LOG( AnimationCompression, TXT("  Non streamable (always loaded)    : %10i bytes"), stats.m_animBufferNonStreamable );
	RED_LOG( AnimationCompression, TXT("  Streamable                        : %10i bytes"), stats.m_animBufferStreamableWhole );
	RED_LOG( AnimationCompression, TXT("  Streaming ratio                   : %10.4f %%"), 100.0f * ( totalSize != 0? (Float)(stats.m_animBufferStreamableWhole) / (Float)totalSize : 0.0f ) );
	RED_LOG( AnimationCompression, TXT(""));
}
