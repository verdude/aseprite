-- Pushboxes share collision editing/ownership while remaining a distinct type.
local s = Sprite(32, 24)
s:newEmptyFrame()
s:newEmptyFrame()
local p = s:newPushbox(Rectangle(-2, 3, 12, 18), s.frames[2])
local p2 = s:newPushbox(Rectangle(2, 3, 12, 18), 2)
local hit = s:newHitbox(Rectangle(3, 4, 8, 10), 2)
local hurt = s:newHurtbox(Rectangle(4, 2, 12, 20), 2)
local regular = s:newSlice(Rectangle(0, 0, 32, 24))
assert(#s.slices == 1 and s.slices[1] == regular)
assert(#s.frames[1].pushboxes == 0 and #s.frames[3].pushboxes == 0)
assert(#s.frames[2].pushboxes == 2)
assert(p ~= p2 and p.name ~= p2.name)
assert(p.kind == 'pushbox' and p.frame == s.frames[2])
assert(p.center == nil and p.pivot == nil)
assert(p.color.rgbaPixel == Color(0, 0, 255).rgbaPixel)
app.preferences.collision.pushbox_color = Color(40, 80, 220)
assert(p.color.rgbaPixel == Color(40, 80, 220).rgbaPixel)
assert(hit.color.rgbaPixel == Color(255, 0, 0).rgbaPixel)
assert(hurt.color.rgbaPixel == Color(0, 255, 0).rgbaPixel)
for _, create in ipairs{
  function() s:newPushbox(Rectangle(0, 0, 1, 1)) end,
  function() s:newPushbox(Rectangle(0, 0, 1, 1), 0) end,
  function() s:newPushbox(Rectangle(0, 0, 1, 1), 4) end,
  function() s:newPushbox(Rectangle(0, 0, 0, 1), 2) end,
  function() p.kind = 'slice' end,
  function() p.kind = 'unknown' end,
  function() regular.kind = 'pushbox' end,
  function() p.center = Rectangle(1, 1, 1, 1) end,
  function() p.pivot = Point(1, 1) end,
  function() p.color = Color(1, 2, 3) end,
} do assert(not pcall(create)) end

p.name = 'push "one"'
p.data = 'gameplay notes'
p.properties = {weight=10}
assert(p.kind == 'pushbox')
local bounds = p.bounds
p.bounds = Rectangle(-3, 2, 14, 20)
assert(p.bounds == Rectangle(-3, 2, 14, 20))
assert(p2.bounds == Rectangle(2, 3, 12, 18))
app.undo(); assert(p.bounds == bounds)
app.redo(); assert(p.bounds == Rectangle(-3, 2, 14, 20))
app.undo()
for _, kind in ipairs{'hitbox', 'hurtbox'} do
  p.kind = kind
  assert(p.kind == kind and p.frame == s.frames[2] and p.bounds == bounds)
  assert(#s.frames[2].pushboxes == 1 and #s.frames[2][kind..'es'] == 2)
  app.undo(); assert(p.kind == 'pushbox' and #s.frames[2].pushboxes == 2)
  app.redo(); assert(p.kind == kind)
  app.undo()
end
for _, box in ipairs{hit, hurt} do
  local kind = box.kind
  box.kind = 'pushbox'
  assert(#s.frames[2].pushboxes == 3)
  app.undo(); assert(box.kind == kind)
end

s:newEmptyFrame(1)
assert(p.frame.frameNumber == 3 and p2.frame.frameNumber == 3)
app.undo(); assert(p.frame.frameNumber == 2)
s:newFrame(2)
assert(#s.frames[3].pushboxes == 2)
local copy = s.frames[3].pushboxes[1]
copy.bounds = Rectangle(0, 0, 1, 1)
assert(p.bounds == bounds and p2.bounds == Rectangle(2, 3, 12, 18))
app.undo(); app.undo()
s:deleteFrame(2)
assert(#s.frames[2].pushboxes == 0)
app.undo(); assert(#s.frames[2].pushboxes == 2)
s:deleteSlice(p2)
assert(#s.frames[2].pushboxes == 1)
app.undo(); assert(#s.frames[2].pushboxes == 2)

s:saveAs('_test_pushboxes.aseprite')
local loaded = Sprite{fromFile='_test_pushboxes.aseprite'}
assert(#loaded.slices == 1 and #loaded.frames[2].pushboxes == 2)
assert(#loaded.frames[2].hitboxes == 1 and #loaded.frames[2].hurtboxes == 1)
assert(#loaded.frames[1].pushboxes == 0 and #loaded.frames[3].pushboxes == 0)
local found = false
for _, box in ipairs(loaded.frames[2].pushboxes) do
  if box.name == p.name then
    assert(box.bounds == bounds and box.data == p.data and box.properties.weight == 10)
    found = true
  end
end
assert(found)
loaded:close()
app.sprite = s
app.command.SaveFileCopyAs{ui=false, filename='_test_pushbox_range.aseprite', fromFrame=1, toFrame=1}
local range = Sprite{fromFile='_test_pushbox_range.aseprite'}
assert(#range.frames == 1 and #range.frames[1].pushboxes == 2)
range:close()
app.sprite = s

app.command.Flip{target='sprite', orientation='horizontal'}
assert(p.bounds.x == 32 - bounds.x - bounds.width)
app.undo(); assert(p.bounds == bounds)
app.command.Rotate{ui=false, target='sprite', angle=90}
assert(p.bounds == Rectangle(24-bounds.y-bounds.height, bounds.x, bounds.height, bounds.width))
app.undo(); assert(p.bounds == bounds)
app.command.SpriteSize{ui=false, width=64, height=48}
assert(p.bounds == Rectangle(bounds.x*2, bounds.y*2, bounds.width*2, bounds.height*2))
app.undo(); assert(p.bounds == bounds)
s:close()
