# Workshop Tool

Simple tool for downloading addons from Steam Workshop. [MIT Licensed](./LICENSE.md)

Tool has been designed for L4D2 dedicated servers (where host_workshop_collection still does not exist), but should work for any other game's workshop (sometimes workshop is private and you need to use Steam or external web services).

## Getting started
- Linux users: install CURL
- [Download](../releases/latest) and unpack app

## How to use
- Create *collections.txt* and *addons.txt* in tool's location
- Paste IDs to appropriate files. One ID per line!
- (https://steamcommunity.com/sharedfiles/filedetails/?id=XXX - XXX is the id of collection or addon)
- Run the tool in terminal. Addons will be downloaded to *addons* directory.

## Notes
- No params **yet**
- Every run will download everything again. No existence/update detection **yet**
- Download progress is not visible

## Build
- Requires CMake 3.15 and installed CURL library
- Linux: requires fmt library
