// Aseprite
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_BOX_FRAME_H_INCLUDED
#define APP_CMD_SET_BOX_FRAME_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_slice.h"
#include "doc/frame.h"

namespace app { namespace cmd {

class SetBoxFrame : public Cmd,
                    public WithSlice {
public:
  SetBoxFrame(doc::Slice* slice, doc::frame_t frame);

protected:
  void onExecute() override;
  void onUndo() override;

private:
  doc::frame_t m_frame;
};

}} // namespace app::cmd

#endif
