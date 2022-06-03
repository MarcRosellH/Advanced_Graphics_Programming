
#ifdef SKYBOX

#if defined(VERTEX) ///////////////////////////////////////////////////

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
	TexCoords = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

layout(location=0) out vec4 oColor;

void main()
{
    FragColor = texture(skybox, TexCoords);
}

#endif
#endif