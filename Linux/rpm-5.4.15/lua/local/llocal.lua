
--  -----------------------------------------------------------------------  --
--                               GLOBAL HACKS
--  -----------------------------------------------------------------------  --

--  provide a generic regular expression constructor
--  based on the most powerful regular expression engine
rex = rex_posix
if rex_pcre ~= nil then
   rex = rex_pcre
end

--  provide additional "util" namespace
util = {}

--  -----------------------------------------------------------------------  --
--                         UNIX grep(1) EMULATIONS
--  -----------------------------------------------------------------------  --

--  "global regular expression print" on file content
function util.grep(expr, filename)
	if not posix.stat(filename, "mode") then
		return nil
	end
	local lines = {}
	local pat = rex.new(expr)
	local pos = 1
	for line in io.lines(filename) do
		if pat:tfind(line) then
			table.insert(lines, pos, line)
		end
		pos = pos + 1
	end
	if table.getn(lines) == 0 then
		return nil
	end
	return lines
end

--  iterator version
function util.igrep(expr, filename)
	return ipairs(rex.grep(expr, filename))
end

--  boolean version
function util.bgrep(expr, filename)
	if not posix.stat(filename, "mode") then
		return nil
	end
	local pat = rex.new(expr)
	for line in io.lines(filename) do
		if pat:tfind(line) then
			return true
		end
	end
	return false
end

--  -----------------------------------------------------------------------  --
--                   REGULAR EXPRESSION MATCHING/SUBSTITUTION
--  -----------------------------------------------------------------------  --

--  regular expression based string matching
function util.rmatch(str, regex, i, cf)
    return rex.new(regex, cf):tfind(str, i)
end

--  regular expression based string substitution
function util.rsubst(str, regex, subst, n, cf)
    return rex.gsub(str, regex, subst, n, cf)
end

--  regular expression based string splitting
function util.rsplit(str, regex, n, cf)
    local result = {}
    if str ~= "" then
        for section, _ in rex.split(str, regex, cf) do
            if n ~= nil then
                if n <= 0 then
                    break
                end
                n = n - 1
            end
            table.insert(result, section)
        end
    end
    return result
end

--  -----------------------------------------------------------------------  --
--                           RECURSIVE DATA DUMPING
--  -----------------------------------------------------------------------  --

--  WORK HORSE:
--  dump a single object recursively into a string
function util.dump_object(obj, compact)
    local dump = "<unknown>"
    if type(obj) == "nil" then
        dump = "nil"
    elseif type(obj) == "number" then
        dump = string.format("%d", obj)
    elseif type(obj) == "string" then
        local str = obj
        str = string.gsub(str, "\\\\", "\\\\")
        str = string.gsub(str, "\"", "\\\"")
        str = string.gsub(str, "\r", "\\r")
        str = string.gsub(str, "\n", "\\n")
        str = string.gsub(str, ".",
            function(c)
                local n = string.byte(c)
                if n < 32 or n >= 127 then
                    c = string.format("\\%03d", n)
                end
                return c
            end
        )
        dump = "\"" .. str .. "\""
    elseif type(obj) == "boolean" then
        if obj then
            dump = "true"
        else
            dump = "false"
        end
    elseif type(obj) == "table" then
        local space = " "
        if compact ~= nil and compact == true then
            space = ""
        end
        dump = "{"
        local first = true
        for k, v in pairs(obj) do
            if not first then
                dump = dump .. ","
            end
            if string.match(k, "^[a-zA-Z_][a-zA-Z0-9_]*$") == nil then
                k = "[" .. util.dump_object(k, true) .. "]"
            end
            dump = dump .. space .. k .. space .. "=" .. space
            dump = dump .. util.dump_object(v, compact)
            first = false
        end
        dump = dump .. space .. "}"
    elseif type(obj) == "function" then
        dump = "<function>"
    elseif type(obj) == "thread" then
        dump = "<thread>"
    elseif type(obj) == "userdata" then
        dump = "<userdata>"
    end
    return dump
end

--  CONVENIENCE FRONTEND:
--  dump one or more objects recursively, one per line
function util.dump(obj1, ...)
    local dump = util.dump_object(obj1) .. "\n"
    for _, obj in ipairs(arg) do
        dump = dump .. util.dump_object(obj) .. "\n"
    end
    return dump
end

--  CONVENIENCE FRONTEND:
--  dump one or more objects recursively, one per line, to stderr
function util.dump_stderr(obj1, ...)
    io.stderr:write(util.dump(obj1, unpack(arg)))
end

--  -----------------------------------------------------------------------  --
--                          TEXT FORMATTING
--  -----------------------------------------------------------------------  --

