#include <iostream>
#include <client/dbclient.h>
#include "mongo_cxx_extension.h"
#include "utils.h"
#include "common.h"

// FIXME: client pointer is keep, review Lua binding in order to avoid
// unexpected garbage collection of DBClientBase

using namespace mongo;

extern void lua_to_bson(lua_State *L, int stackpos, BSONObj &obj);
extern void bson_to_lua(lua_State *L, const BSONObj &obj);
extern int gridfile_create(lua_State *L, GridFile gf);
extern DBClientBase* userdata_to_dbclient(lua_State *L, int stackpos);

namespace {
  inline mongo_cxx_extension::GridFileBuilder* userdata_to_gridfilebuilder(lua_State* L,
                                                                           int index) {
    void *ud = 0;

    ud = luaL_checkudata(L, index, LUAMONGO_GRIDFILEBUILDER);
    mongo_cxx_extension::GridFileBuilder *gridfilebuilder;
    gridfilebuilder = *((mongo_cxx_extension::GridFileBuilder **)ud);

    return gridfilebuilder;
  }
} // anonymous namespace

/*
 * builder, err = mongo.GridFileBuilder.New(connection, dbname[, chunksize[, prefix]])
 */
static int gridfilebuilder_new(lua_State *L) {
  int n = lua_gettop(L);
  int resultcount = 1;

  try {
    DBClientBase *connection = userdata_to_dbclient(L, 1);
    const char *dbname = lua_tostring(L, 2);

    mongo_cxx_extension::GridFileBuilder **builder;
    builder = (mongo_cxx_extension::GridFileBuilder **)
      lua_newuserdata(L, sizeof(mongo_cxx_extension::GridFileBuilder *));

    if (n >= 4) {
      unsigned int chunksize = static_cast<unsigned int>(luaL_checkint(L, 3));
      const char *prefix = luaL_checkstring(L, 4);

      *builder = new mongo_cxx_extension::GridFileBuilder(connection,
                                                          dbname,
                                                          chunksize,
                                                          prefix);
    } else if (n == 3) {
      unsigned int chunksize = static_cast<unsigned int>(luaL_checkint(L, 3));
          
      *builder = new mongo_cxx_extension::GridFileBuilder(connection,
                                                          dbname, chunksize);
    } else {
      *builder = new mongo_cxx_extension::GridFileBuilder(connection, dbname);
    }

    luaL_getmetatable(L, LUAMONGO_GRIDFILEBUILDER);
    lua_setmetatable(L, -2);
  } catch (std::exception &e) {
    lua_pushnil(L);
    lua_pushfstring(L, LUAMONGO_ERR_CONNECTION_FAILED, e.what());
    resultcount = 2;
  }

  return resultcount;
}

/*
 * ok, err = builder:append(data_string)
 */
static int gridfilebuilder_append(lua_State *L) {
  mongo_cxx_extension::GridFileBuilder *builder;
  builder = userdata_to_gridfilebuilder(L, 1);
  int resultcount = 1;
  try {
    size_t length = 0;
    const char *data = luaL_checklstring(L, 2, &length);
    builder->appendChunk(data, length);
    lua_pushboolean(L, 1);
  } catch (std::exception &e) {
    lua_pushnil(L);
    lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_GRIDFILEBUILDER,
                    "append", e.what());
    resultcount = 2;
  }
  return resultcount;
}

/*
 * bson, err = builder:build(remote_file[, content_type])
 */
static int gridfilebuilder_build(lua_State *L) {
  int resultcount = 1;
  mongo_cxx_extension::GridFileBuilder *builder;
  builder = userdata_to_gridfilebuilder(L, 1);
  const char *remote = luaL_checkstring(L, 2);
  const char *content_type = luaL_optstring(L, 3, "");
  try {
    BSONObj res = builder->buildFile(remote, content_type);
    bson_to_lua(L, res);
  } catch (std::exception &e) {
    lua_pushnil(L);
    lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_GRIDFILEBUILDER,
                    "build", e.what());
    resultcount = 2;
  }
  return resultcount;
}

/*
 * __gc
 */
static int gridfilebuilder_gc(lua_State *L) {
  mongo_cxx_extension::GridFileBuilder *builder;
  builder = userdata_to_gridfilebuilder(L, 1);
  
  delete builder;

  return 0;
}

/*
 * __tostring
 */
static int gridfilebuilder_tostring(lua_State *L) {
  mongo_cxx_extension::GridFileBuilder *builder;
  builder = userdata_to_gridfilebuilder(L, 1);

  lua_pushfstring(L, "%s: %p", LUAMONGO_GRIDFILEBUILDER, builder);

  return 1;
}

int mongo_gridfilebuilder_register(lua_State *L) {
  static const luaL_Reg gridfilebuilder_methods[] = {
    {"append", gridfilebuilder_append},
    {"write", gridfilebuilder_append},
    {"build", gridfilebuilder_build},
    {NULL, NULL}
  };

  static const luaL_Reg gridfilebuilder_class_methods[] = {
    {"New", gridfilebuilder_new},
    {NULL, NULL}
  };

  luaL_newmetatable(L, LUAMONGO_GRIDFILEBUILDER);
  luaL_register(L, 0, gridfilebuilder_methods);
  //luaL_setfuncs(L, gridfilebuilder_methods, 0);
  lua_pushvalue(L,-1);
  lua_setfield(L, -2, "__index");

  lua_pushcfunction(L, gridfilebuilder_gc);
  lua_setfield(L, -2, "__gc");

  lua_pushcfunction(L, gridfilebuilder_tostring);
  lua_setfield(L, -2, "__tostring");
    
  lua_pop(L,1);

  luaL_register(L, LUAMONGO_GRIDFILEBUILDER, gridfilebuilder_class_methods);

  return 1;
}
