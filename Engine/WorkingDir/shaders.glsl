///////////////////////////////////////////////////////////////////////
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location=0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vecc3 aBitangent;

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	float intensity;
	vec3 position;
	float radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;	// In worldspace
out vec3 vNormal;	// In worldspace
out vec3 vViewDir;

void main()
{
	vTexCoord = aTexCoord;

	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir = uCameraPosition - vPosition;

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;	// In worldspace
in vec3 vNormal;	// In worldspace
in vec3 vViewDir;

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	float intensity;
	vec3 position;
	float radius;
};

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

vec3 DirectionalLight(Light light)
{
	return vec3(1.0);
}

vec3 PointLight(Light light)
{
	vec3 lightVector = normalize(light.position - vPosition);
	float brightness = dot(lightVector, vNormal);

	return vec3(brightness) * light.color;
}

void main()
{
	vec4 objectColor = texture(uTexture, vTexCoord);
	vec4 spec = vec4(0.0);

	vec3 lightFactor = vec3(0.0);
	for(int i = 0; i < uLightCount; ++i)
	{
		switch(uLight[i].type)
		{
			case 0: // Directional
			{
				lightFactor += DirectionalLight(uLight[i]);
			}
			break;

			case 1: // Point
			{
				lightFactor += PointLight(uLight[i]);
			}
			break;

			default:
			{
				break;
			}
		}
	}

	oColor = vec4(lightFactor, 1.0) * objectColor;
}

#endif
#endif