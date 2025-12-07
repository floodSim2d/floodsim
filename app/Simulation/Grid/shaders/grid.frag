#version 410 core
in vec2 fragTexCoord;
in float terrainHeight;
in float waterDepth;
in float isObstacle;

out vec4 fragColor;

uniform sampler2D heightMap;

void main() {
    // Terrain color based on terrain height
    vec3 grassColor = vec3(0.2, 0.7, 0.2);         // Bright grass green
    vec3 darkGrassColor = vec3(0.15, 0.55, 0.15);  // Dark grass green
    vec3 brownParticle = vec3(0.4, 0.3, 0.2);      // Brown dirt particles
    vec3 snowParticle = vec3(0.95, 0.95, 1.0);     // White snow particles

    // Add some variation with brown particles using fragment position
    float brownNoise = fract(sin(dot(fragTexCoord * 1000.0, vec2(12.9898, 78.233))) * 43758.5453);
    float hasBrownParticle = step(0.85, brownNoise); // 15% chance of brown particle

    // Snow particles - using different noise for variety
    float snowNoise = fract(sin(dot(fragTexCoord * 800.0, vec2(78.233, 12.9898))) * 43758.5453);

    vec3 terrainColor;
    if (terrainHeight < 0.0) {
        // Below sea level - dark grass/mud
        terrainColor = vec3(0.2, 0.45, 0.2);
    } else if (terrainHeight < 15.0) {
        // Low elevation - lush grass with some brown particles, gets darker with height
        float darkenFactor = terrainHeight / 15.0;
        vec3 baseGrass = mix(grassColor, darkGrassColor, darkenFactor * 0.5);
        terrainColor = mix(baseGrass, brownParticle, hasBrownParticle * 0.5);
    } else if (terrainHeight < 100.0) {
        // Mid elevation - darker green grass with more brown particles
        float t = (terrainHeight - 30.0) / 70.0;
        vec3 baseGrass = mix(grassColor, vec3(0.15, 0.45, 0.15), t); // Gets darker
        // More brown particles in mid-range
        float brownAmount = 0.7 + (hasBrownParticle * 0.3);
        terrainColor = mix(baseGrass, brownParticle, hasBrownParticle * brownAmount * 0.4);
    } else {
        // High elevation (100+) - dark grass with snow particles
        vec3 highGrass = vec3(0.1, 0.35, 0.1); // Very dark grass at high elevation

        // Calculate snow particle coverage - increases with height
        float heightAbove100 = terrainHeight - 100.0;
        float snowCoverage = clamp(heightAbove100 / 50.0, 0.0, 0.8); // Max 80% snow coverage

        // Snow particles appear more frequently as we go higher
        float snowThreshold = 1.0 - snowCoverage; // Lower threshold = more snow
        float hasSnowParticle = step(snowThreshold, snowNoise);

        terrainColor = mix(highGrass, snowParticle, hasSnowParticle);
    }

    // Water color based on water depth
    vec3 shallowWaterColor = vec3(0.4, 0.7, 0.9);  // Light blue - shallow water
    vec3 deepWaterColor = vec3(0.0, 0.2, 0.6);     // Dark blue - deep water

    if (waterDepth > 0.0) {
        vec3 waterColor;

        float waterT = clamp(waterDepth / 5.0, 0.0, 1.0);
        waterColor = mix(shallowWaterColor, deepWaterColor, waterT);

        // Add transparency effect for shallow water
        float waterAlpha = clamp(waterDepth / 2.0, 0.3, 0.9);

        // Blend terrain and water colors
        terrainColor = mix(terrainColor, waterColor, waterAlpha);
    }

    // Obstacle overlay (e.g., rocks, buildings)
    if (isObstacle == 1.0) {
        terrainColor = vec3(0.4, 0.4, 0.4); // Gray color for obstacles
    }

    fragColor = vec4(terrainColor, 1.0);
}