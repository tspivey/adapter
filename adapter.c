#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wchar.h>
#include <fcntl.h>
#include <sys/select.h>

#include "acsbridge.h"
#include <espeak/speak_lib.h>
#include <lauxlib.h>
#include <lualib.h>
#include "adapter.h"

#define stringEqual !strcmp

static int rate=650;
// are we continuously reading?
static int reading=0;
static int pipes[2];
lua_State *l;

static int callback(short *wav, int numsamples, espeak_EVENT *events)
{
if (!reading) return 0;
// We're done reading this, so tell our event loop to keep going with the next thing.
//Not sure if we can call espeak within its own callback, but it doesn't sound like a good idea.
if (events->type == espeakEVENT_MSG_TERMINATED) {
write(pipes[1], "x", 1);
return 1;
} //if reading
return 0;
} //callback
// check for config file keys, then lua binds.
static void
keystroke(int key, int ss, int leds)
{
int mkcode = acs_build_mkcode(key, ss);
char *cmd = acs_getspeechcommand(mkcode);
if (!cmd) {
if (ss&ACS_SS_ALT) {
cmd = acs_getspeechcommand(acs_build_mkcode(key, ACS_SS_ALT));
}
}
if (rb->cursor == NULL) {
acs_endbuf();
acs_cursorsync();
}
if (cmd) {
lua_getfield(l, LUA_GLOBALSINDEX, "handle_config");
lua_pushstring(l, cmd);
lua_call(l, 1, 0);
return;
}
lua_getfield(l, LUA_GLOBALSINDEX, "handle");
lua_pushnumber(l, key);
lua_pushnumber(l, ss);
lua_pushnumber(l, leds);
lua_call(l, 3, 0);
} // keystroke
static void conswitch(void)
{
char fgs[40];
lua_getfield(l, LUA_GLOBALSINDEX, "conswitch");
lua_pushnumber(l, acs_fgc);
lua_call(l, 1, 0);
sprintf(fgs, "con %d", acs_fgc);
say(fgs, 1);
}
void morechars(int echo, unsigned int c)
{
lua_getfield(l, LUA_GLOBALSINDEX, "morechars");
if (lua_isnil(l, 1)) return;
lua_pushnumber(l, echo);
lua_pushnumber(l, c);
lua_call(l, 2, 0);
}

int
main(int argc, char **argv)
{
char nullbuf[10]; //null buffer, just for ignoring the output from the pipe
int daemonize = 0;
++argv, --argc;
acs_debug=0;

if(argc > 0 && stringEqual(argv[0], "-d")) {
acs_debug = 1;
++argv, --argc;
}
//This breaks. When espeak talks, everything locks up. Threads?
if(argc > 0 && stringEqual(argv[0], "-D")) {
daemonize = 1;
++argv, --argc;
}
l = luaL_newstate();
luaL_openlibs(l);
luaopen_acs(l);
if(acs_open("/dev/acsint") < 0) {
fprintf(stderr, "cannot open the driver /dev/acsint;\n\
did you make this character special major 11,\n\
and do you have permission to read and write it,\n\
and did you install acsint?\n");
exit(1);
}
acs_key_h = keystroke;
acs_fgc_h = conswitch;
acs_more_h = morechars;
acs_reset_configure();
if (daemonize) {
// don't chdir or close fds. I don't have the config file path stored.
daemon(1, 1);
}
if (!espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 100, NULL, 0)) {
printf("Error initializing espeak.\n");
exit(1);
}
int x = luaL_dofile(l, "init.lua");
if (x==1) {
const char *msg = lua_tostring(l, -1);
printf("%s\n", msg);
exit(1);
}
espeak_SetParameter(espeakRATE, rate, 0);
// match espeakup on pitch and volume.
espeak_SetParameter(espeakPITCH, 55, 0);
espeak_SetParameter(espeakVOLUME, 110, 0);
espeak_SetParameter(espeakPUNCTUATION, espeakPUNCT_ALL, 0);
espeak_SetSynthCallback(callback);
say("Adapter ready", 0);
// This runs forever, you have to hit interrupt to kill it,
// or kill it from another tty.
if (pipe(pipes) == -1) {
perror("pipe");
exit(1);
}
while(1) {
fd_set rfds;
FD_ZERO(&rfds);
FD_SET(acs_fd, &rfds);
FD_SET(pipes[0], &rfds);
// highest fd+1, I hope this works.
int nfds = pipes[0]+1;
if (select(nfds, &rfds, NULL, NULL, NULL) < 0) {
perror("select");
continue;
}
if (FD_ISSET(acs_fd, &rfds))
acs_events();
if (FD_ISSET(pipes[0], &rfds)) {
acs_cr();
//printf("done current\n");
read(pipes[0], nullbuf, 1);
// keep going
acs_cursorset();
if (acs_nextline()) {
acs_cursorsync();
readline();
} else {
reading=0;
} // next line
} // handle pipe
} // while

acs_close();
} // main

void ucsay(unsigned int *unicodes, int interrupt)
{
/* Flagrant cut-and-paste, because it was quick. */
if (interrupt) {
espeak_Cancel();
}
espeak_Synth(unicodes, (wcslen(unicodes)+1) * sizeof(unsigned int), 0, POS_CHARACTER, 0, espeakCHARS_WCHAR, NULL, NULL);
}

void say(const char *str, int interrupt)
{
if (interrupt) {
espeak_Cancel();
}
espeak_Synth(str, strlen(str)+1, 0, POS_CHARACTER, 0, 0, NULL, NULL);
}

/* Say a single wide (unicode) character: */
void say1widechar(unsigned int ourchar)
{
espeak_Char(ourchar);
}

/*Say to the end of the current line. */
void readline(void)
{
unsigned int *l;
unsigned int *start = rb->cursor; //old cursor position
int size;
int ucsize;
acs_cursorset();
if (!acs_endline()) goto error;
acs_cursorsync();
size=(rb->cursor-start)+1;
ucsize = size * sizeof(unsigned int);
l = malloc(ucsize + sizeof(unsigned int));
memset(l, 0, ucsize + sizeof(unsigned int));
memcpy(l, start, ucsize);
ucsay(l, 1);
free(l);
return;
error:
acs_highbeeps();
return;
}
/* copypasta from readline */
void word(void)
{
unsigned int *l;
unsigned int *start = rb->cursor; //old cursor position
int size;
int ucsize;
acs_cursorset();
if (!acs_endword()) goto error;
acs_cursorsync();
size=(rb->cursor-start)+1;
ucsize = size * sizeof(unsigned int);
l = malloc(ucsize+sizeof(unsigned int));
memset(l, 0, ucsize + sizeof(unsigned int));
memcpy(l, start, ucsize);
ucsay(l, 1);
free(l);
return;
error:
acs_highbeeps();
return;
}
void silence(void)
{
reading=0;
espeak_Cancel();
}
void contread(void)
{
if ((rb->start == rb->end) || //buffer empty
(rb->cursor >= rb->end-1) // end of buffer
) {
acs_highbeeps();
return;
} //if error
reading=1;
readline();
}
