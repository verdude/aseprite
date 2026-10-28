// Aseprite
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_box_kind.h"

#include "app/doc.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

void SetBoxKind::onExecute()
{
  auto* box = slice();
  ASSERT(box->isCollisionBox() && m_kind != doc::SliceKind::Slice);
  const auto old = box->kind();
  box->setKind(m_kind);
  m_kind = old;
  box->incrementVersion();
  box->sprite()->incrementVersion();
}

void SetBoxKind::onFireNotifications()
{
  static_cast<Doc*>(slice()->sprite()->document())->notifyGeneralUpdate();
}

}} // namespace app::cmd
