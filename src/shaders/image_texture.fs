#version 420

in vec2 TexCoord;

uniform sampler2D tex_image;

out vec4 outputColor;

void main()
{
   //vec4 texColor = texture(tex_image, TexCoord);
   //int channel = 2;
   //outputColor = vec4(texColor[channel], texColor[channel], texColor[channel], 1.0f);
   
   outputColor = texture(tex_image, TexCoord);
}
