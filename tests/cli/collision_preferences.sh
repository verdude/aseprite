#!/bin/bash

collision_prefs_dir="$t/collision-preferences"
mkdir -p "$collision_prefs_dir"
cat > "$collision_prefs_dir/check.lua" <<'LUA'
assert(app.preferences.collision.create_kind == tonumber(app.params.expected))
assert(app.preferences.collision.pushbox_color.rgbaPixel == Color(0, 0, 255).rgbaPixel)
local s = Sprite(16, 16)
assert(app.preferences.document(s).show.pushboxes)
s:close()
LUA

check_collision_preference() {
    local collision_case_dir="$collision_prefs_dir/$1"
    mkdir -p "$collision_case_dir"
    printf '[collision]\n%s\n%s\n' "$2" "$3" > "$collision_case_dir/aseprite.ini"
    ASEPRITE_USER_FOLDER="$collision_case_dir" "$ASEPRITE" -b \
        --script-param "expected=$4" --script "$collision_prefs_dir/check.lua" || exit 1
}

# SliceKind values: regular slice=0, hitbox=1, hurtbox=2, pushbox=3.
check_collision_preference default '' '' 1
check_collision_preference old-hitbox 'create_hurtbox=false' '' 1
check_collision_preference old-hurtbox 'create_hurtbox=true' '' 2
check_collision_preference new-pushbox 'create_hurtbox=true' 'create_kind=3' 3
check_collision_preference new-hitbox 'create_hurtbox=true' 'create_kind=1' 1
check_collision_preference invalid 'create_hurtbox=true' 'create_kind=99' 1
