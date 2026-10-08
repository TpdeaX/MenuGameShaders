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
    v_texCoord = a_texCoord;
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
uniform float u_pulse;        // 0.0 - 1.0 beat kick
uniform float u_bass;         // live bass amplitude
uniform float u_mids;         // live mids amplitude
uniform float u_intensity;    // overall effect strength
uniform float u_distortion;   // distortion strength
uniform int u_style;          // 0 to 5
uniform int u_colorMode;      // 0 to 5
uniform vec3 u_customColor;
uniform float u_chromatic;    // 0.0 or 1.0

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

    // Continuous expanding rings + reactive pulse ring
    float waveProgress = fract(time * 0.45);
    float ring1 = smoothstep(0.12, 0.0, abs(dist - waveProgress * 1.3));
    float pulseRadius = (1.0 - pulse) * 0.9;
    float ring2 = smoothstep(0.09, 0.0, abs(dist - pulseRadius)) * pulse;

    float combined = (ring1 * 0.7 + ring2 * 1.2) * strength;
    vec2 dir = (dist > 0.0001) ? normalize(diff) : vec2(0.0);
    dir.x /= aspect;

    return uv + dir * combined * 0.035;
}

// Style 1: Audio Waves (Sound frequency waveform ripples)
vec2 audioWaveDistort(vec2 uv, float time, float bass, float pulse, float strength) {
    float wave1 = sin(uv.y * 18.0 + time * 3.5) * (0.012 + bass * 0.025);
    float wave2 = cos(uv.x * 22.0 - time * 4.0) * (0.009 + pulse * 0.02);
    float wave3 = sin((uv.x + uv.y) * 14.0 + time * 2.5) * 0.01;
    return uv + vec2(wave1 + wave3, wave2) * strength;
}

// Style 2: Beat Bulge & Warp (Pinch / bulge expansion on beat)
vec2 beatBulgeDistort(vec2 uv, vec2 center, float pulse, float bass, float strength) {
    vec2 diff = uv - center;
    float dist = length(diff);
    float bulge = (pulse * 0.8 + bass * 0.6) * strength;
    float factor = 1.0 - smoothstep(0.0, 0.65, dist);
    return uv + diff * factor * bulge * 0.22;
}

// Style 3: Neon Chromatic Beat (Distortion driven directly by beat kick)
vec2 neonBeatDistort(vec2 uv, vec2 center, float time, float pulse, float strength) {
    vec2 diff = uv - center;
    float dist = length(diff);
    float wave = sin(dist * 25.0 - time * 5.0) * (0.01 + pulse * 0.02);
    return uv + normalize(diff) * wave * strength;
}

// Style 4: Glitch & Scanline Pulse (Digital kick displacements)
vec2 glitchDistort(vec2 uv, float time, float pulse, float strength) {
    float glitchBar = step(0.96, sin(uv.y * 50.0 + time * 12.0));
    float shift = glitchBar * sin(time * 60.0) * (0.025 + pulse * 0.035);
    return uv + vec2(shift * strength, 0.0);
}

// Style 5: Liquid Ripples (Multi-frequency liquid wave reflections)
vec2 liquidDistort(vec2 uv, float time, float pulse, float strength) {
    vec2 ripple = uv;
    float p = 1.0 + pulse * 1.5;
    ripple.x += sin(uv.y * 24.0 + time * 2.8) * 0.014 * p * strength;
    ripple.y += cos(uv.x * 24.0 + time * 2.6) * 0.014 * p * strength;
    return ripple;
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
    }

    // Blend distorted UV with original based on intensity
    distortedUV = mix(uv, distortedUV, clamp(u_intensity, 0.0, 2.0));

    // Clamp UV to prevent edge artifacts
    distortedUV = clamp(distortedUV, vec2(0.001), vec2(0.999));

    // Chromatic aberration offset driven by beat pulse
    float chromaOffset = 0.0;
    if (u_chromatic > 0.5) {
        chromaOffset = (0.004 + u_pulse * 0.012) * u_distortion;
    }

    vec4 color = sampleChromatic(CC_Texture0, distortedUV, chromaOffset);

    // Apply color tinting (purple, pink, green, etc.)
    color.rgb = applyColorTint(color.rgb, u_colorMode, u_time, u_pulse, u_customColor);

    // Subtle beat glow pulse
    color.rgb += color.rgb * (u_pulse * 0.18 * u_intensity);

    gl_FragColor = color * v_fragmentColor;
}
)";

} // namespace Shaders
