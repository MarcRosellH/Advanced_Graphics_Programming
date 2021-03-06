//
// engine.h: This file contains the types and functions relative to the engine.
//

/*
* TODOs:
* - Comment all code
* - Change update correctly Entity and Camera structs
* - Functions to get and set position/rotation/scale in mat4
* - Framebuffers and different outputs (albedo, normals, position, depth) -> to render later in texture
* - Set all process as deferred rendering
* - Create ImGui combo menu to manipulate positions, element materials, lights...
* - Correct rendering for deferred
*/

#pragma once

#define BINDING(b) b

#include "platform.h"
#include <glad/glad.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute>  attributes;
    u8                                  stride;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute>  attributes;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    VertexShaderLayout vertexInputLayout;
    u64                lastWriteTimestamp;
};

struct Model
{
    u32                 meshIdx;
    std::vector<u32>    materialIdx;
};

struct Vao
{
    GLuint  handle;
    GLuint  programHandle;
};

struct Submesh
{
    VertexBufferLayout  vertexBufferLayout;
    std::vector<float>  vertices;
    std::vector<u32>    indices;
    u32                 vertexOffset;
    u32                 indexOffset;

    std::vector<Vao>    vaos;
};

struct Mesh
{
    std::vector<Submesh>    submeshes;
    GLuint                  vertexBufferHandle;
    GLuint                  indexBufferHandle;
};

struct Material
{
    std::string name;
    vec3        albedo;
    vec3        emissive;
    f32         smoothness;
    u32         albedoTextureIdx;
    u32         emissiveTextureIdx;
    u32         specularTextureIdx;
    u32         normalsTextureIdx;
    u32         bumpTextureIdx;
};

struct Camera
{
    vec3    position;
    vec3    front;
    vec3    up;
    vec3    right;
    vec3    worldUp;

    float   yaw;
    float   pitch;
    float   fov = 60.F;
    float   nearPlane = 0.1F;
    float   farPlane = 10000.0F;
    float   aspectRatio;

    float   speed;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_TexturedMesh,
    Mode_Deferred,
    Mode_Count
};

struct VertexV3V2
{
    vec3 pos;
    vec2 uv;
};

struct OpenGLInfo
{
    std::string  version;
    std::string  renderer;
    std::string  vendor;
    std::string  glslVersion;
    GLint        numExtensions;
    std::string* extensions;
};

struct Entity
{
    vec3        position;
    vec3        rotation;
    vec3        scale;
    u32         modelIdx;
    float       metallic;
    u32         localParamsOffset;
    u32         localParamsSize;
};

enum LightType
{
    LIGHTTYPE_DIRECTIONAL,
    LIGHTTYPE_POINT
};

struct Light
{
    LightType   type;
    vec3        color;
    vec3        position;
    vec3        direction;
    float       radius;
    float       intensity;
};

struct Buffer
{
    GLuint  handle;
    GLenum  type;
    u32     size;
    u32     head;
    void*   data;
};

enum class FBOAttachmentType
{
    POSITION,
    NORMALS,
    DIFFUSE,
    DEPTH,
    FINAL
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    ivec2 displaySize;

    // Model resources
    std::vector<Texture>    textures;
    std::vector<Program>    programs;
    std::vector<Material>   materials;
    std::vector<Mesh>       meshes;
    std::vector<Model>      models;

    // Model indices
    u32 patrickModelIdx;
    u32 roomModelIdx;
    u32 planeModelIdx;

    // Program indices
    u32 texturedGeometryProgramIdx;
    u32 texturedMeshProgramIdx;
    // Deferred program indices
    u32 deferredGeometryPassProgramIdx;
    u32 deferredLightingPassProgramIdx;
    u32 deferredLightProgramIdx;
    //skybox program
    u32 skyBox;
    u32 ConvolutionShader;
    // Water program indices
    u32 clippedMeshIdx;
    u32 waterEffectProgramIdx;

    // Texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;
    u32 skyBoxtext[6];
    u32 waterNormalMapIdx;
    u32 waterDudvMapIdx;

