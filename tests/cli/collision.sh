#!/bin/bash

collision_dir="$t/collision"
mkdir -p "$collision_dir"
cat > "$collision_dir/create.lua" <<'LUA'
local s = Sprite(16, 16)
s.gridBounds = Rectangle(0, 0, 8, 8)
s.cels[1].image:drawPixel(3, 3, app.pixelColor.rgba(255, 255, 255, 255))
local slice = s:newSlice(Rectangle(0, 0, 4, 4))
slice.name = 'regular'
local box = s:newSlice(Rectangle(-2, 1, 12, 9))
box.properties('aseprite.collision').kind = 'hitbox'
box.name = 'regular' -- Export-area lookup must still find the regular slice.
s:newFrame(1)
s:saveAs(app.params.output)
s:close()
LUA
"$ASEPRITE" -b --script-param "output=$collision_dir/source.aseprite" \
    --script "$collision_dir/create.lua" || exit 1

"$ASEPRITE" -b --list-slices "$collision_dir/source.aseprite" \
    > "$collision_dir/slices.txt" || exit 1
[[ "$(cat "$collision_dir/slices.txt")" == "regular" ]] || exit 1

"$ASEPRITE" -b "$collision_dir/source.aseprite" --trim --merge-duplicates \
    --sheet "$collision_dir/sheet.png" --format json-array \
    --data "$collision_dir/sheet.json" || exit 1
"$ASEPRITE" -b --split-grid "$collision_dir/source.aseprite" \
    --sheet "$collision_dir/grid.png" --format json-array \
    --data "$collision_dir/grid.json" || exit 1
"$ASEPRITE" -b "$collision_dir/source.aseprite" --slice regular --frame-range 0,0 \
    --save-as "$collision_dir/area.png" || exit 1
"$ASEPRITE" -b --export-tileset sprites/2x2tilemap2x2tile.aseprite \
    --sheet "$collision_dir/tiles.png" --format json-array \
    --data "$collision_dir/tiles.json" || exit 1

cat > "$collision_dir/check.lua" <<'LUA'
local dir = app.params.dir
local function read(name)
  local f = assert(io.open(dir..'/'..name..'.json', 'r'))
  local data = json.decode(f:read('*a')); f:close()
  return data
end
local data = read('sheet')
assert(#data.frames == 2)
assert(data.meta.slices == nil)
for i = 1, #data.frames do
  local frame = data.frames[i]
  assert(#frame.hitboxes == 1 and #frame.hurtboxes == 0)
  assert(frame.hitboxes[1].bounds.x == -2 and frame.hitboxes[1].bounds.w == 12)
end
for _, name in ipairs{'grid', 'tiles'} do
  data = read(name)
  assert(#data.frames > 0)
  for i = 1, #data.frames do
    assert(data.frames[i].hitboxes == nil and data.frames[i].hurtboxes == nil)
  end
end
local area = Sprite{fromFile=dir..'/area.png'}
assert(area.width == 4 and area.height == 4)
area:close()
LUA
"$ASEPRITE" -b --script-param "dir=$collision_dir" --script "$collision_dir/check.lua" || exit 1
