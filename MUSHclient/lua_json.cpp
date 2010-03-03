#include "stdafx.h"
#include "json/json.h"

// Metatable identifier in the Lua registry
static const char* json_metaname = "mushclient.json";

// JSON null value
static json_object** json_null = NULL;

/***************************************************
  Private utility methods
***************************************************/

// Calculates the largest whole-number index of a table.
static lua_Integer table_maxn(lua_State* L, int stack_idx)
{
	// L:  ...
	luaL_checkstack(L, 2, "Nested too deep!");

	lua_Integer max = 0, num = 0;
	lua_Number dbl = 0;

	lua_pushnil(L); // first key
	if (stack_idx < 0) --stack_idx; // adjust table index
	// L: ... nil
	while (lua_next(L, stack_idx) != 0)
	{
		// L: ... key, value
		lua_pop(L, 1); // don't need value
		// L: ... key
		if (lua_type(L, -1) == LUA_TNUMBER)
		{
			dbl = lua_tonumber(L, -1);
			num = lua_tointeger(L, -1);

			if (num == dbl && num > max)
				max = num;
		}
	}
	// L: ...

	return max;
}

// Pushes a JSON object to the Lua stack
static void push_json_udata(lua_State* L, json_object* obj)
{
	json_object** ud = NULL;
	if (obj == NULL)
	{
		lua_pushlightuserdata(L, json_null);
		lua_gettable(L, LUA_REGISTRYINDEX);
	}
	else
	{
		ud = (json_object**)lua_newuserdata(L, sizeof(json_object*));
		luaL_getmetatable(L, json_metaname);
		lua_setmetatable(L, -2);
		*ud = obj;
	}
}

/***************************************************
  JSON encoding implementation
***************************************************/

static json_object* encode_lua_data(lua_State* L);

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

	// Push it to the stack in case the Lua stack overflows while inserting members.
	push_json_udata(L, obj);
	// L: ... table, udata

	for (lua_Integer i = 1; i <= table_maxn(L, -2); ++i)
	{
		lua_pushnumber(L, i);
		// L: ... table, udata, index
		lua_gettable(L, -3);
		// L: ... table, udata, value

		json_object_array_add(obj, encode_lua_data(L));

		lua_pop(L, 1);
		// L: ... table, udata
	}

	json_object_get(obj); // add a reference for when it's GC'd
	lua_pop(L, 1);
	// L: ... table

	return obj;
}

static json_object* encode_lua_table_object(lua_State* L)
{
	// L: ... table

	luaL_checkstack(L, 2, "Nested too deep!");

	json_object* obj = json_object_new_object();
	json_object* member = NULL;

	// Push it to the stack in case the Lua stack overflows while inserting members.
	push_json_udata(L, obj);
	// L: ... table, udata

	lua_pushnil(L); // first key
	// L: ... table, udata, nil
	while (lua_next(L, -3) != 0)
	{
		// L: ... table, udata, key, value
		if (lua_type(L, -2) == LUA_TSTRING)
		{
			member = encode_lua_data(L);
			json_object_object_add(obj, lua_tostring(L, -2), member);
		}
		lua_pop(L, 1);
		// L: ... table, udata, key
	}
	// L: ... table, udata

	json_object_get(obj); // add a reference for when it's GC'd
	lua_pop(L, 1);
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
	lua_Number dbl = lua_tonumber(L, -1);
	lua_Integer num = lua_tointeger(L, -1);

	if (num == dbl)
		return json_object_new_int(num);
	else
		return json_object_new_double(dbl);
}

static json_object* encode_lua_data(lua_State* L)
{
	// L: ... item

	int ltype = lua_type(L, -1);
	switch (ltype)
	{
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
		case LUA_TTABLE:
			return encode_lua_table(L);
		case LUA_TNUMBER:
			return encode_lua_number(L);
		case LUA_TSTRING:
			return json_object_new_string(lua_tostring(L, -1));
		case LUA_TBOOLEAN:
			return json_object_new_boolean(lua_toboolean(L, -1));
		case LUA_TNIL:
			return NULL;
	}

	luaL_error(L, "Invalid data type: %s", lua_typename(L, ltype));
	return NULL; // never reached
}

/***************************************************
  JSON decoding implementation
***************************************************/

static void decode_json_data(lua_State* L, json_object* obj);

static void decode_json_object(lua_State* L, json_object* obj)
{
	luaL_checkstack(L, 3, "Nested too deep!");

	lua_newtable(L);
	json_object_object_foreach(obj, key, val) {
		lua_pushstring(L, key);
		decode_json_data(L, val);
		lua_rawset(L, -3);
	}
}

static void decode_json_array(lua_State* L, json_object* obj)
{
	luaL_checkstack(L, 3, "Nested too deep!");

	lua_newtable(L);
	for (int i = 0; i < json_object_array_length(obj); ++i)
	{
		lua_pushinteger(L, i+1);
		decode_json_data(L, json_object_array_get_idx(obj, i));
		lua_rawset(L, -3);
	}
}

