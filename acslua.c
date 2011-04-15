#include <lauxlib.h>
#include <string.h>
#include <espeak/speak_lib.h>

#include "acsbridge.h"
#include "adapter.h"

static int say_l(lua_State *l)
{
const char *text = luaL_checkstring(l, 1);
int interrupt = luaL_optint(l, 2, 0);
say(text, interrupt);
return 0;
}

static int setkey(lua_State *l)
{
int key = luaL_checkint(l, 1);
int ss = luaL_checkint(l, 2);
acs_setkey(key, ss);
return 0;
}

static int clicks(lua_State *l)
{
const char *s = luaL_checkstring(l, 1);
int enabled = 0;
luaL_checkany(l, 2);
enabled = lua_toboolean(l, 2);
if (!strcmp(s, "tty"))
acs_tty_clicks(enabled);
else if (!strcmp(s, "tone"))
acs_tone_onoff(enabled);
else if (!strcmp(s, "high"))
acs_highbeeps();
else if (!strcmp(s, "click"))
acs_click();
else if (!strcmp(s, "bell"))
acs_bell();
else if (!strcmp(s, "cr"))
acs_cr();
else
luaL_error(l, "invalid format");
return 0;
}

static int readline_l (lua_State *l)
{
readline();
return 0;
}

static int word_l (lua_State *l)
{
	word();
	return 0;
}

static int test_configure(lua_State *l)
{
FILE *f;
char line[200];
char *s;
int lineno;
const char *filename;
filename = luaL_checkstring(l, 1);

f = fopen(filename, "r");
if(!f) {
fprintf(stderr, "cannot open config file %s\n", filename);
return 0;
}

lineno = 0;
while(fgets(line, sizeof(line), f)) {
++lineno;

// strip off crlf
s = line + strlen(line);
if(s > line && s[-1] == '\n') --s;
if(s > line && s[-1] == '\r') --s;
*s = 0;

if(acs_line_configure(line, NULL))
fprintf(stderr, "syntax error in line %d\n", lineno);
}
fclose(f);
return 0;
}

