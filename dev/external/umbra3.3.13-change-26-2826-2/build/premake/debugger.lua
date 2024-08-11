
-- Umbra Debugger (Viewer 2.0)

if (_ACTION == "vs2012" or _ACTION == "vs2013") then

-- ---------------------------------------------

function qtMocPrebuild(file, outfile)
    prebuildcommands {
        string.format("..\\..\\external\\qt\\moc.exe \"..\\..\\%s\" > \"..\\..\\%s\"", file, outfile)
    }   
end

function qtMoc(pattern)
	for index, f in pairs(os.matchfiles(pattern)) do
		local file = io.open(f, "r")
		local content = file:read("*all")
		file:close()

		if string.find(content, "Q_OBJECT") then			
            moc = path.getname(f)
            moc = string.gsub(moc, "%.", "_")
            moc = path.getdirectory(f) .. "\\" .. moc .. "_moc.hpp";
            
            qtMocPrebuild(f, moc)
		end
	end
end

-- ---------------------------------------------

umbraproject("debugger_core", nil, "source")
folder "debugger"
platforms { "x32", "x64" }
kind "StaticLib"
files {
	"utils/debugger/**.cpp", "utils/debugger/**.hpp",
	"utils/debugger/squirrel/**.nut",
	"utils/debugger/renderer/opengl/glsl/**.vert",
	"utils/debugger/renderer/opengl/glsl/**.frag",
}
excludes {
    "utils/debugger/launcher.cpp",
	"utils/debugger/qt/**.cpp",
	"utils/debugger/qt/**.hpp"
}
includedirs {
	"source/common",
	"utils/debugger",
	"utils/tools",
	"external/glfw/include",
	"external/glew/include",
	"external/squirrel/include",
	"external/sqrat/include",
}

configuration { "windows" }
includedirs { "external/opengl" }

defines { "GLEW_STATIC", "GLEW_MX" }

-- ------------------------------------------
-- ------------------------------------------

umbraproject("launcher", nil, "source")
folder "debugger"
platforms { "x32" }
kind "WindowedApp"

files {
    "utils/debugger/launcher.cpp",
	"utils/debugger/qt/WinResource.h",
	"utils/debugger/qt/WinResource.rc"
}

includedirs {
	"source/common",
}

links {
    "common"
}

-- ------------------------------------------
-- ------------------------------------------

umbraproject("debugger", "Release", "source")
folder "debugger"
platforms { "x32", "x64" }
kind "WindowedApp"

files {
    "utils/debugger/qt/*.cpp",
	"utils/debugger/qt/*.hpp",
	"utils/debugger/qt/umbra.ico",
	"utils/debugger/qt/WinResource.h",
	"utils/debugger/qt/WinResource.rc"
}

excludes {
	"utils/debugger/qt/*_moc.hpp"
}

includedirs {
	"source/common",
	"utils/debugger",
	"utils/tools",
	"external/glew/include",
	"external/squirrel/include",
	"external/sqrat/include",	
    "external/qt/include"
}

links {
	"debugger_core", "runtime", "optimizer_static", 
    "testdb", "common", "glew",
    "squirrel", "squirrelLib", 
    "Qt5Core", "Qt5GUI", "Qt5OpenGL", "Qt5Widgets"
}

defines { "GLEW_STATIC", "GLEW_MX" }

-- find .hpp and .cpp files that contain Q_OBJECT and moc them
qtMoc("utils/debugger/qt/*.hpp")
qtMoc("utils/debugger/qt/*.cpp")
    
configuration { "x32" }
libdirs { "external/qt/win32/lib" }

configuration { "x64" }
libdirs { "external/qt/win64/lib" }

configuration { "x32 or x64" }

postbuildcommands { 
    "if not exist ..\\..\\work\\debugger\\squirrel mkdir ..\\..\\work\\debugger\\squirrel"
}

if hasplatform("x32") then
    os.mkdir("bin/win32")
    os.mkdir("bin/win32/platforms")
    for index, f in pairs(os.matchfiles("external/qt/win32/bin/*.dll")) do
        os.copyfile(f, "bin/win32/" .. path.getname(f));
        files("bin/win32/" .. path.getname(f))
    end
    for index, f in pairs(os.matchfiles("external/qt/win32/plugins/platforms/*.dll")) do
        os.copyfile(f, "bin/win32/platforms/" .. path.getname(f));
        files("bin/win32/platforms/" .. path.getname(f))
    end
end

if hasplatform("x64") then
    os.mkdir("bin/win64")
    os.mkdir("bin/win64/platforms")
    for index, f in pairs(os.matchfiles("external/qt/win64/bin/*.dll")) do
        os.copyfile(f, "bin/win64/" .. path.getname(f));
        files("bin/win64/" .. path.getname(f))
    end
    for index, f in pairs(os.matchfiles("external/qt/win64/plugins/platforms/*.dll")) do
        os.copyfile(f, "bin/win64/platforms/" .. path.getname(f));
        files("bin/win64/platforms/" .. path.getname(f))
    end
end
end

configuration { "windows", "x32" }
libdirs     "external/curl/win32/lib"
configuration { "windows", "x64" }
libdirs     "external/curl/win64/lib"

configuration { "windows", "x32 or x64", "Release or Test" }
links "libcurl"
configuration { "windows", "x32 or x64", "Debug" }
links "libcurl_d"
