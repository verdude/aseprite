// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/util/slice_utils.h"

#include "app/context_access.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/site.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "fmt/format.h"

namespace app {

std::string get_collision_box_type_name(doc::SliceKind kind)
{
  switch (kind) {
    case doc::SliceKind::Hitbox:  return Strings::collision_hitbox();
    case doc::SliceKind::Hurtbox: return Strings::collision_hurtbox();
    case doc::SliceKind::Pushbox: return Strings::collision_pushbox();
    default:                      ASSERT(false); return {};
  }
}

Color get_collision_box_color(doc::SliceKind kind)
{
  auto& colors = Preferences::instance().collision;
  switch (kind) {
    case doc::SliceKind::Hitbox:  return colors.hitboxColor();
    case doc::SliceKind::Hurtbox: return colors.hurtboxColor();
    case doc::SliceKind::Pushbox: return colors.pushboxColor();
    default:                      ASSERT(false); return Color::fromMask();
  }
}

Option<bool>& get_collision_box_visibility(const Doc* doc, doc::SliceKind kind)
{
  auto& show = Preferences::instance().document(doc).show;
  switch (kind) {
    case doc::SliceKind::Hitbox:  return show.hitboxes;
    case doc::SliceKind::Hurtbox: return show.hurtboxes;
    case doc::SliceKind::Pushbox: return show.pushboxes;
    default:                      ASSERT(false); return show.slices;
  }
}

std::string get_unique_slice_name(const doc::Sprite* sprite, const std::string& namePrefix)
{
  std::string prefix = namePrefix.empty() ? "Slice" : namePrefix;
  int max = 0;

  for (doc::Slice* slice : sprite->slices())
    if (std::strncmp(slice->name().c_str(), prefix.c_str(), prefix.size()) == 0)
      max = std::max(max, (int)std::strtol(slice->name().c_str() + prefix.size(), nullptr, 10));

  return fmt::format("{} {}", prefix, max + 1);
}

std::vector<doc::Slice*> get_selected_slices(const Site& site)
{
  std::vector<Slice*> selectedSlices;
  if (site.sprite() && !site.selectedSlices().empty()) {
    for (auto* slice : site.sprite()->slices()) {
      if (site.selectedSlices().contains(slice->id())) {
        selectedSlices.push_back(slice);
      }
    }
  }
  return selectedSlices;
}

} // namespace app
