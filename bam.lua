Import("configure.lua")
Import("other/mysql/mysql.lua")

--- Setup Config -------
config = NewConfig()
config:Add(OptCCompiler("compiler"))
config:Add(OptTestCompileC("stackprotector", "int main(){return 0;}", "-fstack-protector -fstack-protector-all"))
config:Add(OptTestCompileC("minmacosxsdk", "int main(){return 0;}", "-mmacosx-version-min=10.5 -isysroot /Developer/SDKs/MacOSX10.5.sdk"))
config:Add(OptTestCompileC("macosxppc", "int main(){return 0;}", "-arch ppc"))
config:Add(OptLibrary("zlib", "zlib.h", false))
config:Add(Mysql.OptFind("mysql", false))
config:Finalize("config.lua")

-- data compiler
function Script(name)
	if family == "windows" then
		return str_replace(name, "/", "\\")
	end
	return "python3 " .. name
end

function ResCompile(scriptfile)
	scriptfile = Path(scriptfile)
	if config.compiler.driver == "cl" then
		output = PathBase(scriptfile) .. ".res"
		AddJob(output, "rc " .. scriptfile, "rc /fo " .. output .. " " .. scriptfile)
	elseif config.compiler.driver == "gcc" then
		output = PathBase(scriptfile) .. ".coff"
		AddJob(output, "windres " .. scriptfile, "windres -i " .. scriptfile .. " -o " .. output)
	end
	AddDependency(output, scriptfile)
	return output
end

function Dat2c(datafile, sourcefile, arrayname)
	datafile = Path(datafile)
	sourcefile = Path(sourcefile)

	AddJob(
		sourcefile,
		"dat2c " .. PathFilename(sourcefile) .. " = " .. PathFilename(datafile),
		Script("scripts/dat2c.py").. "\" " .. sourcefile .. " " .. datafile .. " " .. arrayname
	)
	AddDependency(sourcefile, datafile)
	return sourcefile
end

function ContentCompile(action, output)
	output = Path(output)
	AddJob(
		output,
		action .. " > " .. output,
		--Script("datasrc/compile.py") .. "\" ".. Path(output) .. " " .. action
		Script("datasrc/compile.py") .. " " .. action .. " > " .. Path(output)
	)
	AddDependency(output, Path("datasrc/content.py")) -- do this more proper
	AddDependency(output, Path("datasrc/network.py"))
	AddDependency(output, Path("datasrc/compile.py"))
	AddDependency(output, Path("datasrc/datatypes.py"))
	return output
end

-- Content Compile
network_source = ContentCompile("network_source", "src/game/generated/protocol.cpp")
network_header = ContentCompile("network_header", "src/game/generated/protocol.h")
server_content_source = ContentCompile("server_content_source", "src/game/generated/server_data.cpp")
server_content_header = ContentCompile("server_content_header", "src/game/generated/server_data.h")

AddDependency(network_source, network_header)
AddDependency(server_content_source, server_content_header)

server_link_other = {}
server_sql_depends = {}

if family == "windows" then
	table.insert(server_sql_depends, CopyToDirectory(".", "other/mysql/vc2005libs/mysqlcppconn.dll"))
	table.insert(server_sql_depends, CopyToDirectory(".", "other/mysql/vc2005libs/libmysql.dll"))

	if config.compiler.driver == "cl" then
		server_link_other = {ResCompile("other/icons/teeworlds_srv_cl.rc")}
	elseif config.compiler.driver == "gcc" then
		server_link_other = {ResCompile("other/icons/teeworlds_srv_gcc.rc")}
	end
end

function Intermediate_Output(settings, input)
	return "objs/" .. string.sub(PathBase(input), string.len("src/")+1) .. settings.config_ext
end

