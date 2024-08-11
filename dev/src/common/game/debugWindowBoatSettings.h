/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../engine/redGuiWindow.h"

#ifndef FINAL

namespace DebugWindows
{
    class CDebugWindowBoatSettings : public RedGui::CRedGuiWindow
    {
    public:
        CDebugWindowBoatSettings();
        ~CDebugWindowBoatSettings();

        Bool RegisterBoat( CBoatComponent* ptr );
        Bool DeRegisterBoat( CBoatComponent* ptr );

    private:

        void CreateSliderBelow( RedGui::CRedGuiControl* parent, Float min, Float max, Float step, const String& name, const String& userDataType, RedGui::RedGuiAny userData, Bool addToList = true );
        void CreateCheckBoxBelow( RedGui::CRedGuiControl* parent, const String& name, RedGui::RedGuiAny userData );

        // Notify events
        void NotifyOnSliderChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value );
        void NotifyOnCheckedChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value );
        void NotifyOnTabChanged( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* selectedTab );
        void NotifyOnButtonClicked( RedGui::CRedGuiEventPackage& eventPackage );

        virtual void OnWindowClosed(CRedGuiControl* control);
        virtual void OnWindowOpened(CRedGuiControl* control);

    private:
        RedGui::CRedGuiTab*		            m_tabs;
        RedGui::CRedGuiButton*              m_saveButton;
        RedGui::CRedGuiButton*              m_loadButton;
        TDynArray<RedGui::CRedGuiAdvancedSlider*>   m_guiSliders;
        TDynArray<RedGui::CRedGuiCheckBox*>         m_guiCheckboxes;

        TDynArray<CBoatComponent*>  m_registeredBoats;

    };
}	// namespace DebugWindows

#endif  // FINAL
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
