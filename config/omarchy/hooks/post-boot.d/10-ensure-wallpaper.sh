#!/bin/bash
# Omarchy post-boot hook: Ensure wallpaper and window transparency are properly active
for i in $(seq 1 30); do
  omarchy-shell shell ping >/dev/null 2>&1 && break
  sleep 0.2
done

/home/enisozen/.local/bin/omarchy-ensure-wallpaper
