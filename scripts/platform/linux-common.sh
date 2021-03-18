#!/bin/sh

wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ./include/xdg-shell-client-protocol.h
wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ./include/wayland/xdg-shell-protocol.c