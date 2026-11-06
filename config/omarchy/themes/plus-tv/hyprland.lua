-- Plus TV Theme - Vivid Cinematic Edition
local active_border_color = { colors = { "rgba(ff0055ee)", "rgba(8b00ffee)", "rgba(00f5ffee)" }, angle = 45 }
local inactive_border_color = "rgba(121428cc)"
local active_shadow_color = "rgba(ff005548)"
local inactive_shadow_color = "rgba(00000050)"

hl.config({
  general = {
    col = {
      active_border = active_border_color,
      inactive_border = inactive_border_color,
    },
  },

  group = {
    col = {
      border_active = active_border_color,
      border_inactive = inactive_border_color,
    },
  },

  decoration = {
    rounding = 8,
    active_opacity = 0.95,
    inactive_opacity = 0.88,

    shadow = {
      enabled = true,
      range = 20,
      render_power = 3,
      color = active_shadow_color,
      color_inactive = inactive_shadow_color,
    },

    blur = {
      enabled = true,
      size = 6,
      passes = 3,
      new_optimizations = true,
      xray = false,
      noise = 0.015,
      contrast = 1.10,
      brightness = 0.95,
      vibrancy = 0.40,
      vibrancy_darkness = 0.20,
      popups = true,
      special = true,
    },
  },
})

-- Layer rules: Frosted glass blur for Omarchy shell surfaces
hl.layer_rule({
  match = { namespace = "^(omarchy-bar|omarchy-menu|omarchy-notifications|omarchy-osd|omarchy-polkit|omarchy-reminders|omarchy-clipboard|omarchy-emojis|omarchy-image-selector|omarchy-keyboard-panel|omarchy-network-qr)$" },
  blur = true,
  ignore_alpha = 0.2,
})

-- Window opacity rules:
-- Terminals and desktop apps: subtle modern translucency, high legibility
o.transparent_window({ tag = "terminal" }, "0.93 0.87")
o.transparent_window("(omawrite|1[pP]assword|com\\.onepassword\\.OnePassword|org\\.gnome\\.Nautilus)", "0.93 0.87")
o.transparent_window({ tag = "transparent" }, "0.93 0.87")

-- Plus TV media application: Always keep 100% opaque for vivid media playback
o.window("^plustv$", { opacity = "1.0 1.0" })
