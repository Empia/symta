use gfx_

GFX_RGB  = 0
GFX_RGBA = 1
GFX_MAP  = 2

ffi_alloc Size = ffi_alloc_ Size
ffi_free Ptr = ffi_free_ Ptr

new_cmap Xs =
| when Xs.size > 256: bad "cant set color map larger than 256"
| P = ffi_alloc Xs.size*4
| for [I E] Xs.i: _ffi_set uint32_t P I E
| P

type widget

type gfx.widget.no_copy{@As} handle
| $handle <= case As
  [W H] | new_gfx_ W H
  [Filename<1.is_text] | gfx_load_png Filename
  Else | bad "cant create gfx from [As]"
gfx.free = free_gfx $handle
gfx.w = gfx_w $handle
gfx.h = gfx_h $handle
gfx.hotspot = [(gfx_hotspot_x $handle) (gfx_hotspot_y $handle)]
gfx.`!hotspot` [X Y] = gfx_set_hotspot $handle X Y
gfx.get X Y = gfx_get $handle X Y
gfx.set X Y Color = gfx_set $handle X Y Color
gfx.clear Color = gfx_clear $handle Color
gfx.line Color A B = gfx_line $handle Color A.0 A.1 B.0 B.1
gfx.rect Color Fill X Y W H = gfx_rect $handle Color Fill X Y W H
gfx.circle Color Fill C R = gfx_circle $handle Color Fill C.0 C.1 R
gfx.triangle Color A B C = gfx_triangle $handle Color A.0 A.1 B.0 B.1 C.0 C.1
gfx.resize W H = gfx_resize $handle W H
gfx.save Filename = gfx_save_png Filename $handle
gfx.cmap raw/0 =
| P = gfx_cmap $handle
| when Raw: leave P
| less P: leave 0
| dup I 256: _ffi_get uint32_t P I
gfx.`!cmap` NewCM =
| when NewCM.size > 256: bad "cant set color map with more than 256 colors"
| P = gfx_enable_cmap $handle
| for [I E] NewCM.i: _ffi_set uint32_t P I E
gfx.blit P Src rect/0 flipX/0 flipY/0 map/0 =
| less Src.is_gfx:
  | Src.draw{Me P}
  | leave 0
| [SX SY SW SH] = if Rect then Rect else [0 0 Src.w Src.h]
| gfx_blit $handle P.0 P.1 Src.handle SX SY SW SH FlipX FlipY Map
gfx.margins =
| P = gfx_margins $handle
| [(_ffi_get uint32_t P 0) (_ffi_get uint32_t P 1)
   (_ffi_get uint32_t P 2) (_ffi_get uint32_t P 3)]
gfx.cut X Y W H =
| G = gfx W H
| G.clear{0}
| CMap = $cmap
| when CMap: G.cmap <= CMap
| G.blit{[0 0] Me rect [X Y W H]}
| G
gfx.copy = $cut{0 0 $w $h}
gfx.deep_copy = $cut{0 0 $w $h}
gfx.frames W H =
| GW = $w
| dup I GW*$h/(W*H): $cut{I*W%GW I*W/GW*H W H}
gfx.render = Me
gfx.as_text = "#gfx{[$w] [$h]}"

rgb R G B = form R*#10000 + G*#100 + B
rgba R G B A = form A*#1000000 + R*#10000 + G*#100 + B

export widget gfx new_cmap ffi_alloc ffi_free rgb rgba 'rgb' 'rgba' 'GFX_RGB' 'GFX_RGBA' 'GFX_MAP'
