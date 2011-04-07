bit = require "bit"
require "constants"
disabled=false
echoing = false
screenmode=false
clipboard = {}
require "functions"
f1=59
f2=60
f3=61
SS_PLAIN=0x10
SS_CTRL=4
disabled_consoles={}
reg(f1+5, SS_PLAIN, function()
if not clicks then clicks = true else clicks = false end
acs.clicks("tty", clicks)
acs.clicks("tone", clicks)
end)
acs.configure("adapter.cfg")
register_custom_command("saytime", true, function()
-- returns something like 00 03 on Tuesday March 29 2011
local t = os.date("%H %M on %A %B %d %Y")
acs.say(t)
end)
function setDisabled(x)
disabled_consoles[curcon] = x
if disabled == x then return end
disabled=x
if x then
acs.reset_configure()
acs.setkey(127, SS_PLAIN)
else
acs.configure("adapter.cfg")
register()
end -- if
end -- setDisabled
function disable()
if disabled then
setDisabled(false)
else
setDisabled(true)
end
acs.clicks("tty", not disabled)
acs.clicks("tone", disabled)
end -- disable
function conswitch(c)
curcon=c
disabled_consoles[c] = disabled_consoles[c] or false
if disabled_consoles[c] then
setDisabled(true)
else
setDisabled(false)
end
acs.clicks("tty", not disabled)
end
custom_commands.markleft = function()
local buf,cursor = acs.getbuf()
clipboard.left = cursor+1
end
custom_commands.markright = function ()
local buf,cursor = acs.getbuf()
clipboard.text = string.sub(buf, clipboard.left, cursor+1)
acs.clicks("tone", true)
end
register_custom_command("clipboard", true, function()
if clipboard.text == nil then
acs.clicks("high",0)
else
acs.say(clipboard.text, 0)
end
end) -- clipboard
register_custom_command("character", true, function()
acs.say(acs.char())
end) -- character
custom_commands.paste = function()
if clipboard.text == nil or clipboard.text == "" then
acs.clicks("tone", 0)
else
--Don't do anything, the clicks should be enough.
acs.injectstring(clipboard.text)
end -- if
end
register_custom_command("parameter", false, function()
acs.clicks("click", false)
char = acs.get1char()
if char == "r" then
acs.say("rate", 0)
local status, str = acs.keystring()
if status == nil or tonumber(str) == nil then
acs.clicks("bell", false)
return
end -- if invalid argument
local rate = tonumber(str)
if rate < 80 then rate = 80 end
if rate > 1000 then rate = 1000 end
acs.espeak_SetParameter(1, rate, 0)
acs.clicks("tone", true)
elseif char == "e" then
echoing = not echoing
acs.clicks("tone", echoing)
elseif char == "s" then
screenmode = not screenmode
acs.screenmode(screenmode)
acs.clicks("tone", screenmode)
else -- unknown key
acs.clicks("bell",false)
end -- if
end) -- parameter
-- compose (applications key, usually to the left of right control) disables us.
reg(127, SS_PLAIN, disable)
-- control+enter silences
reg(28, SS_CTRL, acs.silence)
function morechars(echo, c)
if not echoing or echo ~= 1 then return end
s = acs.getpunc(c)
if s == "" then
s = string.char(c)
end
acs.say(s, 1)
end -- morechars
register()
