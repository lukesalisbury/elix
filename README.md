Elix
====================================
Simple Cross-Platform Helper Functions.

Rewritten in C style C++. Still a work in progress


## Core - *elix_core.h*
### New Macros
`ASSERT(Expression)` * Triggers debugger on failure*

`NULLIFY(Variable)` * If Variable is not null, delete and set to null*

### New Types
elix_colour - 32bit colour value as hex or r,b,g,a values

elix_graphic_data - Used to describe an 2D graphic surface

## C String - *elix_cstring.cpp*
Helper functions to deal with C-Strings
- bool elix_cstring_has_suffix( const char * str, const char * suffix);
- size_t elix_cstring_find_not_of( char * str, char * search, size_t offset = 0);
- void  elix_cstring_sanitise( char * string );
- char * elix_cstring_substr( const char * source, ssize_t pos = 0, ssize_t len = SSIZE_MAX );

## Endian - *elix_endian.hpp*
Switch integers between Network Endianness and Host Endianness.

## Hashmap - *elix_hashmap*


## HTML Parser - *elix_html.hpp*
Reads a HTML and create a tree node structure.

## RGBA Buffer - *elix_rgbabuffer.hpp*
Canvas interface for elix_graphic_data. Borrows from the HTML's Canvas.

## File & File Info
Access file content and details about file.

## Path & Path Info

## Program Info and Settings
 - Name [string]
 - Version [string]
 - Major Version [string] *Used for Directory*
 - Executable Path [string]
 - Base Path [string]

## System Directories/Path File
Gets:
* Private or Public Document directory
* User's Data directory AKA XDG_DATA_HOME/[ProgramName]-[ProgramMajorVersion] or CSIDL_Local_APPDATA/[ProgramName]-[ProgramMajorVersion]
* User's Cache directory
* Program's Resources directory aka /usr/share/[ProgramName]-[ProgramMajorVersion]

## System Windows - *elix_os_window.hpp*
Simple windows creation and event handling. Currently Windows and Wayland

*I recommend using SDL2 instead of this*

## Data Packages


# TODO
* HTTP Client
* HTTP Server
* Tree Node Render
* OS Integration/File Association
* SSL




# Defines, Settings, and other notes

## Defines

### PLATFORM_[Platform Name]

### PLATFORM_BITS
### PLATFORM_ARCH

### PLATFORM_SDL2_ONLY and PLATFORM_SDL2
Force the use of SDL2 instead of the native platform code.


#### ELIX_SKIP_CORE_LOG
Built-ins LOG_+ functions do not print anything.






##Platform Nots
### Wayland
wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml include/wayland/xdg-shell-protocol.c
wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml include/xdg-shell-client-protocol.h
