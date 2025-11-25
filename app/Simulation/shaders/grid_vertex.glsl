#version 330 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

uniform mat4 projection;
uniform mat4 view;
uniform sampler2D heightMap;
uniform float heightScale;
uniform vec2 gridSize;

out vec2 fragTexCoord;
out float height;

void main()
{
    fragTexCoord = texCoord;

    height = texture(heightMap, fragTexCoord).r;

    vec3 pos = vec3(
                    position.x * gridSize.x,
                    height * heightScale,
                    position.y * gridSize.y
                );
    gl_Position = projection * view * vec4(pos, 1.0);
}