#version 410 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 view;
uniform sampler2D heightMap;
uniform vec2 gridSize;
uniform float cellSize;

out vec2 fragTexCoord;
out float terrainHeight;
out float waterDepth;
out float isObstacle;

void main()
{
    fragTexCoord = texCoord;

    vec3 heightData = texture(heightMap, fragTexCoord).rgb;
    isObstacle = heightData.r; // 1.0F - obstacle, 0.0F - free space
    terrainHeight = heightData.g; // Green channel = terrain height
    waterDepth = heightData.b; // Blue channel = water depth

    // X and Y are horizontal plane, Z is height (for top-down view)
    // Position is [0,1], scale by grid dimensions and cell size to get world space
    vec3 pos = vec3(
                    position.x * gridSize.x * cellSize,
                    position.y * gridSize.y * cellSize,
                    terrainHeight
                );
    gl_Position = projection * view * vec4(pos, 1.0);
}