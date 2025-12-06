#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D terrainHeightmap;

void main()
{
    float height = texture(terrainHeightmap, TexCoords).r;

    float color = height / 20.0;
    FragColor = vec4(color, color, color, 1.0); // Skala szarości
}