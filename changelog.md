# Changelog

## v1.4.0
* **Live Shader Builder**: new "Open Live Preview" button in the mod settings (a custom setting, the same pattern Jukebox uses for its index manager) opens an in-game popup with a real-time shader preview and sliders/toggles for Style, Intensity, Distortion, Tint Mode, Custom Shape, Custom Speed, Custom Scale, Chromatic Aberration and Beat Flash. Changes apply to the menu background instantly, and picking a style disables the randomizer so your choice sticks.
* **Live Beat Synchronization**: the live shader preview now pulses and reacts to music kicks and spectral transients in real-time, backed by continuous BPM tempo sync.
* **Layout Polish**: fixed popup element positioning, button clipping in settings, and slider margins so controls stay cleanly framed inside the modal without overflowing.
* **Shockwave Ambient Wave Toggle**: new setting to enable/disable the looping time-based shockwave in Style 0, allowing it to trigger strictly on music beats without periodic idle waves.
* **Cursor Tracking Toggle**: new setting to toggle mouse parallax center-shifting on/off across all shaders.

## v1.3.0
* **New shader — Bass Subwoofer (12)**: an elliptical speaker cone that pumps toward the camera on bass, the opposite of the receding hyperspace tunnel.
* **New shader — Custom Builder (13)**: a fully tunable shader with its own base shape (rings/waves/bulge/kaleidoscope/vortex/tunnel), two gradient colors, speed and scale.
* **Shader Randomizer**: new "Randomize Shaders" toggle picks a random style every time the menu opens.
* **Fixed silence fallback**: the BPM fallback no longer fakes beats after ~5 seconds of genuine silence, so a muted game stops pulsing.

## v1.2.2
* **Fixed System Audio format detection on Windows**: the `WAVEFORMATEXTENSIBLE` subtype GUID was read through the struct at the wrong offset, so 32-bit 48 kHz mix formats were reported as unsupported (`float=false bytesPerSample=4`). The subtype is now read by its raw byte offset (probing 24 and 28) and 32-bit integer PCM is supported in addition to float32 and 16-bit PCM.

## v1.2.1
* **Fixed System Audio on Windows**: the `MMDeviceEnumerator` CLSID had a typo (`467E` instead of `467C`), so system-audio capture always failed with `REGDB_E_CLASSNOTREG` (0x80040154).
* **Fixed crash when enabling System Audio**: the capture thread is now joined before being restarted (reassigning a joinable `std::thread` called `std::terminate()`), plus a 2 s retry cooldown and defensive COM null-checks.

## v1.2.0
* **System Audio Source (Windows)**: New "Audio Source" setting lets the shaders react to the default playback device's output (Spotify, browser, etc.) instead of the in-game music, via WASAPI loopback capture with its own FFT band analysis. FMOD game-music analysis remains the fallback on macOS and Android.

## v1.1.0
* **High-Fidelity Audio Analysis**: Complete rewrite of the audio beat engine using dynamic Spectral Flux and Automatic Gain Control (AGC) for sub-bass and kick drums. The beat pulse is now tightly synchronized and faithful to any music playing.
* **6 New Shader Styles (12 total)**:
  * 6. Radial Audio Equalizer (Pulsing frequency spectrum ring)
  * 7. Hyperspace Warp Tunnel (Cosmic star-speed rush)
  * 8. Retro CRT Arcade / Cyberpunk (Arcade curvature, scanlines, and RGB phosphor displacement)
  * 9. Kaleidoscope Dimension (8-fold crystal symmetry)
  * 10. Pixel Crunch Matrix (8-Bit retro mosaic quantizer)
  * 11. Gravitational Vortex / Black Hole (Hypnotic cosmic twist)
* **Bass Camera Shake**: Added optional physical screen bump/shake on heavy kick drops.
* **Beat Strobe Flash**: Added subtle luminescence flash on drops.
* **Treble & Hi-Hat Shimmer**: Audio engine now isolates high frequencies for subtle glimmers and scanline pulses.
* **Interactive Mouse Parallax**: Shader focal centers subtly track cursor movement across the screen.
* **New Settings**: Added Beat Detection Sensitivity slider, Bass Camera Shake toggle, and Beat Strobe Flash toggle.

## v1.0.3
* Fixed vertical texture inversion permanently via vertex shader coordinate flip.
* Fixed exit crash with safe node cleanup and `geode::Ref<CCRenderTexture>`.

## v1.0.0
* Initial release of Menu Game Shaders for Geometry Dash 2.2081+ / Geode v5.10.1+.
