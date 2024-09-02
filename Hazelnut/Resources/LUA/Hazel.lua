local function getAssemblyFiles(directory, is_windows)
	if is_windows then
        handle = io.popen("dir " .. directory .. " /B /A-D")
    else
        handle = io.popen("ls " .. directory)
	end

	t = {}
	for f in handle:lines() do
		if path.hasextension(f, ".dll") then
			if string.find(f, "System.") then
				table.insert(t, f)
			end
		end
	end

	handle:close()
	return t
end

function linkAppReferences(linkScriptCore)
	local hazelDir = os.getenv("HAZEL_DIR")
	local monoLibsPath
    local monoLibsFacadesPath
	local is_windows = os.istarget('windows')

	if is_windows then
		monoLibsPath = path.join(hazelDir, "Hazelnut", "mono", "lib", "mono", "4.5"):gsub("/", "\\")
		monoLibsFacadesPath = path.join(monoLibsPath, "Facades"):gsub("/", "\\")
	else
		monoLibsPath = path.join(hazelDir, "Hazelnut", "mono", "linux", "lib", "mono", "4.5")
		monoLibsFacadesPath = path.join(monoLibsPath, "Facades")
	end

	-- NOTE(Peter): We HAVE to use libdirs, using the full path in links won't work
	--				this is a known issue with Visual Studio...
	libdirs { monoLibsPath, monoLibsFacadesPath }
	if linkScriptCore ~= false then
		links { "Hazel-ScriptCore" }
	end

	for k, v in ipairs(getAssemblyFiles(monoLibsPath, is_windows)) do
		--print("Adding reference to: " .. v)
		links { v }
	end

	for k, v in ipairs(getAssemblyFiles(monoLibsFacadesPath, is_windows)) do
        --print("Adding reference to: " .. v)
        links { v }
    end
end