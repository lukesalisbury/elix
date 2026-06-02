Elix
====================================
Simple Cross-Platform Helper Functions. Written mostly in C. 

## Building
Elix is designed to be include with your code. For testing and building examples, use genscript.c


## Core - *elix_core.h*
### New Macros
`ASSERT(Expression)` * Triggers debugger on failure*

`NULLIFY(Variable)` * If Variable is not null, delete and set to null*

### New Types
- elix_colour - 32bit colour value as hex or r,b,g,a values
- elix_graphic_data - Used to describe an 2D graphic surface

## C String - *elix_cstring.h*
Helper functions to deal with C-Strings
- bool elix_cstring_has_suffix( const char * str, const char * suffix);
- size_t elix_cstring_find_not_of( char * str, char * search, size_t offset = 0);
- void  elix_cstring_sanitise( char * string );
- char * elix_cstring_substr( const char * source, ssize_t pos = 0, ssize_t len = SSIZE_MAX );

## Endian - *elix_endian.h*
Switch integers between Network Endianness and Host Endianness.

## Hashmap - *elix_hashmap.h*

## HTML Parser - *elix_html.h*
Reads a HTML and create a tree node structure.

## JSON Parser - *elix_json.h*
Reads a JSON into a list of elements.

## RGBA Buffer - *elix_rgbabuffer.h*
Canvas interface for elix_graphic_data. Borrows from the HTML's Canvas.

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

*I recommend using SDL3 instead of this*

## System Windows - *elix_os_window.hpp* - In development
Simple windows creation and event handling. Basic support Windows and Wayland

*I recommend using SDL3 instead of this*

## Network Sockets - *elix_networksocket.h*
Tries to proved nicer interface to sockets, and listing Interfaces.



# TODO
* HTTP Client/Server
* Tree Node Render
* OS Integration/File Association
* Directory Watcher
* Nofications (GUI) 

# Notes

## Examples
- genscript.c
- example/elix_http_client.c
- example/elix_http_server.c


## Platform Notes
### Wayland
Wayland needs a some resources which you create with the following commands
`
wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml include/wayland/xdg-shell-protocol.c
wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml include/xdg-shell-client-protocol.h


wayland-scanner private-code /usr/share/wayland-protocols/unstable/primary-selection/primary-selection-unstable-v1.xml include/wayland/primary-selection-protocol.c
wayland-scanner client-header /usr/share/wayland-protocols/unstable/primary-selection/primary-selection-unstable-v1.xml include/primary-selection-client-protocol.h
`