function build(settings)
	-- apply compiler settings
	config.compiler:Apply(settings)
	
	--settings.objdir = Path("objs")
	settings.cc.Output = Intermediate_Output

	if config.compiler.driver == "cl" then
		settings.cc.flags:Add("/wd4244", "/wd4577")
	else
		settings.cc.flags:Add("-Wall", "-fexceptions")
		if family == "windows" then
			-- disable visibility attribute support for gcc on windows
			settings.cc.defines:Add("NO_VIZ")
		elseif platform == "macosx" then
			settings.cc.flags:Add("-mmacosx-version-min=10.5")
			settings.link.flags:Add("-mmacosx-version-min=10.5")
			if config.minmacosxsdk.value == 1 then
				settings.cc.flags:Add("-isysroot /Developer/SDKs/MacOSX10.5.sdk")
				settings.link.flags:Add("-isysroot /Developer/SDKs/MacOSX10.5.sdk")
			end
		elseif config.stackprotector.value == 1 then
			settings.cc.flags:Add("-fstack-protector", "-fstack-protector-all")
			settings.link.flags:Add("-fstack-protector", "-fstack-protector-all")
		end
	end

	-- set some platform specific settings
	settings.cc.includes:Add("src")

	if family == "unix" then
		if platform == "macosx" then
			settings.cc.flags_cxx:Add("-stdlib=libc++")
			settings.link.libs:Add("c++")
			settings.link.frameworks:Add("Carbon")
			settings.link.frameworks:Add("AppKit")
		else
			settings.link.libs:Add("pthread")
		end
		
		if platform == "solaris" then
		    settings.link.flags:Add("-lsocket")
		    settings.link.flags:Add("-lnsl")
		end
	elseif family == "windows" then
		settings.link.libs:Add("gdi32")
		settings.link.libs:Add("user32")
		settings.link.libs:Add("ws2_32")
		settings.link.libs:Add("ole32")
		settings.link.libs:Add("shell32")
		settings.link.libs:Add("advapi32")
	end

	-- compile zlib if needed
	if config.zlib.value == 1 then
		settings.link.libs:Add("z")
		if config.zlib.include_path then
			settings.cc.includes:Add(config.zlib.include_path)
		end
		zlib = {}
	else
		zlib = Compile(settings, Collect("src/engine/external/zlib/*.c"))
		settings.cc.includes:Add("src/engine/external/zlib")
	end

	-- build the small libraries
	json = Compile(settings, "src/engine/external/json-parser/json.c")
	md5 = Compile(settings, Collect("src/engine/external/md5/*.c"))

	-- build game components
	engine_settings = settings:Copy()
	server_settings = engine_settings:Copy()
	launcher_settings = engine_settings:Copy()

	if family == "unix" then
		if platform == "macosx" then
			launcher_settings.link.frameworks:Add("Cocoa")
		end
		server_settings.link.libs:Add("dl")
		server_settings.link.libs:Add("curl")

		if string.find(settings.config_name, "sql") then
			server_settings.link.libs:Add("ssl")
			server_settings.link.libs:Add("crypto")
			server_settings.link.libs:Add("mysqlcppconn")
		end

	elseif family == "windows" then
		server_settings.link.libpath:Add("other/mysql/vc2005libs")
		if string.find(settings.config_name, "sql") then
			server_settings.link.libpath:Add("other/mysql/vc2005libs")
		end
	end
	
	engine = Compile(engine_settings, Collect("src/engine/shared/*.cpp", "src/base/*.cpp"))
	server = Compile(server_settings, Collect("src/engine/server/*.cpp"))
	game_shared = Compile(settings, Collect("src/game/*.cpp"), network_source)
	game_server = Compile(settings, CollectRecursive("src/game/server/*.cpp"), server_content_source)
	teeother = Compile(server_settings, Collect("src/teeother/*.cpp", "src/teeother/components/*.cpp"))
	
	server_osxlaunch = {}
	if platform == "macosx" then
		server_osxlaunch = Compile(launcher_settings, "src/osxlaunch/server.m")
	end

	-- build server
	server_exe = Link(server_settings, "Alchemist-Server", engine, server,
		game_shared, game_server, zlib, md5, sqlite3, server_link_other, json, teeother)

	serverlaunch = {}
	if platform == "macosx" then
		serverlaunch = Link(launcher_settings, "serverlaunch", server_osxlaunch)
	end

	-- make targets
	s = PseudoTarget("server".."_"..settings.config_name, server_exe, serverlaunch)

	all = PseudoTarget(settings.config_name, c, s, v, m, t)
	return all
end



debug_settings = NewSettings()
debug_settings.config_name = "debug"
debug_settings.config_ext = "-Debug"
debug_settings.debug = 1
debug_settings.optimize = 0
debug_settings.cc.defines:Add("CONF_DEBUG")

debug_sql_settings = NewSettings()
debug_sql_settings.config_name = "sql_debug"
debug_sql_settings.config_ext = "-Debug-SQL"
debug_sql_settings.debug = 1
debug_sql_settings.optimize = 0
debug_sql_settings.cc.defines:Add("CONF_DEBUG", "CONF_SQL")

release_settings = NewSettings()
release_settings.config_name = "release"
release_settings.config_ext = ""
release_settings.debug = 0
release_settings.optimize = 1
release_settings.cc.defines:Add("CONF_RELEASE")

release_sql_settings = NewSettings()
release_sql_settings.config_name = "sql_release"
release_sql_settings.config_ext = "-SQL"
release_sql_settings.debug = 0
release_sql_settings.optimize = 1
release_sql_settings.cc.defines:Add("CONF_RELEASE", "CONF_SQL")

config.mysql:Apply(debug_sql_settings)
config.mysql:Apply(release_sql_settings)

