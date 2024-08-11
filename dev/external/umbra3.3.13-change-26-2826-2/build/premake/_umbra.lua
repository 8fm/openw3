toolset = _ACTION or "none"
packagetype = _OPTIONS.package or "dev"

-- default platforms, if not provided on command line
if not _OPTIONS.platform or _OPTIONS.platform == "all" then
	if toolset >= "vs2012" then
		_OPTIONS.platform = "x32,x64,durango,orbis"
	else
		_OPTIONS.platform = "x32,x64,ps3,xbox360"
	end
end


-- default iOS sdk
if not _OPTIONS.ios_sdk then
	_OPTIONS.ios_sdk = "5.1"
end

-- call clang instead of gcc
if _OPTIONS.clang then
	premake.gcc.cc  = "clang"
	premake.gcc.cxx = "clang++"
	-- TODO: maybe there's a nicer way of making sure these overrides work
	premake.gcc.platforms.iOS_armv7.cc  = "clang"
	premake.gcc.platforms.iOS_armv7.cxx = "clang++"
end

-- wildcards for all c source files in directory
function cfiles(directory)
	dirs = { directory .. "/*.cpp",
			 directory .. "/*.c",
			 directory .. "/*.hpp",
			 directory .. "/*.h",
			 directory .. "/*.inl" }
	if toolset:startswith("vs") then
		table.insert(dirs, directory .. "/*.rc")
		table.insert(dirs, directory .. "/*.ico")
	end
	return dirs
end

-- test if given platform is included in generation
function hasplatform(platform)
	if not _OPTIONS.platform then return false end
	for _, cur in ipairs(string.explode(_OPTIONS.platform, ",")) do
		if (cur == platform) then return true end
	end
	return false
end

-- list of source directories, sets up "files" and "includedirs"
function sourcedirs(list)
	if (type(list) == "table") then
		for _, item in ipairs(list) do
			files(cfiles(item))
			includedirs(item)
		end
	elseif type(list) == "string" then
		files(cfiles(list))
		includedirs(list)
	end
end

-- hack for ps3 settings, spu should be a proper platform

function ps3build(subplatform)

	if subplatform == "spursjob" or subplatform == "spurstask" then
		configuration { "ps3" }
		flags "SPU"

		-- force output to "lib"
		targetdir "lib/ps3"
		includedirs { "$(SN_PS3_PATH)/spu/include/sn" }

		if subplatform == "spursjob" then
			buildoptions { "-fpic" }
			configuration { "ps3", "vs2008" }
			buildoptions { "-ffunction-sections", "-fdata-sections", "-mspurs-job-initialize" }
			linkoptions { "-fpic", "-mspurs-job-initialize", "-Wl,-q", "-Wl,--gc-sections" }
		else
			flags "SpursTask"
			configuration { "ps3", "vs2008" }
			buildoptions { "-mspurs-task", "-ffunction-sections", "-fdata-sections" }
			linkoptions { "-mspurs-task", "-Wl,-q", "-Wl,--gc-sections" }
		end
		defines { "SPU", "SN_TARGET_PS3_SPU", "__MULTI_SPURS__" }

		configuration { "ps3", "*App" }
		links {
			"$(SCE_PS3_ROOT)/target/spu/lib/pic/libdma.a",
			"$(SCE_PS3_ROOT)/target/spu/lib/pic/libm.a",
			"$(SCE_PS3_ROOT)/target/spu/lib/libatomic.a"
		}
	else
		configuration { "ps3", "*App" }
		links {
			"$(SCE_PS3_ROOT)/target/ppu/lib/libspurs_stub.a",
			"$(SCE_PS3_ROOT)/target/ppu/lib/libspurs_jq_stub.a",
			"$(SCE_PS3_ROOT)/target/ppu/lib/libsysmodule_stub.a"
		}

		configuration { "ps3", "vs2008" }
		defines { "SN_TARGET_PS3" }

		configuration { "ps3", "vs2008", "*App" }
		links {
			"$(SN_PS3_PATH)/ppu/lib/sn/libsn.a",
			"$(SCE_PS3_ROOT)/target/ppu/lib/libm.a",
			"$(SCE_PS3_ROOT)/target/ppu/lib/libio_stub.a"
		}
	end
	configuration {}
end

-- extra files to be put into release packages
-- TODO: visiblity filtering for these
function packagefiles(list)
	premake.setfilearray("solution", "packagefiles", list)
