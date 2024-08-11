--[[

== Umbra3 solution generator ==


This requires an internally patched version of premake to work.

* Project file structure

  Project specific setting are stored in project .lua files in the
  directory "build/premake".

  The generated project files will be placed under "build/<action>"

* Umbra project defaults

  Start each project with a call to umbraproject(). Note that multiple
  projects may be defined in a single .lua file.

* Umbra helpers

  TBD

* Target platform support

  The generator supports generating solution configurations for a user
  specified list of platforms. This is accomplished by extending the
  premake "platform" option to support passing in a comma-separated
  list.  Premake does not support configuration-level file lists
  (filtering files to build per platform) and the currently selected
  platforms are not available at the time of parsing, so to hide files
  that are not needed for the platforms we are generating the project
  files for, the hasplatform() utility inspects the platform option
  string directly.

  We dont mess around with "native" platforms, but require platforms
  to be explicitly set. If --platform option is not given on command
  line, the default is to generate for all Umbra target platforms.

* Project "visibility"  

  There are two variables to umbraproject() that dictate what gets placed
  in release packages for the project.

  The first variable is a list of build configurations for which outputs
  will be placed in release packages (regardless of release package type).

  The second variable is a list of release package types that the project
  sources are included in. There are a couple of exceptions to this:

  - all projects are always included in the "dev" package type
  - the special string "all" anywhere in the list means that the
    project is included in all current and future package types

--]]


require "build/premake/_umbra"

if not _OPTIONS.metro then

	solutiondefaults("umbra3")
	local projectfiles = os.matchfiles("build/premake/*.lua")
	for _,proj in ipairs(projectfiles) do
		basename = path.getbasename(proj)
		if not string.startswith(basename, "_") then
			require("build/premake/" .. basename)
		end
	end

else

	solutiondefaults("umbra3-metro")
	require("build/premake/common")
	require("build/premake/runtime")
	require("build/premake/extrafiles")

end

--- Projects yet to be converted
--- * converter?
--- * demo (umbra3)



