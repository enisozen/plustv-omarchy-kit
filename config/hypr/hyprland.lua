-- Learn how to configure Hyprland: https://wiki.hypr.land/Configuring/Start/

-- Omarchy's bootstrap keeps path setup out of this user config.
dofile((os.getenv("OMARCHY_PATH") or "/usr/share/omarchy") .. "/default/hypr/bootstrap.lua")

-- Disable all Omarchy default bindings. Add your own in hypr/bindings.lua.
-- omarchy_default_bindings = false
--
-- Or disable only bindings for Omarchy's preinstalled apps/web apps while
-- keeping core window-manager bindings:
-- omarchy_preinstalled_bindings = false

-- Load Omarchy defaults.
require("default.hypr.omarchy")

-- Put your personal overrides in these files. They're loaded after Omarchy's
-- defaults so package updates can improve the defaults without rewriting your
-- ~/.config/hypr files.
require("hypr.monitors")
require("hypr.input")
require("hypr.bindings")
require("hypr.looknfeel")
require("hypr.autostart")

-- Toggle config flags dynamically.
require("default.hypr.toggles")

-- Add any other personal Hyprland configuration below.
-- o.window("qemu", { workspace = "5" })

-- Pin Plus TV exclusively to Workspace 1 and keep it always fullscreen.
o.window("^plustv$", { workspace = "1", fullscreen = true })

local function enforce_plustv_fullscreen(window)
  if not window or window.class ~= "plustv" then
    return
  end
  if (window.fullscreen or 0) ~= 2 then
    hl.timer(function()
      hl.dispatch(hl.dsp.window.fullscreen({
        mode = "fullscreen",
        action = "set",
        window = "address:" .. window.address,
      }))
    end, { type = "oneshot", timeout = 1 })
  end
end

local function next_app_workspace(ignore_address)
  local occupied = {}
  for _, w in ipairs(hl.get_windows()) do
    if w.address ~= ignore_address and w.class ~= "plustv" then
      local ws = w.workspace and tonumber(w.workspace.id)
      if ws and ws >= 2 then
        occupied[ws] = true
      end
    end
  end
  for ws = 2, 10 do
    if not occupied[ws] then
      return tostring(ws)
    end
  end
  return "2"
end

-- Any non-Plus TV application opened while on Workspace 1 starts opening from Workspace 2 onwards.
hl.on("window.open", function(window)
  if not window or window.pinned then
    return
  end
  if window.class == "plustv" then
    enforce_plustv_fullscreen(window)
    return
  end
  local ws = window.workspace and tonumber(window.workspace.id)
  if ws == 1 then
    local target = next_app_workspace(window.address)
    hl.timer(function()
      hl.dispatch(hl.dsp.window.move({
        workspace = target,
        follow = true,
        window = "address:" .. window.address,
      }))
    end, { type = "oneshot", timeout = 1 })
  end
end)

-- Prevent Plus TV from ever leaving fullscreen mode on Workspace 1.
hl.on("window.fullscreen", function(window)
  enforce_plustv_fullscreen(window)
end)

-- Keep Plus TV locked on Workspace 1 if moved, and prevent any other window from entering Workspace 1.
hl.on("window.move_to_workspace", function(window, workspace)
  if not window or not workspace then
    return
  end
  local ws = tonumber(workspace.id)
  if window.class == "plustv" and ws and ws ~= 1 then
    hl.timer(function()
      hl.dispatch(hl.dsp.window.move({
        workspace = "1",
        follow = false,
        window = "address:" .. window.address,
      }))
      enforce_plustv_fullscreen(window)
    end, { type = "oneshot", timeout = 1 })
  elseif window.class ~= "plustv" and ws == 1 then
    local target = next_app_workspace(window.address)
    hl.timer(function()
      hl.dispatch(hl.dsp.window.move({
        workspace = target,
        follow = false,
        window = "address:" .. window.address,
      }))
      os.execute("notify-send -u low -a 'Omarchy' 'Plus TV Masaüstü Koruması' 'Plus TV masaüstüne başka uygulama taşınamaz.' &")
    end, { type = "oneshot", timeout = 1 })
  end
end)

-- Ensure wallpaper is active whenever switching to a desktop workspace
hl.on("workspace.active", function()
  local ws = hl.get_active_workspace()
  if ws and ws.id and tonumber(ws.id) ~= 1 then
    hl.exec_cmd("/home/enisozen/.local/bin/omarchy-ensure-wallpaper")
  end
end)
