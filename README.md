# WorkShop Tool

Simple tool for downloading addons from Steam Workshop.

Tool has been designed for L4D2 dedicated servers (where host_workshop_collection still does not exist), but should work for any other game's workshop.

[MIT Licensed](./LICENSE)

Prebuilt binaries are not available due to Work In Progress project stage.

## Build Tips
- HTTPLIB_REQUIRE_OPENSSL should be ON
- Change CMAKE_INSTALL_DIR
- All other options should be left default
- Ignore *cmake* and *include* in install dir

## How to use
- No params **yet**
- Create *steam.wst* at working directory (if you're not familiar, just create this file at tool's location)
- Inside the file paste ids
- First line is an collection id. If no collection, type '0'.
- Next lines are addons. Paste as many as possible.
- Run. If Steam/your connection is not down, new directory *wst* will appear.
- Inside this directory there are all your addons.
- Every run will redownload everything. No existence/update detection **yet**
- No duplication check **yet**. If you pasted any id twice, it will be downloaded twice
