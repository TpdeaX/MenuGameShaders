# Changelog

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
