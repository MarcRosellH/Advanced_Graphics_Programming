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
	float metallic;
};

out vec2 vTexCoord;
out vec3 vPosition;	// In worldspace
out vec3 vNormal;	// In worldspace
out vec3 vViewDir;
out float metallicness;

void main()
{
	vTexCoord = aTexCoord;

	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir = uCameraPosition - vPosition;
	metallicness = metallic;

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;	// In worldspace
in vec3 vNormal;	// In worldspace
in vec3 vViewDir;
in float metallicness;

uniform vec3 uColor;
uniform vec3 cameraPos;

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
uniform samplerCube skybox;
uniform samplerCube irradianceMap;

layout(location = 0) out vec4 oColor;

vec3 DirectionalLight(Light light)
{
	float brightness = dot(normalize(light.direction), vNormal);
	return light.color * vec3(brightness);
}

vec3 PointLight(Light light)
{
	vec3 lightVector = normalize(light.position - vPosition);
	float brightness = dot(lightVector, vNormal);

	return vec3(brightness) * light.color;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
	vec3 objectColor = texture(uTexture, vTexCoord).rgb;
	vec3 c = objectColor*uColor;
	vec4 spec = vec4(0.0);

	vec3 lightFactor = vec3(1.0);
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
	vec3 V = normalize(cameraPos - vPosition);
	float ao = 0.5;
	float Lo = 0.5;
	vec3 F0 = vec3(0.04); 
	vec3 albedo = vec3(0.2); 

    F0 = mix(F0, albedo, 0.5);
	vec3 I = normalize(vPosition - cameraPos);
    vec3 R = reflect(I, normalize(vNormal));
	vec4 ReflectionColor = vec4(texture(skybox, R).rgb, 1.0);


	  vec3 kS = fresnelSchlick(max(dot(vNormal, V), 0.0), F0);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - 0.5;	  
    vec3 irradiance = texture(irradianceMap, vNormal).rgb;
    vec3 diffuse      = irradiance * albedo;
    vec3 ambient = (kD * diffuse) * ao;
    // vec3 ambient = vec3(0.002);
    
    vec3 color = ambient;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    oColor =mix(vec4(lightFactor, 1.0)  * vec4(uColor, 1.0), ReflectionColor, metallicness) ;


	//oColor = vec4(lightFactor, 1.0) * vec4(c, 1.0);
	gl_FragDepth = gl_FragCoord.z - 0.2;
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
#ifdef DEFERRED_GEOMETRY_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
	float metallic;
};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out float metallicness;

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal = vec3(transpose(inverse(uWorldMatrix)) * vec4(aNormal, 1.0));
	metallicness = metallic;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in float metallicness;

uniform sampler2D uTexture;
uniform vec3 uColor;
uniform vec3 cameraPos;
uniform samplerCube skybox;
uniform samplerCube irradianceMap;

layout(location = 0) out vec4 oPosition;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oColor;

out float gl_FragDepth;

void main()
{
	vec3 c = texture(uTexture, vTexCoord).rgb;
	
	oPosition = vec4(vPosition, 1.0);
	oNormals = vec4(normalize(vNormal), 1.0);
	oColor = vec4(c*uColor, 1.0);

	vec3 I = normalize(vPosition - cameraPos);
    vec3 R = reflect(I, normalize(vNormal));
	vec4 ReflectionColor = vec4(texture(skybox, R).rgb, 1.0);

	vec3 ambient = texture(irradianceMap, vNormal).rgb;


    oColor = mix(vec4(uColor, 1.0), ReflectionColor, metallicness) * 1.7*vec4(ambient, 1.0);

	// * vec4(ambient, 1.0)

	gl_FragDepth = gl_FragCoord.z - 0.2;
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
#ifdef DEFERRED_LIGHTING_PASS

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

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

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;

	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

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

layout(location = 0) out vec4 oFinalRender;

uniform sampler2D uGPosition;
uniform sampler2D uGNormals;
uniform sampler2D uGDiffuse;

vec3 DirectionalLight(Light light, vec3 Normal, vec3 Diffuse)
{
	float cosAngle = max(dot(Normal, -light.direction), 0.0); 
    vec3 ambient = 0.1 * light.color;
    vec3 diffuse = 0.9 * light.color * cosAngle * light.intensity;

    return (ambient + diffuse) * Diffuse;
}

vec3 PointLight(Light light, vec3 FragPos, vec3 Normal)
{
	vec3 N = normalize(Normal);
	vec3 L = normalize(light.position - FragPos);

	float threshold = 1.0;

	float shadowIntensity = 1.0;

	float dist = distance(light.position, FragPos);

	if(dist > light.radius)
		shadowIntensity = 1.0 - ((dist - light.radius) / threshold);

	// Hardcoded specular parameter
    vec3 specularMat = vec3(1.0);


	// Specular
    float specularIntensity = pow(max(0.0, dot(N, L)), 1.0);
    vec3 specular = specularMat * specularIntensity;

	// Diffuse
    float diffuseIntensity = max(0.0, dot(N, L));

	return vec3(specular + diffuseIntensity) * shadowIntensity * light.intensity * light.color;
}

void main()
{
	vec3 FragPos = texture(uGPosition, vTexCoord).rgb;
    vec3 Normal = texture(uGNormals, vTexCoord).rgb;
    vec3 Diffuse = texture(uGDiffuse, vTexCoord).rgb;

	vec3 viewDir = normalize(uCameraPosition - FragPos);

	vec3 lighting = Diffuse * 1.0;
    for(int i = 0; i < uLightCount; ++i)
    {
		switch(uLight[i].type)
		{
			case 0: // Directional
			{
                lighting += DirectionalLight(uLight[i], Normal, Diffuse);
			}
			break;

			case 1: // Point
			{
				float distance = length(uLight[i].position - FragPos);
				if(distance < uLight[i].radius)
				{
					lighting += PointLight(uLight[i], FragPos, Normal);
				}
			}
			break;

			default:
			{
				
			}
			break;
		}
    }

	oFinalRender = vec4(lighting * Diffuse, 1.0);

}

#endif
#endif

///////////////////////////////////////////////////////////////////////
#ifdef CLIPPED_MESHES

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

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
	Light uLight[50];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

uniform vec4 uClippingPlane;

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;

void main()
{
	vTexCoord = aTexCoord;

	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));

	vNormal = vec3(transpose(inverse(uWorldMatrix)) * vec4(aNormal, 1.0));

	vec4 clipDistanceDisplacement = vec4(0.0, 0.0, 0.0, length(vec3(uView * vec4(aPosition, 1.0)))/100.0);
	gl_ClipDistance[0] = dot(vec4(vPosition, 1.0), uClippingPlane);

	gl_Position = uProj * uView * uModel * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;

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
	Light uLight[50];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

uniform sampler2D uTexture;
uniform samplerCube uSkybox;
uniform vec3 uColor;

out float gl_FragDepth;

layout(location=0) out vec4 oColor;

void main()
{
	vec3 c = texture(uTexture, vTexCoord).rgb;
	c *= uColor;
	

	/*vec3 I = normalize(vPosition - uCameraPosition);
	vec3 R = reflect(I, normalize(vNormal));
	vec4 reflections = vec4(texture(uSkybox, R).rgb, 1.0);*/
	oColor = vec4(c, 1.0);

	gl_FragDepth = gl_FragCoord.z - 0.2;
}

#endif
#endif

///////////////////////////////////////////////////////////////////////
#ifdef WATER_EFFECT

#if defined(VERTEX) ///////////////////////////////////////////////////

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uProj;
uniform mat4 uView;

out Data
{
	vec3 positionViewspace;
	vec3 normalViewspace;
}VSOut;
out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;

	VSOut.positionViewspace = vec3(uView * vec4(aPosition, 1.0));
	VSOut.normalViewspace = vec3(uView * vec4(aNormal, 0.0));
	gl_Position = uProj * vec4(VSOut.positionViewspace, 1.0);

}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

