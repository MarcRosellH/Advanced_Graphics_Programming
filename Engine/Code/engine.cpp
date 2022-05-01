//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "buffer_management.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <iostream>

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

void Init(App* app)
{

    // Set up error callback
    if (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 3))
    {
        glDebugMessageCallback(OnGlError, app);
    }

    // Get and save current opengl version
    app->info.version = (char*)(glGetString(GL_VERSION));

    // Get and save current renderer
    app->info.renderer = (char*)(glGetString(GL_RENDERER));

    // Get and save current vendor
    app->info.vendor = (char*)(glGetString(GL_VENDOR));

    // Get and save glsl version
    app->info.glslVersion = (char*)(glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Get and save number of extensions and extension names
    glGetIntegerv(GL_NUM_EXTENSIONS, &app->info.numExtensions);
    app->info.extensions = new std::string[app->info.numExtensions];
    for (int i = 0; i < app->info.numExtensions; ++i)
    {
        app->info.extensions[i] = (char*)glGetStringi(GL_EXTENSIONS, GLuint(i));
    }

    // Geometry
    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(app->vertices), app->vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ARRAY_BUFFER, sizeof(app->indices), app->indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Attribute state
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    /*
    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");
    */

    // Create uniform buffers
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    app->uniformBuffer = CreateConstantBuffer(app->maxUniformBufferSize);

    // Load models
    app->patrickModelIdx = LoadModel(app, "Patrick/Patrick.obj");
    //app->roomModelIdx = LoadModel(app, "Room/Room #1.obj");

    // Entities initalization
    app->entities.push_back(Entity{ vec3(-3,0,0), vec3(90,0,0), vec3(0.5,0.5,0.5), app->patrickModelIdx });
    app->entities.push_back(Entity{ vec3(0,0,0), vec3(0,0,0), vec3(1,1,1), app->patrickModelIdx });

    // Lights initialization
    app->lights.push_back(Light{ LIGHTTYPE_DIRECTIONAL, vec3(1,0,0), vec3(1,1,1), vec3(0,0,0), 0.0F, 1.0F });
    app->lights.push_back(Light{ LIGHTTYPE_POINT, vec3(1,0,0), vec3(1,1,1), vec3(5,0,0), 3.0F, 1.0F });
    
    // Camera initialization
    app->cam.position = vec3(0, 0, 6);
    app->cam.projection = glm::perspective(glm::radians(60.F), (float)app->displaySize.x / (float)app->displaySize.y, 0.1F, 1000.F);
    app->cam.view = glm::lookAt(app->cam.position, vec3(0.F, 0.F, 0.F), vec3(0.F, 1.F, 0.F));

    // Shader loading and attribute management
    app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
    GLint attributeCount;
    glGetProgramiv(texturedMeshProgram.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);
    for (GLint i = 0; i < attributeCount; ++i)
    {
        GLchar attrName[32];
        GLsizei attrLen;
        GLint attrSize;
        GLenum attrType;

        glGetActiveAttrib(texturedMeshProgram.handle, i,
            ARRAY_COUNT(attrName),
            &attrLen,
            &attrSize,
            &attrType,
            attrName);

        GLint attrLocation = glGetAttribLocation(texturedMeshProgram.handle, attrName);

        texturedMeshProgram.vertexInputLayout.attributes.push_back({ (u8)attrLocation, GetAttribComponentCount(attrType) });
    }
    
    app->texturedMeshProgram_uTexture = glGetUniformLocation(texturedMeshProgram.handle, "uTexture");

    app->mode = Mode_TexturedMesh;
}

void Gui(App* app)
{
    ImGui::Begin("Menu");
    ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
    if (ImGui::CollapsingHeader("Lights"))
    {
        std::string name;
        for (u32 i = 0; i < app->lights.size(); ++i)
        {
            Light& l = app->lights[i];
            name = (l.type == LIGHTTYPE_DIRECTIONAL) ? ("Directional Light " + std::to_string(i)):
                                                        ("Point Light " + std::to_string(i));
            if (ImGui::TreeNode(name.c_str()))
            {
                // Position edit
                ImGui::DragFloat3("Position", (float*)&l.position, 0.01F);
                ImGui::Spacing();

                // Direction edit
                ImGui::DragFloat3("Direction", (float*)&l.direction, 0.01F);
                ImGui::Spacing();

                // Color edit
                float c[3] = { l.color.r, l.color.g, l.color.b };
                ImGui::ColorEdit3("Color", c);
                l.color.r = c[0];
                l.color.g = c[1];
                l.color.b = c[2];
                ImGui::Spacing();

                // Intensity edit
                ImGui::DragFloat("Intensity", &l.intensity, 0.01F, 0.0F, 0.0F, "%.04f");
                l.intensity = (l.intensity < 0.0F) ? 0.0F : l.intensity;
                ImGui::Spacing();

                // Radius edit
                if (l.type == LIGHTTYPE_POINT)
                {
                    ImGui::DragFloat("Radius", &l.radius, 0.01F, 0.0F, 0.0F, "%.04f");
                    l.radius = (l.radius < 0.0F) ? 0.0F : l.radius;
                    ImGui::Spacing();
                }

                ImGui::TreePop();
            }
        }
    }
    if (ImGui::CollapsingHeader("Info"))
    {
        ImGui::Text("OpenGL version: %s", app->info.version.c_str());
        ImGui::Text("OpenGL renderer: %s", app->info.renderer.c_str());
        ImGui::Text("OpenGL vendor: %s", app->info.vendor.c_str());
        ImGui::Text("OpenGL GLSL version: %s", app->info.glslVersion.c_str());
        ImGui::Text("OpenGL %d extensions:", app->info.numExtensions);
        for (int i = 0; i < app->info.numExtensions; ++i)
        {
            ImGui::Text("\t%s", app->info.extensions[i].c_str());
        }
    }
    ImGui::End();
}

