#pragma once

class CTraitData;

class CTraitValidator
{
private:
	CTraitData* m_traitData;

	Bool                m_isValid;
	TDynArray< String > m_errorMessages;
	Int32				m_index;

public:
	CTraitValidator( CTraitData* traitData );
	
	// Validates trait data, returns true if everything is OK.
	// If returned value is false, than error message will be put into 'errorMsg' variable.
	Bool Validate( String& errorMsg /* out */ );

	// Data modification
	void UpdateSkillWithTraitData();
	void UpdateTraitWithSkillData();

private:
	// compares skill data to trait data, if there are differences between trait requirements and skill trait list
	// removes additional or adds missing data
	void CompareSkillsToTraitRequirements( bool addAndNotRemove );
	void CompareTraitRequirementsToSkills( bool addAndNotRemove );

	void CheckUniqueTraitNames();
	void CheckUniqueSkillNames();
	void CheckUniqueUnlockedTraitNames();
	void CheckSkillTrait();
	void DeleteSkillRequirement( CName traitName, CName skillName, Int32 levelId );
	void GenerateNewName( String& oldName, TSet< CName > names, CName dataInfo, String& newName );

	void AddErrorMsg( const String &errorMsg );
	String GetErrorMsg();

};