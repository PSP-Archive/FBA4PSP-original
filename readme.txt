FB Alpha  - http://fba.emuunlim.com
          - http://www.barryharris.me.uk
----------------------------------------

FB Alpha is an arcade emulator supporting the following hardware platforms;

 - Capcom CPS-1
 - Capcom CPS-2
 - Capcom CPS-3
 - Cave
 - Neo Geo
 - Sega System 16 (and similar), System 18, X-Board, Y-Board
 - Toaplan
 - Taito
 - Psikyo 68EC020 based hardware
 - misc stuff we like


What's New?
-----------

See the seperate whatsnew.txt file

System requirements
-------------------

OS:	You need Windows 98 or higher, preferably Windows 2000 or higher.
	DirectX7 or higher must be installed, some features require DirectX 9.
	Normal (unicode) builds will run only on Windows 2000/XP (or higher).
	non-unicode builds will run on Windows 98/Me as well.
CPU:	CPU speed should be at least equivalent to a Pentium 233MHz, to play
        all games at reasonable	framerates at least 366MHz is required. Exact
        requirements can vary by system or game, as well as video and audio
        settings.
	Normal builds require a Pentium II or higher, or an Athlon or higher.
	A seperate build is available for Pentium and K6 CPUs (MMX is not
	required, although some effects will not work without).
	The about box and the version info both show what CPU your build was
	optimised for (you can check the version info by right-clicking
	fba.exe	in the explorer and selecting properties, then the version
	tab on the window that appears). i586 optimised builds run on
	Pentium/K6 CPUs.
RAM:	192MB or more is recommended. At least 128MB is needed to run all
	games, 64MB should be considered the bare minimum.
VIDEO:	15/16/24/32bpp displays are supported.
	Basic blitter: DirectDraw7 support is required.
	The Basic blitter will work with any videocard that supports
	the minimum requirements.

	Enhanced blitter: DirectDraw7 + Direct3D7 support is required.
	The Enhanced blitter will not work with every video card available,
	specifically older Voodoo cards (up Voodoo 3000) will not work.
	Experimental blitter: DirectX Graphics 9 support is required.

	The Experimental blitter has limited functionality with DirectX 8.1
	compatible hardware, but needs DirectX 9 for full functionality.
	For optimal results, Geforce FX 5900 series or Radeon 9800 series
	(or faster) graphics cards are required. For these cards, select
	the appropriate filter version from the Advanced options submenu.
	For all other compatible cards (e.g. Geforce FX series or Radeon 8500
	and higher), select the high-performance option.

SOUND:	Any sound hardware supporting DirectSound3 should work.
	Sound is optional.
INPUT:	The keyboard and system mouse will always work. There is preliminary
	support for additional mice, and devices such as the X Arcade joystick
	which act as a keyboard are supported. All standard or DirectInput7
	compatible joysticks/joypads, and mice should work.


How to use FB Alpha
-------------------

Make sure your ROMs are zipped and correct for use with FB Alpha (it can
write out dat files for ClrMame pro and Romcenter). Make sure FB Alpha knows
where to find your ROMs by specifying them in the ROM dirs dialog (accessible
from via the "Load Game" option). When adding new games, use the Rescan Roms
button, again accessible via the "Load Game" option. When viewing the "Load
Game" dialog, if a game doesn't appear as available, you do not have a
compatible or complete ROMset for that game. If you are missing only some
non-default optional files (e.g. alternative Neo Geo BIOSes), the game will
be faintly hightlighted. Uncheck the "Show available only" option to show
unavailable (they will show up highlighted).

Do NOT ask us where to get the ROMs for any game.

Select the game you want to play and click "OK". When the game is loaded, you
can set up inputs for it by selecting "Map game inputs" from the Game menu.
Double-click an input to assign it, or choose a preset for each player using
the drop-down boxes, and click the "Use preset" button to use that preset for
just this game or session, or click "Make default" to make that preset the
default for all games.

You can also play against people on the internet by selecting the "Play via
Kaillera" option from the Game menu instead of "Load game".


Commandline interface
---------------------

FB Alpha can also be invoked with command line options. When invoked this
way, FB Alpha will automatically switch to fullscreen mode when a game is
loaded, and the Escape key quits FB Alpha. The options are as follows;

fba <game> [-listinfo|-w|-a|-r <width>x<height>x<depth>]

<game>      = The game's romname. You can specify the filename of
              a savestate or input recording instead.
-w          = Run in a window instead of fullscreen.
-a          = Use the same resolution as the original arcade game.
-r          = Specify a resolution. Depth is optional.

If neither -a nor -r are specified, the default fullscreen resolution is
used.

Examples:

fba sfa3 -r 800x600x32

fba awesome_sfzch_recording.fr -w

fba vsav2 -a

For front-ends, you can also do fba -listinfo, -listinfowithmd, or
-listinfomdonly which will output information about the supported games to
stdout, and the ROM files they need in a MAME compatible format.

You can also do fba -listextrainfo. This will output the following data in
a tab-delimited format to stdout; setname, visible size, aspect ratio,
hardware code, hardware description, working status, max players, comments.