    // Entities
    std::vector<Entity>     entities;
    // Lights
    std::vector<Light>      lights;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    // Uniforms
    GLint texturedMeshProgram_uTexture;
    GLint texturedMeshProgram_uColor;

    GLint deferredGeometryProgram_uTexture;
    GLint deferredGeometryProgram_uColor;

    GLint deferredGeometryProgram_uSkybox;
    GLint deferredGeometryProgram_uIrradiance;

    GLint convolutionProgram_uSkybox;

    GLint skyboxProgram_uSkybox;

    GLint deferredLightingProgram_uGPosition;
    GLint deferredLightingProgram_uGNormals;
    GLint deferredLightingProgram_uGDiffuse;

    GLint clippedProgram_uProj;
    GLint clippedProgram_uView;
    GLint clippedProgram_uModel;
    GLint clippedProgram_uClippingPlane;
    GLint clipperProgram_uTexture;
    GLint clipperProgram_uSkybox;
    GLint clipperProgram_uColor;

    GLint waterEffectProgram_uProj;
    GLint waterEffectProgram_uView;
    GLint waterEffectProgram_uViewportSize;
    GLint waterEffectProgram_uViewMatInv;
    GLint waterEffectProgram_uProjMatInv;
    GLint waterEffectProgram_uReflectionMap;
    GLint waterEffectProgram_uReflectionDepth;
    GLint waterEffectProgram_uRefractionMap;
    GLint waterEffectProgram_uRefractionDepth;
    GLint waterEffectProgram_uNormalMap;
    GLint waterEffectProgram_uDudvMap;
    GLint waterEffectProgram_uSkybox;

    // Framebuffers ---------------------
    // Deferred
    GLuint gBuffer;
    GLuint positionAttachmentHandle;
    GLuint normalsAttachmentHandle;
    GLuint diffuseAttachmentHandle;
    GLuint depthAttachmentHandle;
    GLuint cubeMapId;
    GLuint irradianceMapId;

    GLuint cubeMapFBO;
    GLuint captureFBO;
    GLuint captureRBO;

    GLuint fBuffer;
    GLuint finalRenderAttachmentHandle;

    // Forward
    GLuint forwardRenderAttachmentHandle;
    GLuint forwardDepthAttachmentHandle;

    GLuint forwardFrameBuffer;

    // Water reflection
    GLuint waterReflectionAttachmentHandle;
    GLuint waterReflectionDepthAttachmentHandle;

    GLuint waterReflectionFrameBuffer;

    // Water refraction
    GLuint waterRefractionAttachmentHandle;
    GLuint waterRefractionDepthAttachmentHandle;

    GLuint waterRefractionFrameBuffer;

    // ---------------------------------

    FBOAttachmentType currentFBOAttachmentType;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    // OpenGL context information
    OpenGLInfo info;

    // Camera
    Camera cam;

    glm::mat4 projectionMat;
    glm::mat4 viewMat;

    // Uniform buffers data management
    GLint   maxUniformBufferSize;
    GLint   uniformBlockAlignment;
    Buffer  uniformBuffer;
    u32     globalParamsOffset;
    u32     globalParamsSize;

    // Final quad rendering (deferred)
    GLuint quadVAO = 0u;
    GLuint quadVBO;

    GLuint SKyboxVAO = 0u;
    GLuint SKyboxIBO = 0u;

    GLuint SkyboxVBO;

    GLuint sphereVAO = 0u;
    GLuint sphereIdxCount;

    bool isFocused = true;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

u32 LoadTexture2D(App* app, const char* filepath);

u32 LoadModel(App* app, const char* filename);

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);

u8 GetAttribComponentCount(const GLenum& type);

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

glm::mat4 MatrixFromPositionRotationScale(const vec3& position, const vec3& rotation, const vec3& scale);

void RenderQuad(App* app);

void RenderSkybox(App* app);
void SetCamera(Camera& cam);

void SetAspectRatio(Camera& cam, float dsX, float dsY);

glm::mat4 GetViewMatrix(Camera& cam);

glm::mat4 GetProjectionMatrix(Camera& cam);

void HandleInput(App* app);

void LoadSphere(App* app);

void RenderSphere(App* app);

void LoadIrradianceMap(App* app);