layout(location = 0) out vec4 oColor;

uniform vec2 viewportSize;

uniform mat4 viewMatInv;
uniform mat4 projectionMatInv;

uniform sampler2D reflectionMap;
uniform sampler2D reflectionDepth;

uniform sampler2D refractionMap;
uniform sampler2D refractionDepth;

uniform sampler2D normalMap;
uniform sampler2D dudvMap;

uniform samplerCube skyBox;

in Data
{
	vec3 positionViewspace;
	vec3 normalViewspace;
} FSIn;

in vec2 vTexCoord;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 reconstructPixelPosition(float depth)
{
	vec2 texCoords = gl_FragCoord.xy / viewportSize;
	vec3 positionNDC = vec3(texCoords * 2.0 - vec2(1.0), depth * 2.0 - 1.0);
	vec4 positionEyespace = projectionMatInv * vec4(positionNDC, 1.0);
	positionEyespace.xyz /= positionEyespace.w;
	return positionEyespace.xyz;
}

out float gl_FragDepth;

void main()
{
	vec3 N = normalize(FSIn.normalViewspace);
	vec3 V = normalize(-FSIn.positionViewspace);
	vec3 Pw = vec3(viewMatInv * vec4(FSIn.positionViewspace, 1.0));
	vec2 texCoord = gl_FragCoord.xy / viewportSize;
	vec3 I = normalize(vec3(texCoord.x, 0.0, texCoord.y) - V);
	vec3 R = reflect(I, N);
	vec4 ref = vec4(texture(skyBox, R).rgb, 1.0);

	const vec2 waveLength = vec2(20.0);
	const vec2 waveStrength = vec2(0.05);
	const float turbidityDistance = 10.0;

	vec2 distortion = (2.0 * texture(dudvMap, Pw.xz / waveLength).rg - vec2(1.0)) * waveStrength + waveStrength/7.0;

	vec2 reflectionTexCoord = vec2(texCoord.s, 1.0 - texCoord.t) + distortion;
	vec2 refractionTexCoord = texCoord + distortion;
	vec3 reflectionColor = texture(reflectionMap, reflectionTexCoord).rgb;
	vec3 refractionColor = texture(refractionMap, refractionTexCoord).rgb;

	float distortedGroundDepth = texture(refractionDepth, refractionTexCoord).x;
	vec3 distortedGroundPosViewspace = reconstructPixelPosition(distortedGroundDepth);
	float distortedWaterDepth = FSIn.positionViewspace.z - distortedGroundPosViewspace.z;
	float tintFactor = clamp(distortedWaterDepth / turbidityDistance, 0.0, 1.0);
	vec3 waterColor = vec3(0.25, 0.4, 0.6);
	refractionColor = mix(refractionColor, waterColor, tintFactor*2);

	vec3 F0 = vec3(0.1);
	vec3 F = fresnelSchlick(max(0.0, dot(V, N)), F0);
	oColor = vec4(mix(refractionColor, reflectionColor, F), 1.0);
	oColor = mix(ref, oColor, 0.5);
	//oColor = vec4(texture(dudvMap, vTexCoord).rgb, 1.0);
	//oColor = vec4(reflectionColor, 1.0);

	gl_FragDepth = gl_FragCoord.z - 0.2;

}

#endif
#endif