void Update(App* app)
{
    // TODO: Input

    // Shader hot-reload
    for (u64 i = 0; i < app->programs.size(); ++i)
    {
        Program& program = app->programs[i];
        u64 currentTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());
        if (currentTimestamp > program.lastWriteTimestamp)
        {
            glDeleteProgram(program.handle);
            String programSource = ReadTextFile(program.filepath.c_str());
            const char* programName = program.programName.c_str();
            program.handle = CreateProgramFromSource(programSource, programName);
            program.lastWriteTimestamp = currentTimestamp;
        }
    }

    // Push buffer parameters
    BindBuffer(app->uniformBuffer);
    app->uniformBuffer.data = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    app->uniformBuffer.head = 0;

    // Global parameters
    app->globalParamsOffset = app->uniformBuffer.head;

    PushVec3(app->uniformBuffer, app->cam.position);
    PushUInt(app->uniformBuffer, app->lights.size());

    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->uniformBuffer, sizeof(vec4));

        Light& light = app->lights[i];
        PushUInt(app->uniformBuffer, light.type);
        PushVec3(app->uniformBuffer, light.color);
        PushVec3(app->uniformBuffer, light.direction);
        PushVec3(app->uniformBuffer, light.position);
    }

    app->globalParamsSize = app->uniformBuffer.head - app->globalParamsOffset;

    // Local parameters
    for (u32 i = 0; i < app->entities.size(); ++i)
    {
        AlignHead(app->uniformBuffer, app->uniformBlockAlignment);

        Entity& ref = app->entities[i];

        glm::mat4 world = MatrixFromPositionRotationScale(ref.position, ref.rotation, ref.scale);
        glm::mat4 worldViewProjectionMatrix = app->cam.projection * app->cam.view * world;

        ref.localParamsOffset = app->uniformBuffer.head;

        PushMat4(app->uniformBuffer, world);
        PushMat4(app->uniformBuffer, worldViewProjectionMatrix);

        ref.localParamsSize = app->uniformBuffer.head - ref.localParamsOffset;
    }

    // Unmap buffer
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Render(App* app)
{
    glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, app->displaySize.x, app->displaySize.y);
    switch (app->mode)
    {
    case Mode_TexturedQuad:
    {
            Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
            glUseProgram(programTexturedGeometry.handle);
            glBindVertexArray(app->vao);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);
            GLuint textureHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);
            glUseProgram(0);
        }
        break;
    case Mode_TexturedMesh:
    {
        Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
        glUseProgram(texturedMeshProgram.handle);

        for (u32 it = 0; it < app->entities.size(); ++it)
        {
            Entity& ref = app->entities[it];
            Model& model = app->models[ref.modelIdx];
            Mesh& mesh = app->meshes[model.meshIdx];

            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->uniformBuffer.handle, ref.localParamsOffset, ref.localParamsSize);

            for (u32 i = 0; i < mesh.submeshes.size(); ++i)
            {
                GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                glBindVertexArray(vao);

                u32 submeshMaterialIdx = model.materialIdx[i];
                Material& submeshMaterial = app->materials[submeshMaterialIdx];

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx < UINT32_MAX ? submeshMaterial.albedoTextureIdx : app->whiteTexIdx].handle);
                glUniform1i(app->texturedMeshProgram_uTexture, 0);

                Submesh& submesh = mesh.submeshes[i];
                glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
            }
        }
    }
        break;
    default:break;
    }
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
    {
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;
    }

    GLuint vaoHandle = 0;

    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

    for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
    {
        bool attributeWasLinked = false;

        for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
        {
            if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
            {
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset+submesh.vertexOffset;
                const u32 stride = submesh.vertexBufferLayout.stride;

                glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
        }
    }

    glBindVertexArray(0);

    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    ELOG("OpenGL debug message: %s", message);

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:               ELOG(" - source: GL_DEBUG_SOURCE_API"); break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:     ELOG(" - source: GL_DEBUG_SOURCE_WINDOW_SYSTEM"); break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:   ELOG(" - source: GL_DEBUG_SOURCE_SHADER_COMPILER"); break; 
    case GL_DEBUG_SOURCE_THIRD_PARTY:       ELOG(" - source: GL_DEBUG_SOURCE_THIRD_PARTY"); break; 
    case GL_DEBUG_SOURCE_APPLICATION:       ELOG(" - source: GL_DEBUG_SOURCE_APPLICATION"); break; 
    case GL_DEBUG_SOURCE_OTHER:             ELOG(" - source: GL_DEBUG_SOURCE_OTHER"); break; 
    }

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               ELOG(" - type: GL_DEBUG_TYPE_ERROR"); break; 
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ELOG(" - type: GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"); break; 
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  ELOG(" - type: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"); break;
    case GL_DEBUG_TYPE_PORTABILITY:         ELOG(" - type: GL_DEBUG_TYPE_PORTABILITY"); break; 
    case GL_DEBUG_TYPE_PERFORMANCE:         ELOG(" - type: GL_DEBUG_TYPE_PERFORMANCE"); break;
    case GL_DEBUG_TYPE_MARKER:              ELOG(" - type: GL_DEBUG_TYPE_MARKER"); break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          ELOG(" - type: GL_DEBUG_TYPE_PUSH_GROUP"); break;
    case GL_DEBUG_TYPE_POP_GROUP:           ELOG(" - type: GL_DEBUG_TYPE_POP_GROUP"); break;
    case GL_DEBUG_TYPE_OTHER:               ELOG(" - type: GL_DEBUG_TYPE_OTHER"); break;
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:            ELOG(" - severity: GL_DEBUG_SEVERITY_HIGH"); break;
    case GL_DEBUG_SEVERITY_MEDIUM:          ELOG(" - severity: GL_DEBUG_SEVERITY_MEDIUM"); break;
    case GL_DEBUG_SEVERITY_LOW:             ELOG(" - severity: GL_DEBUG_SEVERITY_LOW"); break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:    ELOG(" - severity: GL_DEBUG_SEVERITY_NOTIFICATION"); break;
    }
}

