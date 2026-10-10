#version 140

uniform sampler2D texUnit;
uniform mat4 colorMatrix;
uniform float offset;
uniform vec2 halfpixel;
uniform vec4 box;
uniform vec4 cornerRadius;
uniform float opacity;
// LightlyShaders: 2 for circular corners, the squircle ratio for squircles
uniform float cornerExponent;

in vec2 uv;
in vec2 vertex;

out vec4 fragColor;

// KWin's sdfRoundedBox() with the corner distance measured in the Lp norm, p = cornerExponent,
// so corners can be squircles too. Same as the original for p = 2.
float sdfRoundedBox(vec2 position, vec2 center, vec2 extents, vec4 radius) {
    vec2 p = position - center;
    float r = p.x > 0.0
        ? (p.y < 0.0 ? radius.y : radius.w)
        : (p.y < 0.0 ? radius.x : radius.z);
    vec2 q = abs(p) - extents + vec2(r);
    vec2 outside = max(q, 0.0);
    float corner_distance = pow(pow(outside.x, cornerExponent) + pow(outside.y, cornerExponent), 1.0 / cornerExponent);
    return min(max(q.x, q.y), 0.0) + corner_distance - r;
}

void main(void)
{
    vec4 sum = texture(texUnit, uv + vec2(-halfpixel.x * 2.0, 0.0) * offset);
    sum += texture(texUnit, uv + vec2(-halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(0.0, halfpixel.y * 2.0) * offset);
    sum += texture(texUnit, uv + vec2(halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(halfpixel.x * 2.0, 0.0) * offset);
    sum += texture(texUnit, uv + vec2(halfpixel.x, -halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(0.0, -halfpixel.y * 2.0) * offset);
    sum += texture(texUnit, uv + vec2(-halfpixel.x, -halfpixel.y) * offset) * 2.0;

    fragColor = (sum / 12.0) * colorMatrix * opacity;

    float f = sdfRoundedBox(vertex, box.xy, box.zw, cornerRadius);
    float df = fwidth(f);
    fragColor *= 1.0 - clamp(0.5 + f / df, 0.0, 1.0);
}
