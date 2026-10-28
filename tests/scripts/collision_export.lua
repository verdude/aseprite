-- Collision data belongs directly to each exported animation frame.
local s = Sprite(32, 32)
s.cels[1].image:drawPixel(12, 12, app.pixelColor.rgba(255, 255, 255, 255))
local regular = s:newSlice(Rectangle(0, 0, 32, 32))
regular.name = 'regular'
-- Construct the persisted representation without depending on the convenience API.
local function box(kind, name, bounds)
  local b = s:newSlice(bounds)
  b.name = name
  b.properties('aseprite.collision').kind = kind
  return b
end
box('hitbox', 'attack "one"', Rectangle(-2, 4, 12, 8))
box('hitbox', 'attack two', Rectangle(6, 4, 12, 8))
box('hurtbox', 'body', Rectangle(8, 3, 16, 26))
s:newFrame(1)
s:newEmptyFrame()
s:newEmptyFrame(1)
-- Owners are now frames 2 and 3, with empty frames at either end.
-- Native data and its sidecar must not produce duplicate boxes on reload.
local sidecar = assert(io.open('_test_collision_export.aseprite-data', 'w'))
sidecar:write('<sprite><slices /></sprite>')
sidecar:close()
s:saveAs('_test_collision_export.aseprite')
s:close()
s = Sprite{ fromFile='_test_collision_export.aseprite' }
app.command.ShowHitboxes()
app.command.ShowHurtboxes()

local function export(format, ignoreEmpty, listSlices)
  app.command.ExportSpriteSheet{
    ui=false, type=SpriteSheetType.PACKED, dataFormat=format,
    textureFilename='_test_collision_export.png', dataFilename='_test_collision_export.json',
    filenameFormat='f{frame}', trim=true, trimSprite=false,
    mergeDuplicates=true, extrude=true, ignoreEmpty=ignoreEmpty,
    listSlices=listSlices, listTags=false, listLayers=false
  }
  local f = assert(io.open('_test_collision_export.json', 'r'))
  local data = json.decode(f:read('*a'))
  f:close()
  assert(data.meta.hitboxes == nil and data.meta.hurtboxes == nil)
  if listSlices then
    assert(#data.meta.slices == 1 and data.meta.slices[1].name == 'regular')
  else
    assert(data.meta.slices == nil)
  end
  return data
end

for _, format in ipairs{SpriteSheetDataFormat.JSON_HASH, SpriteSheetDataFormat.JSON_ARRAY} do
  for _, ignoreEmpty in ipairs{false, true} do
    local data = export(format, ignoreEmpty, true)
    local count, withBoxes = 0, 0
    local entries = {}
    if format == SpriteSheetDataFormat.JSON_ARRAY then
      for i = 1, #data.frames do entries[i] = data.frames[i] end
    else
      for _, frame in pairs(data.frames) do entries[#entries + 1] = frame end
    end
    for _, frame in ipairs(entries) do
      count = count + 1
      assert(frame.hitboxes ~= nil and frame.hurtboxes ~= nil)
      if #frame.hitboxes > 0 then
        withBoxes = withBoxes + 1
        assert(#frame.hitboxes == 2 and #frame.hurtboxes == 1)
        local body = frame.hurtboxes[1]
        assert(body.name == 'body')
        assert(body.bounds.x == 8 and body.bounds.y == 3)
        assert(body.bounds.w == 16 and body.bounds.h == 26)
        local found = false
        for _, hit in ipairs(frame.hitboxes) do
          if hit.name == 'attack "one"' then
            assert(hit.bounds.x == -2 and hit.bounds.y == 4)
            found = true
          end
        end
        assert(found)
      else
        assert(#frame.hurtboxes == 0)
      end
    end
    assert(count == (ignoreEmpty and 2 or 4))
    assert(withBoxes == 2)
  end
end
export(SpriteSheetDataFormat.JSON_HASH, false, false)
-- A selected range must resolve source-frame numbers, not output entry numbers.
local tag = s:newTag(2, 3)
tag.name = 'attack'
app.command.ExportSpriteSheet{
  ui=false, type=SpriteSheetType.HORIZONTAL, dataFormat=SpriteSheetDataFormat.JSON_ARRAY,
  tag='attack', textureFilename='_test_collision_tag.png', dataFilename='_test_collision_tag.json',
  ignoreEmpty=false
}
local f = assert(io.open('_test_collision_tag.json', 'r'))
local data = json.decode(f:read('*a')); f:close()
assert(#data.frames == 2)
for _, frame in ipairs(data.frames) do
  assert(#frame.hitboxes == 2 and #frame.hurtboxes == 1)
end
s:close()
