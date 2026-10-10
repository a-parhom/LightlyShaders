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
    vec2 coord0;
    vec4 outColor;
    float start_x;
    float start_y;

    float f_shadow_sample_offset = float(shadow_sample_offset);
    float f_radius = float(radius);

    //Window without shadow
    if(expanded_size == frame_size) {
        coord0 = vec2(texcoord0.x*frame_size.x, texcoord0.y*frame_size.y);
        //Left side
        if (coord0.x < f_radius) {
            //Bottom left corner
            if (coord0.y < f_radius) {
                outColor = shapeWindow(tex, coord0, vec2(f_radius, f_radius), f_radius);

                //Inner outline
                if(draw_inner_outline) {
                    outColor = cornerOutline(outColor, true, coord0, f_radius-outer_outline_width, vec2(f_radius, f_radius), inner_outline_width, false);
                }
                //Outer outline
                if(draw_outer_outline) {
                    outColor = cornerOutline(outColor, false, coord0, f_radius, vec2(f_radius, f_radius), outer_outline_width, true);
                }
            //Top left corner
            } else if (coord0.y > frame_size.y - f_radius) {
                outColor = shapeWindow(tex, coord0, vec2(f_radius, frame_size.y - f_radius), f_radius);

                //Inner outline
                if(draw_inner_outline) {
                    outColor = cornerOutline(outColor, true, coord0, f_radius-outer_outline_width, vec2(f_radius, frame_size.y - f_radius), inner_outline_width, false);
                }
                //Outer outline
                if(draw_outer_outline) {
                    outColor = cornerOutline(outColor, false, coord0, f_radius, vec2(f_radius, frame_size.y - f_radius), outer_outline_width, true);
                }
            //Center
            } else {
                outColor = tex;

                //Outline
                if(coord0.y > f_radius && coord0.y < frame_size.y - f_radius) {
                    //Inner outline
                    if(draw_inner_outline) {
                        if(coord0.x >= outer_outline_width && coord0.x <= outer_outline_width+inner_outline_width) {
                            outColor = mix(outColor, vec4(inner_outline_color.rgb,1.0), inner_outline_color.a);
                        }
                    }
                    //Outer outline
                    if(draw_outer_outline) {
                        if(
                            (coord0.x >= 0.0 && coord0.x <= outer_outline_width)
                        ) {
                            outColor = mix(outColor, vec4(outer_outline_color.rgb,1.0), outer_outline_color.a);
                        }
                    }
                }
            }
        //Right side
        } else if (coord0.x > frame_size.x - f_radius) {
            //Bottom right corner
            if (coord0.y < f_radius) {
                outColor = shapeWindow(tex, coord0, vec2(frame_size.x - f_radius, f_radius), f_radius);

                //Inner outline
                if(draw_inner_outline) {
                    outColor = cornerOutline(outColor, true, coord0, f_radius-outer_outline_width, vec2(frame_size.x - f_radius, f_radius), inner_outline_width, false);
                }
                //Outer outline
                if(draw_outer_outline) {
                    outColor = cornerOutline(outColor, false, coord0, f_radius, vec2(frame_size.x - f_radius, f_radius), outer_outline_width, true);
                }
            //Top right corner
            } else if (coord0.y > frame_size.y - f_radius) {
                outColor = shapeWindow(tex, coord0, vec2(frame_size.x - f_radius, frame_size.y - f_radius), f_radius);

                //Inner outline
                if(draw_inner_outline) {
                    outColor = cornerOutline(outColor, true, coord0, f_radius-outer_outline_width, vec2(frame_size.x - f_radius, frame_size.y - f_radius), inner_outline_width, false);
                }
                //Outer outline
                if(draw_outer_outline) {
                    outColor = cornerOutline(outColor, false, coord0, f_radius, vec2(frame_size.x - f_radius, frame_size.y - f_radius), outer_outline_width, true);
                }
            //Center
            } else {
                outColor = tex;

                //Outline
                if(coord0.y > f_radius && coord0.y < frame_size.y - f_radius) {
                    //Inner outline
                    if(draw_inner_outline) {
                        if(coord0.x >= frame_size.x - inner_outline_width - outer_outline_width && coord0.x <= frame_size.x - outer_outline_width) {
                            outColor = mix(outColor, vec4(inner_outline_color.rgb,1.0), inner_outline_color.a);
                        }
                    }
                    //Outer outline
                    if(draw_outer_outline) {
                        if(
                            (coord0.x >= frame_size.x - outer_outline_width && coord0.x <= frame_size.x )
                        ) {
                            outColor = mix(outColor, vec4(outer_outline_color.rgb,1.0), outer_outline_color.a);
                        }
                    }
                }
            }
        //Center
        } else {
            outColor = tex;

            //Outline
            if(coord0.x > f_radius && coord0.x < frame_size.x - f_radius) {
                //Inner outline
                if(draw_inner_outline) {
                    if(
                        (coord0.y >= frame_size.y - inner_outline_width - outer_outline_width && coord0.y <= frame_size.y - outer_outline_width )
                        || (coord0.y >= outer_outline_width && coord0.y <= outer_outline_width + inner_outline_width)
                    ) {
                        outColor = mix(outColor, vec4(inner_outline_color.rgb,1.0), inner_outline_color.a);
                    }
                }
                //Outer outline
                if(draw_outer_outline) {
                    if(
                        (coord0.y >= frame_size.y - outer_outline_width  && coord0.y <= frame_size.y)
                        || (coord0.y >= 0.0 && coord0.y <= outer_outline_width)
                    ) {
                        outColor = mix(outColor, vec4(outer_outline_color.rgb,1.0), outer_outline_color.a);
                    }
                }
            }
        }
    //Window with shadow
    } else if(expanded_size != frame_size) {
        coord0 = vec2(texcoord0.x*expanded_size.x, texcoord0.y*expanded_size.y);
        //Left side
        if (coord0.x > shadow_size.x - max(f_shadow_sample_offset, outer_outline_width) && coord0.x < f_radius + shadow_size.x) {
            //Top left corner
            if (coord0.y > frame_size.y + shadow_size.z - f_radius && coord0.y < frame_size.y + shadow_size.z + max(f_shadow_sample_offset, outer_outline_width)) {
                start_x = (shadow_size.x - f_shadow_sample_offset)/expanded_size.x;
                start_y = (shadow_size.z + frame_size.y + f_shadow_sample_offset)/expanded_size.y;
                
                outColor = shapeShadowWindow(vec2(start_x, start_y), tex, coord0, vec2(f_radius + shadow_size.x, frame_size.y + shadow_size.z - f_radius), f_radius);

                //Inner outline
                if(draw_inner_outline) {
                    outColor = cornerOutline(outColor, true, coord0, f_radius, vec2(f_radius + shadow_size.x, frame_size.y + shadow_size.z - f_radius), inner_outline_width, false);
                }
                //Outer outline
                if(draw_outer_outline) {
                    outColor = cornerOutline(outColor, false, coord0, f_radius, vec2(f_radius + shadow_size.x, frame_size.y + shadow_size.z - f_radius), outer_outline_width, false);
                }
            //Bottom left corner
            } else if (coord0.y > shadow_size.z - max(f_shadow_sample_offset, outer_outline_width) && coord0.y < f_radius + shadow_size.z) {
                start_x = (shadow_size.x - f_shadow_sample_offset)/expanded_size.x;
                start_y = (shadow_size.z - f_shadow_sample_offset)/expanded_size.y;

                outColor = shapeShadowWindow(vec2(start_x, start_y), tex, coord0, vec2(f_radius + shadow_size.x, shadow_size.z + f_radius), f_radius);

                //Inner outline
                if(draw_inner_outline) {
                    outColor = cornerOutline(outColor, true, coord0, f_radius, vec2(f_radius + shadow_size.x, shadow_size.z + f_radius), inner_outline_width, false);
                }
                //Outer outline
                if(draw_outer_outline) {
                    outColor = cornerOutline(outColor, false, coord0, f_radius, vec2(f_radius + shadow_size.x, shadow_size.z + f_radius), outer_outline_width, false);
                }
            //Center
            } else {
                outColor = tex;

                //Outline and shadow
                if(coord0.y > f_radius + shadow_size.z && coord0.y < shadow_size.z + frame_size.y - f_radius) {
                    //Left shadow padding
                    if(coord0.x > shadow_size.x - f_shadow_sample_offset && coord0.x <= shadow_size.x) {
                        start_x = (shadow_size.x - f_shadow_sample_offset)/expanded_size.x;                        
                        vec4 texShadowEdge = texture(sampler, vec2(start_x, texcoord0.y));
                        outColor = texShadowEdge;
                    }

                    //Inner outline
                    if(draw_inner_outline) {
                        if(coord0.x >= shadow_size.x && coord0.x <= shadow_size.x+inner_outline_width) {
                            outColor = mix(outColor, vec4(inner_outline_color.rgb,1.0), inner_outline_color.a);
                        }
                    }
                    //Outer outline
                    if(draw_outer_outline) {
                        if(
                            (coord0.x >= shadow_size.x - outer_outline_width && coord0.x <= shadow_size.x)
                        ) {
                            outColor = mix(outColor, vec4(outer_outline_color.rgb,1.0), outer_outline_color.a);
                        }
                    }
                }
            }
        //Right side
        } else if (coord0.x > shadow_size.x + frame_size.x - f_radius && coord0.x < shadow_size.x + frame_size.x + max(f_shadow_sample_offset, outer_outline_width)) {
            //Top right corner
            if (coord0.y > frame_size.y + shadow_size.z - f_radius && coord0.y < frame_size.y + shadow_size.z + max(f_shadow_sample_offset, outer_outline_width)) {
                start_x = (shadow_size.x + frame_size.x + f_shadow_sample_offset)/expanded_size.x;
                start_y = (shadow_size.z + frame_size.y + f_shadow_sample_offset)/expanded_size.y;

                outColor = shapeShadowWindow(vec2(start_x, start_y), tex, coord0, vec2(shadow_size.x + frame_size.x - f_radius, frame_size.y + shadow_size.z - f_radius), f_radius);

                //Inner outline
                if(draw_inner_outline) {
                    outColor = cornerOutline(outColor, true, coord0, f_radius, vec2(shadow_size.x + frame_size.x - f_radius, frame_size.y + shadow_size.z - f_radius), inner_outline_width, false);
                }
                //Outer outline
                if(draw_outer_outline) {
                    outColor = cornerOutline(outColor, false, coord0, f_radius, vec2(shadow_size.x + frame_size.x - f_radius, frame_size.y + shadow_size.z - f_radius), outer_outline_width, false);
                }
            //Bottom right corner
            } else if (coord0.y > shadow_size.z - max(f_shadow_sample_offset, outer_outline_width) && coord0.y < f_radius + shadow_size.z) {
                start_x = (shadow_size.x + frame_size.x + f_shadow_sample_offset)/expanded_size.x;
                start_y = (shadow_size.z - f_shadow_sample_offset)/expanded_size.y;

                outColor = shapeShadowWindow(vec2(start_x, start_y), tex, coord0, vec2(shadow_size.x + frame_size.x - f_radius, shadow_size.z + f_radius), f_radius);

                //Inner outline
                if(draw_inner_outline) {
                    outColor = cornerOutline(outColor, true, coord0, f_radius, vec2(shadow_size.x + frame_size.x - f_radius, shadow_size.z + f_radius), inner_outline_width, false);
                }
                //Outer outline
                if(draw_outer_outline) {
                    outColor = cornerOutline(outColor, false, coord0, f_radius, vec2(shadow_size.x + frame_size.x - f_radius, shadow_size.z + f_radius), outer_outline_width, false);
                }
            //Center
            } else {
                outColor = tex;

                //Outline and shadow
                if(coord0.y > f_radius + shadow_size.z && coord0.y < shadow_size.z + frame_size.y - f_radius) {
                    //Right shadow padding
                    if(coord0.x >= shadow_size.x + frame_size.x && coord0.x < shadow_size.x + frame_size.x + f_shadow_sample_offset) {
                        start_x = (shadow_size.x + frame_size.x + f_shadow_sample_offset)/expanded_size.x;
                        vec4 texShadowEdge = texture(sampler, vec2(start_x, texcoord0.y));
                        outColor = texShadowEdge;
                    }

                    //Inner outline
                    if(draw_inner_outline) {
                        if(coord0.x >= frame_size.x + shadow_size.x - inner_outline_width && coord0.x <= frame_size.x + shadow_size.x) {
                            outColor = mix(outColor, vec4(inner_outline_color.rgb,1.0), inner_outline_color.a);
                        }
                    }
                    //Outer outline
                    if(draw_outer_outline) {
                        if(
                            (coord0.x >= frame_size.x + shadow_size.x && coord0.x <= frame_size.x + shadow_size.x + outer_outline_width)
                        ) {
                            outColor = mix(outColor, vec4(outer_outline_color.rgb,1.0), outer_outline_color.a);
                        }
                    }
                }
            }
        //Center
        } else {
            outColor = tex;

            //Outline and shadow
            if(coord0.x > f_radius + shadow_size.x && coord0.x < shadow_size.x + frame_size.x - f_radius) {
                //Top shadow padding
                if(coord0.y >= frame_size.y + shadow_size.z && coord0.y < frame_size.y + shadow_size.z + f_shadow_sample_offset) {
                    start_y = (shadow_size.z + frame_size.y + f_shadow_sample_offset)/expanded_size.y;
                    vec4 texShadowEdge = texture(sampler, vec2(texcoord0.x, start_y));
                    outColor = texShadowEdge;
                //Bottom shadow padding
                } else if(coord0.y <= shadow_size.z && coord0.y > shadow_size.z - f_shadow_sample_offset) {
                    start_y = (shadow_size.z - f_shadow_sample_offset)/expanded_size.y;
                    vec4 texShadowEdge = texture(sampler, vec2(texcoord0.x, start_y));
                    outColor = texShadowEdge;
                }

                //Inner outline
                if(draw_inner_outline) {
                    if(
                        (coord0.y >= frame_size.y + shadow_size.z - inner_outline_width && coord0.y <= frame_size.y + shadow_size.z)
                        || (coord0.y >= shadow_size.z && coord0.y <= shadow_size.z + inner_outline_width)
                    ) {
                        outColor = mix(outColor, vec4(inner_outline_color.rgb,1.0), inner_outline_color.a);
                    }
                }
                //Outer outline
                if(draw_outer_outline) {
                    if(
                        (coord0.y >= frame_size.y + shadow_size.z && coord0.y <= frame_size.y + shadow_size.z + outer_outline_width)
                        || (coord0.y >= shadow_size.z - outer_outline_width && coord0.y <= shadow_size.z)
                    ) {
                        outColor = mix(outColor, vec4(outer_outline_color.rgb,1.0), outer_outline_color.a);
                    }
                }
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