end

--
-- Project configuration defaults
--

umbraconfigfilters = {}
umbraconfigfilters.win32 =   { "windows", "x32" }
umbraconfigfilters.win64 =   { "windows", "x64" }
umbraconfigfilters.osx32 =   { "macosx", "x32" }
umbraconfigfilters.osx64 =   { "macosx", "x64" }
umbraconfigfilters.linux32 = { "linux", "x32" }
umbraconfigfilters.linux64 = { "linux", "x64" }
umbraconfigfilters.ps3 =     { "ps3" }
umbraconfigfilters.xbox360 = { "xbox360" }
umbraconfigfilters.psvita = { "psvita" }
umbraconfigfilters.ios_armv6 = { "ios_armv6" }
umbraconfigfilters.ios_armv7 = { "ios_armv7" }
umbraconfigfilters.ios_sim   = { "ios_sim" }
umbraconfigfilters.durango = { "durango" }
umbraconfigfilters.cafe     = {"cafe" }
umbraconfigfilters.orbis = { "orbis" }

function allowedpackagetype(pkg)
	if pkg == "all" then return true end
	for _, value in ipairs(premake.option.get("package").allowed) do
		if value[1] == pkg then return true end
	end
	return false
end

function defaultproject(_name, _packageconfigs, _includedinpackages)

	-- common defaults
	project(_name)
	targetname(_name)
	location("build/" .. toolset)
	objdir("intermediates/" .. _name)
	deploymentpath("devkit:\\" .. solution().umbraname .. "\\" .. _name)
	project().entitlement_template = "build/ios/entitlement.xcent.template"
	project().info_plist_template = "build/ios/Info.plist.template"

	flatinclude = {}
	table.insertflat(flatinclude, _includedinpackages)
	for _, value in ipairs(flatinclude) do
		if not allowedpackagetype(value) then
			error("Bad package type filter '" .. tostring(value) .. "' in project " .. _name)
		end
	end
	project().isincluded =
		(packagetype == "dev") or
		table.contains(flatinclude, packagetype) or
		table.contains(flatinclude, "all")

	project().packageconfigs = {}
	table.insertflat(project().packageconfigs, _packageconfigs)
	for _, value in ipairs(project().packageconfigs) do
		if not table.contains(solution().configurations, value) then
			error("Bad package config '" .. tostring(value) .. "' in project " .. _name)
		end
	end

	configuration { "durango", "*App", "Debug" }
	imagepath("bin/durango/layout/" .. _name .. "_d")
	configuration { "durango", "*App", "Test" }
	imagepath("bin/durango/layout/" .. _name .. "_t")
	configuration { "durango", "*App", "Release" }
	imagepath("bin/durango/layout/" .. _name)

	-- Place binaries in separate directories for some configs
	for config,filter in pairs(umbraconfigfilters) do
		if config:startswith("ios_") or config == "durango" or _OPTIONS.metro then
			if _OPTIONS.metro then
				config = config .. "-metro"
			end
			configuration { filter, "*App", "Debug" }
			targetdir("bin/" .. config .. "/" .. _name .. "_d")
			configuration { filter, "*App", "Test" }
			targetdir("bin/" .. config .. "/" .. _name .. "_t")
			configuration { filter, "*App", "Release" }
			targetdir("bin/" .. config .. "/" .. _name)
		end
	end

	-- because of osx dylib path
	configuration { "macosx", "SharedLib" }
	targetdir("bin/Frameworks")

	-- TODO: get these from somewhere instead of assuming libumbraXYZ(32|64)(_d).dylib
	configuration { "macosx", "SharedLib", "Release", "x32" }
	linkoptions { "-install_name @executable_path/../Frameworks/libumbra" .. _name .."32.dylib" }
	configuration { "macosx", "SharedLib", "Release", "x64" }
	linkoptions { "-install_name @executable_path/../Frameworks/libumbra" .. _name .."64.dylib" }
	configuration { "macosx", "SharedLib", "Debug", "x32" }
	linkoptions { "-install_name @executable_path/../Frameworks/libumbra" .. _name .."32_d.dylib" }
	configuration { "macosx", "SharedLib", "Debug", "x64" }
	linkoptions { "-install_name @executable_path/../Frameworks/libumbra" .. _name .."64_d.dylib" }
	configuration { "macosx", "SharedLib", "Test", "x32" }
	linkoptions { "-install_name @executable_path/../Frameworks/libumbra" .. _name .."32_t.dylib" }
	configuration { "macosx", "SharedLib", "Test", "x64" }
	linkoptions { "-install_name @executable_path/../Frameworks/libumbra" .. _name .."64_t.dylib" }

	configuration {}

