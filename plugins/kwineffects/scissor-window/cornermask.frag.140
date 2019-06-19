#version 140

uniform sampler2D sampler;
uniform vec4 modulation;
uniform float saturation;
uniform sampler2D mask;
uniform vec2 scale;

in vec2 texcoord0;

out vec4 fragColor;

void main()
{
    // 左上角的mask材质坐标
    vec2 tex_top_left = texcoord0 * scale;
    // 右下角的mask材质坐标，由于mask本身为左上角，因此还需要将材质按垂直方向翻转
    vec2 tex_bottom_left = texcoord0 * scale - vec2(0, scale.t - 1.0);
    tex_bottom_left.t = 1.0 - tex_bottom_left.t; // 将材质按垂直方向翻转
    // 右上角的mask材质坐标，由于mask本身为左上角，因此还需要将材质按水平方向翻转
    vec2 tex_top_right = texcoord0 * scale - vec2(scale.s - 1.0, 0);
    tex_top_right.s = 1.0 - tex_top_right.s; // 将材质按水平方向翻转
    // 右下角的mask材质坐标, 由于mask本身为左上角，因此还需要将材质翻转
    vec2 tex_bottom_right = 1.0 - ((texcoord0 - 1.0) * scale + 1.0);

    // 从mask材质中去除对应坐标的颜色
    vec4 mask_top_left = texture(mask, tex_top_left);
    vec4 mask_bottom_left = texture(mask, tex_bottom_left);
    vec4 mask_top_right = texture(mask, tex_top_right);
    vec4 mask_bottom_right = texture(mask, tex_bottom_right);

    vec4 tex = texture(sampler, texcoord0);

    if (saturation != 1.0) {
        vec3 desaturated = tex.rgb * vec3( 0.30, 0.59, 0.11 );
        desaturated = vec3( dot( desaturated, tex.rgb ));
        tex.rgb = tex.rgb * vec3( saturation ) + desaturated * vec3( 1.0 - saturation );
    }

    // 统一计算此像素点被模板遮盖后的颜色，此处不需要区分点是否在某个区域，不在此区域时取出的mask颜色的alpha值必为1
    tex *= modulation * mask_top_left * mask_bottom_left * mask_top_right * mask_bottom_right;

    fragColor = tex;
}
