// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/copy_frame.h"

#include "app/cmd/add_frame.h"
#include "app/cmd/add_slice.h"
#include "app/cmd/copy_cel.h"
#include "app/cmd/set_frame_duration.h"
#include "doc/layer.h"
#include "doc/slice.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

using namespace doc;

CopyFrame::CopyFrame(Sprite* sprite, frame_t fromFrame, frame_t newFrame)
  : WithSprite(sprite)
  , m_fromFrame(fromFrame)
  , m_newFrame(newFrame)
{
}

void CopyFrame::onExecute()
{
  Sprite* sprite = this->sprite();
  frame_t fromFrame = m_fromFrame;
  int msecs = sprite->frameDuration(fromFrame);

  executeAndAdd(new cmd::AddFrame(sprite, m_newFrame));
  executeAndAdd(new cmd::SetFrameDuration(sprite, m_newFrame, msecs));

  if (fromFrame >= m_newFrame)
    ++fromFrame;
  // Snapshot pointers: adding a slice changes the collection's iteration order.
  std::vector<Slice*> boxes;
  for (auto* box : sprite->slices())
    if (box->isCollisionBox() && box->boxFrame() == fromFrame)
      boxes.push_back(box);
  for (auto it = boxes.rbegin(); it != boxes.rend(); ++it) {
    auto* copy = new Slice(**it);
    copy->setBoxFrame(m_newFrame);
    executeAndAdd(new AddSlice(sprite, copy));
  }

  // Do not copy cels (cmd::CopyCel must be called from outside)
}

}} // namespace app::cmd
