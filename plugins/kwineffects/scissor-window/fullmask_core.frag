#version 110

uniform sampler2D sampler;
uniform vec4 modulation;
uniform float saturation;
uniform sampler2D mask;

varying vec2 texcoord0;

void main()
{
    vec4 tex = texture2D(sampler, texcoord0);
    vec4 tex_mask = texture2D(mask, texcoord0);

    tex.a *= tex_mask.a;

    if (saturation != 1.0) {
        vec3 desaturated = tex.rgb * vec3( 0.30, 0.59, 0.11 );
        desaturated = vec3( dot( desaturated, tex.rgb ));
        tex.rgb = tex.rgb * vec3( saturation ) + desaturated * vec3( 1.0 - saturation );
    }

    tex *= modulation * tex_mask;

    gl_FragColor = tex;
}
