# Menu Game Shaders (Geode Mod)

Geometry Dash 2.2 Geode mod that adds music and BPM-reactive post-processing shaders to the main menu background (`MenuGameLayer`), replicating the aesthetics of RobTop's official 2.2 sneak peek shader teasers.

## Features
- **Audio-Reactive**: Connects directly to FMOD to analyze music frequencies (bass/mids/treble) and synchronize shockwaves and ripples with the beat.
- **Configurable Styles**: Shockwave Pulse, Audio Waves, Beat Bulge, Neon Chromatic, Glitch, and Liquid Ripples.
- **Color Themes**: Sneak Peek Cycling, Neon Violet, Magenta, Acid Green, or custom RGB.
- **Optimized**: Low overhead off-screen FBO rendering that leaves main menu buttons and UI crisp.

## Building
Requires [Geode SDK](https://geode-sdk.org).
```bash
geode build
```
Or use the GitHub Actions CI workflow included in `.github/workflows/build.yml`.
