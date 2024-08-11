#pragma once

class CStoryScene;
class CStorySceneSection;
class CStorySceneElement;
class CStorySceneEvent;
class CStorySceneGraph;
class CStorySceneDialogsetInstance;
class CStorySceneActor;
class CStorySceneProp;
class CStorySceneLight;

class CEdSceneValidator
{
public:

	enum MessageType
	{
		Error,
		Warning,
		Info,
	};

	struct SValidationOutputMessage
	{
		SValidationOutputMessage() {}
		SValidationOutputMessage( MessageType msgType, const String& message )
			: m_msgType( msgType ), m_message( message )
		{}

		MessageType		m_msgType;
		String			m_message;
	};

	struct SValidationOutput 
	{
		TDynArray<SValidationOutputMessage> m_messages;

		Uint32 NumOfErrors() const	{ return NumOfMessagesWithType( Error ); }
		Uint32 NumOfWarnings() const	{ return NumOfMessagesWithType( Warning ); }

		Uint32 NumOfMessagesWithType( MessageType t ) const
		{
			Uint32 count = 0;
			for( const SValidationOutputMessage& msg : m_messages )
			{
				count += msg.m_msgType == t;
			}
			return count;
		}
	};

private:

	struct SValidationContext
	{
		const CStoryScene*		scene;	

		TDynArray<CName>		usedIds;
		TDynArray<CName>		dialogsetNames;

		TDynArray<CName>		actorsWithNoMimic;		
		
		//scene section to dialogset map;
		TDynArray< const CStorySceneSection* >							section2dialogsetKeys;
		TDynArray< TDynArray< const CStorySceneDialogsetInstance* > >	section2dialogsetVals;

		SValidationOutput		result;
	};

public:

	CEdSceneValidator();
	SValidationOutput Process( const CStoryScene* scene ) const;

	//helper for generating reports	
	static String CEdSceneValidator::GenerateHTMLReport( const TDynArray< CEdSceneValidator::SValidationOutputMessage >  messages, const TDynArray< String >* links = nullptr, Bool addControlElements = false );

private:

	const TDynArray< const CStorySceneDialogsetInstance*>& GetDialogsetsForSection( SValidationContext& context, const CStorySceneSection* section ) const;

	void ProcessSection( SValidationContext& context, const CStorySceneSection* section ) const;
	void ProcessElement( SValidationContext& context, const CStorySceneElement*	element ) const;
	void ProcessEvent( SValidationContext& context, const CStorySceneEvent* event ) const;
	void ProcessSceneGraph( SValidationContext& context, const CStorySceneGraph* graph ) const;
	void ProcessDialogset( SValidationContext& context, const CStorySceneDialogsetInstance* dialogset ) const;
	void ProcessDialogsetSlot( SValidationContext& context, const CStorySceneDialogsetSlot* dialogset ) const;
	void ProcessActorDef( SValidationContext& context, const CStorySceneActor* actor ) const;
	void ProcessActorDefDuplication( SValidationContext& context ) const;
	void ProcessPropDef( SValidationContext& context, const CStorySceneProp* prop ) const;
	void ProcessLightDef( SValidationContext& context, const CStorySceneLight* light ) const;
};