if platform == "macosx" then
	debug_settings_ppc = debug_settings:Copy()
	debug_settings_ppc.config_name = "debug_ppc"
	debug_settings_ppc.config_ext = "-PPC-Debug"
	debug_settings_ppc.cc.flags:Add("-arch ppc")
	debug_settings_ppc.link.flags:Add("-arch ppc")
	debug_settings_ppc.cc.defines:Add("CONF_DEBUG")

	debug_sql_settings_ppc = debug_settings:Copy()
	debug_sql_settings_ppc.config_name = "sql_debug_ppc"
	debug_sql_settings_ppc.config_ext = "-PPC-Debug-SQL"
	debug_sql_settings_ppc.cc.flags:Add("-arch ppc")
	debug_sql_settings_ppc.link.flags:Add("-arch ppc")
	debug_sql_settings_ppc.cc.defines:Add("CONF_DEBUG", "CONF_SQL")

	release_settings_ppc = release_settings:Copy()
	release_settings_ppc.config_name = "release_ppc"
	release_settings_ppc.config_ext = "-PPC"
	release_settings_ppc.cc.flags:Add("-arch ppc")
	release_settings_ppc.link.flags:Add("-arch ppc")
	release_settings_ppc.cc.defines:Add("CONF_RELEASE")

	release_sql_settings_ppc = release_settings:Copy()
	release_sql_settings_ppc.config_name = "sql_release_ppc"
	release_sql_settings_ppc.config_ext = "-SQL"
	release_sql_settings_ppc.cc.flags:Add("-arch ppc")
	release_sql_settings_ppc.link.flags:Add("-arch ppc")
	release_sql_settings_ppc.cc.defines:Add("CONF_RELEASE", "CONF_SQL")

	ppc_d = build(debug_settings_ppc)
	ppc_r = build(release_settings_ppc)
	sql_ppc_d = build(debug_sql_settings_ppc)
	sql_ppc_r = build(release_sql_settings_ppc)

	if arch == "ia32" or arch == "amd64" then
		debug_settings_x86 = debug_settings:Copy()
		debug_settings_x86.config_name = "debug_x86"
		debug_settings_x86.config_ext = "-Debug-x86"
		debug_settings_x86.cc.flags:Add("-arch i386")
		debug_settings_x86.link.flags:Add("-arch i386")
		debug_settings_x86.cc.defines:Add("CONF_DEBUG")

		debug_sql_settings_x86 = debug_settings:Copy()
		debug_sql_settings_x86.config_name = "sql_debug_x86"
		debug_sql_settings_x86.config_ext = "_sql_x86_d"
		debug_sql_settings_x86.cc.flags:Add("-arch i386")
		debug_sql_settings_x86.link.flags:Add("-arch i386")
		debug_sql_settings_x86.cc.defines:Add("CONF_DEBUG", "CONF_SQL")

		release_settings_x86 = release_settings:Copy()
		release_settings_x86.config_name = "release_x86"
		release_settings_x86.config_ext = "-x86"
		release_settings_x86.cc.flags:Add("-arch i386")
		release_settings_x86.link.flags:Add("-arch i386")
		release_settings_x86.cc.defines:Add("CONF_RELEASE")

		release_sql_settings_x86 = release_settings:Copy()
		release_sql_settings_x86.config_name = "sql_release_x86"
		release_sql_settings_x86.config_ext = "_sql_x86"
		release_sql_settings_x86.cc.flags:Add("-arch i386")
		release_sql_settings_x86.link.flags:Add("-arch i386")
		release_sql_settings_x86.cc.defines:Add("CONF_RELEASE", "CONF_SQL")
	
		x86_d = build(debug_settings_x86)
		x86_r = build(release_settings_x86)
		sql_x86_d = build(debug_sql_settings_x86)
		sql_x86_r = build(release_sql_settings_x86)
	end

	if arch == "amd64" then
		debug_settings_x86_64 = debug_settings:Copy()
		debug_settings_x86_64.config_name = "debug_x86_64"
		debug_settings_x86_64.config_ext = "-Debug-x86_64"
		debug_settings_x86_64.cc.flags:Add("-arch x86_64")
		debug_settings_x86_64.link.flags:Add("-arch x86_64")
		debug_settings_x86_64.cc.defines:Add("CONF_DEBUG")

		debug_sql_settings_x86_64 = debug_settings:Copy()
		debug_sql_settings_x86_64.config_name = "debug_sql_x86_64"
		debug_sql_settings_x86_64.config_ext = "-Debug-SQL-x86_64"
		debug_sql_settings_x86_64.cc.flags:Add("-arch x86_64")
		debug_sql_settings_x86_64.link.flags:Add("-arch x86_64")
		debug_sql_settings_x86_64.cc.defines:Add("CONF_DEBUG", "CONF_SQL")

		release_settings_x86_64 = release_settings:Copy()
		release_settings_x86_64.config_name = "release_x86_64"
		release_settings_x86_64.config_ext = "-x86_64"
		release_settings_x86_64.cc.flags:Add("-arch x86_64")
		release_settings_x86_64.link.flags:Add("-arch x86_64")
		release_settings_x86_64.cc.defines:Add("CONF_RELEASE")

		release_sql_settings_x86_64 = release_settings:Copy()
		release_sql_settings_x86_64.config_name = "release_sql_x86_64"
		release_sql_settings_x86_64.config_ext = "-SQL-x86_64"
		release_sql_settings_x86_64.cc.flags:Add("-arch x86_64")
		release_sql_settings_x86_64.link.flags:Add("-arch x86_64")
		release_sql_settings_x86_64.cc.defines:Add("CONF_RELEASE", "CONF_SQL")

		x86_64_d = build(debug_settings_x86_64)
		x86_64_r = build(release_settings_x86_64)
		sql_x86_64_d = build(debug_sql_settings_x86_64)
		sql_x86_64_r = build(release_sql_settings_x86_64)
	end

	DefaultTarget("game_debug_x86")
	
	if config.macosxppc.value == 1 then
		if arch == "ia32" then
			PseudoTarget("release", ppc_r, x86_r)
			PseudoTarget("debug", ppc_d, x86_d)
			PseudoTarget("server_release", "server_release_ppc", "server_release_x86")
			PseudoTarget("server_debug", "server_debug_ppc", "server_debug_x86")
			PseudoTarget("sql_release", sql_ppc_r, sql_x86_r)
			PseudoTarget("sql_debug", sql_ppc_d, sql_x86_d)
			PseudoTarget("server_sql_release", "server_sql_release_ppc", "server_sql_release_x86")
			PseudoTarget("server_sql_debug", "server_sql_debug_ppc", "server_sql_debug_x86")
		elseif arch == "amd64" then
			PseudoTarget("release", ppc_r, x86_r, x86_64_r)
			PseudoTarget("debug", ppc_d, x86_d, x86_64_d)
			PseudoTarget("server_release", "server_release_ppc", "server_release_x86", "server_release_x86_64")
			PseudoTarget("server_debug", "server_debug_ppc", "server_debug_x86", "server_debug_x86_64")
			PseudoTarget("sql_release", sql_ppc_r, sql_x86_r, sql_x86_64_r)
			PseudoTarget("sql_debug", sql_ppc_d, sql_x86_d, sql_x86_64_d)
			PseudoTarget("server_sql_release", "server_sql_release_ppc", "server_sql_release_x86", "server_sql_release_x86_64")
			PseudoTarget("server_sql_debug", "server_sql_debug_ppc", "server_sql_debug_x86", "server_sql_debug_x86_64")
				else
			PseudoTarget("release", ppc_r)
			PseudoTarget("debug", ppc_d)
			PseudoTarget("server_release", "server_release_ppc")
			PseudoTarget("server_debug", "server_debug_ppc")
			PseudoTarget("sql_release", sql_ppc_r)
			PseudoTarget("sql_debug", sql_ppc_d)
			PseudoTarget("server_sql_release", "server_sql_release_ppc")
			PseudoTarget("server_sql_debug", "server_sql_debug_ppc")
		end
	else
		if arch == "ia32" then
			PseudoTarget("release", x86_r)
			PseudoTarget("debug", x86_d)
			PseudoTarget("server_release", "server_release_x86")
			PseudoTarget("server_debug", "server_debug_x86")
			PseudoTarget("sql_release", sql_x86_r)
			PseudoTarget("sql_debug", sql_x86_d)
			PseudoTarget("server_sql_release", "server_sql_release_x86")
			PseudoTarget("server_sql_debug", "server_sql_debug_x86")
		elseif arch == "amd64" then
			PseudoTarget("release", x86_r, x86_64_r)
			PseudoTarget("debug", x86_d, x86_64_d)
			PseudoTarget("server_release", "server_release_x86", "server_release_x86_64")
			PseudoTarget("server_debug", "server_debug_x86", "server_debug_x86_64")
			PseudoTarget("sql_release", sql_x86_r, sql_x86_64_r)
			PseudoTarget("sql_debug", sql_x86_d, sql_x86_64_d)
			PseudoTarget("server_sql_release", "server_sql_release_x86", "server_sql_release_x86_64")
			PseudoTarget("server_sql_debug", "server_sql_debug_x86", "server_sql_debug_x86_64")
		end
	end
else
	build(debug_settings)
	build(release_settings)
	build(debug_sql_settings)
	build(release_sql_settings)
	DefaultTarget("server_debug")
end