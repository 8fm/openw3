/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "idBasicBlocks.h"
#include "idLine.h"

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
class CIDGraphBlockText : public CIDGraphBlock
{
	DECLARE_ENGINE_CLASS( CIDGraphBlockText, CIDGraphBlock, 0 );

protected:
	TDynArray< SIDTextLine >	m_lines;	

	TInstanceVar< Uint32 >		i_nextLine;

public:
	CIDGraphBlockText() {}

	//! Get the name of the block
	virtual CName GetDefaultName() const { return CNAME( Text ); }
	virtual String GetCaption() const;

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetTitleColor() const;

	virtual void OnPropertyPostChange( IProperty* prop );

	virtual Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );

	RED_INLINE Uint32 GetNumLines() const { return m_lines.Size(); }
	RED_INLINE const SIDTextLine& GetLine( Uint32 i ) const { return m_lines[ i ]; }

	String GetVoiceoverFileNameForLine( Uint32 lineIndex ) const;
	String GetStringKeyForLine( Uint32 lineIndex ) const;
	String GetStringInCurrentLanguageForLine( Uint32 lineIndex ) const;

	virtual void OnInitInstance( InstanceBuffer& data ) const;
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual const CIDGraphBlock* ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate = true ) const;
	virtual const CIDGraphBlock* Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const;
	virtual const CIDGraphBlock* ActivateOutput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* output, Bool evaluate = true ) const;

	// ILocalizableObject interface
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */;

	//! Get client color
	virtual Color GetClientColor() const;

	void RefreshVoiceovers();

	#ifndef NO_EDITOR
		RED_INLINE void SetLine( Uint32 lineIndex, const SIDLineStub& line ) 
		{ 
			R6_ASSERT( GIsEditor ); 
			if ( lineIndex >= m_lines.Size() )
			{
				m_lines.Resize( lineIndex + 1 ); 
			}
			m_lines[ lineIndex ] = line; 
		}	
		RED_INLINE void SetNumLines( Uint32 num )				{ m_lines.Resize( num ); }
	#endif

private:
	void					PlayNextLine( CIDInterlocutorComponent* interlocutor, CIDTopicInstance* topicInstance )	const;
	const	CIDGraphBlock*	TryToPlayNextLine( CIDInterlocutorComponent* interlocutor, CIDTopicInstance* topicInstance, Float timeDelta )	const;
	const	CIDGraphBlock*	EndBlock( CIDTopicInstance* topicInstance, Float timeDelta )	const;	
};

BEGIN_CLASS_RTTI( CIDGraphBlockText )
	PARENT_CLASS( CIDGraphBlock )
	PROPERTY_EDIT( m_lines, TXT("") )
END_CLASS_RTTI()
