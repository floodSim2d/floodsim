#version 410 core
in vec2 fragTexCoord;
in float terrainHeight;
in float waterDepth;
in float isObstacle;
in vec3 fragWorldPos;
in vec3 fragNormal;

out vec4 fragColor;

uniform sampler2D heightMap;
uniform vec3 lightDirection;
uniform vec3 viewPos;

// ============================================================================
// NOISE FUNCTIONS — terrain texture (coś jak w minecracie, tylko ze generowane sztucznie)
// ============================================================================

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

vec2 hash2(vec2 p) {
    return vec2(
        fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453),
        fract(sin(dot(p, vec2(269.5, 183.3))) * 43758.5453)
    );
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractional Brownian Motion - layered noise for natural patterns
float fbm(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p);
        p *= 2.17;
        amplitude *= 0.48;
    }
    return value;
}

// Voronoi for cracked/cellular patterns (rock, dry earth)
float voronoi(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float minDist = 1.0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash2(i + neighbor);
            float dist = length(neighbor + point - f);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

// ============================================================================
// TERRAIN COLOR FUNCTIONS
// ============================================================================

// Below sea level — dark muddy ground
vec3 getBelowSeaLevelColor(float height) {
    float depthFactor = clamp(-height / 20.0, 0.0, 1.0);
    vec3 surface = vec3(0.08, 0.22, 0.06);
    vec3 deep    = vec3(0.03, 0.08, 0.03);
    vec3 base = mix(surface, deep, depthFactor);

    // muddy texture
    vec2 wp = fragWorldPos.xy;
    float mud = fbm(wp * 0.8, 3) * 0.08 - 0.04;
    base += vec3(mud * 0.6, mud, mud * 0.4);

    return base;
}

// Low grassland (0.0 - 50.0)
vec3 getLowGrasslandColor(float height) {
    vec2 wp = fragWorldPos.xy;

    vec3 grassBright = vec3(0.14, 0.52, 0.10);
    vec3 grassDark   = vec3(0.06, 0.32, 0.04);
    vec3 grassYellow = vec3(0.22, 0.45, 0.08);
    vec3 dirtPatch   = vec3(0.18, 0.14, 0.08);

    float patchNoise = fbm(wp * 0.06, 3);
    vec3 grassBase = mix(grassBright, grassDark, patchNoise);

    float yellowPatch = fbm(wp * 0.04 + vec2(50.0, 80.0), 2);
    grassBase = mix(grassBase, grassYellow, smoothstep(0.45, 0.65, yellowPatch) * 0.4);

    float blades = noise(vec2(wp.x * 3.0, wp.y * 8.0));
    float blades2 = noise(vec2(wp.x * 7.0 + 31.0, wp.y * 4.0 + 17.0));
    float bladePattern = blades * 0.5 + blades2 * 0.5;
    grassBase += vec3(-0.01, bladePattern * 0.08 - 0.04, -0.01);

    float dirtMask = fbm(wp * 0.3, 3);
    float dirtAmount = smoothstep(0.58, 0.72, dirtMask) * 0.5;
    grassBase = mix(grassBase, dirtPatch, dirtAmount);

    float grain = hash(floor(wp * 5.0)) * 0.06 - 0.03;
    grassBase += vec3(grain * 0.3, grain, grain * 0.3);

    float heightFade = height / 50.0;
    grassBase *= mix(1.0, 0.85, heightFade);

    return grassBase;
}

// Brown terrain (50.0 - 100.0) — dirt, dry earth, rocky soil
vec3 getPaintedTerrainColor(float height) {
    vec2 wp = fragWorldPos.xy;

    vec3 lightBrown = vec3(0.55, 0.42, 0.28);
    vec3 darkBrown  = vec3(0.32, 0.22, 0.12);
    vec3 reddish    = vec3(0.45, 0.28, 0.15);

    float t = (height - 50.0) / 50.0;
    vec3 base = mix(lightBrown, darkBrown, t);

    // Large rocky patches
    float rockPatch = fbm(wp * 0.08, 3);
    base = mix(base, reddish, smoothstep(0.4, 0.6, rockPatch) * 0.35);

    float cracks = voronoi(wp * 0.4);
    base *= 0.9 + cracks * 0.15;

    float grain = fbm(wp * 1.5, 2) * 0.1 - 0.05;
    base += vec3(grain);

    float stones = hash(floor(wp * 2.0));
    if (stones > 0.92) {
        base = mix(base, vec3(0.5, 0.48, 0.44), 0.4);
    }

    return base;
}

// High terrain (100.0 - 150.0) — rocky with patches of moss
vec3 getRockyMountainColor(float height) {
    vec2 wp = fragWorldPos.xy;

    vec3 rock     = vec3(0.42, 0.40, 0.38);
    vec3 darkRock = vec3(0.28, 0.26, 0.24);
    vec3 moss     = vec3(0.15, 0.28, 0.10);

    float t = (height - 100.0) / 50.0;
    vec3 base = mix(darkRock, rock, t);

    float strata = noise(vec2(wp.x * 0.15, wp.y * 0.15 + height * 0.1));
    base = mix(base, darkRock, strata * 0.3);

    float cracks = voronoi(wp * 0.25);
    base *= 0.85 + cracks * 0.2;

    float mossMask = smoothstep(0.0, 0.15, cracks);
    base = mix(moss, base, mossMask);

    float grain = hash(floor(wp * 3.0)) * 0.06 - 0.03;
    base += vec3(grain);

    return base;
}

// Snow level (150.0+)
vec3 getSnowLevelColor(float height) {
    vec2 wp = fragWorldPos.xy;

    vec3 snow     = vec3(0.95, 0.95, 0.98);
    vec3 blueSnow = vec3(0.82, 0.86, 0.95);
    vec3 rockPeek = vec3(0.45, 0.43, 0.40);

    float t = clamp((height - 150.0) / 100.0, 0.0, 1.0);

    // snow coverage increases with height
    vec3 base = mix(rockPeek, snow, t);

    // wind-blown snow pattern
    float windPattern = fbm(vec2(wp.x * 0.1 + wp.y * 0.05, wp.y * 0.08), 3);
    base = mix(base, blueSnow, windPattern * 0.25);

    // exposed rock patches at lower snow levels
    if (t < 0.6) {
        float exposure = fbm(wp * 0.15, 3);
        float rockMask = smoothstep(0.5, 0.7, exposure) * (1.0 - t);
        base = mix(base, rockPeek, rockMask * 0.5);
    }

    // fine snow sparkle
    float sparkle = hash(floor(wp * 8.0));
    if (sparkle > 0.95) {
        base += vec3(0.05);
    }

    // subtle blue shadows in dips
    float shadow = fbm(wp * 0.3, 2);
    base = mix(base, blueSnow, shadow * 0.1);

    return base;
}

/*
* main function rendering terrain
*/
void main() {
    vec3 terrainColor;

    // each pixel gets a slightly different threshold so biome borders are irregular not straight lines.
    vec2 wp = fragWorldPos.xy;
    float transitionNoise = fbm(wp * 0.12, 3) * 20.0 - 10.0; // ±10 height units of variation
    float h = terrainHeight + transitionNoise;

    // transition width - the zone where two biomes blend together
    float tw = 12.0;

    vec3 belowSea   = getBelowSeaLevelColor(terrainHeight);
    vec3 grass      = getLowGrasslandColor(terrainHeight);
    vec3 dirt       = getPaintedTerrainColor(terrainHeight);
    vec3 highRock   = getRockyMountainColor(terrainHeight);
    vec3 snow       = getSnowLevelColor(terrainHeight);

    // blend between adjacent biomes using smoothstep on the noise-perturbed height
    if (h < 0.0) {
        float blend = smoothstep(-tw, 0.0, h);
        terrainColor = mix(belowSea, grass, blend);
    } else if (h < 50.0) {
        float blend = smoothstep(50.0 - tw, 50.0, h);
        terrainColor = mix(grass, dirt, blend);
    } else if (h < 100.0) {
        float blend = smoothstep(100.0 - tw, 100.0, h);
        terrainColor = mix(dirt, highRock, blend);
    } else if (h < 150.0) {
        float blend = smoothstep(150.0 - tw, 150.0, h);
        terrainColor = mix(highRock, snow, blend);
    } else {
        terrainColor = snow;
    }

    // Obstacle overlay
    if (isObstacle == 1.0) {
        terrainColor = vec3(0.4, 0.4, 0.4);
    }

    // Darken terrain under water
    if (waterDepth > 0.01) {
        float darkening = clamp(waterDepth * 0.15, 0.0, 0.7);
        terrainColor *= (1.0 - darkening);
    }

    // blinn-phong lighting
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(lightDirection);
    vec3 viewDir = normalize(viewPos - fragWorldPos);

    // ambient
    float ambientStrength = 0.35;
    vec3 ambient = ambientStrength * terrainColor;

    // diffuse - lambertian
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * terrainColor;

    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    float specularStrength = 0.08; // terrain is matte, low specular
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec3 litColor = ambient + diffuse + specular;

    fragColor = vec4(litColor, 1.0);
}