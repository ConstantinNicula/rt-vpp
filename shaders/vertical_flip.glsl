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
    vec2 vflip_v_texcoord = vec2(v_texcoord.x, 1.0 - v_texcoord.y);
    gl_FragColor = texture2D(tex, vflip_v_texcoord);
}