static int wrap_acs_back (lua_State *l)
{
int r = acs_back();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_forward (lua_State *l)
{
int r = acs_forward();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_prevline (lua_State *l)
{
int r = acs_prevline();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_nextline (lua_State *l)
{
int r = acs_nextline();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_startline (lua_State *l)
{
int r = acs_startline();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_endline (lua_State *l)
{
int r = acs_endline();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_startword (lua_State *l)
{
int r = acs_startword();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_endword (lua_State *l)
{
int r = acs_endword();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_prevword (lua_State *l)
{
int r = acs_prevword();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_nextword (lua_State *l)
{
int r = acs_nextword();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_bypass (lua_State *l)
{
int r = acs_bypass();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_cursorset (lua_State *l)
{
	acs_cursorset();
	return 0;
}

static int wrap_acs_cursorsync (lua_State *l)
{
	acs_cursorsync();
	return 0;
}

static int wrap_acs_lspc (lua_State *l)
{
	acs_lspc();
	return 0;
}

static int wrap_acs_rspc (lua_State *l)
{
	acs_rspc();
	return 0;
}

static int wrap_acs_refresh (lua_State *l)
{
int r = acs_refresh();
lua_pushnumber(l, r);
return 1;
}

static int wrap_acs_reset_configure (lua_State *l)
{
	acs_reset_configure();
	return 0;
}

static int wrap_acs_startbuf (lua_State *l)
{
	acs_startbuf();
	return 0;
}

static int wrap_acs_endbuf (lua_State *l)
{
	acs_endbuf();
	return 0;
}

static int wrap_silence (lua_State *l)
{
	silence();
	return 0;
}

static int wrap_acs_getc(lua_State *l)
{
char buf[2];
char c = acs_getc();
sprintf(buf, "%c", c);
lua_pushstring(l, buf);
return 1;
}

/*
 * Return the buffer, and the cursor position.
 * I don't know how well this will actually work, because we're passing
 * in the entire buffer when we may only need a part of it.
 */
static int getbuf(lua_State *l)
{
lua_pushlstring(l, (const char *) rb->start, rb->end-rb->start);
lua_pushinteger(l, rb->cursor-rb->start);
return 2;
}

static int wrap_acs_keystring(lua_State *l)
{
char buf[1024];
buf[0] = '\0';
int properties = luaL_optint(l, 1, ACS_KS_DEFAULT);
if (acs_keystring(buf, 1024, properties) == -1) {
lua_pushboolean(l, 0);
} else {
lua_pushboolean(l, 1);
}
lua_pushstring(l, (const char *) buf);
return 2;
}

static int character(lua_State *l)
{
char buf[100];
char *p;
char c;
c = acs_getc();
p = acs_getpunc(c);
if (p) {
sprintf(buf, "%s", p);
} else {
sprintf(buf, "%c", c);
}
lua_pushstring(l, buf);
return 1;
}

static int wrap_acs_injectstring(lua_State *l)
{
const char *s = luaL_checkstring(l, 1);
acs_injectstring(s);
return 0;
}

static int get1key(lua_State *l)
{
int key, ss;
int result;
result = acs_get1key(&key, &ss);
if (result == -1) key = -1;
lua_pushinteger(l, key);
lua_pushinteger(l, ss);
return 2;
}
static int get1char(lua_State *l)
{
char c;
char buf[2];
if (acs_get1char(&c) == -1) {
lua_pushnil(l);
}
else {
sprintf(buf, "%c", c);
lua_pushstring(l, buf);
}
return 1;
}

static int wrap_espeak_SetParameter(lua_State *l)
{
int parameter = luaL_checkint(l, 1);
int value = luaL_checkint(l, 2);
int rel = luaL_optint(l, 3, 0);
espeak_SetParameter(parameter, value, rel);
return 0;
}

static int wrap_contread(lua_State *l)
{
	contread();
	return 0;
}
static int wrap_acs_getpunc(lua_State *l)
{
char *p;
int i;
i = luaL_checkint(l, 1);
p = acs_getpunc((char)i);
if (!p) {
lua_pushstring(l, "");
} else {
lua_pushstring(l, p);
}
return 1;
}

static int wrap_acs_screenmode(lua_State *l)
{
luaL_checkany(l, 1);
int enabled = lua_toboolean(l, 1);
acs_screenmode(enabled);
acs_refresh();
if (rb->cursor == NULL)
rb->cursor = rb->v_cursor;
return 0;
}

static const struct luaL_Reg acs[] = {
	{ "setkey", setkey, },
	{ "clicks", clicks, },
	{ "back", wrap_acs_back, },
	{ "forward", wrap_acs_forward, },
	{ "prevline", wrap_acs_prevline, },
	{ "nextline", wrap_acs_nextline, },
	{ "startline", wrap_acs_startline, },
	{ "endline", wrap_acs_endline, },
	{ "startword", wrap_acs_startword, },
	{ "endword", wrap_acs_endword, },
	{ "prevword", wrap_acs_prevword, },
	{ "nextword", wrap_acs_nextword, },
	{ "startbuf", wrap_acs_startbuf, },
	{ "endbuf", wrap_acs_endbuf, },
	{ "cursorset", wrap_acs_cursorset, },
	{ "cursorsync", wrap_acs_cursorsync, },
	{ "reset_configure", wrap_acs_reset_configure, },
	{ "bypass", wrap_acs_bypass, },
	{ "keystring", wrap_acs_keystring, },
	{ "lspc", wrap_acs_lspc, },
	{ "rspc", wrap_acs_rspc, },
	{ "refresh", wrap_acs_refresh, },
	{ "char", character, },
	{ "line", readline_l, },
	{ "word", word_l, },
	{ "configure", test_configure, },
	{ "say", say_l, },
	{ "silence", wrap_silence, },
	{ "getc", wrap_acs_getc, },
	{ "getbuf", getbuf, },
	{ "injectstring", wrap_acs_injectstring, },
	{ "get1key", get1key, },
	{ "get1char", get1char, },
	{ "espeak_SetParameter", wrap_espeak_SetParameter, },
	{ "contread", wrap_contread, },
{"getpunc", wrap_acs_getpunc, },
{"screenmode", wrap_acs_screenmode},
	{ NULL, NULL },
};

LUALIB_API int luaopen_acs(lua_State *l)
{
luaL_register(l, "acs", acs);
return 1;
}
