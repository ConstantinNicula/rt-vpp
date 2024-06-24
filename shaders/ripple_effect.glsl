/* Adapted from: https://www.shadertoy.com/view/4Xc3DM */

#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texcoord;
uniform sampler2D tex;
uniform float time;
uniform float width;
uniform float height;

#define speed 4.
#define strength 40.
#define distortion .03

void main()
{
    vec2 iResolution = vec2(width, height);
    vec2 fragCoord = v_texcoord * iResolution; 

    vec2 uv = fragCoord/iResolution.xy;
    float legsx = 1. - length((2. * fragCoord - iResolution.xy) / iResolution.x);
    float legsy = 1. - length((2. * fragCoord - iResolution.xy) / iResolution.y);
    
    vec4 tex = texture2D(tex, uv + vec2(sin((time * speed) + uv.y * strength) * (distortion * legsx), sin((time * speed) + uv.x * strength) * (distortion * legsy)));
   
    gl_FragColor = vec4(tex);
}