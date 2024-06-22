/* Adapted from: https://www.shadertoy.com/view/WsVSzV */

#version 100
#ifdef GL_ES
precision mediump float;
#endif
varying vec2 v_texcoord;
uniform sampler2D tex;
uniform float time;
uniform float width;
uniform float height;

/*simulate curvature of CRT monitor*/ 
const float warp = 0.75; 
/*simulate darkness between scanlines*/
const float scan = 0.75; 

void main()
{
    /* squared distance from center */
    vec2 uv = v_texcoord; 
    vec2 dc = abs(0.5-uv);
    dc *= dc;

    /* warp the fragment coordinates */
    uv.x -= 0.5; uv.x *= 1.0+(dc.y*(0.3*warp)); uv.x += 0.5;
    uv.y -= 0.5; uv.y *= 1.0+(dc.x*(0.4*warp)); uv.y += 0.5;

    /* sample inside boundaries, otherwise set to black */
    if (uv.y > 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0)
        gl_FragColor = vec4(0.0,0.0,0.0,1.0);
    else
    {
        /* determine if we are drawing in a scanline */
        float apply = abs(sin(v_texcoord.y * height)*0.5*scan);
        /* sample the texture */
        gl_FragColor = vec4(mix(texture2D(tex, uv).rgb,vec3(0.0),apply),1.0);
    }
}