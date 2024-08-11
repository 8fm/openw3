/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "animationReporterRecords.h"
#include "animationReporterNodes.h"
#include "animationReporterTodoList.h"
#include "smartLayout.h"
#include "animationReporterSound.h"

typedef TPair< String, String > TJobAnimset;

class CEdAnimationReporterWindow : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE();

	enum EReporterView
	{
		RV_EntityList,
		RV_ApList,
		RV_AnimsetList,
		RV_BehaviorList,
		RV_EntityData,
		RV_AnimsetData,
		RV_BehaviorData,
		RV_ExternalAnims
	};

	enum EAnimsetViewSort
	{
		AVS_AnimSize,
		AVS_AnimNum,
		AVS_AnimUsed,
		AVS_AnimUnused,
		AVS_Names,
		AVS_AnimWithoutPose,
		AVS_AnimWithoutBox,
		AVS_AnimInvalidPoses,
		AVS_AnimInvalid,
	};

	enum EAnimationViewSort
	{
		ANVS_Name,
		ANVS_Used,
		ANVS_AnimSize,
		ANVS_MotionExSize,
		ANVS_MotionExNum,
		ANVS_Duration,
	};

	enum EBehaviorViewSort
	{
		BVS_Size,
		BVS_BlockNums,
		BVS_AnimNums,
		BVS_Names,
	};

	enum EEntityViewSort
	{
		EVS_Name,
		EVS_Animset,
		EVS_Behavior,
	};

	enum EExternalAnimViewSort
	{
		EAVS_Name,
		EAVS_Animset,
		EAVS_Owner,
	};

	enum ECachedClass
	{
		CC_CReactionPlaySlotAnimation,
		CC_CReactionMoveToWaypointPlayAnimations,
		CC_CPushedReaction,
		CC_CBTTaskPlaySlotAnimation,
		CC_CActorLatentActionPlayAnimation,
		CC_Last,
	};

public:
	struct SExternalAnim
	{
		String	m_animset;
		String	m_animation;
		String	m_owner;
		String	m_desc;
		Bool	m_any;

		SExternalAnim() : m_any( true ) {}

		void Serialize( IFile& file )
		{
			file << m_animset;
			file << m_animation;
			file << m_owner;
			file << m_desc;
			file << m_any;
		}

		/*static int CmpFuncByName( const void* elem0, const void* elem1 )
		{
			const SExternalAnim* record0 = *(const SExternalAnim**)elem0;
			const SExternalAnim* record1 = *(const SExternalAnim**)elem1;
			if ( record0->m_size < record1->m_size ) return 1;
			if ( record0->m_size > record1->m_size ) return -1;
			return 0;
		}

		static int CmpFuncByOwner( const void* elem0, const void* elem1 )
		{
			const SExternalAnim* record0 = *(const SExternalAnim**)elem0;
			const SExternalAnim* record1 = *(const SExternalAnim**)elem1;
			if ( record0->m_blockNums < record1->m_blockNums ) return 1;
			if ( record0->m_blockNums > record1->m_blockNums ) return -1;
			return 0;
		}*/
	};
private:

	typedef TPair< String, TDynArray< SAnimReportSound* >* > TSoundTabPair;

	Bool									m_isUnusedAnimationRemovalTaskCancelled;

protected:
	TDynArray< String >						m_cookList;

	TDynArray< ACNode >						m_acNodes;
	TDynArray< APNode >						m_apNodes;

	TDynArray< SAnimReportSound* >			m_usedSoundEvents;
	TDynArray< SAnimReportSound* >			m_unusedSoundEvents;
	TDynArray< SAnimReportSound* >			m_depotSoundEvents;
	TDynArray< SAnimReportSound* >			m_tocheckSoundEvents;
	TDynArray< SAnimReportSound* >			m_allSoundEvents;

	TDynArray< EdAnimReportAnimset* >		m_animsetRecords;
	TDynArray< EdAnimReportAnimEvents* >	m_animEventsRecords;
	TDynArray< EdAnimReportBehavior* >		m_behaviorRecords;
	TDynArray< EdAnimReportJobTree* >		m_jobRecords;

	TDynArray< SExternalAnim >				m_externalAnims;

	TDynArray< CName >						m_doNotRemoveAnimationList;

	EdAnimReportAnimset*					m_selectedAnimset;

	EAnimsetViewSort						m_animsetSort;
	EBehaviorViewSort						m_behaviorSort;
	EAnimationViewSort						m_animationSort;
	EEntityViewSort							m_entitiesSort;
	EExternalAnimViewSort					m_externalAnimSort;
	ETODOViewSort							m_todoSort;

	Bool									m_apShowAllOwners;
	Bool									m_acShowAllOwners;
	Bool									m_soundShowDep;
	TDynArray< TSoundTabPair >				m_soundTabsToDiff;

	TDynArray< CEntityTemplate* >			m_preloadedTemplates;
	TDynArray< CClass* >					m_cachedClasses;

	EdAnimationReporterTodoList				m_todoList;

