#version 140

uniform sampler2D sampler;

uniform vec2 expanded_size;
uniform vec2 frame_size;
uniform vec2 shadow_origin; // frame position inside the texture: left and bottom shadow size
uniform float radius;
uniform float shadow_sample_offset;
uniform bool draw_inner_outline;
uniform bool draw_outer_outline;
uniform float inner_outline_width;
uniform float outer_outline_width;
uniform vec4 inner_outline_color;
uniform vec4 outer_outline_color;
uniform int squircle_ratio;
uniform bool is_squircle;

uniform vec4 modulation;

in vec2 texcoord0;
out vec4 fragColor;

#include "colormanagement.glsl"
#include "saturation.glsl"

// Used code from https://github.com/yilozt/rounded-window-corners project
float squircleBounds(vec2 p, vec2 center, float clip_radius)
{
    vec2 delta = abs(p - center);
    float f_squircle_ratio = float(squircle_ratio);

    float pow_dx = pow(delta.x, f_squircle_ratio);
    float pow_dy = pow(delta.y, f_squircle_ratio);

    float dist = pow(pow_dx + pow_dy, 1.0 / f_squircle_ratio);

    return clamp(clip_radius - dist + 0.5, 0.0, 1.0);
}

// Used code from https://github.com/yilozt/rounded-window-corners project
float circleBounds(vec2 p, vec2 center, float clip_radius)
{
    vec2 delta = p - center;
    float dist_squared = dot(delta, delta);

    // The radius gets negative when outlines are wider than the corner, check the
    // signs before squaring so such a circle covers nothing instead of everything
    float outer_radius = clip_radius + 0.5;
    if (outer_radius <= 0.0 || dist_squared >= (outer_radius * outer_radius))
        return 0.0;

    float inner_radius = clip_radius - 0.5;
    if (inner_radius > 0.0 && dist_squared <= (inner_radius * inner_radius))
        return 1.0;

    return outer_radius - sqrt(dist_squared);
}

float cornerBounds(vec2 p, vec2 center, float clip_radius)
{
    if (is_squircle) {
        return squircleBounds(p, center, clip_radius);
    }
    return circleBounds(p, center, clip_radius);
}

// The texture is premultiplied, so coverage scales all four channels
vec4 shapeWindow(vec4 tex, vec2 p, vec2 center, float clip_radius)
{
    return tex * cornerBounds(p, center, clip_radius);
}

// Like shapeWindow, but fills the area cut off by the rounding with the shadow continued
// from just outside the corner (start), so the shadow follows the rounded shape
vec4 shapeShadowWindow(vec2 start, vec4 tex, vec2 p, vec2 center, float clip_radius)
{
    vec4 shadow_horizontal = texture(sampler, vec2(texcoord0.x, start.y));
    vec4 shadow_vertical = texture(sampler, vec2(start.x, texcoord0.y));
    vec4 shadow_corner = texture(sampler, start);
    vec4 shadow = shadow_horizontal + (shadow_vertical - shadow_corner);

    float alpha = cornerBounds(p, center, clip_radius);
    return mix(shadow, tex * alpha, alpha);
}

// Whether a distance inwards from the window edge is within an outline band
bool inBand(float across, float band_start, float width)
{
    return across >= band_start && across <= band_start + width;
}

// Blends an outline into the corner. Like the straight edge outlines, the band is
// band_start to band_start + width inwards from the window edge (negative is outside).
vec4 cornerOutline(vec4 color, vec4 outline_color, vec2 p, vec2 center, float band_start, float width)
{
    float coverage_outer = cornerBounds(p, center, radius - band_start);
    float coverage_inner = cornerBounds(p, center, radius - band_start - width);
    float outline_alpha = clamp(coverage_outer - coverage_inner, 0.0, 1.0);
    return mix(color, vec4(outline_color.rgb, 1.0), outline_alpha * outline_color.a);
}

