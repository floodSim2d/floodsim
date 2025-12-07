#version 410 core
in vec2 fragTexCoord;
in float terrainHeight;
in float waterDepth;
in float isObstacle;

out vec4 fragColor;

uniform sampler2D heightMap;

void main() {
    // Calculate normals using total height for proper shading
    vec2 texelSize = vec2(1.0) / textureSize(heightMap, 0);

    float hL = texture(heightMap, fragTexCoord - vec2(texelSize.x, 0.0)).b;
    float hR = texture(heightMap, fragTexCoord + vec2(texelSize.x, 0.0)).b;
    float hD = texture(heightMap, fragTexCoord - vec2(0.0, texelSize.y)).b;
    float hU = texture(heightMap, fragTexCoord + vec2(0.0, texelSize.y)).b;

    vec3 normal = normalize(vec3(hL - hR, 2.0, hD - hU));

    // Terrain color based on terrain height
    vec3 terrainLowColor = vec3(0.2, 0.5, 0.2);   // Green - low terrain
    vec3 terrainMidColor = vec3(0.6, 0.6, 0.4);   // Yellow-brown - mid terrain
    vec3 terrainHighColor = vec3(0.9, 0.9, 0.9);  // White/gray - high terrain (mountains)

    vec3 terrainColor;
    if (terrainHeight < 0.0) {
        // Below sea level - dark green/brown
        terrainColor = vec3(0.15, 0.3, 0.15);
    } else if (terrainHeight < 5.0) {
        float t = terrainHeight / 5.0;
        terrainColor = mix(terrainLowColor, terrainMidColor, t);
    } else {
        float t = clamp((terrainHeight - 5.0) / 10.0, 0.0, 1.0);
        terrainColor = mix(terrainMidColor, terrainHighColor, t);
    }

    // Water color based on water depth
    vec3 shallowWaterColor = vec3(0.4, 0.7, 0.9);  // Light blue - shallow water
    vec3 deepWaterColor = vec3(0.0, 0.2, 0.6);     // Dark blue - deep water

    vec3 waterColor;
    if (waterDepth > 0.0) {
        float waterT = clamp(waterDepth / 5.0, 0.0, 1.0);
        waterColor = mix(shallowWaterColor, deepWaterColor, waterT);

        // Add transparency effect for shallow water
        float waterAlpha = clamp(waterDepth / 2.0, 0.3, 0.9);

        // Blend terrain and water colors
        terrainColor = mix(terrainColor, waterColor, waterAlpha);
    }

    // lightning
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    float diff = max(dot(normal, lightDir), 0.0);
    terrainColor *= diff * 0.8 + 0.2;

    // Obstacle overlay (e.g., rocks, buildings)
    if (isObstacle > 0.5) {
        terrainColor = mix(terrainColor, vec3(0.3, 0.3, 0.3), 0.5); // Dark gray overlay
    }

    fragColor = vec4(terrainColor, 1.0);
}