function util.textwrap(prefix, text, col_min, col_max)
    if col_min == nil then col_min = 78      end
    if col_max == nil then col_max = col_min end
    text = text .. " "
    text = util.rsubst(text, "(?s)\\r?\\n[ \\t]*", " ")
    text = util.rsubst(text,
        "(?s)(" ..
            ".{" .. col_min .. "," .. col_max .. "}?([ \\t])" ..
        "|" ..
            ".{" .. col_max .. "}" ..
        ")",
        function (s, ws)
            if ws then
                return s .. "\n"
            else
                return s .. "-\n"
            end
        end
    )
    text = util.rsubst(text, "(?s)([ \\t]*)$", "")
    text = util.rsubst(text, "(?s)([^\\n])$", "%1\n")
    text = util.rsubst(text, "(?m)^", prefix)
    return text
end

--  -----------------------------------------------------------------------  --
--                          I/O UTILITIES
--  -----------------------------------------------------------------------  --

--  slurp in all data of a file
function util.slurp(filename)
    local data = nil
    if filename ~= nil then
        local fp, error = io.open(filename, "r")
        if fp ~= nil then
            data = fp:read("*all")
            io.close(fp)
        end
    end
    return data
end

--  -----------------------------------------------------------------------  --
--                   INTEGRITY PROCESSOR GLUE CODE
--  -----------------------------------------------------------------------  --

--  provide additional "integrity" namespace
integrity = {}

--  extract plaintext from a PGP clearsigned text
function integrity.plaintext(text)
    local s, _, m = util.rmatch(
        text,
        "(?s)" ..
        "^\\s*" ..
        "-----BEGIN PGP SIGNED MESSAGE-----" ..
        ".*?" ..
        "\\r?\\n" ..
        "\\r?\\n" ..
        "(.+?\\r?\\n)" ..
        "-----BEGIN PGP SIGNATURE-----" ..
        "\\r?\\n" ..
        ".*" ..
        "$"
    )
    if s == nil then
        return nil
    else
        local plaintext = m[1]
        plaintext = util.rsubst(plaintext, "(?m)^- ", "")
        return plaintext
    end
end

--  integrity processor
function integrity.processor(progname, progmode, spec_fn, spec_cfg, proc_fn, proc_lua, proc_minver)
    --  optionally read integrity configuration specification
    if spec_cfg == nil then
        local fp, error = io.open(spec_fn, "r")
        if fp == nil then
            return "ERROR: unable to read Integrity Configuration Specification file: " .. error
        end
        spec_cfg = fp:read("*all")
        io.close(fp)
    end

    --  optionally read integrity validation processor
    if proc_lua == nil then
        local fp, error = io.open(proc_fn, "r")
        if fp == nil then
            return "ERROR: unable to read Integrity Validation Processor file: " .. error
        end
        proc_lua = fp:read("*all")
        io.close(fp)
    end

    --  extract plaintext from "clearsigned" integrity configuration specification
    spec_cfg = integrity.plaintext(spec_cfg)
    if spec_cfg == nil then
        return "ERROR: failed to extract Integrity Configuration Specification plaintext from clearsigned text"
    end

    --  extract plaintext from "clearsigned" integrity validation processor
    proc_lua = integrity.plaintext(proc_lua)
    if proc_lua == nil then
        return "ERROR: failed to extract Integrity Validation Processor plaintext from clearsigned text"
    end

    --  parse integrity configuration specification
    local cfg = {}
    util.rsubst(spec_cfg,
        "(?s)" ..
        "([A-Za-z0-9][A-Za-z0-9-]*):[ \\t]*" ..
        "([^\\r\\n]*" ..
        "(?:\\r?\\n(?:[ \\t]*|[ \\t]+[^\\r\\n]+))*" ..
        ")\\r?\\n",
        function (key, value)
            value = util.rsubst(value, "(?s)^\\s*(.*?)\\s*$", "%1")
            cfg[key] = value
        end
    )

    --  create validation context object
    local ctx = {}
    ctx.rpm = {}
    ctx.rpm.name = progname;
    ctx.rpm.mode = progmode;

    --  load integrity validation processor
    integrity.version = nil
    integrity.validate = nil
    local f, e = loadstring(proc_lua)
    if f == nil then
        return "ERROR: failed to load Integrity Validation Processor (syntax problem):\n" ..
               "rpm: Lua: " .. e
    end
    f();
    if type(integrity.validate) ~= "function" then
        return "ERROR: failed to load Integrity Validation Processor (semantic problem)"
    end

    --  ensure that integrity validation processor is at least of the minimum required version
    if type(integrity.version) ~= "string" then
        return "ERROR: invalid Integrity Validation Processor version (syntax problem)"
    end
    if rpm.vercmp(integrity.version, proc_minver) < 0 then
        return "ERROR: too old Integrity Validation Processor version (semantic problem)"
    end

    --  call integrity validation processor
    local result = integrity.validate(ctx, cfg)

    --  return validation result
    if type(result) ~= "string" then
        result = "ERROR: invalid result from Integrity Validation Processor"
    end
    return result
end

