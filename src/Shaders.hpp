#pragma once

namespace Shaders {

inline const char* const VERTEX_SHADER = R"(
attribute vec4 a_position;
attribute vec2 a_texCoord;
attribute vec4 a_color;

#ifdef GL_ES
varying lowp vec4 v_fragmentColor;
varying mediump vec2 v_texCoord;
#else
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
#endif

void main() {
    gl_Position = CC_MVPMatrix * a_position;
    v_fragmentColor = a_color;
    // Invert Y coordinate so CCRenderTexture output renders right side up
    v_texCoord = vec2(a_texCoord.x, 1.0 - a_texCoord.y);
}
)";

inline const char* const FRAGMENT_SHADER = R"(
#ifdef GL_ES
precision mediump float;
#endif

varying vec4 v_fragmentColor;
varying vec2 v_texCoord;

uniform sampler2D CC_Texture0;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_pulse;        // Combined audio punch & groove
uniform float u_bass;         // Continuous bass & kick energy
uniform float u_mids;         // Snare / vocals / synths
uniform float u_treble;       // Hi-hats & percussion
uniform float u_intensity;    // User sensitivity setting
uniform float u_distortion;   // User distortion strength
uniform int u_style;          // 0 to 11
uniform int u_colorMode;      // 0 to 5
uniform vec3 u_customColor;
uniform float u_chromatic;    // 0.0 or 1.0
uniform float u_beatFlash;    // Instant kick strobe
uniform vec2 u_mouse;         // Cursor coordinates

#define PI 3.14159265359

// Samples texture with chromatic aberration (RGB channel splitting)
vec4 sampleChromatic(sampler2D tex, vec2 uv, float offset) {
    if (offset <= 0.0002) {
        return texture2D(tex, uv);
    }
    float r = texture2D(tex, uv + vec2(offset, 0.0)).r;
    float g = texture2D(tex, uv).g;
    float b = texture2D(tex, uv - vec2(offset, 0.0)).b;
    float a = texture2D(tex, uv).a;
    return vec4(r, g, b, a);
}

// Style 0: Shockwave Pulse (RobTop GD 2.2 Sneak Peek style radial ripples)
vec2 shockwaveDistort(vec2 uv, vec2 center, float time, float pulse, float strength) {
    vec2 diff = uv - center;
    float aspect = u_resolution.x / max(u_resolution.y, 1.0);
    diff.x *= aspect;
    float dist = length(diff);

    float waveProgress = fract(time * 0.45);
    float ring1 = smoothstep(0.12, 0.0, abs(dist - waveProgress * 1.3));
    float pulseRadius = (1.0 - clamp(pulse * 0.7, 0.0, 1.0)) * 0.9;
    float ring2 = smoothstep(0.09, 0.0, abs(dist - pulseRadius)) * pulse;

    float combined = (ring1 * 0.7 + ring2 * 1.3) * strength;
    vec2 dir = (dist > 0.0001) ? normalize(diff) : vec2(0.0);
    dir.x /= aspect;

    return uv + dir * combined * 0.04;
}

// Style 1: Audio Waves (Sound frequency waveform ripples)
vec2 audioWaveDistort(vec2 uv, float time, float bass, float pulse, float strength) {
    float wave1 = sin(uv.y * 18.0 + time * 3.5) * (0.012 + bass * 0.035);
    float wave2 = cos(uv.x * 22.0 - time * 4.0) * (0.009 + pulse * 0.03);
    float wave3 = sin((uv.x + uv.y) * 14.0 + time * 2.5) * (0.01 + bass * 0.02);
    return uv + vec2(wave1 + wave3, wave2) * strength;
}

// Style 2: Beat Bulge & Warp (Pinch / bulge expansion on beat)
vec2 beatBulgeDistort(vec2 uv, vec2 center, float pulse, float bass, float strength) {
    vec2 diff = uv - center;
    float dist = length(diff);
    float bulge = (pulse * 0.9 + bass * 0.6) * strength;
    float factor = 1.0 - smoothstep(0.0, 0.65, dist);
    return uv + diff * factor * bulge * 0.25;
}

// Style 3: Neon Chromatic Beat (Distortion driven directly by beat kick)
vec2 neonBeatDistort(vec2 uv, vec2 center, float time, float pulse, float strength) {
    vec2 diff = uv - center;
    float dist = length(diff);
    float wave = sin(dist * 25.0 - time * 5.0) * (0.01 + pulse * 0.035);
    return uv + normalize(diff) * wave * strength;
}

// Style 4: Glitch & Scanline Pulse (Digital kick displacements)
vec2 glitchDistort(vec2 uv, float time, float pulse, float strength) {
    float glitchBar = step(0.95, sin(uv.y * 50.0 + time * 14.0));
    float shift = glitchBar * sin(time * 60.0) * (0.03 + pulse * 0.05);
    return uv + vec2(shift * strength, 0.0);
}

