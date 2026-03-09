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
out vec3 fragWorldPos;
out vec3 fragNormal;

void main()
{
    fragTexCoord = texCoord;

    // Sample all three channels from the heightMap texture
    vec3 heightData = texture(heightMap, fragTexCoord).rgb;
    isObstacle = heightData.r;
    terrainHeight = heightData.g;
    waterDepth = heightData.b;

    // World-space position
    vec3 pos = vec3(
        position.x * gridSize.x * cellSize,
        position.y * gridSize.y * cellSize,
        terrainHeight
    );
    fragWorldPos = pos;

    vec2 texelSize = vec2(1.0) / gridSize;
    float hL = texture(heightMap, fragTexCoord + vec2(-texelSize.x, 0.0)).g;
    float hR = texture(heightMap, fragTexCoord + vec2( texelSize.x, 0.0)).g;
    float hD = texture(heightMap, fragTexCoord + vec2(0.0, -texelSize.y)).g;
    float hU = texture(heightMap, fragTexCoord + vec2(0.0,  texelSize.y)).g;

    // dx and dy are the world-space distances between samples
    float dx = cellSize * 2.0;
    float dy = cellSize * 2.0;

    // normal from height gradient: N = normalize(-dh/dx, -dh/dy, 1)
    fragNormal = normalize(vec3(
        (hL - hR) / dx,
        (hD - hU) / dy,
        1.0
    ));

    gl_Position = projection * view * vec4(pos, 1.0);
}