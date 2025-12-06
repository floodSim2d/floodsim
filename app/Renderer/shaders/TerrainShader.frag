#version 410 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D terrainHeightmap;
uniform sampler2D waterDepthmap;
uniform sampler2D attributesMap;

void main()
{
    float terrainHeight = texture(terrainHeightmap, TexCoords).r;
    float waterDepth = texture(waterDepthmap, TexCoords).r;
    vec3 attributes = texture(attributesMap, TexCoords).rgb;

    float isObstacle = attributes.r;
    float isRiver = attributes.g;
    float isWaterSource = attributes.b;

    // Kolejność rysowania jest ważna: Przeszkoda > Rzeka > Źródło > Woda > Teren
    if (isObstacle > 0.5) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0); // Czarny dla przeszkody
    } else if (isRiver > 0.5) {
        FragColor = vec4(0.0, 0.0, 1.0, 1.0); // Niebieski dla rzeki
    } else if (isWaterSource > 0.5) {
        FragColor = vec4(0.0, 1.0, 0.78, 1.0); // Turkusowy dla źródła wody (0, 255, 200)
    } else if (waterDepth > 0.01) {
        // Rysuj wodę
        // Im głębiej, tym ciemniejszy niebieski
        float waterColorFactor = 1.0 - clamp(waterDepth / 10.0, 0.0, 0.7);
        vec3 waterColor = vec3(0.2, 0.5, 1.0) * waterColorFactor;
        FragColor = vec4(waterColor, 1.0);
    } else {
        // Rysuj teren
        // Odwrócona logika: im wyżej, tym ciemniej. Tło (wysokość 0) jest białe.
        float terrainColor = 1.0 - (terrainHeight / 20.0);
        FragColor = vec4(terrainColor, terrainColor, terrainColor, 1.0);
    }
}