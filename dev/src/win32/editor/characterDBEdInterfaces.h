#pragma once

#include "../../../../games/r6/r6GameResource.h"
#include "../../../../common/game/characterResource.h"
#include "../../../../common/game/character.h"

class CProperty;

class IEdCharacterDBFolderManagement
{
public:
	// Check outs character
	virtual Bool CheckOutCharacter( CCharacter* character, Bool failedMessage ) = 0;
	// Deletes character
	virtual Bool DeleteCharacter( CCharacter* character ) = 0;
	// Updates character data
	virtual void CharacterModified( CCharacter* character ) = 0;

};

class IEdCharacterDBInheritanceManagement
{
public:
	// Updates children of removed character to have it's grandparent as a parent
	virtual void UpdateInheritanceForRemovedCharacter( CCharacter* character ) = 0;
	// check outs inheritance tree, as deep as given property id inherited by descendants
	virtual Bool CheckOutInheritanceTree( CCharacter* character, const CProperty* prop = nullptr ) = 0;
	// Adds character to inheritance tree
	virtual void AddCharacter( CCharacter* character ) = 0;
	// Updates inheritance data when character is renamed
	virtual void RenameCharacter( CCharacter* character ) = 0;
	// Updates inheritance tree when tree was changed
	virtual void UpdateTree() = 0;

};