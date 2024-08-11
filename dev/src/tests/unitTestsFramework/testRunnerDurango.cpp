/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "../../common/redSystem/architecture.h"
#include "../../common/redSystem/os.h"
#include "../../common/core/types.h"
#include "test.h"
#include "helper.h"
#include <stdlib.h>

#ifdef RED_PLATFORM_DURANGO

using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;


ref class UnitTestApplicationView sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:

	virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
	virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
	virtual void Load(Platform::String^ entryPoint) {}
    virtual void Run();
	virtual void Uninitialize();

protected:

	void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
};

void UnitTestApplicationView::Initialize( Windows::ApplicationModel::Core::CoreApplicationView^ applicationView )
{
	applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &UnitTestApplicationView::OnActivated);
}

void UnitTestApplicationView::SetWindow(Windows::UI::Core::CoreWindow^ )
{}

void UnitTestApplicationView::Uninitialize()
{}

void UnitTestApplicationView::OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args)
{
	 if (args->Kind == ActivationKind::Launch)
	 {
		LaunchActivatedEventArgs^ launchArgs = static_cast<LaunchActivatedEventArgs^>(args);
		const UniChar * commandLine = launchArgs->Arguments->Data();
		
		std::vector< std::wstring > arguments;
		arguments.push_back( launchArgs->TileId->Data() );
		std::wstringstream stringStream( commandLine );
		std::wstring item;
		while ( stringStream >> item ) 
		{
			arguments.push_back(item);
		}

		testing::InitGoogleTest( arguments );
	 }
}

void UnitTestApplicationView::Run()
{   
	Red::UnitTest::RunAllTests();
}

ref class UnitTestApplicationFactory : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView()
	{
		return ref new UnitTestApplicationView();
	}
};

// Application entry point
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
    UnitTestApplicationFactory^ factory = ref new UnitTestApplicationFactory();
    Windows::ApplicationModel::Core::CoreApplication::Run(factory);

    return 0;
}
#endif
