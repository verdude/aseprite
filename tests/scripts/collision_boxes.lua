-- The optional Lua API reuses slice objects and normal undo transactions.
local s = Sprite(32, 24)
s:newEmptyFrame()
s:newEmptyFrame()
local b = s:newHitbox(Rectangle(2, 3, 8, 10), s.frames[2])
local h = s:newHurtbox(Rectangle(4, 1, 12, 20), 2)
local b2 = s:newHitbox(Rectangle(3, 3, 8, 10), 2)
local regular = s:newSlice(Rectangle(0, 0, 32, 24))
assert(#s.slices == 1 and s.slices[1] == regular)
assert(#s.frames[1].hitboxes == 0 and #s.frames[3].hitboxes == 0)
assert(#s.frames[2].hitboxes == 2 and #s.frames[2].hurtboxes == 1)
assert(b.kind == 'hitbox' and h.kind == 'hurtbox')
assert(b.frame == s.frames[2])
assert(not pcall(function() s:newHitbox(Rectangle(0, 0, 1, 1)) end))
assert(not pcall(function() s:newHitbox(Rectangle(0, 0, 1, 1), 4) end))
assert(not pcall(function() s:newHitbox(Rectangle(0, 0, 0, 1), 2) end))
assert(not pcall(function() b.center = Rectangle(1, 1, 1, 1) end))
assert(not pcall(function() b.pivot = Point(1, 1) end))
assert(not pcall(function() b.color = Color(1, 2, 3) end))
assert(not pcall(function() regular.kind = 'hitbox' end))
b.name = 'attack'
b.data = 'gameplay notes'
b.properties = {damage=10}
assert(b.kind == 'hitbox')
b.bounds = Rectangle(-3, 2, 14, 6)
assert(b.bounds == Rectangle(-3, 2, 14, 6))
app.undo()
assert(b.bounds == Rectangle(2, 3, 8, 10))
app.redo()
b.kind = 'hurtbox'
assert(#s.frames[2].hitboxes == 1 and #s.frames[2].hurtboxes == 2)
app.undo()
assert(b.kind == 'hitbox')

-- Frame insertion, deletion, duplication, and undo/redo preserve ownership.
s:newEmptyFrame(1)
assert(b.frame.frameNumber == 3)
app.undo(); assert(b.frame.frameNumber == 2)
app.redo(); assert(b.frame.frameNumber == 3)
app.undo()
s:newFrame(2)
assert(#s.frames[3].hitboxes == 2)
local copy = s.frames[3].hitboxes[1]
local before = b.bounds
copy.bounds = Rectangle(0, 0, 2, 2)
assert(b.bounds == before)
app.undo(); app.undo()
s:deleteFrame(2)
assert(#s.frames[2].hitboxes == 0)
app.undo()
assert(b.frame.frameNumber == 2)
s:deleteSlice(b2)
assert(#s.frames[2].hitboxes == 1)
app.undo(); assert(#s.frames[2].hitboxes == 2)

s:saveAs('_test_collision_boxes.aseprite')
local loaded = Sprite{ fromFile='_test_collision_boxes.aseprite' }
assert(#loaded.slices == 1)
assert(#loaded.frames[2].hitboxes == 2 and #loaded.frames[2].hurtboxes == 1)
assert(#loaded.frames[1].hitboxes == 0 and #loaded.frames[3].hitboxes == 0)
loaded:close()
app.sprite = s
-- Command parameters use zero-based frame indices.
app.command.SaveFileCopyAs{ui=false, filename='_test_collision_range.aseprite', fromFrame=1, toFrame=1}
local range = Sprite{fromFile='_test_collision_range.aseprite'}
assert(#range.frames == 1)
assert(#range.frames[1].hitboxes == 2 and #range.frames[1].hurtboxes == 1)
range:close()
app.sprite = s

-- Whole-sprite transforms also transform boxes outside the canvas.
local bounds = b.bounds
app.command.Flip{target='sprite', orientation='horizontal'}
assert(b.bounds.x == 32 - bounds.x - bounds.width)
app.undo(); assert(b.bounds == bounds)
app.command.Rotate{ui=false, target='sprite', angle=90}
assert(s.width == 24 and s.height == 32)
assert(b.bounds == Rectangle(24-bounds.y-bounds.height, bounds.x, bounds.height, bounds.width))
app.undo(); assert(b.bounds == bounds)
app.command.SpriteSize{ui=false, width=64, height=48}
assert(b.bounds == Rectangle(bounds.x*2, bounds.y*2, bounds.width*2, bounds.height*2))
app.undo(); assert(b.bounds == bounds)
app.command.SpriteSize{ui=false, width=1, height=1}
assert(b.bounds.width == 1 and b.bounds.height == 1)
assert(b.frame.frameNumber == 2)
app.undo(); assert(b.bounds == bounds)
s:close()
