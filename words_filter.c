#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "words_filter.h"

static wtoken *inserttoken(wtoken *tok,char c)     
{
	wtoken *child = calloc(1,sizeof(*child));
	child->code = c;
	if(tok->children_size == 0){
		tok->children = calloc(tok->children_size+1,sizeof(child));
		tok->children[0] = child;
	}else{
		wtoken **tmp = calloc(tok->children_size+1,sizeof(*tmp));
		int i = 0;
		int flag = 0;
		for(; i < tok->children_size; ++i){
			if(!flag && tok->children[i]->code > c){
				tmp[i] = child;
				flag = 1;
			}else
				tmp[i] = tok->children[i];
		}
		if(!flag) 
			tmp[tok->children_size] = child;
		else
			tmp[tok->children_size] = tok->children[tok->children_size-1];
		free(tok->children);
		tok->children = tmp;
	}
	tok->children_size++;
	return child;	
}     

static wtoken *getchild(wtoken *tok,char c)     
{   
	
	if(!tok->children_size) return NULL;
	int left = 0;
	int right = tok->children_size - 1;
	for( ; ; )
	{
		if(right - left <= 0)
			return tok->children[left]->code == c ? tok->children[left]:NULL; 
		int index = (right - left)/2 + left;
		if(tok->children[index]->code == c)
			return tok->children[index];
		else if(tok->children[index]->code > c)
			right = index-1;
		else
			left = index+1;
	} 
}


static wtoken *addchild(wtoken *tok,char c){
	wtoken *child = getchild(tok,c);
	if(!child)
		return inserttoken(tok,c);
	return child;
}

static void next_char(wtoken *tok,const char *str,int i,int *maxmatch,char *out,int *out_pos)     
{ 
	if(str[i] == 0) return;      
    wtoken *childtok = getchild(tok,str[i]);  
    if(childtok)     
    {   
    	if(out) {
    		out[(*out_pos)++] = str[i];
    	}  
        if(childtok->end)     
            *maxmatch = i + 1;     
        next_char(childtok,str,i+1,maxmatch,out,out_pos);     
    }
	else{
		if(tok->end)
			*maxmatch = i;
	}
}   


static int process_words(words_filter *filter,const char *str,int *pos,char *out,int *out_pos)     
{   
	wtoken *tok = filter->tokarry[(uint8_t)str[*pos]];
	if(out) {
		out[(*out_pos)++] = str[*pos];
	}
	if(tok == NULL)
	{
		(*pos) += 1;
		return 0;
	}else{
		int maxmatch = 0;     
        next_char(tok,str,(*pos)+1,&maxmatch,out,out_pos);                      
        if(maxmatch == 0)     
        {     
            (*pos) += 1;
			if(tok->end)
				return 1;
            return 0;     
        }     
        else     
        {     
            (*pos) = maxmatch;     
            return 1;     
        }   
	}
	return 0;
}

int  words_filter_init(words_filter *filter,const char **invaild_words) {
	if(!filter || !invaild_words) {
		return -1;
	}
	memset(filter,0,sizeof(*filter));
	int i,j;
	for( i = 0; invaild_words[i] != NULL; ++i){
		const char *str = invaild_words[i];
		int size = strlen(str);
		wtoken *tok = filter->tokarry[(uint8_t)str[0]];
		if(!tok){
			tok = calloc(1,sizeof(*tok));
			tok->code = str[0];
			filter->tokarry[(uint8_t)str[0]] = tok;
		} 
		for(j = 1; j < size; ++j)     
			tok = addchild(tok,str[j]);
		tok->end = 1; 
	}
	return 0;
}

static void free_wtoken(wtoken *tok) {
	if(tok) {
		if(tok->children_size > 0) {
			int i;
			for(i = 0; i < tok->children_size; ++i) {
				free_wtoken(tok->children[i]);
			}
			free(tok->children);
		}
		free(tok);
	}
}

void words_filter_finalize(words_filter *filter) {
	if(filter) {
		int i;
		for( i = 0; i < sizeof(filter->tokarry)/sizeof(*filter->tokarry); ++i) {
			if(filter->tokarry[i]) {
				free_wtoken(filter->tokarry[i]);
			}
		}
	}
}

int  words_check(words_filter *filter,const char *str,size_t size) {
	if(!filter || !str) {
		return -1;
	}
	int i = 0;
    for(; i < size;)     
    {       
        if(process_words(filter,str,&i,NULL,NULL)){
        	return -1;
		}
    }
    return 0;
}

