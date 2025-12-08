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
// TERRAIN LEVEL FUNCTIONS - Each handles a specific elevation range
// ============================================================================

vec3 getWaterColor(vec2 texCoord, float depth, float totalHeight) {
    vec3 shallowWater = vec3(0.2, 0.5, 0.7);
    vec3 deepWater = vec3(0.0, 0.05, 0.3);
    vec3 waterParticle = vec3(0.1, 0.2, 0.5);

    float blueNoise = hash21(texCoord * 500.0);

    // Deeper water = lower total height
    // Map total height to darkness: lower height (negative values) = darker water
    // Normalize based on expected range: 0 (sea level) to -50 (very deep)
    float depthFromSurface = -totalHeight; // Invert so lower = more positive
    float t = clamp(depthFromSurface / 30.0, 0.0, 1.0);

    vec3 color = mix(shallowWater, deepWater, t);

    float particleChance = 0.3 + 0.2 * t; // More particles as we go deeper
    if (blueNoise < particleChance) {
        float pAmt = mix(0.3, 0.7, blueNoise);
        return mix(color, waterParticle, pAmt * 0.4);
    }
    return color;
}

// Below sea level (height < 0.0)
vec3 getBelowSeaLevelColor(vec2 texCoord, float height) {
    // Muddy dark green but not pitch black
    float depthFactor = clamp(-height / 20.0, 0.0, 1.0);
    float greenIntensity = mix(0.18, 0.40, depthFactor);

    return vec3(0.05, greenIntensity, 0.05);
}

// Low grassland (0.0 - 50.0)
vec3 getLowGrasslandColor(vec2 texCoord, float height) {
    vec3 grassColor = vec3(0.12, 0.50, 0.08);
    vec3 brownParticle = vec3(0.45, 0.34, 0.25);

    float brownNoise = hash21(texCoord * 800.0);

    vec3 color = grassColor;

    // Add brown dirt particles - more visible at low elevations
    float brownChance = 0.25;
    if (brownNoise < brownChance) {
        float bAmt = mix(0.3, 0.7, brownNoise);
        color = mix(color, brownParticle, bAmt * 0.6); // Increased blend strength
    }

    return color;
}

// Mid grassland (50.0 - 100.0)
vec3 getMidGrasslandColor(vec2 texCoord, float height) {
    vec3 grassColor = vec3(0.12, 0.50, 0.08);
    vec3 darkerGrass = vec3(0.08, 0.40, 0.06);
    vec3 brownParticle = vec3(0.45, 0.34, 0.25);
    vec3 grayParticle = vec3(0.5, 0.5, 0.5);

    float brownNoise = hash21(texCoord * 800.0);
    float grayNoise = hash21(texCoord * 650.0 + 23.4);

    // Gradually darken grass
    float t = (height - 50.0) / 50.0;
    vec3 color = mix(grassColor, darkerGrass, t * 0.3);

    // Brown particles - very visible in mid range
    float brownChance = 0.35;
    if (brownNoise < brownChance) {
        float bAmt = mix(0.35, 0.75, brownNoise);
        color = mix(color, brownParticle, bAmt * 0.65);
    }

    // Gray rocky particles start appearing
    float grayChance = 0.15;
    if (grayNoise < grayChance) {
        float gAmt = mix(0.25, 0.65, grayNoise);
        color = mix(color, grayParticle, gAmt * 0.5);
    }

    return color;
}

// High grassland approaching snow (100.0 - 150.0)
vec3 getHighGrasslandColor(vec2 texCoord, float height) {
    vec3 darkGrass = vec3(0.05, 0.30, 0.04);
    vec3 brownParticle = vec3(0.45, 0.34, 0.25);
    vec3 grayParticle = vec3(0.5, 0.5, 0.5);

    float brownNoise = hash21(texCoord * 800.0);
    float grayNoise = hash21(texCoord * 650.0 + 23.4);

    vec3 color = darkGrass;

    // Brown particles still present
    float brownChance = 0.30;
    if (brownNoise < brownChance) {
        float bAmt = mix(0.3, 0.7, brownNoise);
        color = mix(color, brownParticle, bAmt * 0.55);
    }

    // More gray rocky particles
    float grayChance = 0.40;
    if (grayNoise < grayChance) {
        float gAmt = mix(0.3, 0.7, grayNoise);
        color = mix(color, grayParticle, gAmt * 0.6);
    }

    return color;
}

// Snow level (150.0+)
vec3 getSnowLevelColor(vec2 texCoord, float height) {
    vec3 darkGrass = vec3(0.05, 0.30, 0.04);
    vec3 grayParticle = vec3(0.5, 0.5, 0.5);
    vec3 snowParticle = vec3(0.98, 0.98, 1.0);

    float grayNoise = hash21(texCoord * 650.0 + 23.4);
    float snowNoise = hash21(texCoord * 1000.0 + 77.7);

    // Start with dark grass and gray rocks
    vec3 color = darkGrass;

    float grayChance = 0.50;
    if (grayNoise < grayChance) {
        float gAmt = mix(0.35, 0.75, grayNoise);
        color = mix(color, grayParticle, gAmt * 0.65);
    }

    // Add snow particles - increases with height
    float heightAbove = height - 150.0;
    float snowCoverage = clamp(heightAbove / 100.0, 0.0, 0.95);

    float snowThreshold = 1.0 - snowCoverage;
    float hasSnow = step(snowThreshold, snowNoise);

    color = mix(color, snowParticle, hasSnow * snowCoverage);

    // Extra snow flakes for detail
    float snowFlake = hash21(texCoord * 4000.0 + 3.14);
    if (snowFlake > 0.995) {
        color = mix(color, snowParticle, 0.85);
    }

    return color;
}

/*
* main function
*/
void main() {
    vec3 terrainColor;

    // Determine terrain color based on height level
    if (terrainHeight < 0.0) {
        terrainColor = getBelowSeaLevelColor(fragTexCoord, terrainHeight);
    } else if (terrainHeight < 50.0) {
        terrainColor = getLowGrasslandColor(fragTexCoord, terrainHeight);
    } else if (terrainHeight < 100.0) {
        terrainColor = getMidGrasslandColor(fragTexCoord, terrainHeight);
    } else if (terrainHeight < 150.0) {
        terrainColor = getHighGrasslandColor(fragTexCoord, terrainHeight);
    } else {
        terrainColor = getSnowLevelColor(fragTexCoord, terrainHeight);
    }

    // Water overlay
    if (waterDepth > 0.0) {
        float totalHeight = terrainHeight + waterDepth;
        vec3 waterColor = getWaterColor(fragTexCoord, waterDepth, totalHeight);
        // More opaque blending - water covers terrain more at greater depths
        float waterOpacity = clamp(waterDepth / 3.0, 0.0, 0.95);
        terrainColor = mix(terrainColor, waterColor, waterOpacity);
    }

    // Obstacle overlay
    if (isObstacle == 1.0) {
        terrainColor = vec3(0.4, 0.4, 0.4);
    }

    fragColor = vec4(terrainColor, 1.0);
}