end

if os.getenv("UMBRA_BUILD_ID")
then
	UMBRA_BUILD_ID = os.getenv("UMBRA_BUILD_ID")
else
	UMBRA_BUILD_ID = 0
end

function umbraproject(_name, _packageconfigs, _includedinpackages, subplatform)

	defaultproject(_name, _packageconfigs, _includedinpackages)
	targetname("umbra" .. _name)
	debugdir("work/" .. _name)

	-- umbra project defaults
	includedirs { "interface", "source" }
	flags { "ExtraWarnings", "FatalWarnings" }

	defines { "UMBRA_BUILD_ID=" .. UMBRA_BUILD_ID }

	configuration "Debug or Test"
	defines { "UMBRA_DEBUG" }

	configuration { "windows", "x32 or x64 or durango" }
	defines { "_CRT_SECURE_NO_WARNINGS" }

	-- Compile against WinXP API
	configuration { "windows", "x32 or x64", "not metro" }
	defines { "_WIN32_WINNT=0x0501" }

	configuration { "vs2012 or vs2013", "*App" }
	files "samples/resources/*"

	configuration { "psvita", "not StaticLib" }
	links { "libSceFios2_stub_weak.a", "libSceRtc_stub.a" }

    -- TODO: don't link curl into every app,
    -- it'd be better to include it inside the shared libs only
    configuration { "linux", "ConsoleApp or WindowedApp", "x64" }
    libdirs     "external/curl/linux64/lib"
    links { "ssl64", "crypto64", "curl64", "dl" }
    configuration { "linux", "ConsoleApp or WindowedApp", "x32" }
    libdirs     "external/curl/linux32/lib"
    links { "ssl32", "crypto32", "curl32", "dl" }

    -- use system libcurl on osx because of lack of cacert bundle with OpenSSL
    -- it's a tad older but seems to work with a few workarounds
    configuration { "macosx", "not StaticLib", "x32 or x64" }
    links { "curl" }

	configuration { "ios_armv7" }
	buildoptions { "-Wno-deprecated-declarations" }   -- TODO: fix these in code

	configuration { "macosx", "x32 or x64" }
	buildoptions { "-Wno-tautological-constant-out-of-range-compare" }
	configuration { "linux", "not StaticLib" }
	links { "pthread", "rt" }

	configuration { "cafe" }
	buildoptions { "-no_ansi_alias --g++" }

	configuration { "cafe", "Debug" }
	buildoptions { "-Omaxdebug" }

	configuration { "cafe", "Release or Test" }
	buildoptions { "--max_inlining -Omax" }
	flags { "Optimize" } -- OptimizeSpeed causes compiler bugs

	ps3build(subplatform)

	configuration { "vs20*", "windows", "x32 or x64", "*App" }
	postbuildcommands { "if not exist ..\\..\\work\\" .. _name .. " mkdir ..\\..\\work\\" .. _name }

	configuration {}
end

--
-- Solution configuration defaults
--

