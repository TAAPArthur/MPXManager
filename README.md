# My Personal XWindow Manager (MPX Manager)

## Intro
MPX Manager started off as a project to fill void of MPX aware window managers. Before its creation I tried to hack MPX support into existing window managers namely Xmonad. Due to the limitations of hacked support and inconvenience of maintaining, I ended up leaving Xmonad and exploring other WMs. I wanted to compile all the MPX hacks into an independent project that could run in conduction with any EWMH/ICCCM compliant window manager. One of the hacks was setting the  border color of the active window depending on which master device had focus. This involved telling the WM to not set the border color itself. This was a ~1 line change, but the thought occurred to me: why should I have to dig through the source code to make a simple one line change in these supposed customizable WMs? Clearly the level of customization the developers planned did not meet my expectation. So I decided to create MPX Manager.

# Features
## Full MPX Support
MPX Manager was designed from the ground up to support a variable number of master devices. By default master devices operate independently and do not interfere with each other whenever possible.

## Configurable
We strived to make as few assumptions as possible while still be compliant with the EWMH/ICCCM spec. Any ones we use can be tuned with a variable in the configuration file. For the changes we did account, the code base is well documented to make hacks as painless as possible

## Multi monitor support
We support having multiple monitors that each will hold one workspace.

##  EWMH/ICCCM compliment

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

# Scriptable
MPXManager creates and reads from a named pipe to allow arbitary clients to affect that state in addition to regular EWMH/ICCCM methods


## Missing Features
  * Advanced layouts
  * Live config editing (needs to be recompiled)


# Install
`make install`


