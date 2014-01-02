# Tascam US-2400 Control Surface Extension for Cockos Reaper

## Disclaimer

This is a working beta. Install and use at your own risk. It’s more or less the second half-serious thing I did in C++, I’m sure it could be done much better - any takers?

## Needs

* Cockos Reaper min. v4.57 (get it [here](http://www.reaper.fm/download.php "Download Reaper"))
* SWS Extension min. v2.4.0 (get it [here](http://www.standingwaterstudios.com/ "Standing Water Studios"))

## Install

* If you’ve never heard of GitHub – the ‘Download ZIP‘-Button should be somewhere on the right, use it to download the files, extract them and proceed as follows
* reaper_csurf_us2400.dll goes into Programs\REAPER\Plugins
* All scripts (‘Add Sends’ / ‘Remove Sends’) go into Reaper’s Scripts directory: Documents and Settings\YourUsername\Application Data\REAPER\Scripts"
* Open Reaper Preferences / Control Surfaces and select ‘Tascam US-2400’ with its first MIDI-Port (it shows five on my PC)
* The US-2400 has to be in Native Mode, which AFAIK is a novelty of firmware 1.31, so you might have to update it. To activate Native Mode put the US-2400 to standby (so that the Power button ‘breathes’), then keep Master Sel + Chan pushed while turning it back on – if you have the right firmware and Native Mode is active Chan should blink three times. (To put it back in the MCU emulation mode do the same with Master Sel + Aux 5)

## Uninstall

* Delete reaper_csurf_us2400.dll
* If you want, remove the scripts.

That‘s about it.

## Notes

* The extensions identify any track, anywhere in your tracklist, that has ‘aux---1’ to ‘aux---6’ in its name as the respective Aux. You can call them anything – as long as ‘aux---X’ is in there somewhere it should work. Also, the search is not case sensitive so you can use ‘AUX’ or ‘Aux’ if you want. It has to be three dashes to avoid confusion with tracknames ending in aux, though. For example, I don’t speak French, but I think some words end on ‘...aux’, so maybe there are tracknames like ‘plateaux 2’ ... well, you can use those and mark aux-busses with three dashes, no space.
* For the SWS Grouping / Ungrouping actions to work I had to set/save the default flags once (Shift+G opens the Grouping Settings, tweak yours and hit Save)
* For other functionality refer to the [Keymap](https://github.com/DavidLichtenberger/reaper-csurf-us2400/blob/master/Tascam-US-400-Reaper-Keymap.gif "Keymap – GIF") (also, you can print that and stick it on your master section).
* You can throw away the source (it’s the c++ code) if you just want to run the extension. I would prefer it of course, if - after a short test drive - you would identify the flaws, put on your hacker hat, fork the extension and fix them.
* There is a thread about the extension [here](http://forum.cockos.com/showthread.php?t=132165 "Cockos Reaper Forums").

## Things I (or Maybe You?) Might Do At A Later Date:

*   <del>As of now, to avoid swamping the US-2400 faders/encoders with MIDI updates (especially in auto read mode), the extension doesn’t use the volume/pan ‘events’ of the API (there are none existing for FX param updates anyway, AFAIK). Instead it queries values and then updates faders/encoders in the central event loop (‘run’ function). This fixes the problem but also slows down response. Maybe this is just a temporary fix?</del>

    √ *This is now as good as it can be. It still uses the run loop but has a cleverer way (and a cache) to find out which faders must be updated, so as many updates as possible find their way to the faders. I tested it against the fake MCU setup and didn’t find it to be much slower anymore.*

    *BTW, the new version also compensates for higher CSurf update rates (it’s in the Preferences, default 15 Hz), so you can tweak that without fearing it might fall apart. Although I played around with it and didn’t find it to make a big difference – even in MCU mode, which (theoretically, technically) should have a higher limit, because it employs three MIDI connections instead of one. I guess the motors of the faders have their limit, too.*

*   <del>Maybe there is a way to make the US-2400 CSurf a module of it’s own so one doesn’t have to overwrite the reaper_csurf.dll?</del>

    √ *Turns out this is even easier done than said ;-)*

*   It would be cool to be able to assign custom actions to the ‘action buttons’ (like Shift-/FKey-Aux) but as of now I don’t really know how to do it.

*   There is a meter mode (where the encoder rings emulate VU meters) that works when you use the US-2400 as a fake MCU, but I didn’t really try to implement it in this extension ... I must admit, I don’t really see the use – after all it’s only 15 diodes per encoder?