function solutiondefaults(_name)

	solution(_name .. "-" .. toolset .. "-" .. packagetype)
	solution().umbraname = _name
	location "."
	configurations  { "Debug", "Test", "Release" }
	language "C++" -- Specify at project-level to override

	flags { "Symbols", "NoPCH",
			"NoEditAndContinue", "NoMinimalRebuild", "NoRTTI",
			"NoManifest", "NoIncrementalLink", "ParallelBuild",
			"NoBundle", "C7Compatible" }

	if not (packagetype == "dev") then
		flags "NoDeploy"
	end

	if _OPTIONS.metro then
		packagemanifest "build/scripts/win8.appxmanifest"
	elseif hasplatform("durango") then
		packagemanifest "build/scripts/durango.appxmanifest"
	end

	configuration { "not metro", "not durango" }
	flags "StaticRuntime"

	configuration { "metro" }
	flags { "Metro", "Unicode" }

	-- set platform & kind specific output directories
	for config,filter in pairs(umbraconfigfilters) do
		if _OPTIONS.metro then
			config = config .. "-metro"
		end
		if filter[1] == "windows" then
			configuration { filter }
			implibdir("lib/" .. config)
		end
		configuration { filter, "not StaticLib" }
		targetdir("bin/" .. config)
		configuration { filter, "StaticLib" }
		targetdir("lib/" .. config)
	end

	-- platform specifics

	-- Generating SSE code is a bit of a mess right now..
	configuration { "x32 or x64", "linux" }
	buildoptions { "-msse2", "-mfpmath=sse" }
	configuration { "x32", "linux" }
	buildoptions { "-march=pentium" }
	configuration { "x32 or x64", "macosx" }
	buildoptions { "-msse2" }

	configuration "xbox360"
	-- xdk requires this define
	defines { "_XBOX" }
	flags { "NoExceptions" }

	configuration "ps3"
	flags { "NoExceptions" }
	configuration { "ps3", "vs2008" }
	defines { "__PS3__", "__GCC__" }
	linkoptions { "-fno-exceptions" }

	configuration "orbis"
	flags { "NoExceptions" }
	buildoptions {"-Wno-unused-private-field" }

	configuration "durango"

	configuration { "xbox360", "Release or Test" }
	buildoptions { "/Ob1", "/Oz" }

	-- suppress "cast increases required alignment" warning on psvita
	configuration { "psvita" }
	buildoptions { "--diag_suppress=1783", "--diag_suppress=1786", "-Xthumb=0"}

	-- add standard windows libs for make builds, link statically against compiler libs
	configuration { "gmake", "windows", "x32 or x64", "not StaticLib" }
	links { "kernel32", "user32", "gdi32", "winspool", "comdlg32", "advapi32", "shell32", "ole32", "oleaut32", "uuid", "odbc32", "odbccp32" }
	linkoptions { "-static-libgcc", "-static-libstdc++" }

	configuration { "gmake", "linux" }
	buildoptions { "-fno-strict-aliasing" }

	-- we want PIC for static libs, too
	configuration { "gmake", "linux", "StaticLib" }
	buildoptions { "-fPIC" }

	local platformdir = "/Applications/Xcode.app/Contents/Developer/Platforms/"
	configuration { "ios_armv6" }
	local sdk_path = platformdir .. "iPhoneOS.platform/Developer/SDKs/iPhoneOS" .. _OPTIONS.ios_sdk .. ".sdk/"
	buildoptions { "-isysroot " .. sdk_path}
	linkoptions { "-isysroot " .. sdk_path}
	configuration { "ios_armv7" }
	local sdk_path = platformdir .. "iPhoneOS.platform/Developer/SDKs/iPhoneOS" .. _OPTIONS.ios_sdk .. ".sdk/"
	buildoptions { "-isysroot " .. sdk_path}
	linkoptions { "-isysroot " .. sdk_path}
	configuration { "ios_sim" }
	local sdk_path = platformdir .. "iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator" .. _OPTIONS.ios_sdk .. ".sdk/"
	buildoptions { "-isysroot " .. sdk_path}
	linkoptions { "-isysroot " .. sdk_path}

	configuration { "x32 or x64", "macosx" }
	local sdk_path = "/Developer/SDKs/MacOSX10.6.sdk/"
	--local sdk_path = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/"
	buildoptions { "-isysroot " .. sdk_path, "-mmacosx-version-min=10.6"}
	linkoptions {  "-isysroot " .. sdk_path, "-mmacosx-version-min=10.6"}

	-- build flavor specifics

	configuration "Debug"
	defines { "DEBUG", "_DEBUG" }
	targetsuffix "_d"
	configuration { "Debug", "x32" }
	targetsuffix "32_d"
	configuration { "Debug", "x64" }
	targetsuffix "64_d"

	configuration "Test"
	flags "OptimizeSpeed"
	targetsuffix "_t"
	configuration { "Test", "x32" }
	targetsuffix "32_t"
	configuration { "Test", "x64" }
	targetsuffix "64_t"

	configuration "Release"
	flags "OptimizeSpeed"
	defines "NDEBUG"
	configuration { "Release", "x32" }
	targetsuffix "32"
	configuration { "Release", "x64" }
	targetsuffix "64"
end

---
--- Umbra options
---

