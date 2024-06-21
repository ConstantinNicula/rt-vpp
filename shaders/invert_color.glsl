#version 100
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texcoord;
uniform sampler2D tex;
uniform float time;
uniform float width;
uniform float height;

void main () {
    vec2 v_hflip_texcoord = vec2(v_texcoord.x, 1.0 - v_texcoord.y);
    vec4 color = texture2D(tex, v_hflip_texcoord);
    gl_FragColor = vec4(1.0 - color.rgb, color.a);
}