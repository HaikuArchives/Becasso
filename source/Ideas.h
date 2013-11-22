/* Todo, log and ideas for Becasso

// FEATURES

* Pattern editor
* Alpha masking.
* Freehand tools (optionally) shouldn't auto-scroll.
* More Wacom support, natural media
* Store/restore of selection
* Multi-level clipboard [?]
* Exception handling for memory shortage robustness
* Layer thumbnails draggable!
* Thumbnails in file icons
* Color drops directly from canvas
* Alpha blending in ConstructCanvas top-down?

// BUGS

* Scaling: Fix Text tool
* Undo:
  - faster setup
  - only store the changed part.  Offset-table for diagonals?
  - Crop destroys undo!
* Pattern alignment bug in >100% view. <- Not my fault?!
* Screenchanged also in Main Window!  <===
* Some strange clipping thing:  Undo in 200% view...
* OpenAddon/CloseAddon snooze() hacks!
* After Undo, set the mouse cursor correctly!
* Clipboard
* "Real" Lab colors, once.
* File format with text fields for comments et al.  <=========
* Becasso CRASHES if it tries to open a "Fake" Becasso file!!
* Add Invert Alpha channel menu item
* Padding is broken.

// DONE FOR 1.1

* Port to Intel / R3
* InterfaceElements BBP
* Basic scripting support
* Basic link with ImageElements
* Symlinks
* Capture import
* Double click to access tool inspector / color editor (Loren Petrich)
* Channel operations
* Layer merging
* Layer menu accessible from canvas window
* Preserve selection after cut/copy
* More add-ons
* Splash screen shows what Becasso is doing
* Hotkey to attach selection to mouse (Enter)
* Hotkey to switch selection/drawing mode (Tab)
* Better support for single-pixel editing in freehand tool
* Wacom tablet support
* Separate thread for mouse tracking.  Smoother operation.
* Channel <-> Selection operations.
* Accepts RRaster and Icon drops
* Accepts command line file names and options
* Single dynamic Tool/Mode inspector windows
* Parametrizable brush
* Tear off tool menus
* More debugging output for add-on developers
* Improved Undo functionality
* Settable Anti-Aliasing in Text tool
* Printing
* Demo Version
* [API] CutOrCopy with NULL selection
* Now uses libz.so dynamically
* Handles images (or braindead translators) using alpha = transparency better

// DONE FOR 1.2

* Full Scripting support
* Color and Pattern Menus now also tearable
* Tablet support improved: every tool works with the tablet now
  (with extensions where it makes sense -> Spraycan)
* Fixed bug in serial port handling
* Improved LayerNameWindow
* Improved Spray Can (especially for selections)
* Improved Colorize
* Motion Blur add-on
* Added MMX code
* Spacing automatically co-adjusted with brush size
* Tertiary mouse button closes Polygon
* -c cli option to show the colors
* Show version and build date on startup with -D1
* AskForAlpha scripting switch

// DONE FOR 1.3

* Initial port to R4
* Spray can tool improved again
* Tertiary mouse button ends polyline
* Improved MMX code (Up to twice as fast!  Whoopee!)
* Much better performance with big images
* Fill tool fixed
* Thumbnail preview in Open File Panel
* new (anti-aliasing) Wave add-on
* new Ripple add-on
* Scale add-on improved
* Help about not being able to open files.  [<== Bug w.r.t. bgebruik.txt!]
* Improved Slider (keyboard navigable)
* Improved TabView (keyboard navigable)
* Canvases accept drops from ShowImage as well as files from the Tracker
* Dragging clippings onto Tracker works too
* Extensions when exporting
* Added Color Quantization routines
* 1.3.1: Fixed these for use with MALLOC_DEBUG (i.e. added bzero())

BUGS IN 1.3:

* Spray can tool broken [FIXED]
* Strange line in top left -- overwriting screenbitmap?! [FIXED - In ColorMenuButton?]
* Freehand, large pen size <== BeOS bug!!! [WORKED AROUND]
* Resources with MALLOC_DEBUG [FIXED]
* Delete layer [FIXED?]
* Redraw bugs in magnified view (result of speedup in Draw() method...) [WORKED AROUND]
* drop _to_ ShowImage not working [ShowImage bug]
* Scale add-on in magnified view


// DONE FOR 1.4:

* Better feedback for selection maps (Arvid Nilsson)
* Selection now according to alpha level of background/foreground colors
* Fixed bugs in selection dragging & center brush pixel
* Use MouseMoved instead of polling
* Moved Tablet support to input_server level (old behaviour through -t)
* Use new libpng-1.0.3 for PNGTranslator
* Added EPSTranslator
* GIFTranslator bug fixed (Chris Van Buskirk)
* Camera add-on (thanks to Fredrik Roubert)
* Fixed an (embarassing) bug in bgra2cmyk on x86
* Added "Open Recent" submenu
* Window position is now saved
* More feedback through mouse pointer shape (AN)
* Color picker now continuous (AN)
* Layer/Selection Translate/Rotate functionality added
* AN - Dragging a selection doesn't scroll at mag 100% [FIXED]
* Move mouse to last center/corner menu option
* Localization: German version, French version
* Fixed bug in printing
* Improved Create canvas panel (resolution aware)
* Improved AutoCrop
* Added Solarize, Sepia, Negate, Palmcam, and ColorCurves add-ons


// DONE FOR 1.5:

* Revisited all the translators
* Fixed problem with Gobe translators
* Some new strings (6, 7, 8, 4, 108, 305, 306, 400, 401, 403, 404, 116), as well as fixing some forgotten ones
* Moved Translate/Rotate to Layers menu
* Added Flip Horizontal/Vertical
* Added Resize To…
* Added BumpMap add-on
* Added scaling in ripple addon
* Fixed Generate Palette not invalidating onscreen palette bug
* Anti aliasing of text in TabViews fixed
* (Re)Size selector now uses correct values when pressing Enter before tabbing out
* Param windows now accept first click without needing to focus first
* Canvas now centered if the window is bigger.
* Drawing can now start on the gray background area too.
* Improved brush for distance > 1 drawing
* Added "Hardness" parameter in Brush tool
* Added Magnify Window for pixel-perfect editing (undo/redo on per pixel basis)
* Added color picker to Magnify window
* Invisible operation for use in scripting
* New window with selection
* New layer with selection
* Fixed printing in R5
* Use BPropertyInfo
* hey Becasso Crop with rect=BRect[10,10,100,100]
* Improved dragging to desktop (shows file format info in popup now)
* Translators: fix B_OP_OVER in config views! [DONE]
* GP Text translator bug in OutputFormatWindow [WORKED AROUND]
* Added BMP Translator
* Added PSD Translator
* Changed Sepia into DuoTone filter
* Don't add the extension to the window title after exporting
* Improved extension suggestion / MIME updater heuristic
* aPreview() when palette changed
* No longer a separate demo version; it works with a keyfile now
* Cosmetic changes in the Tool/Mode parameter windows (show icon now)
* Slider now accepts Esc key
* Switch (-d) for Demonstrator (ignore window positions etc.)
* In combination with -d: -w will write out settings anyway
* Better performance on low-memory machines

// DONE FOR 1.5.1:

* VideoGrabber and WinGrab add-on added to distribution
* Small bug fixes to add-ons (mainly compiler setting issues)

// DONE FOR 2.0

* Tool menu has 2x7 matrix layout
* Clone tool added (string #35)
* Text tool improved (multiline texts supported, easier text positioning)
* Selection mode now visible in cursor shape
* New add-on API, View-based instead of Window-based (reduces number of threads)
* Add-Ons: better notification on color change (not just invalidate)
* Brightness/Contrast and AutoContrast Add-Ons added
* Updated Translators, upgraded to libpng-1.2.0
* Added (r/w) TIFF Translator
* Fixed rotate-while-scaled bug
* Eye candy: Tear-offs are now translucent
* Tear-off Menu positions are remembered across restarts
* Copy to New Layer / New Canvas are now more consistent: Allow copies from
  other canvas.  Renamed to "Paste". (String #404)
* Arrow keys move cursor (Control-Arrow = move 10 pixels)
* Space bar == mouse click (Shift-Space = right click)
* More information (delta/radius) in PosView
* Tip of the Day (string 387 prefs, 500+)
* Color of new canvas (#406 - 409)
* General: Better responsiveness in tear-offs, removed flicker
* "Register online" menu item (#9)
* Add-on selection string #386

// DONE FOR 2.1

* Fixed bug with multibyte characters in GUI strings (Sergei Dolgov)
* Fixed "layer names have space prepended when loading" bug (Alexander Smith)
* Added ChromaKey filter
* Fixed PPC registration problems
* Zooming (Alt -/+) centers around cursor

TODO/IDEAS

* Drag to desktop while brush is on leaves trace
* Show all translators, not all formats (for multiple translators)
* More basic: Pixel alignment in magnified view [FIXED?]
* Problem with other versioned images (R'alf) [FIXED - -fomit-frame-pointer!!]
* Test run for timing - preview of filters?
* push ebx?
* Scriptable Translators? [Naah]
* CWD Property
* Colin Cashman:
  - Add "Paste Align"
  - Add "Delete Selection"
* Fix off-screen opening of windows (Color Editor!)
* Fix undo tool (once and for all...)
* AN - undo while in add-on
* AN - polygon tool: redundant redraws. [?]
* (PS) Keymap stuff
* hey Becasso delete Canvas
* Tip of the Day
* Preference: Save/Export location
* Translators: Go to new API (BTranslator) (oh, why bother)
* -S3: Progress alerts... (documentation: hey Becasso quit Canvas 0 !!)
* Add-ons for P5!!!
* palette loading
* Warn when exporting to lo-color format
* WinGrabbing one of our own windows (fixed) - add GUI.
* Note Credits in docs
* Drops to ShowImage?
* Scripting documentation (command strings)
* Document Gobe Productive external editing
* IMPORTANT NOTE: When burning a CD-ROM, make sure its volume name is BECASSO and 
  it contains a file README (the registration checks for that).
* Color Editor window too large for small screen
* Crash when using tools in Generator mode
* Pete: Export Preview
* Add-Ons: notify on mode change so they can say whether it makes sense for selections
*          ADDON_RESIZE
* -mpentium compilation flag default?
* NOTE: File loading routines in three places: CanvasView, BitmapStuff and Translator.
* Pre-render and cache all layers below and above (separately) the current layer for 
  quicker updates during editing
* Dragging a selection can crash?  <---
* Ability to track lines or polygons with the brush tool (Christian Wulff)
* Sub-pixel brush tool (Stephan Assmus)

*/