glm::mat4 MatrixFromPositionRotationScale(const vec3& position, const vec3& rotation, const vec3& scale)
{
    glm::mat4 ret = glm::translate(position);
    glm::vec3 radianRotation = glm::radians(rotation);
    ret = glm::rotate(ret, radianRotation.x, glm::vec3(1.F, 0.F, 0.F));
    ret = glm::rotate(ret, radianRotation.y, glm::vec3(0.F, 1.F, 0.F));
    ret = glm::rotate(ret, radianRotation.z, glm::vec3(0.F, 0.F, 1.F));
    return glm::scale(ret, scale);
}

u8 GetAttribComponentCount(const GLenum& type)
{
    switch (type)
    {
    case GL_FLOAT: return 1; break; 
    case GL_FLOAT_VEC2: return 2; break; 
    case GL_FLOAT_VEC3: return 3; break; 
    case GL_FLOAT_VEC4: return 4; break;
    case GL_FLOAT_MAT2: return 4; break; 
    case GL_FLOAT_MAT3: return 9; break; 
    case GL_FLOAT_MAT4: return 16; break;
    case GL_FLOAT_MAT2x3: return 6; break; 
    case GL_FLOAT_MAT2x4: return 8; break;
    case GL_FLOAT_MAT3x2: return 6; break; 
    case GL_FLOAT_MAT3x4: return 12; break;
    case GL_FLOAT_MAT4x2: return 8; break; 
    case GL_FLOAT_MAT4x3: return 12; break;
    case GL_INT: return 1; break; 
    case GL_INT_VEC2: return 2; break; 
    case GL_INT_VEC3: return 3; break; 
    case GL_INT_VEC4: return 4; break;
    case GL_UNSIGNED_INT: return 1; break; 
    case GL_UNSIGNED_INT_VEC2: return 2; break; 
    case GL_UNSIGNED_INT_VEC3: return 3; break; 
    case GL_UNSIGNED_INT_VEC4: return 4; break;
    case GL_DOUBLE: return 1; break; 
    case GL_DOUBLE_VEC2: return 2; break; 
    case GL_DOUBLE_VEC3: return 3; break; 
    case GL_DOUBLE_VEC4: return 4; break;
    case GL_DOUBLE_MAT2: return 4; break; 
    case GL_DOUBLE_MAT3: return 9; break; 
    case GL_DOUBLE_MAT4: return 16;
    case GL_DOUBLE_MAT2x3: return 6; break; 
    case GL_DOUBLE_MAT2x4: return 8; break;
    case GL_DOUBLE_MAT3x2: return 6; break; 
    case GL_DOUBLE_MAT3x4: return 12; break;
    case GL_DOUBLE_MAT4x2: return 8; break; 
    case GL_DOUBLE_MAT4x3: return 12; break;
    default: return 0; break;
    }

    // default should return always 0 if no defined type is sent in
    // but let's be sure
    return 0;
}