newoption {
	trigger = "package",
	value = "PACKAGE",
	description = "Choose release package type",
	allowed = {
		{ "dev", "Umbra internal development build (default)" },
		{ "source", "Source release package" },
		{ "rt-source", "Runtime sources" },
		{ "bin", "Binary release package" }
	}
}

newoption {
	trigger = "list",
	description = "List files for release package"
}

newoption {
	trigger = "metro",
	description = "Build Metro-style apps"
}

newoption {
	trigger = "ios_sdk",
	description = "iOS SDK version to target"
}

newoption {
	trigger = "clang",
	description = "use clang instead of GCC"
}
---
--- Wrapper action for Umbra-specific tweaks
---

if _OPTIONS.list then
	_QUIET = true
end

local realaction = premake.action.current()
local fakeaction = {}
local platforms = {}

if realaction then
	fakeaction = table.merge(realaction)
	premake.action.list[realaction.trigger] = fakeaction
end

-- overriding generate to print generated file names
local _generate = premake.generate
function premake.generate(obj, filename, callback)
	if _OPTIONS.list then
		print(premake.project.getfilename(obj, filename))
	end
	_generate(obj, filename, callback)
end

-- overriding filterplatforms to obtain active platform list
local _filterplatforms = premake.filterplatforms
function premake.filterplatforms(sln, map, defaults)
	platforms = _filterplatforms(sln, map, defaults)
	return platforms
end

function converthiddenlinks(prj)
	for _, cfg in pairs(prj.project.__configs) do
		convlinks = {}
		deppaths = {}
		for _, link in ipairs(cfg.links) do
			newlink = link
			local dep = premake.findproject("HIDDEN-" .. link)
			if dep then
				prjcfg = premake.getconfig(dep, cfg.name, cfg.platform)
				if prjcfg then
					newlink = path.rebase(prjcfg.linktarget.fullpath, prjcfg.location, cfg.basedir)
					-- windows configs want to add .lib themselves
					if newlink:sub(newlink:len() - 3) == ".lib" then
						newlink = newlink:sub(1, newlink:len() - 4)
					end
				end
			end
			table.insert(convlinks, newlink)
		end
		cfg.links = convlinks
	end
end

fakeaction.onproject =
	function(prj)
		local hidden = string.startswith(prj.name, "HIDDEN-")
		-- generate project file if not filtered
		if (not hidden) then
			converthiddenlinks(prj)
			realaction.onproject(prj)
			if _OPTIONS.list then
				for fcfg in premake.project.eachfile(prj) do
					printf(fcfg.vpath)
				end
			end
		end
		-- outputs
		if (_OPTIONS.list and not table.isempty(prj.project.packageconfigs)) then
			for _, cfg in pairs(prj.project.__configs) do
				skip = false
				-- user filter
				if not table.contains(prj.project.packageconfigs, cfg.name) then
					skip = true
				end
				-- don't include wrong platform binaries
				if not table.contains(platforms, cfg.platform) then
					skip = true
				end
				if (cfg.name == "Test") then
					skip = true
				end
				-- todo: report skipped somewhere
				if not skip then
					printf(path.rebase(cfg.buildtarget.fullpath, cfg.location, prj.solution.location))
					if (cfg.kind == "SharedLib" and cfg.buildtarget.fullpath ~= cfg.linktarget.fullpath) then
						printf(path.rebase(cfg.linktarget.fullpath, cfg.location, prj.solution.location))
					end
					if (cfg.kind ~= "StaticLib" and
							(cfg.platform == "Xbox360" or
							 ((cfg.platform == "x32" or cfg.platform == "x64") and _ACTION:startswith("vs")))) then
						local pdbfile  = cfg.buildtarget.directory .. "/" .. path.getbasename(cfg.buildtarget.name) .. ".pdb"
						printf(path.rebase(pdbfile, cfg.location, prj.solution.location))
					end
				end
			end
		end
	end

fakeaction.onsolution =
	function(sln)
		-- filter out projects
		local allprj = sln.projects
		sln.projects = {}
		for _,prj in ipairs(allprj) do
			if (prj.isincluded) then
				table.insert(sln.projects, prj)
			else
				prj.name = "HIDDEN-" .. prj.name
			end
		end
		realaction.onsolution(sln)
		sln.projects = allprj
		if _OPTIONS.list and sln.packagefiles then
			for _,f in pairs(sln.packagefiles) do
				printf(path.getrelative(sln.location, f))
			end
		end
	end
