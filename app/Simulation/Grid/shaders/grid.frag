#version 410 core
in vec2 fragTexCoord;
in float terrainHeight;
in float waterDepth;
in float isObstacle;

out vec4 fragColor;

uniform sampler2D heightMap;

// Simple hash/noise helper
float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// ============================================================================
// TERRAIN LEVEL FUNCTIONS
// ============================================================================

vec3 getBelowSeaLevelColor(vec2 texCoord, float height) {
    float depthFactor = clamp(-height / 20.0, 0.0, 1.0);
    vec3 surface = vec3(0.08, 0.22, 0.06);
    vec3 deep    = vec3(0.03, 0.08, 0.03);
    return mix(surface, deep, depthFactor);
}

// Low grassland (0.0 - 50.0) - PRZYWRÓCONO ZIELONY
vec3 getLowGrasslandColor(vec2 texCoord, float height) {
    vec3 grassColor = vec3(0.12, 0.50, 0.08);
    vec3 darkGrass = vec3(0.08, 0.40, 0.06);
    float t = height / 50.0;
    return mix(grassColor, darkGrass, t);
}

// "Painted" terrain (50.0 - 100.0) - ZMIENIONO NA BRĄZOWY
vec3 getPaintedTerrainColor(vec2 texCoord, float height) {
    vec3 baseBrown = vec3(0.55, 0.42, 0.28);
    vec3 darkBrown = vec3(0.35, 0.25, 0.15);
    float t = (height - 50.0) / 50.0;
    return mix(baseBrown, darkBrown, t);
}

// High grassland approaching snow (100.0 - 150.0)
vec3 getHighGrasslandColor(vec2 texCoord, float height) {
    vec3 brown = vec3(0.35, 0.25, 0.15);
    vec3 grayRock = vec3(0.5, 0.5, 0.5);
    float t = (height - 100.0) / 50.0;
    return mix(brown, grayRock, t);
}

// Snow level (150.0+)
vec3 getSnowLevelColor(vec2 texCoord, float height) {
    vec3 grayRock = vec3(0.5, 0.5, 0.5);
    vec3 snow = vec3(0.98, 0.98, 1.0);
    float t = clamp((height - 150.0) / 100.0, 0.0, 1.0);
    return mix(grayRock, snow, t);
}

/*
* main function rendering terrain
*/
void main() {
    vec3 terrainColor;

    // Determine terrain color based on height level
    if (terrainHeight < 0.0) {
        terrainColor = getBelowSeaLevelColor(fragTexCoord, terrainHeight);
    } else if (terrainHeight < 50.0) {
        terrainColor = getLowGrasslandColor(fragTexCoord, terrainHeight);
    } else if (terrainHeight < 100.0) {
        terrainColor = getPaintedTerrainColor(fragTexCoord, terrainHeight);
    } else if (terrainHeight < 150.0) {
        terrainColor = getHighGrasslandColor(fragTexCoord, terrainHeight);
    } else {
        terrainColor = getSnowLevelColor(fragTexCoord, terrainHeight);
    }

    // todo: refactor, add some noise-based buildings
    // Obstacle overlay
    if (isObstacle == 1.0) {
        terrainColor = vec3(0.4, 0.4, 0.4);
    }

    if (waterDepth > 0.01) {
        float darkening = clamp(waterDepth * 0.15, 0.0, 0.7);
        terrainColor *= (1.0 - darkening);
    }

    fragColor = vec4(terrainColor, 1.0);
}