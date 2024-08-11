
-- Files to put in all packages

packagefiles
{
	"interface/**",
	"utils/main/umbraMain.hpp",
	"utils/main/default/defaultMain.hpp"
}

if _OPTIONS.os == "windows" and (hasplatform("x32") or hasplatform("x64")) then
packagefiles
{
    "bin/win32/platforms/qwindows.dll",
    "bin/win32/icudt52.dll",
    "bin/win32/icuin52.dll",
    "bin/win32/icuuc52.dll",
    "bin/win32/Qt5Core.dll",
    "bin/win32/Qt5Gui.dll",
    "bin/win32/Qt5OpenGL.dll",
    "bin/win32/Qt5Widgets.dll",    
    "bin/win64/platforms/qwindows.dll",
    "bin/win64/icudt52.dll",
    "bin/win64/icuin52.dll",
    "bin/win64/icuuc52.dll",
    "bin/win64/Qt5Core.dll",
    "bin/win64/Qt5Gui.dll",
    "bin/win64/Qt5OpenGL.dll",
    "bin/win64/Qt5Widgets.dll"
}
end

if hasplatform("durango") then 
	packagefiles { "utils/main/xboxone/xboxoneMain.hpp" }
end

if os.get() == "windows" and hasplatform("x32") then
    packagefiles
    {
        "external/curl/win32/include/*",
        "external/curl/win32/lib/*"
    }
end
if os.get() == "windows" and hasplatform("x64") then
    packagefiles
    {
        "external/curl/win64/include/curl/*",
        "external/curl/win64/lib/*"
    }
end

if os.get() == "linux" and hasplatform("x32") then
    packagefiles
    {
        "external/curl/linux32/include/curl/*",
        "external/curl/linux32/lib/*"
    }
end
if os.get() == "linux" and hasplatform("x64") then
    packagefiles
    {
        "external/curl/linux64/include/curl/*",
        "external/curl/linux64/lib/*"
    }
end

if os.get() == "macosx" and hasplatform("x32") then
    packagefiles
    {
        "external/curl/osx32/include/curl/*",
        "external/curl/osx32/lib/*"
    }
end
if os.get() == "osx" and hasplatform("x64") then
    packagefiles
    {
        "external/curl/osx64/include/curl/*",
        "external/curl/osx64/lib/*"
    }
end

if _OPTIONS.package == "source" then
	packagefiles
	{
		"external/eigen3/**",
		"external/premake/**",
		"build/premake/*.lua",
		"premake4.lua",
	}
    
    if _OPTIONS.os == "windows" then
        packagefiles
        {
            "external/qt/**"
        }
        configuration { }
    end
end