public:
	CEdAnimationReporterWindow( wxWindow* parent );
	~CEdAnimationReporterWindow();

	void Save();
	void Load();

protected: // Logic
	void Serialize( IFile& file );

	void CacheClasses();
	void LoadPreloadedTemplates();
	void UnloadPreloadedTemplates();
	void FillSoundDiffTab();

	void ReloadReportFile( const String& path );
	void ParseAllUsedResourceList();

	void ClearAllRecords();
	void ParseQuests();
	void ParseScripts();
	void ParseBehaviorTrees();
	void ParseEntityTemplates();
	void ParseTemplParams( const CEntityTemplate* templ );

	void FindDependences();
	void FillDependences( ACNode& node, EdAnimationReporterTodoList& todo );
	void FillDependences( APNode& node, EdAnimationReporterTodoList& todo );

	EdAnimReportAnimset* AddAnimset( const String& path );
	void AddExternalAnimFromProperty( CObject* obj, const CName& propName, const String& owner, const String& desc );
	void AddExternalAnimFromArrayProperty( CObject* obj, const CName& arrayPropName, const String& owner, const String& desc );
	void AddExternalAnimsToList();

	void CollectAnimationsFromScript( const String& absFilePath );

	void GenerateSoundReport();
	SAnimReportSound* FindSoundEventReportByName( const TDynArray< SAnimReportSound* >& list, const String& eventName ) const;
	SAnimReportSound::SParentAnimation* FindSoundEventDep( SAnimReportSound* sound, const CName& animation, const String& animset ) const;
	void FillSoundEventList( TDynArray< SAnimReportSound* >& list, CSkeletalAnimationSet* set, const String& localPath ) const;
	void FillSoundEventList( TDynArray< SAnimReportSound* >& list, EdAnimReportAnimset* set, Bool usedAnims ) const;
	void FillSoundEventList( TDynArray< SAnimReportSound* >& list, const CSkeletalAnimationSetEntry* animEntry, const String& localPath ) const;
	void FillSoundEventList( TDynArray< SAnimReportSound* >& list, const CSkeletalAnimationSetEntry* entry, CExtAnimSoundEvent* soundEvent, const String& localPath, Bool local ) const;
	void FillSoundEventList( TDynArray< SAnimReportSound* >& list, const String& soundEvent ) const;
	void ParseAndAddSoundToTree( SAnimReportSound* sound, wxTreeItemId root, wxTreeCtrl* tree );

	template< typename N >
	N* FindRecordByPath( const String& path, const TDynArray< N* >& recordList ) const
	{
		for ( Uint32 i=0; i<recordList.Size(); ++i )
		{
			if ( recordList[ i ]->m_path == path )
			{
				return recordList[ i ];
			}
		}
		return NULL;
	}

	template< typename T, typename N >
	void CreateDiffHtmlCode( const TDynArray< String >& depotFiles, TDynArray< N* >& recordList, const wxString& caption, wxString& code )
	{
		code += wxT("<table border=1>");
		code += wxT("<tr>");
		code += wxString::Format( wxT("<th>Num</th>") );
		code += wxString::Format( wxT("<th>%s</th>"), caption.wc_str() );
		code += wxT("</tr>");

		Uint32 unused = 0;

		const Uint32 size = depotFiles.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const String& depotFile = depotFiles[ i ];

			Bool found = false;

			const Uint32 recordsSize = recordList.Size();
			for ( Uint32 j=0; j<recordsSize; ++j )
			{
				const N* record = recordList[ j ];
				if ( record->m_absPath == depotFile )
				{
					found = true;
					break;
				}
			}

			if ( !found )
			{
				String localPath;
				GDepot->ConvertToLocalPath( depotFile, localPath );
				code += wxString::Format( wxT("<tr><th>%d</th><th align=left>%s</th></tr>"), ++unused, localPath.AsChar() );
			}
		}

		code += wxT("<tr>");
		code += wxString::Format( wxT("<th>Sum</th>") );
		code += wxString::Format( wxT("<th>All %d, used %d, unused %d</th>"), depotFiles.Size(), depotFiles.Size() - unused, unused );
		code += wxT("</tr>");

		code += wxT("</table>");
	}

	template< typename T, typename N >
	void CollectRecords( const String& caption, TDynArray< N* >& recordList )
	{
		ASSERT( !m_cookList.Empty() );
		ASSERT( recordList.Empty() );

		GFeedback->UpdateTaskInfo( TXT("Collect '%s' ..."), caption.AsChar() );

		String fileExtension = T::GetFileExtension();

		Uint32 size = m_cookList.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			GFeedback->UpdateTaskProgress( i, size );

			const String& cookedFile = m_cookList[ i ];

			if ( cookedFile.EndsWith( fileExtension ) )
			{
				T* resource = LoadResource< T >( cookedFile );
				if ( resource )
				{
					recordList.PushBack( new N( resource, m_todoList ) );
				}
				else
				{
					// Warnings...
				}
			}
		}
	}

	void CalcAnimMems( Uint32& total, Uint32& used, Uint32& unused ) const;

	wxString MemSizeToText( Uint32 memSize );

