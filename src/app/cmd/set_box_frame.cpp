// Aseprite
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_box_frame.h"

#include "doc/slice.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

SetBoxFrame::SetBoxFrame(doc::Slice* slice, doc::frame_t frame) : WithSlice(slice), m_frame(frame)
{
}

void SetBoxFrame::onExecute()
{
  auto* box = slice();
  const auto old = box->boxFrame();
  box->setBoxFrame(m_frame);
  m_frame = old;
  box->incrementVersion();
  box->sprite()->incrementVersion();
}

void SetBoxFrame::onUndo()
{
  onExecute();
}

}} // namespace app::cmd