// Style 5: Liquid Ripples (Multi-frequency liquid wave reflections)
vec2 liquidDistort(vec2 uv, float time, float pulse, float strength) {
    vec2 ripple = uv;
    float p = 1.0 + pulse * 1.8;
    ripple.x += sin(uv.y * 24.0 + time * 2.8) * 0.015 * p * strength;
    ripple.y += cos(uv.x * 24.0 + time * 2.6) * 0.015 * p * strength;
    return ripple;
}

// Style 6: Radial Equalizer Rings (Visualizer audio ring)
vec2 radialEqualizerDistort(vec2 uv, vec2 center, float time, float bass, float mids, float pulse, float strength) {
    vec2 diff = uv - center;
    float aspect = u_resolution.x / max(u_resolution.y, 1.0);
    diff.x *= aspect;
    float dist = length(diff);
    float angle = atan(diff.y, diff.x);

    // Frequency spikes around the circle
    float spikes = sin(angle * 16.0 + time * 3.0) * bass * 0.06;
    float spikes2 = cos(angle * 32.0 - time * 4.0) * mids * 0.03;
    float ringRadius = 0.35 + pulse * 0.08;
    float ring = smoothstep(0.08, 0.0, abs(dist - ringRadius - spikes - spikes2));

    vec2 dir = (dist > 0.001) ? normalize(diff) : vec2(0.0);
    dir.x /= aspect;
    return uv + dir * ring * 0.05 * strength;
}

// Style 7: Hyperspace Warp Tunnel (Speed rush towards camera)
vec2 hyperspaceTunnelDistort(vec2 uv, vec2 center, float time, float pulse, float bass, float strength) {
    vec2 diff = uv - center;
    float dist = length(diff);
    float speedMult = 1.0 + pulse * 2.5 + bass * 1.5;
    float factor = pow(dist, 1.4) * speedMult * 0.18 * strength;
    return mix(uv, center, -factor);
}

// Style 8: Retro CRT Arcade (Barrel distortion, scanlines, arcade curvature)
vec2 retroCRTDistort(vec2 uv, vec2 center, float pulse, float strength) {
    vec2 diff = uv - center;
    float distSq = dot(diff, diff);
    // Barrel warp
    vec2 warped = center + diff * (1.0 + distSq * 0.35 * strength * (1.0 + pulse * 0.3));
    return warped;
}

// Style 9: Kaleidoscope Dimension (8-fold sacred geometry folding)
vec2 kaleidoscopeDistort(vec2 uv, vec2 center, float time, float pulse, float strength) {
    vec2 p = uv - center;
    float angle = atan(p.y, p.x) + time * 0.4;
    float r = length(p);

    // 8-fold fold
    float segments = 8.0;
    angle = mod(angle, 2.0 * PI / segments);
    angle = abs(angle - PI / segments);

    vec2 folded = center + vec2(cos(angle), sin(angle)) * r;
    // Add pulsing ripple
    folded += normalize(p) * sin(r * 20.0 - time * 3.0) * 0.02 * (1.0 + pulse);
    return mix(uv, folded, clamp(strength * 0.85, 0.0, 1.0));
}

// Style 10: Pixel Crunch Matrix (Digital mosaic quantization on beat drops)
vec2 pixelCrunchDistort(vec2 uv, float pulse, float strength) {
    float pixelCount = mix(160.0, 48.0, clamp(pulse * strength, 0.0, 1.0));
    vec2 pixelSize = vec2(1.0 / pixelCount, 1.0 / (pixelCount * (u_resolution.y / max(u_resolution.x, 1.0))));
    return floor(uv / pixelSize) * pixelSize;
}

// Style 11: Gravitational Vortex (Swirling black hole vortex)
vec2 vortexDistort(vec2 uv, vec2 center, float time, float bass, float pulse, float strength) {
    vec2 diff = uv - center;
    float r = length(diff);
    float angle = atan(diff.y, diff.x);

    float twist = (1.0 - smoothstep(0.0, 0.7, r)) * (4.5 + bass * 6.0 + pulse * 4.0) * strength;
    angle += twist;

    vec2 twisted = center + vec2(cos(angle), sin(angle)) * r;
    return twisted;
}

