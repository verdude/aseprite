// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <gtest/gtest.h>

#include "app/app.h"
#include "app/cli/app_options.h"
#include "app/cmd/add_slice.h"
#include "app/cmd/set_box_kind.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/doc_range.h"
#include "app/pref/preferences.h"
#include "app/tools/active_tool.h"
#include "app/tools/ink.h"
#include "app/tools/tool_box.h"
#include "app/tools/tool_loop_manager.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_hit.h"
#include "app/ui/editor/moving_slice_state.h"
#include "app/ui/editor/tool_loop_impl.h"
#include "app/ui/main_window.h"
#include "app/ui/slice_window.h"
#include "app/util/clipboard.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "doc/slice.h"
#include "os/system.h"
#include "os/window.h"
#include "ui/display.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/window.h"

using namespace app;

const char* g_exeName = nullptr;

TEST(App, JustClose)
{
  os::SystemRef system = os::System::make();
  const char* argv[] = { g_exeName };
  app::AppOptions options(sizeof(argv) / sizeof(argv[0]), argv);
  app::App app;
  app.initialize(options);
  app.run(false);
}

TEST(App, ExitCommand)
{
  os::SystemRef system = os::System::make();
  const char* argv[] = { g_exeName };
  app::AppOptions options(sizeof(argv) / sizeof(argv[0]), argv);
  app::App app;
  app.initialize(options);

  ui::execute_from_ui_thread(
    [&app] { app.context()->executeCommand(Commands::instance()->byId(CommandId::Exit())); });

  app.run(false);
}

TEST(App, ExitWithOneDoc)
{
  os::SystemRef system = os::System::make();
  const char* argv[] = { g_exeName };
  app::AppOptions options(sizeof(argv) / sizeof(argv[0]), argv);
  app::App app;
  app.initialize(options);

  ui::execute_from_ui_thread([&app] {
    Params params;
    params.set("ui", "false");
    app.context()->executeCommand(Commands::instance()->byId(CommandId::NewFile()), params);
    app.context()->executeCommand(Commands::instance()->byId(CommandId::Exit()));
  });

  app.run(false);
}