static void decode_json_data(lua_State* L, json_object* obj)
{
	switch (json_object_get_type(obj))
	{
		case json_type_object:
			decode_json_object(L, obj);
			break;
		case json_type_array:
			decode_json_array(L, obj);
			break;
		case json_type_string:
			lua_pushstring(L, json_object_get_string(obj));
			break;
		case json_type_double:
			lua_pushnumber(L, json_object_get_double(obj));
			break;
		case json_type_int:
			lua_pushinteger(L, json_object_get_int(obj));
			break;
		case json_type_boolean:
			lua_pushboolean(L, json_object_get_boolean(obj));
			break;
		case json_type_null:
			push_json_udata(L, NULL);
			break;
	}
}

/***************************************************
  JSON library methods
***************************************************/

// Converts a Lua datum into a compiled JSON chunk
static int Ljson_encode(lua_State* L)
{
	lua_settop(L, 1);
	// L: item
	json_object* obj = encode_lua_data(L);
	lua_pop(L, 1);
	// L: -

	push_json_udata(L, obj);
	return 1;
}

// Converts a JSON string into a compiled JSON chunk
static int Ljson_decode(lua_State* L)
{
	lua_settop(L, 1);
	// L: json
	json_object* obj = json_tokener_parse(luaL_checkstring(L, 1));
	lua_pop(L, 1);
	// L: -

	if (is_error(obj))
	{
		lua_pushnil(L);
		lua_pushstring(L, json_tokener_errors[-(int)obj]); // yes, it's weird
		return 2;
	}
	else
	{
		push_json_udata(L, obj);
		return 1;
	}
}

// Converts a Lua table as a JSON array into a compiled JSON chunk
static int Ljson_array(lua_State* L)
{
	lua_settop(L, 1);
	luaL_argcheck(L, lua_type(L, 1) == LUA_TTABLE, 1, "expected table");
	json_object* obj = encode_lua_table_array(L);
	lua_pop(L, 1);

	push_json_udata(L, obj);
	return 1;
}

// Converts a Lua table as a JSON object into a compiled JSON chunk
static int Ljson_object(lua_State* L)
{
	lua_settop(L, 1);
	luaL_argcheck(L, lua_type(L, 1) == LUA_TTABLE, 1, "expected table");
	json_object* obj = encode_lua_table_object(L);
	lua_pop(L, 1);

	push_json_udata(L, obj);
	return 1;
}

/***************************************************
  JSON object meta table and methods
***************************************************/

// Converts a compiled JSON chunk into a JSON string
static int Ljson_tojson(lua_State* L)
{
	json_object* obj = *((json_object**)luaL_checkudata(L, 1, json_metaname));
	lua_pop(L, 1);
	lua_pushstring(L, json_object_to_json_string(obj));
	return 1;
}

// Converts a compiled JSON chunk into a Lua datum
static int Ljson_tolua(lua_State* L)
{
	json_object* obj = *((json_object**)luaL_checkudata(L, 1, json_metaname));
	lua_pop(L, 1);
	decode_json_data(L, obj);
	return 1;
}

// Cleans up the JSON chunk once it's no longer accessible
static int Ljson_gc_(lua_State* L)
{
	json_object* obj = *((json_object**)luaL_checkudata(L, 1, json_metaname));
	lua_pop(L, 1);
	json_object_put(obj);
	return 0;
}

static int Ljson_tostring_(lua_State* L)
{
	json_object** obj = (json_object**)luaL_checkudata(L, 1, json_metaname);
	lua_pop(L, 1);
	lua_pushfstring(L, "json_object: %p", obj);
	return 1;
}

static const luaL_reg json_meta[] = {
	{"to_json", &Ljson_tojson},
	{"to_lua", &Ljson_tolua},
	{"__gc", &Ljson_gc_},
	{"__tostring", &Ljson_tostring_},
	{NULL, NULL}
};


/***************************************************
  Library setup data
***************************************************/

static const luaL_reg json_lib[] = {
	{"encode", &Ljson_encode},
	{"decode", &Ljson_decode},
	{"array", &Ljson_array},
	{"object", &Ljson_object},
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
	// json, "null"
	json_null = (json_object**)lua_newuserdata(L, sizeof(json_object*));
	luaL_getmetatable(L, json_metaname);
	lua_setmetatable(L, -2);
	*json_null = NULL;
	// json, "null", null

	// stick it in the registry
	lua_pushlightuserdata(L, json_null);
	// json, "null", null, light
	lua_pushvalue(L, -2);
	// json, "null", null, light, null
	lua_settable(L, LUA_REGISTRYINDEX);
	// json, "null", null

	lua_rawset(L, -3);
	// json

	return 1;
}