//
// engine.h: This file contains the types and functions relative to the engine.
//

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
    u64                lastWriteTimestamp; // What is this for?
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

struct Transform
{
    vec3 position;
    vec3 rotation;
    vec3 scale;
    bool updated = true;
};

struct Camera
{
    glm::mat4   projection;
    glm::mat4   view;
    vec3        position;
    bool        updated = false;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_TexturedMesh,
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
    std::string  glslVerstion;
    GLint        numExtensions;
    std::string* extensions;
};

struct Entity
{
    Transform   transform;
    glm::mat4   worldMatrix;
    u32         model;
    u32         localParamsOffset;
    u32         localParamsSize;
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    // Model resources
    std::vector<Texture>    textures;
    std::vector<Program>    programs;
    std::vector<Material>   materials;
    std::vector<Mesh>       meshes;
    std::vector<Model>      models;

    // Program indices
    u32 texturedGeometryProgramIdx;
    u32 texturedMeshProgramIdx;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Entities
    std::vector<Entity>     entities;

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

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

    // OpenGL context information
    OpenGLInfo info;

    // Camera
    Camera cam;

    // Uniform buffers data management
    GLint   maxUniformBufferSize;
    GLint   uniformBlockAlignment;
    GLuint  bufferHandle;
    GLint   bufferData;

    // TO delete
    const VertexV3V2 vertices[4] = {
        {vec3(-0.5,-0.5, 0.0),vec2(0.0,0.0)},
        {vec3(0.5,-0.5, 0.0),vec2(1.0,0.0)},
        {vec3(0.5, 0.5, 0.0),vec2(1.0,1.0)},
        {vec3(-0.5, 0.5, 0.0),vec2(0.0,1.0)}
    };

    const u16 indices[6] = { 0,1,2,0,2,3 };
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

u32 LoadTexture2D(App* app, const char* filepath);

u32 LoadModel(App* app, const char* filename);

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);

u8 GetAttribComponentCount(const GLenum& type);

glm::mat4 TransformPositionRotationScale(const Transform& t);

u32 Align(u32 value, u32 alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);