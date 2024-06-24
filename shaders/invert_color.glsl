#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texcoord;
uniform sampler2D tex;
uniform float time;
uniform float width;
uniform float height;

void main () {
    vec4 color = texture2D(tex, v_texcoord);
    gl_FragColor = vec4(1.0 - color.rgb, color.a);
}