-- if one of these fails, I probably don't want my cursor moved. Which commands
-- start reading, so we can sync then, because they're unlikely to fail.
starters = {line=true, word=true}
custom_commands = {}
-- taken from http://lua-users.org/wiki/SplitJoin
function string:split(sep)
        local sep, fields = sep or ":", {}
        local pattern = string.format("([^%s]+)", sep)
        self:gsub(pattern, function(c) fields[#fields+1] = c end)
        return fields
end
keys = {}
keyfuncs = {}
function reg(key, ss, func)
keys[key] = keys[key] or 0
keyfuncs[key] = keyfuncs[key] or {}
keys[key] = bit.bor((keys[key] or 0), ss)
keyfuncs[ss] = keyfuncs[ss] or {}
keyfuncs[ss][key] = func
end
function handle(key, ss, leds)
--acs.say("handling "..key)
if not keyfuncs[ss] then return end
if not keyfuncs[ss][key] then return end
keyfuncs[ss][key]()
end
function register()
for k,v in pairs(keys) do acs.setkey(k, v) end
end
-- handle config file commands, such as f2 readline.
-- We'll be passed the readline, so do something with it.
function handle_config(line)
local commands = line:split(" ")
acs.cursorset()
for k,v in ipairs(commands) do
if acs[v] == nil  and custom_commands[v] == nil then
acs.say("unknown function "..v)
return
end -- if
local cmd = (acs[v] or custom_commands[v])
-- stop after executing a read command. If I want to send 2 or 3 things to my synth, I'll do it in lua.
if starters[v] then
acs.cursorsync()
cmd()
return
elseif cmd() == 0 then
acs.clicks("high",0)
return
end -- if
end -- for
acs.clicks("click",0)
end
function register_custom_command(name, starter, func)
starters[name]=starter
custom_commands[name] = func
end