Creating your own input presets
-------------------------------

You can add to the included presets by creating a .ini file in the
config\presets folder. A file example.ini is provided already as an example.
The preset name will be the filename without the ".ini", eg, example.ini is
displayed as example. In the .ini file it is possible to use specific names
such as "P1 Low Punch" or generic names such as "P1 Fire 1". When a game is
allocating inputs it will look for the specific names first, and then the
generic ones. The example provided has generic inputs only (unzip
"preset-example.zip" in the FB Alpha directory).


Usage/optimisation tips
-----------------------

Video

 - If the Enhanced blitter works on your system, use it, esp. for 32-bit
   screenmodes. Try to switch off the "Use DirectX texture management",
   as this can sometimes result in better speed, esp. if you have an nVidia
   card.
 - If the Enhanced blitter is very slow in windowed mode, hit alt-enter
   twice, going into full-screen mode and back. This will force DirectX to
   reorganise the memory on your video card.
 - If you have a monitor that can be rotated to a vertical orientation,
   uncheck "Rotate vertically aligned games" in the video menu. If vertical
   games don't have the correct orientation, check "Mirrored vertical
   alignment" in the "Monitor Properties" submenu as well.
 - Use "Double size", "Triple size", etc. options to control the window size
   and the resolution used for fullscreen mode. This will ensure all games
   will look similar without per-system or per-game settings.
   Note that: To view some games at 4x zoom requires a resolution higher than
	      1600x1200. If you can add custom resolutions to your videocard,
	      1712x1284 is a good choice.
	    - At high resolutions, RGB effects and triple-buffering require
	      large amounts of video memory. You may not be able to use them
	      both at the same time for all games if your videocard has less
	      then 64MB video RAM.
 - If you can add custom resolutions to your videocard, you can instruct
   FB Alpha to try to use the exact same resolutions the original game uses.
   To do this, select "Arcade" from the fullscreen resolution submenu. Then
   load a game and hit Alt-Enter to go to fullscreen mode. When this fails,
   FB Alpha will report the exact resolution you need to add to display this
   particular game so the image fits the screen without stretching (it will
   also take into account rotation for vertical games).

Sound

 - For the best sound sound quality, enable 4-point interpolation and
   FM interpolation.
 - If you have a really slow PC, set the sound samplerate at 22050 or 11025
   Hz, and make sure FM interpolation is disabled.

Input

 - If the mouse sensitivity is not to your liking, it can be changed for each
   game individually in the corresponding .ini file in the config\games
   directory. The label is "analog" and the default is 0x0100 (256 in decimal
   notation). Use higher values for faster movement, lower values result in
   slower movement.
 - If you turn off Auto-save input mapping, all games will have their input
   mapped according to the currently selected preset.

Misc

 - There are 4 directories that FB Alpha will search for ROMs available via
   the GUI, but there are a total of 8, the last 4 of which are accessible
   via editing the config\fba.ini file.
 - For Neo Geo games, FB Alpha can read\write (though not create) MAME memory
   cards.
 - You can replace the F logo image used for the blitter preview with your
   own image. Create a 32bpp .bmp image, in a resolution similar to the games
   FB Alpha supports (e.g. 304x224), and edit the szPlaceHolder in
   config\fba.ini
 - You can use multiple versions of FB Alpha from the same directory. Just
   rename one of the two fba.exe files. If you name the second .exe file
   fba2.exe, the application settings wil be stored in config\fba2.ini. Note
   that there might be problems if you use both versions to play the same
   game; the game's settings, but not control assignments, can be lost.
 - Should you ever want to return to the initial, default settings, delete
   the config subdirectory in your FB Alpha directory.
 - To hide the menu, right-click on the window. A second right mouse-click
   will restore it. While the menu is hidden in windowed mode, drag anywhere
   on the window to move it, and left-click to show the menu. Alternatively,
   you can use Ctrl + Enter to hide/show the menu.
 - If you select "associate with .fs and .fr extensions" from the Misc menu,
   FB Alpha will add some registry keys so that your savestates and input
   recordings can be started by simply double-clicking them in the explorer
   or on the desktop. Select "disassociate from .fs and .fr extensions" to
   remove the created registry keys completely.
 - You can still load the savestates created using savestate slots normally
   via the dialog or by double-clicking them.
 - There are some shortcut keys not listed in the menu. They are:
   Escape	Return to Windowed mode (from full-screen mode)
   F1		When in pause mode, run emulation for one frame
		When not in pause mode, unthrottle emulation while held down
   ALT + -	Sound volume down
   ALT + +	Sound volume up
   backspace	Toggle Frames per Second indicator
   T		On-Screen chat (during netplay only)


Acknowledgements
----------------

Thanks to: Dave, ElSemi, Gangta, OG, Razoola, Logiqx, TRAC, CrashTest,
           Andrea Mazzoleni, Derek Liauw Kie Fa, Dirk Stevens, Maxim Stepin,
           the MAME team, Shawn and Craig at XGaming.

Thanks also to everyone who has made suggestions, submitted code, or helped
in any other way.

