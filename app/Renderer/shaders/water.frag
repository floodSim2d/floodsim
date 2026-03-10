#version 410 core

in vec2 fragTexCoord;
in float vTerrainHeight;
in float vWaterDepth;
in float vWaterSurfaceHeight;
in float vVelocityMag;
in vec2 vWorldPos;
in vec3 vFragWorldPos;
in vec3 vFragNormal;

out vec4 fragColor;

uniform vec3 lightDirection;
uniform vec3 viewPos;


float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

vec2 hash2(vec2 p) {
    return vec2(
        fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453),
        fract(sin(dot(p, vec2(269.5, 183.3))) * 43758.5453)
    );
}

// Smooth value noise
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

// Voronoi cell pattern
float voronoi(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    float minDist = 1.0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash2(i + neighbor);
            vec2 diff = neighbor + point - f;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

float particle(vec2 p, float cellScale, float radius) {
    vec2 cell = floor(p * cellScale);
    vec2 cellFrac = fract(p * cellScale);

    // one random point per cell
    vec2 particlePos = hash2(cell) * 0.6 + 0.2; // keep away from edges
    float dist = length(cellFrac - particlePos);

    // sharp dot
    return smoothstep(radius, radius * 0.3, dist);
}

vec3 getWaterColor(float depth) {
    // based on water depth
    vec3 shallowWater = vec3(0.15, 0.55, 0.75);  // light blue-cyan
    vec3 mediumWater  = vec3(0.05, 0.30, 0.60);   // medium blue
    vec3 deepWater    = vec3(0.01, 0.08, 0.30);    // dark navy

    // interpolate colors based on depth thresholds (internal units, ×10 = displayed meters)
    float t1 = clamp(depth / 5.0, 0.0, 1.0);   // shallow -> medium (0-5 units = 0-50 m)
    float t2 = clamp(depth / 20.0, 0.0, 1.0);   // medium -> deep (0-20 units = 0-200 m)

    vec3 color = mix(shallowWater, mediumWater, t1);
    color = mix(color, deepWater, t2);

    return color;
}

float getWaterOpacity(float depth) {
    // opacity increases with depth
    // shallow water is translucent, deep water is nearly opaque
    return clamp(sqrt(depth) / 2.5, 0.25, 0.92);
}

void main()
{
    // case when there is no water
    if (vWaterDepth < 0.01) {
        discard;
    }

    vec3 waterColor = getWaterColor(vWaterDepth);

    // ─── Velocity visualization ────────────────────────────────────────
    // Normalize velocity: ~0.5 = gentle flow, ~2+ = fast rapids
    float velNorm = clamp(vVelocityMag / 3.0, 0.0, 1.0);

    // --- Voronoi cell pattern — intensity modulated by velocity ---
    float cells1 = voronoi(vWorldPos * 0.4);
    float cells2 = voronoi(vWorldPos * 0.9 + vec2(17.3, 31.7));
    float cellPattern = cells1 * 0.6 + cells2 * 0.4;
    waterColor += vec3(cellPattern * 0.06 - 0.03);

    // --- Scattered bright specks ---
    float specks1 = particle(vWorldPos, 0.8, 0.08);
    float specks2 = particle(vWorldPos + vec2(43.7, 91.2), 1.5, 0.06);
    float specks3 = particle(vWorldPos + vec2(71.1, 23.4), 3.0, 0.04);

    float allSpecks = specks1 * 0.5 + specks2 * 0.35 + specks3 * 0.2;
    vec3 speckColor = vec3(0.6, 0.8, 0.95);
    waterColor = mix(waterColor, speckColor, allSpecks * 0.4);

    float colorVar = noise(vWorldPos * 0.03) * 0.08 - 0.04;
    waterColor += vec3(colorVar * 0.5, colorVar, colorVar * 0.8);

    // --- Turbulent foam (velocity-driven) ---
    // More noise layers and brighter foam when water flows fast
    if (velNorm > 0.05) {
        float turbNoise1 = noise(vWorldPos * 1.5);
        float turbNoise2 = noise(vWorldPos * 3.0 + vec2(55.0, 33.0));
        float turbulence = turbNoise1 * 0.6 + turbNoise2 * 0.4;

        // Foam mask: appears more with higher velocity
        float foamThreshold = mix(0.75, 0.3, velNorm); // lower threshold = more foam
        float turbFoam = smoothstep(foamThreshold, foamThreshold + 0.15, turbulence) * velNorm;

        vec3 turbFoamColor = vec3(0.85, 0.92, 0.98); // white foam
        waterColor = mix(waterColor, turbFoamColor, turbFoam * 0.6);
    }

    // --- Whitewater rapids for very fast flow ---
    if (velNorm > 0.4) {
        float rapidsFactor = smoothstep(0.4, 0.9, velNorm);
        float rapidsNoise = noise(vWorldPos * 5.0) * noise(vWorldPos * 2.0 + vec2(17.0, 41.0));
        float rapidsMask = smoothstep(0.15, 0.4, rapidsNoise) * rapidsFactor;

        vec3 whitewater = vec3(0.92, 0.96, 1.0);
        waterColor = mix(waterColor, whitewater, rapidsMask * 0.5);
    }

    // foam at edges (within ~0.8 units = ~8 m of shoreline)
    float edgeFactor = clamp(1.0 - vWaterDepth / 0.8, 0.0, 1.0);
    float foamNoise = noise(vWorldPos * 2.0);
    float foamMask = edgeFactor * smoothstep(0.3, 0.6, foamNoise);
    vec3 foamColor = vec3(0.75, 0.88, 0.95);
    waterColor = mix(waterColor, foamColor, foamMask * 0.45);

    // blinn-phong lighting
    vec3 normal = normalize(vFragNormal);
    vec3 lightDir = normalize(lightDirection);
    vec3 viewDir = normalize(viewPos - vFragWorldPos);

    // ambient
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * waterColor;

    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * waterColor;

    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    float specularStrength = 0.5;
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 0.95);

    vec3 litColor = ambient + diffuse + specular;
    float alpha = getWaterOpacity(vWaterDepth);

    fragColor = vec4(litColor, alpha);
}