int  words_filtrate(words_filter *filter,const char *str,size_t str_size,char replace,char *out,size_t *out_size) {
	if(!filter || !str || !out || !out_size) {
		return -1;
	}

	if(str_size > *out_size) {
		return -1;
	}

	int i,j;	
	for(i = 0,j = 0; i < str_size;)     
    {   
    	int o = i;
    	if(process_words(filter,str,&i,out,&j)){
    		out[o] = replace;
    		j = o + 1;
    	}
    }
    if(j < *out_size){
    	out[j] = 0;
    }
    *out_size = j;
    return 0;
}


#ifdef _LUA

#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>

#define SET_FUNCTION(L,NAME,FUNC) do{\
	lua_pushstring(L,NAME);\
	lua_pushcfunction(L,FUNC);\
	lua_settable(L, -3);\
}while(0)

#define LUA_NEWUSERDATA(L,TYPE)   					({\
	TYPE *ret = lua_newuserdata(L, sizeof(TYPE));	  \
	if(ret) memset(ret,0,sizeof(TYPE));				  \
	ret;})

#define WORDS_FILTER_METATABLE "lua_words_filter"

#define lua_check_words_filter(L,I)(words_filter*)luaL_checkudata(L,I,WORDS_FILTER_METATABLE)

static int lua_words_filter_gc(lua_State *L) {
	words_filter *filter = lua_check_words_filter(L,1);
	words_filter_finalize(filter);
	return 0;
}

static int lua_new_words_filter(lua_State *L) {
	if(lua_type(L, -1) != LUA_TTABLE) {
		lua_pushnil(L);
		lua_pushstring(L,"missing forbid_words table");
		return 2;
	}

	words_filter *filter = LUA_NEWUSERDATA(L,words_filter);
	if(!filter){
		lua_pushnil(L);
		lua_pushstring(L,"new words_filter failed");
		return 1;		
	}

	memset(filter,0,sizeof(*filter));

	int top = lua_gettop(L);

	lua_pushnil(L);

	for(;;){		
		if(!lua_next(L,-3)){
			break;
		}

		if(lua_type(L,-1) == LUA_TSTRING) {
			size_t size,j;
			const char *str = lua_tolstring(L,-1,&size);
			wtoken *tok = filter->tokarry[(uint8_t)str[0]];
			if(!tok){
				tok = calloc(1,sizeof(*tok));
				tok->code = str[0];
				filter->tokarry[(uint8_t)str[0]] = tok;
			} 
			for(j = 1; j < size; ++j)     
				tok = addchild(tok,str[j]);
			tok->end = 1;

		}
		lua_pop(L,1);
	}

	lua_settop(L,top);
	luaL_getmetatable(L, WORDS_FILTER_METATABLE);
	lua_setmetatable(L, -2);
	return 1;
}


static int lua_check(lua_State *L) {
	words_filter *filter = lua_check_words_filter(L,1);
	size_t size;
	const char *str = lua_tolstring(L,2,&size);
	if(NULL == str) {
		lua_pushnil(L);
		lua_pushstring(L,"missing str");
		return 2;
	}

	if(words_check(filter,str,size) == 0) {
		lua_pushboolean(L,1);
	}else {
		lua_pushboolean(L,0);
	}
	return 1;
}

static int lua_filtrate(lua_State *L) {
	static char out[65535*4];
	out[0] = 0;
	words_filter *filter = lua_check_words_filter(L,1);
	char r = '*';
	size_t size,replace_size,out_size;
	const char *str = lua_tolstring(L,2,&size);
	if(NULL == str) {
		lua_pushnil(L);
		lua_pushstring(L,"missing str");
		return 2;
	}
	const char *replace = lua_tolstring(L,3,&replace_size);
	if(replace && replace_size > 0) {
		r = replace[0];
	}
	out_size = sizeof(out);
	if(0 == words_filtrate(filter,str,size,r,out,&out_size)) {
		lua_pushlstring(L,out,out_size);
		return 1;
	} else {
		lua_pushnil(L);
		lua_pushstring(L,"filtrate error");
		return 2;
	}
}


int32_t luaopen_words_filter(lua_State *L)
{

	luaL_Reg _mt[] = {
		{"__gc", lua_words_filter_gc},
		{NULL, NULL}
	};

	luaL_Reg _methods[] = {
		{"Filtrate",   lua_filtrate},
		{"Check",   lua_check},
		{NULL,     NULL}
	};

	luaL_newmetatable(L, WORDS_FILTER_METATABLE);

	luaL_setfuncs(L, _mt, 0);

	luaL_newlib(L, _methods);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	lua_newtable(L);
	SET_FUNCTION(L,"WordsFilter",lua_new_words_filter);
	return 1;
}


#endif