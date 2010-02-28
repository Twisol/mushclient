#include "stdafx.h"
#include <cmath>
#include "json/json.h"

static json_object* encode_lua_data(lua_State* L);

static json_object* encode_lua_table(lua_State* L)
{
	json_object* obj = NULL;

	obj = json_object_new_array();
	json_object_array_add(obj, json_object_new_string("Got table!"));

	return obj;
}

static json_object* encode_lua_number(lua_State* L)
{
	return json_object_new_double(lua_tonumber(L, -1));
}

static json_object* encode_lua_string(lua_State* L)
{
	return json_object_new_string(lua_tostring(L, -1));
}

static json_object* encode_lua_boolean(lua_State* L)
{
	return json_object_new_boolean(lua_toboolean(L, -1));
}

static json_object* encode_lua_nil(lua_State* L)
{
	return NULL;
}

static json_object* encode_lua_data(lua_State* L)
{
	int ltype = lua_type(L, 1);
	switch (ltype)
	{
		//case LUA_TTABLE:
		//	return encode_lua_table(L);
		case LUA_TNUMBER:
			return encode_lua_number(L);
		case LUA_TSTRING:
			return encode_lua_string(L);
		case LUA_TBOOLEAN:
			return encode_lua_boolean(L);
		case LUA_TNIL:
			return encode_lua_nil(L);
		default:
			luaL_error(L, "Invalid data type: %s", lua_typename(L, ltype));
			lua_error(L);
	}

	luaL_error(L, "Shouldn't be here!");
	return NULL; // never executed
}

static int Ljson_encode(lua_State* L)
{
	lua_settop(L, 1);
	const char* encoded = json_object_to_json_string(encode_lua_data(L));
	lua_pop(L, 1);
	lua_pushstring(L, encoded);

	return 1;
}

static const luaL_reg json_meta[] = {
	{NULL, NULL}
};

static const luaL_reg json_lib[] = {
	{"encode", &Ljson_encode},
	{NULL, NULL}
};

static void createmeta(lua_State *L, const char *name)
{
	luaL_newmetatable(L, name);   /* create new metatable */
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);         /* push metatable */
	lua_rawset(L, -3);            /* metatable.__index = metatable */
}

LUALIB_API int luaopen_json(lua_State *L)
{
	createmeta(L, "json-c");
	luaL_register(L, NULL, json_meta);
	lua_pop(L, 1);
	luaL_register(L, "json", json_lib);
	return 1;
}