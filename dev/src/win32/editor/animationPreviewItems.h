
#pragma once

enum EAnimPreviewItem
{
	APItem_Sword,
	APItem_Shield,
	APItem_Last
};

class AnimPreviewItemIterator
{
	String				m_items[ APItem_Last ];
	String				m_itemCategory[ APItem_Last ];
	String				m_itemName[ APItem_Last ];
	EItemDisplaySlot	m_itemSlot[ APItem_Last ];

	Uint32				m_index;

public:
	AnimPreviewItemIterator()
		: m_index( 0 )
	{
		Load();
	}

	RED_INLINE operator Bool () const
	{
		return m_index < APItem_Last;
	}

	RED_INLINE void operator++ ()
	{
		m_index++;
	}

	RED_INLINE void GoTo( Uint32 index )
	{
		m_index = index;
	}

	RED_INLINE String operator* ()
	{
		return m_items[ m_index ];
	}

	RED_INLINE String GetCategory()
	{
		return m_itemCategory[ m_index ];
	}

	RED_INLINE String GetName()
	{
		return m_itemName[ m_index ];
	}

	RED_INLINE EItemDisplaySlot GetSlot()
	{
		return m_itemSlot[ m_index ];
	}

private:
	void Load()
	{
		m_items[ APItem_Sword ] = TXT("Sword");
		m_itemCategory[ APItem_Sword ] = TXT("steelsword");
		m_itemName[ APItem_Sword ] = TXT("Long Steel Sword");
		m_itemSlot[ APItem_Sword ] = IDS_RightHand;

		m_items[ APItem_Shield ] = TXT("Shield");
		m_itemCategory[ APItem_Shield ] = TXT("opponent_shield");
		m_itemName[ APItem_Shield ] = TXT("Knight Shield");
		m_itemSlot[ APItem_Shield ] = IDS_LeftHand;
	}
};