TEST(App, CollisionToolWorkflow)
{
  os::SystemRef system = os::System::make();
  const char* argv[] = { g_exeName };
  app::AppOptions options(std::size(argv), argv);
  app::App app;
  app.initialize(options);
  auto* ctx = app.context();
  auto command = [ctx](const char* id) { ctx->executeCommand(Commands::instance()->byId(id)); };
  Params newFile;
  newFile.set("ui", "false");
  newFile.set("width", "32");
  newFile.set("height", "24");
  ctx->executeCommand(Commands::instance()->byId(CommandId::NewFile()), newFile);
  auto* editor = Editor::activeEditor();
  ASSERT_NE(nullptr, editor);
  auto* doc = editor->document();
  auto* sprite = editor->sprite();
  auto* collision = app.toolBox()->getToolById("collision");
  ASSERT_NE(nullptr, collision);
  ASSERT_TRUE(collision->getInk(0)->isCollisionBox());
  app.activeToolManager()->setSelectedTool(collision);
  Preferences::instance().collision.createKind(doc::SliceKind::Hitbox);
  Preferences::instance().slices.useKeys(false);
  editor->docPref().show.hitboxes(true);
  editor->docPref().show.hurtboxes(true);
  editor->docPref().show.slices(false);
  doc->markAsSaved();
  editor->layer()->setEditable(false);
  auto image = doc::ImageRef(doc::Image::createCopy(editor->layer()->cel(0)->image()));

  // Exercise the actual drag controller and ink, even on a locked layer.
  auto drawBox = [&](const gfx::Point& from, const gfx::Point& to) {
    std::unique_ptr<tools::ToolLoop> loop(
      create_tool_loop(editor, ctx, tools::Pointer::Left, false, false));
    ASSERT_NE(nullptr, loop);
    tools::ToolLoopManager manager(loop.get());
    const tools::Pointer first(from, {}, tools::Pointer::Left, ui::PointerType::Mouse, 1.0f);
    const tools::Pointer last(to, {}, tools::Pointer::Left, ui::PointerType::Mouse, 1.0f);
    manager.prepareLoop(first);
    manager.pressButton(first);
    manager.movement(last);
    manager.releaseButton(last);
    manager.end();
  };
  drawBox(gfx::Point(2, 3), gfx::Point(9, 12));
  ASSERT_EQ(1, sprite->slices().size());
  auto* hit = *sprite->slices().begin();
  EXPECT_EQ(doc::SliceKind::Hitbox, hit->kind());
  EXPECT_EQ(gfx::Rect(2, 3, 8, 10), hit->getByFrame(0)->bounds());
  EXPECT_TRUE(doc->isModified());
  command(CommandId::Undo());
  EXPECT_TRUE(sprite->slices().empty());
  EXPECT_FALSE(doc->isModified());
  command(CommandId::Redo());
  EXPECT_TRUE(doc->isModified());
  {
    Tx tx(doc);
    doc->getApi(tx).addEmptyFrame(sprite, 1);
    tx.commit();
  }
  editor->setFrame(1);
  EXPECT_TRUE(editor->getSite().selectedSlices().empty());
  EXPECT_FALSE(editor->isSliceVisible(hit));
  Preferences::instance().collision.createKind(doc::SliceKind::Hurtbox);
  drawBox(gfx::Point(3, 4), gfx::Point(10, 13));
  ASSERT_EQ(2, sprite->slices().size());
  auto* hurt = *sprite->slices().begin();
  EXPECT_EQ(doc::SliceKind::Hurtbox, hurt->kind());
  EXPECT_EQ(1, hurt->boxFrame());
  command(CommandId::DuplicateSlice());
  ASSERT_EQ(3, sprite->slices().size());
  auto* duplicate = *sprite->slices().begin();
  {
    Tx tx(doc);
    tx(new cmd::SetBoxKind(duplicate, doc::SliceKind::Hitbox));
    tx.commit();
  }
  editor->slicesTransforms(true);
  EXPECT_FALSE(editor->slicesTransforms());
  EXPECT_EQ(duplicate, editor->calcHit(editor->editorToScreen(gfx::Point(8, 9))).slice());
  editor->clearSlicesSelection();
  editor->selectSlice(hurt);
  EXPECT_TRUE(editor->getSite().selectedSlices().contains(hurt->id()));
  command(CommandId::ShowHurtboxes());
  EXPECT_FALSE(editor->isSliceVisible(hurt));
  EXPECT_TRUE(editor->getSite().selectedSlices().empty());
  editor->selectAllSlices();
  EXPECT_EQ(1, editor->getSite().selectedSlices().size());
  EXPECT_TRUE(editor->getSite().selectedSlices().contains(duplicate->id()));
  {
    doc::SelectedObjects selected;
    selected.insert(hurt->id());
    class InspectableSliceWindow : public SliceWindow {
    public:
      using SliceWindow::boxKindFields;
      using SliceWindow::center;
      using SliceWindow::pivot;
      using SliceWindow::SliceWindow;
      using SliceWindow::userData;
    };
    InspectableSliceWindow window(sprite, selected, 1);
    EXPECT_FALSE(window.boxKindFields()->hasFlags(ui::HIDDEN));
    EXPECT_TRUE(window.center()->hasFlags(ui::HIDDEN));
    EXPECT_TRUE(window.pivot()->hasFlags(ui::HIDDEN));
    EXPECT_TRUE(window.userData()->hasFlags(ui::HIDDEN));
  }
  ctx->clipboard()->copySlices({ hurt });
  editor->setFrame(0);
  ctx->clipboard()->paste(ctx, true, nullptr);
  ASSERT_EQ(4, sprite->slices().size());
  auto* pasted = *sprite->slices().begin();
  EXPECT_EQ(doc::SliceKind::Hurtbox, pasted->kind());
  EXPECT_EQ(0, pasted->boxFrame());
  EXPECT_TRUE(editor->isSliceVisible(pasted));
  EXPECT_EQ(0, doc::count_diff_between_images(image.get(), editor->layer()->cel(0)->image()));

  // Multiple pushboxes use the same overlap order, visibility and clipboard behavior.
  Preferences::instance().collision.createKind(doc::SliceKind::Pushbox);
  editor->docPref().show.pushboxes(false);
  drawBox(gfx::Point(20, 3), gfx::Point(27, 12));
  ASSERT_EQ(5, sprite->slices().size());
  auto* push = *sprite->slices().begin();
  EXPECT_EQ(doc::SliceKind::Pushbox, push->kind());
  EXPECT_EQ(gfx::Rect(20, 3, 8, 10), push->getByFrame(0)->bounds());
  EXPECT_TRUE(editor->isSliceVisible(push));
  command(CommandId::DuplicateSlice());
  ASSERT_EQ(6, sprite->slices().size());
  auto* pushDuplicate = *sprite->slices().begin();
  EXPECT_EQ(pushDuplicate, editor->calcHit(editor->editorToScreen(gfx::Point(24, 8))).slice());
  editor->clearSlicesSelection();
  editor->selectSlice(push);
  command(CommandId::ShowPushboxes());
  EXPECT_FALSE(editor->isSliceVisible(push));
  EXPECT_FALSE(editor->isSliceVisible(pushDuplicate));
  EXPECT_TRUE(editor->getSite().selectedSlices().empty());
  EXPECT_TRUE(editor->isSliceVisible(hit));
  EXPECT_TRUE(editor->isSliceVisible(pasted));
  command(CommandId::ShowPushboxes());
  {
    doc::SelectedObjects selected;
    selected.insert(push->id());
    class InspectableBoxWindow : public SliceWindow {
    public:
      using SliceWindow::boxKind;
      using SliceWindow::SliceWindow;
    };
    InspectableBoxWindow window(sprite, selected, 0);
    EXPECT_EQ(doc::SliceKind::Pushbox, window.boxKindValue());
    EXPECT_EQ(3, window.boxKind()->getItemCount());
    for (int index = 0; index < 3; ++index) {
      window.boxKind()->setSelectedItemIndex(index);
      {
        Tx tx(doc);
        tx(new cmd::SetBoxKind(push, window.boxKindValue()));
        tx.commit();
      }
      EXPECT_EQ(doc::CollisionBoxKinds[index], push->kind());
      EXPECT_EQ(0, push->boxFrame());
      EXPECT_EQ(gfx::Rect(20, 3, 8, 10), push->getByFrame(0)->bounds());
      command(CommandId::Undo());
      EXPECT_EQ(doc::SliceKind::Pushbox, push->kind());
    }
  }
  // Moving/resizing one overlapping box leaves the other and pixels untouched.
  auto transformPush = [&](int border, const gfx::Point& delta) {
    doc::SelectedObjects selected;
    selected.insert(push->id());
    EditorHit hit(EditorHit::SliceBounds);
    hit.setSlice(push);
    hit.setBorder(border);
    const gfx::Point start(24, 8);
    ui::MouseMessage down(ui::kMouseDownMessage,
                          ui::PointerType::Mouse,
                          ui::kButtonLeft,
                          ui::kKeyNoneModifier,
                          editor->editorToScreen(start));
    ui::MouseMessage move(ui::kMouseMoveMessage, down, editor->editorToScreen(start + delta));
    down.setDisplay(editor->display());
    move.setDisplay(editor->display());
    auto state = std::make_shared<MovingSliceState>(editor, &down, hit, selected);
    editor->setState(state);
    state->onMouseMove(editor, &move);
    state->onMouseUp(editor, &move);
    state.reset();
    // Normal event dispatch releases the completed state's transaction.
    ui::MouseMessage cursor(ui::kSetCursorMessage, move, move.position());
    cursor.setDisplay(editor->display());
    editor->sendMessage(&cursor);
  };
  transformPush(ui::CENTER | ui::MIDDLE, gfx::Point(2, 1));
  EXPECT_EQ(gfx::Rect(22, 4, 8, 10), push->getByFrame(0)->bounds());
  EXPECT_EQ(gfx::Rect(22, 5, 8, 10), pushDuplicate->getByFrame(0)->bounds());
  command(CommandId::Undo());
  EXPECT_EQ(gfx::Rect(20, 3, 8, 10), push->getByFrame(0)->bounds());
  transformPush(ui::RIGHT | ui::BOTTOM, gfx::Point(2, 1));
  EXPECT_EQ(gfx::Rect(20, 3, 10, 11), push->getByFrame(0)->bounds());
  command(CommandId::Undo());
  EXPECT_EQ(gfx::Rect(20, 3, 8, 10), push->getByFrame(0)->bounds());
  ctx->clipboard()->copySlices({ push });
  editor->setFrame(1);
  EXPECT_FALSE(editor->isSliceVisible(push));
  ctx->clipboard()->paste(ctx, true, nullptr);
  auto* pastedPush = *sprite->slices().begin();
  ASSERT_EQ(7, sprite->slices().size());
  EXPECT_EQ(doc::SliceKind::Pushbox, pastedPush->kind());
  EXPECT_EQ(1, pastedPush->boxFrame());
  EXPECT_TRUE(editor->isSliceVisible(pastedPush));
  EXPECT_EQ(0, doc::count_diff_between_images(image.get(), editor->layer()->cel(0)->image()));
  editor->setFrame(0);

  // Regular slice selection still spans frames and excludes collision boxes.
  app.activeToolManager()->setSelectedTool(app.toolBox()->getToolById("slice"));
  editor->docPref().show.slices(true);
  auto* regular = new doc::Slice;
  regular->insert(0, doc::SliceKey(gfx::Rect(0, 0, 32, 24)));
  {
    Tx tx(doc);
    tx(new cmd::AddSlice(sprite, regular));
    tx.commit();
  }
  editor->selectAllSlices();
  EXPECT_EQ(1, editor->getSite().selectedSlices().size());
  editor->setFrame(1);
  EXPECT_TRUE(editor->isSliceSelected(regular));

  // Whole-frame clipboard copies carry boxes into another document.
  {
    ContextReader reader(ctx);
    DocRange range;
    range.startRange(nullptr, 1, DocRange::kFrames);
    range.endRange(nullptr, 1);
    ctx->clipboard()->copyRange(reader, range);
  }
  ctx->executeCommand(Commands::instance()->byId(CommandId::NewFile()), newFile);
  auto* destination = ctx->activeDocument()->sprite();
  ctx->clipboard()->paste(ctx, true, nullptr);
  EXPECT_EQ(2, destination->totalFrames());
  EXPECT_EQ(3, destination->slices().size());
  for (auto* box : destination->slices()) {
    EXPECT_TRUE(box->isCollisionBox());
    EXPECT_EQ(0, box->boxFrame());
  }
  command(CommandId::Undo());
  EXPECT_EQ(1, destination->totalFrames());
  EXPECT_TRUE(destination->slices().empty());
  app.run(false);
}

int app_main(int argc, char* argv[])
{
  g_exeName = argv[0];
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
