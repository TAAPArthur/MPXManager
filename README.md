# My Personal XWindow Manager (MPX Manager)


# Install & Run
`make install`

`mpxmanager`

See `man mpxmanager` or `mpxmanager --help` for details on arguments

Just run `make` if you want to use it without installing
# Intro
MPX Manager is a X11, tiling window manager with [MPX (multiple pointer x)](https://wiki.archlinux.org/index.php/Multi-pointer_X) support.

## Motivation
MPX Manager started off as a project to fill void of MPX aware window managers. Before its creation I tried to hack MPX support into existing window managers namely Xmonad. Due to the limitations of hacked support and inconvenience of maintaining, I ended up leaving Xmonad and exploring other WMs. I wanted to compile all the MPX hacks into an independent project that could run in conduction with any EWMH/ICCCM compliant window manager. One of the hacks was setting the  border color of the active window depending on which master device had focus. This involved telling the WM to not set the border color itself. This was a ~1 line change, but the thought occurred to me: why should I have to dig through the source code to make a simple one line change in these supposed customizable WMs? Clearly the level of customization the developers planned did not meet my expectation. So I decided to create MPX Manager.

# Features
## Full MPX Support
MPX Manager was designed from the ground up to support a variable number of master devices. By default master devices operate independently and do not interfere with each other whenever possible. However, there is no true isolation by default (an optional extension may be developed later). Master device can interfer with by deliberate action like switching workspaces.

## Configurable
We strived to make as few assumptions as possible while still be compliant with the EWMH/ICCCM spec. The assumptions we made fo user convenience can be toggled in the configuration file. For the changes we did account for, the code base is well documented to make hacks as painless as possible. And hacks, no matter how pointless are encouraged. The goal is for users to be able to do whatever they want.

##  EWMH/ICCCM compliment (mainly)
There is currently no support for  `_NET_CLIENT_LIST_STACKING`
There is currently no support for window gravity

## Multi monitor support
Each Workspace maps to at most 1 monitors.

## Key/mouse bindings
Key/mouse press/release/motion bindings.  We also support chain bindings.

## Hooks
Users can run custom commands upon any arbitrary X11 event being detected. Users can run custom commands for set points in the MPXManager source like when starting the Xsession or retiling windows

## Sane defaults
   * A useable set a layouts
   * Xmonad-like default keybindings
   * Alt-tab window cycling
   * Default dock detection
   * Auto-float dialog windows
   * Always on top/bottom windows
   * Sticky windows

## Scriptable
MPXManager creates and reads from a named pipe to allow arbitrary clients to affect that state in addition to regular EWMH/ICCCM methods

## Documentation
All structs/vars/methods/files have Doxygen documentation. But there is no tutorial. The code base had around ~6000 lines of useable CPP (including header files).

# Key Terms/Classes
* BoundFunction -- fancy function pointer
* Binding   -- triggers a specified BoundFunction when a set of modifer and specific key/button is pressed/release or a mouse is moved.
* WindowStack -- a collection of windows
  * Workspace Stack - list of windows in a workspace
  * Master Stack - list of windows a master has focused
* WindowMask -- describe how windows should be treated. Many correspond to many the [_NET_WM_STATE](https://standards.freedesktop.org/wm-spec/wm-spec-latest.html#idm140200472615568) options, generic window state like if the client wants it to be mapped and some user defined state like if it is floating

# Examples
You would put the following code in your config file which is located by default in $HOME/.mpxmanager/
You want some window to always be above the others. You found the window class, C, by using the `--dump` command ( or used a tool like xprop, xwininfo etc).

For more examples see settings.cpp for all the defaults

# Troubleshooting
## Cannot enter/leave fullscreen in application X
Some applications don't differentiate between application level fullscreen (hiding tabs/tools bars) from WM level full screen (taking up the entire screen). MPXManager by default does and denies applications that wish to change the WM_STATE. This can cause applications like Firefox, to subsequently ignore users who are trying to enter fullscreen (like through F11 or by trying to fullscreen a video).
If you agree with us, then fix, the application. For firefox, this is as simple as full-screen-api.ignore-widgets field in about:config to true.

If you don't, add `MASKS_TO_SYNC |= FULLSCREEN_MASK` to your config file. This causes FULLSCREEN_MASK to be in sync with _NET_WM_STATE_FULLSCREEN. So causing the application to be "full screen" will likely cause the window to actually take up the entire screen automatically. Alternatively you remove the ewmh rules thus making other applications not known their is a EWMH compliant WM running and they won't try and ask us. This is not recommended as it can cause other random problems, see below.

## Applications X steals focus overtime a use a key binding
This problem has been reported in Chromium, when ewmh rules are not set. The application doesn't know there isn't a EWMH complainant WM running (because we don't tell anyone) so some applications will steal focus. By default we call `addEWMHRules()` so this problem shouldn't occur (or its a bug in that application) unless the user explicitly removed it.


# License
MIT
