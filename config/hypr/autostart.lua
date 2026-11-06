-- Extra autostart processes.
-- o.launch_on_start("my-service")

-- Launch Plus TV Media Hub automatically on startup (after shell and background initialize) and keep it alive on Workspace 1.
o.exec_on_start("sh -c 'for i in $(seq 1 60); do alpha=$(hyprctl layers -j 2>/dev/null | jq -r \".. | select(.namespace? == \\\"omarchy-background\\\") | .alpha\" 2>/dev/null | head -n 1); if [[ \"$alpha\" == \"1\" ]]; then break; fi; sleep 0.25; done; sleep 0.5; while true; do pgrep -x plustv >/dev/null 2>&1 || uwsm-app -- /home/enisozen/.local/bin/plustv; sleep 2; done'")
