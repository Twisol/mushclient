#include "stdafx.h"
#include <cstring>
#include "json/json.h"

static const char* json_metaname = "mushclient.json";

static json_object* encode_lua_data(lua_State* L);

static ptrdiff_t table_maxn(lua_State* L)
{
	// L:  ... table
	luaL_checkstack(L, 2, "Nested too deep!");

	ptrdiff_t max = 0;

	lua_pushnil(L); // first key
	// L: ... table, nil
	while (lua_next(L, -2) != 0)
	{
		// L: ... table, key, value
		lua_pop(L, 1); // don't need value
		// L: ... table, key
		if (lua_type(L, -1) == LUA_TNUMBER)
		{
			double dbl = lua_tonumber(L, -1);
			ptrdiff_t num = lua_tointeger(L, -1);

			if (num == dbl && num > max)
				max = num;
		}
	}
	// L: ... table

	return max;
}

// Returns 1 if it's a JSON array, 2 if it's a JSON object,
// 3 if it's invalid, and 0 for uncertain (empty table {})
static unsigned char lua_table_json_type(lua_State* L)
{
	// L: ... table
	luaL_checkstack(L, 2, "Nested too deep!");

	unsigned int numArrayEntries = 0;
	unsigned char type = 0;
	int ltype;

	lua_pushnil(L); // first key
	// L: ... table, nil
	while (lua_next(L, -2) != 0)
	{
		// L: .. table, key, value
		lua_pop(L, 1); // don't need the value
		// L: .. table, key
		ltype = lua_type(L, -1);

		if (type != 2 && ltype == LUA_TNUMBER)
		{
			if (type != 1) type = 1;
		}
		else if (type != 1 && ltype == LUA_TSTRING)
		{
			if (type != 2) type = 2;
		}
		else
		{
			type = 3;
			break;
		}
	}
	// L: ... table

	return type;
}

static json_object* encode_lua_table_array(lua_State* L)
{
	// L: ... table

	luaL_checkstack(L, 2, "Nested too deep!");

	json_object* obj = json_object_new_array();
	json_object* member = NULL;

	for (ptrdiff_t i = 1; i <= table_maxn(L); ++i)
	{
		lua_pushnumber(L, i);
		// L: ... table, index
		lua_gettable(L, -2);
		// L: ... table, value

		member = encode_lua_data(L);
		json_object_array_add(obj, member);

		lua_pop(L, 1);
		// L: ... table
	}

	return obj;
}

static json_object* encode_lua_table_object(lua_State* L)
{
	// L: ... table

	luaL_checkstack(L, 2, "Nested too deep!");

	json_object* obj = json_object_new_object();
	json_object* member = NULL;

	lua_pushnil(L); // first key
	// L: ... table, nil
	while (lua_next(L, -2) != 0)
	{
		// L: ... table, key, value
		if (lua_type(L, -2) == LUA_TSTRING)
		{
			member = encode_lua_data(L);
			json_object_object_add(obj, lua_tostring(L, -2), member);
		}
		lua_pop(L, 1);
		// L: ... table, key
	}
	// L: ... table

	return obj;
}

static json_object* encode_lua_table(lua_State* L)
{
	// L: ... table

	json_object* obj = NULL;

	switch (lua_table_json_type(L))
	{
		case 0:
			luaL_error(L, "Ambiguity: empty table could be either array or object.");
			break;
		case 1:
			obj = encode_lua_table_array(L);
			break;
		case 2:
			obj = encode_lua_table_object(L);
			break;
		case 3:
			luaL_error(L, "Unable to convert mixed table!");
			break;
	}

	return obj;
}

static json_object* encode_lua_json_udata(lua_State* L)
{
	json_object* obj = *((json_object**)lua_touserdata(L, -1));

	// increment the reference count
	json_object_get(obj);

	// return the same object
	return obj;
}

static json_object* encode_lua_number(lua_State* L)
{
	// L: ... number
	double dbl = lua_tonumber(L, -1);
	ptrdiff_t num = lua_tointeger(L, -1);

	if (num == dbl)
		return json_object_new_int(num);
	else
		return json_object_new_double(dbl);
}

static json_object* encode_lua_string(lua_State* L)
{
	// L: ... string
	return json_object_new_string(lua_tostring(L, -1));
}

static json_object* encode_lua_boolean(lua_State* L)
{
	// L: ... bool
	return json_object_new_boolean(lua_toboolean(L, -1));
}

static json_object* encode_lua_nil(lua_State* L)
{
	// L: ... nil
	return NULL;
}

static json_object* encode_lua_data(lua_State* L)
{
	// L: ... item

	int ltype = lua_type(L, -1);
	switch (ltype)
	{
		case LUA_TTABLE:
			return encode_lua_table(L);
		case LUA_TUSERDATA:
		{
			luaL_getmetatable(L, json_metaname);
			lua_getmetatable(L, -2);
			bool is_json = lua_rawequal(L, -1, -2);
			lua_pop(L, 2);
			
			if (is_json)
				return encode_lua_json_udata(L);
			else
				break;
		}
		case LUA_TNUMBER:
			return encode_lua_number(L);
		case LUA_TSTRING:
			return encode_lua_string(L);
		case LUA_TBOOLEAN:
			return encode_lua_boolean(L);
		case LUA_TNIL:
			return encode_lua_nil(L);
	}

	luaL_error(L, "Invalid data type: %s", lua_typename(L, ltype));
	return NULL; // never reached
}

static int Ljson_encode(lua_State* L)
{
	lua_settop(L, 1);
	// L: item
	json_object* obj = encode_lua_data(L);
	lua_pop(L, 1);
	// L: -

	json_object** ud = (json_object**)lua_newuserdata(L, sizeof(json_object*));
	// L: userdata
	luaL_getmetatable(L, json_metaname);
	lua_setmetatable(L, -2);
	*ud = obj;

	return 1;
}

static int Ljson_tostring(lua_State* L)
{
	json_object* obj = *((json_object**)luaL_checkudata(L, 1, json_metaname));
	lua_pushstring(L, json_object_to_json_string(obj));
	return 1;
}

static int Ljson_tolua(lua_State* L)
{
	return 0;
}

static int Ljson_gc_(lua_State* L)
{
	json_object* obj = *((json_object**)luaL_checkudata(L, 1, json_metaname));
	json_object_put(obj);

	return 0;
}

static const luaL_reg json_meta[] = {
	{"to_string", &Ljson_tostring},
	{"to_lua", &Ljson_tolua},
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
	createmeta(L, json_metaname);
	luaL_register(L, NULL, json_meta);
	lua_pop(L, 1);

	// register 'json' library
	luaL_register(L, "json", json_lib);

	// add "null" JSON item
	lua_pushstring(L, "null");
	json_object** ud = (json_object**)lua_newuserdata(L, sizeof(json_object*));
	luaL_getmetatable(L, json_metaname);
	lua_setmetatable(L, -2);
	*ud = NULL;
	lua_rawset(L, -3);

	return 1;
}