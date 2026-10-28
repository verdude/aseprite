// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_box_kind.h"
#include "app/cmd/set_slice_key.h"
#include "app/cmd/set_slice_name.h"
#include "app/pref/preferences.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/userdata.h"
#include "app/tx.h"
#include "doc/slice.h"
#include "doc/sprite.h"

namespace app { namespace script {

using namespace doc;

namespace {

int Slice_eq(lua_State* L)
{
  const auto a = may_get_docobj<Slice>(L, 1);
  const auto b = may_get_docobj<Slice>(L, 2);
  lua_pushboolean(L, (!a && !b) || (a && b && a->id() == b->id()));
  return 1;
}

int Slice_get_sprite(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  push_docobj(L, slice->owner()->sprite());
  return 1;
}

int Slice_get_kind(lua_State* L)
{
  auto* slice = get_docobj<Slice>(L, 1);
  lua_pushstring(L,
                 slice->kind() == SliceKind::Hitbox  ? "hitbox" :
                 slice->kind() == SliceKind::Hurtbox ? "hurtbox" :
                                                       "slice");
  return 1;
}
int Slice_set_kind(lua_State* L)
{
  auto* slice = get_docobj<Slice>(L, 1);
  const std::string kind = luaL_checkstring(L, 2);
  if (!slice->isCollisionBox() || (kind != "hitbox" && kind != "hurtbox"))
    return luaL_error(L, "only hitbox/hurtbox type changes are supported");
  Tx tx(slice->sprite());
  tx(new cmd::SetBoxKind(slice, kind == "hitbox" ? SliceKind::Hitbox : SliceKind::Hurtbox));
  tx.commit();
  return 0;
}
int Slice_get_frame(lua_State* L)
{
  auto* slice = get_docobj<Slice>(L, 1);
  if (slice->isCollisionBox())
    push_sprite_frame(L, slice->sprite(), slice->boxFrame());
  else
    lua_pushnil(L);
  return 1;
}
int Slice_get_color(lua_State* L)
{
  auto* slice = get_docobj<Slice>(L, 1);
  if (!slice->isCollisionBox())
    return UserData_get_color<Slice>(L);
  auto& pref = Preferences::instance().collision;
  push_obj<app::Color>(
    L,
    slice->kind() == SliceKind::Hitbox ? pref.hitboxColor() : pref.hurtboxColor());
  return 1;
}
int Slice_set_color(lua_State* L)
{
  if (get_docobj<Slice>(L, 1)->isCollisionBox())
    return luaL_error(L, "collision colors are configured by type in Preferences");
  return UserData_set_color<Slice>(L);
}

int Slice_get_name(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  lua_pushstring(L, slice->name().c_str());
  return 1;
}

int Slice_get_bounds(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  if (!slice->empty())
    push_new<gfx::Rect>(L, slice->begin()->value()->bounds());
  else
    lua_pushnil(L);
  return 1;
}

int Slice_get_center(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  if (!slice->isCollisionBox() && !slice->empty() && slice->begin()->value()->hasCenter())
    push_new<gfx::Rect>(L, slice->begin()->value()->center());
  else
    lua_pushnil(L);
  return 1;
}

int Slice_get_pivot(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  if (!slice->isCollisionBox() && !slice->empty() && slice->begin()->value()->hasPivot())
    push_new<gfx::Point>(L, slice->begin()->value()->pivot());
  else
    lua_pushnil(L);
  return 1;
}

int Slice_set_name(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  const char* name = lua_tostring(L, 2);
  if (name) {
    Tx tx(slice->sprite());
    tx(new cmd::SetSliceName(slice, name));
    tx.commit();
  }
  return 0;
}

int Slice_set_bounds(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  gfx::Rect bounds = convert_args_into_rect(L, 2);
  if (slice->isCollisionBox() && bounds.isEmpty())
    return luaL_error(L, "collision box bounds must have positive width and height");
  const auto frame = slice->isCollisionBox() ? slice->boxFrame() : 0;
  SliceKey key;
  if (const SliceKey* srcKey = slice->getByFrame(frame))
    key = *srcKey;
  key.setBounds(bounds);
  Tx tx(slice->sprite());
  tx(new cmd::SetSliceKey(slice, frame, key));
  tx.commit();
  return 0;
}

int Slice_set_center(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  if (slice->isCollisionBox())
    return luaL_error(L, "collision boxes do not have a center");
  gfx::Rect center = convert_args_into_rect(L, 2);
  SliceKey key;
  if (const SliceKey* srcKey = slice->getByFrame(0))
    key = *srcKey;
  key.setCenter(center);
  Tx tx(slice->sprite());
  tx(new cmd::SetSliceKey(slice, 0, key));
  tx.commit();
  return 0;
}

int Slice_set_pivot(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  if (slice->isCollisionBox())
    return luaL_error(L, "collision boxes do not have a pivot");
  gfx::Point pivot = convert_args_into_point(L, 2);
  SliceKey key;
  if (const SliceKey* srcKey = slice->getByFrame(0))
    key = *srcKey;
  key.setPivot(pivot);
  Tx tx(slice->sprite());
  tx(new cmd::SetSliceKey(slice, 0, key));
  tx.commit();
  return 0;
}

const luaL_Reg Slice_methods[] = {
  { "__eq",  Slice_eq },
  { nullptr, nullptr  }
};

const Property Slice_properties[] = {
  { "kind",       Slice_get_kind,                 Slice_set_kind                 },
  { "frame",      Slice_get_frame,                nullptr                        },
  { "sprite",     Slice_get_sprite,               nullptr                        },
  { "name",       Slice_get_name,                 Slice_set_name                 },
  { "bounds",     Slice_get_bounds,               Slice_set_bounds               },
  { "center",     Slice_get_center,               Slice_set_center               },
  { "pivot",      Slice_get_pivot,                Slice_set_pivot                },
  { "color",      Slice_get_color,                Slice_set_color                },
  { "data",       UserData_get_text<Slice>,       UserData_set_text<Slice>       },
  { "properties", UserData_get_properties<Slice>, UserData_set_properties<Slice> },
  { nullptr,      nullptr,                        nullptr                        }
};

} // anonymous namespace

DEF_MTNAME(Slice);

void register_slice_class(lua_State* L)
{
  using doc::Slice;
  REG_CLASS(L, Slice);
  REG_CLASS_PROPERTIES(L, Slice);
}

}} // namespace app::script
