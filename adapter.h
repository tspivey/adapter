#ifndef ADAPTER_H
#define ADAPTER_H

extern lua_State *l;

LUALIB_API int luaopen_acs(lua_State *l);

void say(const char *str, int interrupt);
void say1widechar(unsigned int ourchar);
void readline(void);
void contread(void);
void word(void);
void silence(void);

#endif