void main()
{
    vec4 tex = texture(sampler, texcoord0);

    // The texture covers the whole window including its shadow, if there is one. Work in
    // coordinates relative to the frame's bottom left corner (texture coordinates are y-up).
    bool has_shadow = expanded_size != frame_size;
    vec2 tex_size = has_shadow ? expanded_size : frame_size;
    vec2 origin = has_shadow ? shadow_origin : vec2(0.0);
    vec2 p = texcoord0 * tex_size - origin;

    // All four corners and edges look the same, so mirror the position onto the nearest
    // horizontal and vertical edge. d is the distance inside the frame from those edges,
    // negative in the shadow outside of it.
    bvec2 far_side = greaterThan(p, frame_size - p);
    vec2 d = mix(p, frame_size - p, vec2(far_side));

    // Whether the nearest vertical and horizontal edge have a shadow. Shadows can be lopsided,
    // a side without one is handled like a window without a shadow: there is no shadow to
    // continue into the rounded off area and nothing to draw the outer outline on.
    vec4 shadow_extent = has_shadow ? vec4(origin, tex_size - frame_size - origin) : vec4(0.0); // left, bottom, right, top
    // Less than half a pixel is rounding noise from scaling, not a shadow
    bvec2 edge_shadow = greaterThan(mix(shadow_extent.xy, shadow_extent.zw, vec2(far_side)), vec2(0.5));
    bool corner_shadow = all(edge_shadow);

    // Point just outside the frame on the nearest edges, where the shadow is sampled
    // to fill in the area cut off by the rounded corners
    vec2 shadow_sample = mix(vec2(-shadow_sample_offset), frame_size + shadow_sample_offset, vec2(far_side));
    vec2 shadow_sample_tex = (shadow_sample + origin) / tex_size;

    // Without a shadow both outlines are drawn inside the frame, with one the outer
    // outline is drawn on the shadow just outside of it. Per edge, x for the vertical one.
    vec2 inner_outline_start = mix(vec2(outer_outline_width), vec2(0.0), vec2(edge_shadow));
    vec2 outer_outline_start = mix(vec2(0.0), vec2(-outer_outline_width), vec2(edge_shadow));
    float corner_reach = max(shadow_sample_offset, outer_outline_width);

    bool near_vertical_edge = d.x > -corner_reach && d.x < radius;
    bool near_horizontal_edge = d.y > -corner_reach && d.y < radius;

    vec4 outColor = tex;

    if (near_vertical_edge && near_horizontal_edge) {
        // Corner
        vec2 center = vec2(radius);
        if (corner_shadow) {
            outColor = shapeShadowWindow(shadow_sample_tex, tex, d, center, radius);
        } else {
            outColor = shapeWindow(tex, d, center, radius);
        }

        // A corner with a shadow on only one side has its outlines inside, like without a shadow
        float corner_inner_start = corner_shadow ? 0.0 : outer_outline_width;
        float corner_outer_start = corner_shadow ? -outer_outline_width : 0.0;
        if (draw_inner_outline) {
            outColor = cornerOutline(outColor, inner_outline_color, d, center, corner_inner_start, inner_outline_width);
        }
        if (draw_outer_outline) {
            outColor = cornerOutline(outColor, outer_outline_color, d, center, corner_outer_start, outer_outline_width);
        }
    } else {
        // Straight edges, only between the corners along them. Outlines wider than the corner
        // radius reach past it, so a pixel can be near a vertical and a horizontal edge at once.
        bool along_vertical_edge = d.y >= radius;
        bool along_horizontal_edge = d.x >= radius;

        // Shadow padding left by the rounding, continue the shadow from just outside the frame
        if (along_vertical_edge && edge_shadow.x && d.x > -shadow_sample_offset && d.x <= 0.0) {
            outColor = texture(sampler, vec2(shadow_sample_tex.x, texcoord0.y));
        } else if (along_horizontal_edge && edge_shadow.y && d.y > -shadow_sample_offset && d.y <= 0.0) {
            outColor = texture(sampler, vec2(texcoord0.x, shadow_sample_tex.y));
        }

        if (draw_inner_outline
            && ((along_vertical_edge && inBand(d.x, inner_outline_start.x, inner_outline_width))
                || (along_horizontal_edge && inBand(d.y, inner_outline_start.y, inner_outline_width)))) {
            outColor = mix(outColor, vec4(inner_outline_color.rgb, 1.0), inner_outline_color.a);
        }
        if (draw_outer_outline
            && ((along_vertical_edge && inBand(d.x, outer_outline_start.x, outer_outline_width))
                || (along_horizontal_edge && inBand(d.y, outer_outline_start.y, outer_outline_width)))) {
            outColor = mix(outColor, vec4(outer_outline_color.rgb, 1.0), outer_outline_color.a);
        }
    }

    // Same output steps as KWin's own window shader
    outColor = sourceEncodingToNitsInDestinationColorspace(outColor);
    outColor = adjustSaturation(outColor);
    outColor *= modulation;
    outColor.rgb = doTonemapping(outColor.rgb);
    fragColor = nitsToDestinationEncoding(outColor);
}