// Color tinting inspired by RobTop 2.2 teasers (Purple, Magenta, Cyan, Green)
vec3 applyColorTint(vec3 baseCol, int mode, float time, float pulse, vec3 customCol) {
    if (mode == 0) {
        return baseCol;
    }

    vec3 targetTint = vec3(1.0);
    if (mode == 1) {
        // Dynamic sneak peek cycle: Purple -> Magenta -> Cyan -> Green
        float phase = mod(time * 0.3, 4.0);
        if (phase < 1.0) {
            targetTint = mix(vec3(0.55, 0.20, 0.95), vec3(0.95, 0.20, 0.60), fract(phase));
        } else if (phase < 2.0) {
            targetTint = mix(vec3(0.95, 0.20, 0.60), vec3(0.15, 0.75, 0.95), fract(phase));
        } else if (phase < 3.0) {
            targetTint = mix(vec3(0.15, 0.75, 0.95), vec3(0.25, 0.95, 0.35), fract(phase));
        } else {
            targetTint = mix(vec3(0.25, 0.95, 0.35), vec3(0.55, 0.20, 0.95), fract(phase));
        }
    } else if (mode == 2) {
        // Purple / Violet (Sneak Peek 1)
        targetTint = vec3(0.55, 0.25, 0.95);
    } else if (mode == 3) {
        // Neon Pink / Magenta (Sneak Peek 2)
        targetTint = vec3(0.95, 0.20, 0.55);
    } else if (mode == 4) {
        // Acid Green (Sneak Peek 3)
        targetTint = vec3(0.25, 0.95, 0.35);
    } else if (mode == 5) {
        // Custom Color
        targetTint = customCol;
    }

    float lum = dot(baseCol, vec3(0.299, 0.587, 0.114));
    vec3 tinted = targetTint * (lum * 1.35 + 0.15);
    float blend = clamp(0.55 + pulse * 0.30, 0.0, 1.0);
    return mix(baseCol, tinted, blend);
}

void main() {
    vec2 uv = v_texCoord;
    vec2 center = vec2(0.5, 0.5);

    // Cursor influence (subtle interactive center shift)
    center = mix(center, u_mouse, 0.1);

    // Apply distortion based on selected style
    vec2 distortedUV = uv;
    if (u_style == 0) {
        distortedUV = shockwaveDistort(uv, center, u_time, u_pulse, u_distortion);
    } else if (u_style == 1) {
        distortedUV = audioWaveDistort(uv, u_time, u_bass, u_pulse, u_distortion);
    } else if (u_style == 2) {
        distortedUV = beatBulgeDistort(uv, center, u_pulse, u_bass, u_distortion);
    } else if (u_style == 3) {
        distortedUV = neonBeatDistort(uv, center, u_time, u_pulse, u_distortion);
    } else if (u_style == 4) {
        distortedUV = glitchDistort(uv, u_time, u_pulse, u_distortion);
    } else if (u_style == 5) {
        distortedUV = liquidDistort(uv, u_time, u_pulse, u_distortion);
    } else if (u_style == 6) {
        distortedUV = radialEqualizerDistort(uv, center, u_time, u_bass, u_mids, u_pulse, u_distortion);
    } else if (u_style == 7) {
        distortedUV = hyperspaceTunnelDistort(uv, center, u_time, u_pulse, u_bass, u_distortion);
    } else if (u_style == 8) {
        distortedUV = retroCRTDistort(uv, center, u_pulse, u_distortion);
    } else if (u_style == 9) {
        distortedUV = kaleidoscopeDistort(uv, center, u_time, u_pulse, u_distortion);
    } else if (u_style == 10) {
        distortedUV = pixelCrunchDistort(uv, u_pulse, u_distortion);
    } else if (u_style == 11) {
        distortedUV = vortexDistort(uv, center, u_time, u_bass, u_pulse, u_distortion);
    }

    // Blend distorted UV with original based on intensity
    distortedUV = mix(uv, distortedUV, clamp(u_intensity, 0.0, 2.0));

    // Clamp UV to prevent edge artifacts
    distortedUV = clamp(distortedUV, vec2(0.001), vec2(0.999));

    // Chromatic aberration offset driven by beat pulse & treble
    float chromaOffset = 0.0;
    if (u_chromatic > 0.5) {
        chromaOffset = (0.003 + u_pulse * 0.014 + u_treble * 0.006) * u_distortion;
    }

    vec4 color = sampleChromatic(CC_Texture0, distortedUV, chromaOffset);

    // Apply color tinting (purple, pink, green, etc.)
    color.rgb = applyColorTint(color.rgb, u_colorMode, u_time, u_pulse, u_customColor);

    // CRT Scanlines & phosphors for style 8
    if (u_style == 8) {
        float scanline = sin(distortedUV.y * u_resolution.y * 1.5) * 0.08;
        color.rgb -= scanline;
    }

    // Subtle beat glow & treble sparkle
    color.rgb += color.rgb * (u_pulse * 0.20 * u_intensity);
    color.rgb += vec3(u_treble * 0.08 * u_intensity);

    // Kick Strobe Flash (if enabled)
    color.rgb += vec3(u_beatFlash);

    gl_FragColor = color * v_fragmentColor;
}
)";

} // namespace Shaders