public:
	void FillACNode( ACNode& node, Uint64 hash, CAnimatedComponent* ac, EdAnimationReporterTodoList& todo );
	Uint64 CalcHashForAC( CAnimatedComponent* ac ) const;
	ACNode& GetOrCreateACNodeByComp( CAnimatedComponent* ac, EdAnimationReporterTodoList& todo );

	void FillAPNode( APNode& node, Uint64 hash, CActionPointComponent* ap, EdAnimationReporterTodoList& todo );
	Uint64 CalcHashForAP( CActionPointComponent* ap, EdAnimationReporterTodoList& todo ) const;
	APNode& GetOrCreateAPNodeByComp( CActionPointComponent* ap, EdAnimationReporterTodoList& todo );

protected: // Editor
	void RefreshReporterWindows();
	void RefreshViewAnimsets();
	void RefreshViewBehaviors();
	void RefreshViewAnimations();
	void RefreshViewAp();
	void RefreshViewEntities();
	void RefreshTodoList();
	void RefreshReportLog();
	void RefreshDiffPages();
	void RefreshExternalAnimPage();
	void RefreshSoundsPage();
	void RefreshSoundTab( const TDynArray< SAnimReportSound* >& list, const wxString& htmlName );
	void RefreshSoundTree( const TDynArray< SAnimReportSound* >& list, const wxString& treeName );
	void RefreshSoundDiffTab();

	void ShowView( EReporterView view );
	void ShowAndRefreshView( EReporterView view );

	void SelectAnimset( Int32 num );
	void SelectEntity( Int32 num );

	void DumpAnimsetListToFile();
	void ExportCompressedPoses();
	void ImportCompressedPoses();
	void DeleteCompressedPoseFiles();
	void RecreateCompressedPoses();
	void RecreateSkeletonCompression();

	void RemoveUnusedAnimations();
	void RemoveUnusedAnimationFromAnimset( EdAnimReportAnimset* animset );

protected: // wxEvents
	void OnReloadReportFilePC( wxCommandEvent& event );
	void OnReloadReportFileXBox( wxCommandEvent& event );
	void OnSaveReport( wxCommandEvent& event );
	void OnLoadReport( wxCommandEvent& event );
	void OnTmp( wxCommandEvent& event );
	void OnExportCompressedPoses( wxCommandEvent& event );
	void OnImportCompressedPoses( wxCommandEvent& event );
	void OnDeleteCompressedPoseFiles( wxCommandEvent& event );
	void OnRecreateCompressedPoseFiles( wxCommandEvent& event );
	void OnRecreateSkeletonCompressionFiles( wxCommandEvent& event );
	void OnRemoveUnusedAnimation( wxCommandEvent& event );
	void OnAnimsetViewLinkClicked( wxHtmlLinkEvent& event );
	void OnBehaviorViewLinkClicked( wxHtmlLinkEvent& event );
	void OnAnimationViewLinkClicked( wxHtmlLinkEvent& event );
	void OnEntityViewLinkClicked( wxHtmlLinkEvent& event );
	void OnApViewLinkClicked( wxHtmlLinkEvent& event );
	void OnExternalAnimsViewLinkClicked( wxHtmlLinkEvent& event );
	void OnTodoLinkClicked( wxHtmlLinkEvent& event );
	void OnGenerateSoundReport( wxCommandEvent& event );
	void OnSoundDepotLinkClicked( wxHtmlLinkEvent& event );
	void OnSoundDiffClicked( wxCommandEvent& event );
	void OnReportWinLinkClicked( wxHtmlLinkEvent& event );
	void OnStreamingReport( wxCommandEvent& event );
};
