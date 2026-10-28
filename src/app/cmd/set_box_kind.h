// Aseprite
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_BOX_KIND_H_INCLUDED
#define APP_CMD_SET_BOX_KIND_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_slice.h"
#include "doc/slice.h"

namespace app { namespace cmd {

class SetBoxKind : public Cmd,
                   public WithSlice {
public:
  SetBoxKind(doc::Slice* slice, doc::SliceKind kind) : WithSlice(slice), m_kind(kind) {}

protected:
  void onExecute() override;
  void onUndo() override { onExecute(); }
  void onFireNotifications() override;

private:
  doc::SliceKind m_kind;
};

}} // namespace app::cmd

#endif
