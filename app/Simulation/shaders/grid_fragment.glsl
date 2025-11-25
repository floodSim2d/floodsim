#version 330 core
in vec2 fragTexCoord;
in float height;

out vec4 fragColor;

uniform sampler2D heightMap;

void main() {
    vec2 texelSize = vec2(1.0) / textureSize(heightMap, 0);

    float hL = texture(heightMap, fragTexCoord - vec2(texelSize.x, 0.0)).r;
    float hR = texture(heightMap, fragTexCoord + vec2(texelSize.x, 0.0)).r;
    float hD = texture(heightMap, fragTexCoord - vec2(0.0, texelSize.y)).r;
    float hU = texture(heightMap, fragTexCoord + vec2(0.0, texelSize.y)).r;

    vec3 normal = normalize(vec3(hL - hR, 2.0, hD - hU));

    vec3 lowColor = vec3(0.2, 0.5, 0.2);
    vec3 midColor = vec3(0.6, 0.6, 0.4);
    vec3 highColor = vec3(0.9, 0.6, 0.6);

    vec3 color;
    if (height < 10.0) {
        float t = height / 10.0;
        color = mix(lowColor, midColor, t);
    } else {
        float t = clamp((height - 10.0) / 20.0, 0.0, 1.0);
        color = mix(midColor, highColor, t);
    }

    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normal, lightDir), 0.0);
    float ambient = 0.4;

    color *= (ambient + diff * 0.6);

    fragColor = vec4(color, 1.0);
}