# Menu Game Shaders

**Menu Game Shaders** transforms the Geometry Dash 2.2 main menu background (`MenuGameLayer`) with dynamic, music and BPM-reactive post-processing shaders, inspired by RobTop's 2.2 sneak peeks and shader previews!

### Features
* **Music & BPM Reactivity**: Live FMOD frequency analysis (FFT) detects bass kicks and tempo transients, pulsing the shaders in real-time with whatever track is playing.
* **Fallback BPM Clock**: Ensures rhythmic pulsation even during quiet song sections or custom music.
* **Multiple Visual Styles**:
  * **Shockwave Pulse**: Expanding radial shockwave rings that ripple across the background on beat drops (RobTop 2.2 Sneak Peek style).
  * **Audio Waves**: Fluid sinusoidal waveform ripples flowing across the background.
  * **Beat Bulge & Warp**: Central pinch and bulge deformations that expand and contract to the beat.
  * **Neon Chromatic Beat**: Dynamic RGB channel splitting and chromatic aberration.
  * **Glitch & Scanline Pulse**: High-energy digital scanline pulses.
  * **Liquid Ripples**: Calming water ripples distorting background tiles and icons.
* **Sneak Peek Color Tints**:
  * Cycling palette (Purple -> Magenta -> Cyan -> Green)
  * Neon Violet, Magenta, Acid Green, or Custom Color picker.
* **Non-Intrusive**: Front menu buttons, logos, daily chests, and popups remain 100% crisp and unaffected.
