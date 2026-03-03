#version 410 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 view;
uniform sampler2D heightMap;
uniform vec2 gridSize;
uniform float cellSize;

out vec2 fragTexCoord;
out float vTerrainHeight;
out float vWaterDepth;
out float vWaterSurfaceHeight;
out vec2 vWorldPos;

void main()
{
    fragTexCoord = texCoord;

    // R = obstacle, G = terrain height, B = water depth
    vec3 heightData = texture(heightMap, fragTexCoord).rgb;
    vTerrainHeight = heightData.g;
    vWaterDepth = heightData.b;

    vWaterSurfaceHeight = vTerrainHeight + vWaterDepth;

    vec2 worldPos2D = position * gridSize * cellSize;
    vWorldPos = worldPos2D;

    // Flat water surface — no wave displacement
    // No water → push below terrain to hide
    float zHeight = vWaterDepth > 0.01
        ? vWaterSurfaceHeight
        : vTerrainHeight - 1.0;

    vec3 pos = vec3(worldPos2D.x, worldPos2D.y, zHeight);

    gl_Position = projection * view * vec4(pos, 1.0);
}
