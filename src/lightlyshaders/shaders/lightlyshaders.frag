#version 140

uniform sampler2D sampler;

uniform vec2 expanded_size;
uniform vec2 frame_size;
uniform vec3 shadow_size;
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

//Used code from https://github.com/yilozt/rounded-window-corners project
float squircleBounds(vec2 p, vec2 center, float clip_radius)
{
    vec2 delta = abs(p - center);
    float f_squircle_ratio = float(squircle_ratio);

    float pow_dx = pow(delta.x, f_squircle_ratio);
    float pow_dy = pow(delta.y, f_squircle_ratio);

    float dist = pow(pow_dx + pow_dy, 1.0 / f_squircle_ratio);

    return clamp(clip_radius - dist + 0.5, 0.0, 1.0);
}

//Used code from https://github.com/yilozt/rounded-window-corners project
float circleBounds(vec2 p, vec2 center, float clip_radius)
{
    vec2 delta = p - center;
    float dist_squared = dot(delta, delta);

    float outer_radius = clip_radius + 0.5;
    if(dist_squared >= (outer_radius * outer_radius))
        return 0.0;

    float inner_radius = clip_radius - 0.5;
    if(dist_squared <= (inner_radius * inner_radius))
        return 1.0;

    return outer_radius - sqrt(dist_squared);
}

float cornerBounds(vec2 p, vec2 center, float clip_radius)
{
    if(is_squircle) {
        return squircleBounds(p, center, clip_radius);
    }
    return circleBounds(p, center, clip_radius);
}

vec4 shapeWindow(vec4 tex, vec2 p, vec2 center, float clip_radius)
{
    float alpha = cornerBounds(p, center, clip_radius);
    return vec4(tex.rgb*alpha, min(alpha, tex.a));
}

vec4 shapeShadowWindow(vec2 start, vec4 tex, vec2 p, vec2 center, float clip_radius)
{
    vec2 ShadowHorCoord = vec2(texcoord0.x, start.y);
    vec2 ShadowVerCoord = vec2(start.x, texcoord0.y);

    vec4 texShadowHorCur = texture(sampler, ShadowHorCoord);
    vec4 texShadowVerCur = texture(sampler, ShadowVerCoord);
    vec4 texShadow0 = texture(sampler, start);

    vec4 texShadow = texShadowHorCur + (texShadowVerCur - texShadow0);

    float alpha = cornerBounds(p, center, clip_radius);

    if(alpha == 0.0) {
        return texShadow;
    } else if(alpha < 1.0) {
        return mix(vec4(tex.rgb*alpha, min(alpha, tex.a)), texShadow, 1.0-alpha);
    } else {
        return tex;
    }
}

vec4 cornerOutline(vec4 outColor, bool inner, vec2 coord0, float radius, vec2 center, float outline_width, bool invert)
{
    vec4 outline_color;
    float radius_delta_inner;
    float radius_delta_outer;

    if(inner) {
        outline_color = inner_outline_color;
        radius_delta_outer = 0.0;
        radius_delta_inner = -outline_width;

        if(invert) {
            radius_delta_inner = 0.0;
            radius_delta_outer = outline_width;
        }
    } else {
        outline_color = outer_outline_color;
        radius_delta_inner = 0.0;
        radius_delta_outer = outline_width;

        if(invert) {
            radius_delta_outer = 0.0;
            radius_delta_inner = -outline_width;
        }
    }

    float outline_alpha_inner = cornerBounds(coord0, center, radius + radius_delta_inner);
    float outline_alpha_outer = cornerBounds(coord0, center, radius + radius_delta_outer);
    float outline_alpha = 1.0 - clamp(abs(outline_alpha_outer - outline_alpha_inner), 0.0, 1.0);
    outColor = mix(outColor, vec4(outline_color.rgb,1.0), (1.0-outline_alpha) * outline_color.a);
    return outColor;
}

void main()
{
    vec4 tex = texture(sampler, texcoord0);

    // The texture covers the whole window including its shadow, if there is one. Work in
    // coordinates relative to the frame's bottom left corner (texture coordinates are y-up).
    bool has_shadow = expanded_size != frame_size;
    vec2 tex_size = has_shadow ? expanded_size : frame_size;
    vec2 shadow_origin = has_shadow ? shadow_size.xz : vec2(0.0);
    vec2 p = texcoord0 * tex_size - shadow_origin;

    // All four corners and edges look the same, so mirror the position onto the nearest
    // horizontal and vertical edge. d is the distance inside the frame from those edges,
    // negative in the shadow outside of it.
    bvec2 far_side = greaterThan(p, frame_size - p);
    vec2 d = mix(p, frame_size - p, vec2(far_side));

    // Point just outside the frame on the nearest edges, where the shadow is sampled
    // to fill in the area cut off by the rounded corners
    vec2 shadow_sample = mix(vec2(-shadow_sample_offset), frame_size + shadow_sample_offset, vec2(far_side));
    vec2 shadow_sample_tex = (shadow_sample + shadow_origin) / tex_size;

    // Without a shadow both outlines are drawn inside the frame, with one the outer
    // outline is drawn on the shadow just outside of it
    float inner_outline_start = has_shadow ? 0.0 : outer_outline_width;
    float outer_outline_start = has_shadow ? -outer_outline_width : 0.0;
    float corner_reach = max(shadow_sample_offset, outer_outline_width);

    bool near_vertical_edge = d.x > -corner_reach && d.x < radius;
    bool near_horizontal_edge = d.y > -corner_reach && d.y < radius;

    vec4 outColor = tex;

    if (near_vertical_edge && near_horizontal_edge) {
        // Corner
        vec2 center = vec2(radius);
        if (has_shadow) {
            outColor = shapeShadowWindow(shadow_sample_tex, tex, d, center, radius);
        } else {
            outColor = shapeWindow(tex, d, center, radius);
        }

        if (draw_inner_outline) {
            float inner_radius = has_shadow ? radius : radius - outer_outline_width;
            outColor = cornerOutline(outColor, true, d, inner_radius, center, inner_outline_width, false);
        }
        if (draw_outer_outline) {
            outColor = cornerOutline(outColor, false, d, radius, center, outer_outline_width, !has_shadow);
        }
    } else if (near_vertical_edge || d.x > radius) {
        // Straight edge: the distance across it, and whether we're between the corners along it
        bool vertical = near_vertical_edge;
        float across = vertical ? d.x : d.y;
        bool between_corners = vertical ? d.y > radius : d.x > radius;

        if (between_corners) {
            if (has_shadow && across > -shadow_sample_offset && across <= 0.0) {
                // Shadow padding left by the rounding, continue the shadow from just outside the frame
                vec2 coord = vertical ? vec2(shadow_sample_tex.x, texcoord0.y) : vec2(texcoord0.x, shadow_sample_tex.y);
                outColor = texture(sampler, coord);
            }
            if (draw_inner_outline && across >= inner_outline_start && across <= inner_outline_start + inner_outline_width) {
                outColor = mix(outColor, vec4(inner_outline_color.rgb, 1.0), inner_outline_color.a);
            }
            if (draw_outer_outline && across >= outer_outline_start && across <= outer_outline_start + outer_outline_width) {
                outColor = mix(outColor, vec4(outer_outline_color.rgb, 1.0), outer_outline_color.a);
            }
        }
    }

    // Same output steps as KWin's own window shader
    outColor = sourceEncodingToNitsInDestinationColorspace(outColor);
    outColor = adjustSaturation(outColor);
    outColor *= modulation;
    outColor.rgb = doTonemapping(outColor.rgb);
    fragColor = nitsToDestinationEncoding(outColor);
}
