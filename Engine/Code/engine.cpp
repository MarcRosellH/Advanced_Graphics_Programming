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

Image LoadSkyboxPixels(App* app, const char* filepath)
{
    Image image = LoadImage(filepath);

    return image;
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
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

    /*// Geometry
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
    glBindVertexArray(0);*/

    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

    //Image pixels[6];
    //pixels[0] = LoadSkyboxPixels(app, "Skybox/skyboxX+.png");
    //pixels[1] = LoadSkyboxPixels(app, "Skybox/skyboxX-.png");
    //pixels[2] = LoadSkyboxPixels(app, "Skybox/skyboxY+.png");
    //pixels[3] = LoadSkyboxPixels(app, "Skybox/skyboxY-.png");
    //pixels[4] = LoadSkyboxPixels(app, "Skybox/skyboxZ+.png");
    //pixels[5] = LoadSkyboxPixels(app, "Skybox/skyboxZ-.png");
   
    //glGenTextures(1, &app->cubeMapId);
    //glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubeMapId);

    //for (unsigned int i = 0; i < 6; ++i)
    //{
    //    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
    //        pixels[i].size.x, pixels[i].size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels[i].pixels);

    //    //FreeImage(pixels[i]);

    //}
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   // CreateCubeMap(app, pixels);
    std::vector<std::string> faces = 
    {
        "right.jpg",
        "left.jpg",
        "top.jpg",
        "bottom.jpg",
        "front.jpg",
        "back.jpg"
    };
    app->cubeMapId = loadCubemap(faces);
    // Create uniform buffers
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    app->uniformBuffer = CreateConstantBuffer(app->maxUniformBufferSize);

    // Load models
    app->patrickModelIdx = LoadModel(app, "Patrick/Patrick.obj");
    app->roomModelIdx = LoadModel(app, "Lake/Erlaufsee.obj");

    // Entities initalization
    /*app->entities.push_back(Entity{vec3(6,0,0), vec3(90,0,0), vec3(1,1,1), app->patrickModelIdx});
    app->entities.push_back(Entity{ vec3(-6,0,0), vec3(0,0,90), vec3(1,1,1), app->patrickModelIdx });
    app->entities.push_back(Entity{ vec3(0,0,0), vec3(0,0,0), vec3(1,1,1), app->patrickModelIdx });
    app->entities.push_back(Entity{ vec3(6,0,-6), vec3(45,0,90), vec3(1,1,1), app->patrickModelIdx });
    app->entities.push_back(Entity{ vec3(0,0,-6), vec3(0,90,0), vec3(1,1,1), app->patrickModelIdx });
    app->entities.push_back(Entity{ vec3(-6,0,-6), vec3(45,45,45), vec3(1,1,1), app->patrickModelIdx });
    app->entities.push_back(Entity{ vec3(0,0,-50), vec3(0,0,0), vec3(10,10,10), app->patrickModelIdx });*/
    app->entities.push_back(Entity{ vec3(0,0,0), vec3(0,0,0), vec3(1,1,1), app->roomModelIdx });

    // Lights initialization
    app->lights.push_back(Light{ LIGHTTYPE_DIRECTIONAL, vec3(1,1,1), vec3(0,0,0), vec3(1,-1,1), 100.0F, 10.0F });
    //app->lights.push_back(Light{ LIGHTTYPE_DIRECTIONAL, vec3(0,0,1), vec3(0,0,0), vec3(0,0,1), 100.0F, 100.0F });
    //app->lights.push_back(Light{ LIGHTTYPE_POINT, vec3(1,1,1), vec3(0,1,1), vec3(0,0,0), 5.0F, 1.0F });
    //app->lights.push_back(Light{ LIGHTTYPE_POINT, vec3(1,1,0), vec3(6,1,1), vec3(0,0,0), 5.0F, 1.0F });
    //app->lights.push_back(Light{ LIGHTTYPE_POINT, vec3(1,0,1), vec3(-6,1,1), vec3(0,0,0), 5.0F, 1.0F });
    //app->lights.push_back(Light{ LIGHTTYPE_POINT, vec3(1,0,0), vec3(-6,1,-5), vec3(0,0,0), 5.0F, 1.0F });
    //app->lights.push_back(Light{ LIGHTTYPE_POINT, vec3(0,1,1), vec3(0,0,-5), vec3(0,0,0), 5.0F, 1.0F });
    //app->lights.push_back(Light{ LIGHTTYPE_POINT, vec3(0,1,0), vec3(6,0,-5), vec3(0,0,0), 5.0F, 1.0F });
    //app->lights.push_back(Light{ LIGHTTYPE_POINT, vec3(1,0,0), vec3(0,0,-42), vec3(0,0,0), 30.0F, 1.0F });
    
    // Camera initialization
    SetCamera(app->cam);

    LoadSphere(app);

    app->skyBox = LoadProgram(app, "Skybox.glsl", "SKYBOX");

    // Shader loading and attribute management 
    // [Forward Render]
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
    app->texturedMeshProgram_uColor = glGetUniformLocation(texturedMeshProgram.handle, "uColor");

    // [Deferred Render] Geometry Pass Program
    app->deferredGeometryPassProgramIdx = LoadProgram(app, "shaders.glsl", "DEFERRED_GEOMETRY_PASS");

    Program& deferredGeoPassProgram = app->programs[app->deferredGeometryPassProgramIdx];
    GLint deferredGeoAttributeCount;
    glGetProgramiv(deferredGeoPassProgram.handle, GL_ACTIVE_ATTRIBUTES, &deferredGeoAttributeCount);

    for (GLint i = 0; i < deferredGeoAttributeCount; ++i)
    {
        GLchar attrName[32];
        GLsizei attrLen;
        GLint attrSize;
        GLenum attrType;

        glGetActiveAttrib(deferredGeoPassProgram.handle, i,
            ARRAY_COUNT(attrName),
            &attrLen,
            &attrSize,
            &attrType,
            attrName);

        GLint attrLocation = glGetAttribLocation(deferredGeoPassProgram.handle, attrName);

        deferredGeoPassProgram.vertexInputLayout.attributes.push_back({ (u8)attrLocation, GetAttribComponentCount(attrType) });
    }

    app->deferredGeometryProgram_uTexture = glGetUniformLocation(deferredGeoPassProgram.handle, "uTexture");
    app->deferredGeometryProgram_uColor = glGetUniformLocation(deferredGeoPassProgram.handle, "uColor");

    // [Deferred Render] Lighting Pass Program
    app->deferredLightingPassProgramIdx = LoadProgram(app, "shaders.glsl", "DEFERRED_LIGHTING_PASS");

    Program& deferredLightingPassProgram = app->programs[app->deferredLightingPassProgramIdx];
    GLint deferredLightAttributeCount;
    glGetProgramiv(deferredLightingPassProgram.handle, GL_ACTIVE_ATTRIBUTES, &deferredLightAttributeCount);

    for (GLint i = 0; i < deferredLightAttributeCount; ++i)
    {
        GLchar attrName[32];
        GLsizei attrLen;
        GLint attrSize;
        GLenum attrType;

        glGetActiveAttrib(deferredLightingPassProgram.handle, i,
            ARRAY_COUNT(attrName),
            &attrLen,
            &attrSize,
            &attrType,
            attrName);

        GLint attrLocation = glGetAttribLocation(deferredLightingPassProgram.handle, attrName);

        deferredLightingPassProgram.vertexInputLayout.attributes.push_back({ (u8)attrLocation, GetAttribComponentCount(attrType) });
    }

    app->deferredLightingProgram_uGPosition = glGetUniformLocation(deferredLightingPassProgram.handle, "uGPosition");
    app->deferredLightingProgram_uGNormals = glGetUniformLocation(deferredLightingPassProgram.handle, "uGNormals");
    app->deferredLightingProgram_uGDiffuse = glGetUniformLocation(deferredLightingPassProgram.handle, "uGDiffuse");

    // [Framebuffers]

    // FORWARD BUFFERS
    // [Framebuffer] Forward Buffer
    // [Texture] Depth
    glGenTextures(1, &app->forwardDepthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->forwardDepthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // [Texture] Render
    glGenTextures(1, &app->forwardRenderAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->forwardRenderAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->forwardFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->forwardFrameBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->forwardRenderAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->forwardDepthAttachmentHandle, 0);

    GLenum drawForwardBuffer[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(ARRAY_COUNT(drawForwardBuffer), drawForwardBuffer);

    GLenum forwardFrameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (forwardFrameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (forwardFrameBufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                          ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:              ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:      ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                        ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:           ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;

        default: ELOG("Unknown framebuffer status error"); break;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // DEFERRED BUFFERS
    // [Framebuffer] GBuffer
    // [Texture] Positions
    glGenTextures(1, &app->positionAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // [Texture] Normals
    glGenTextures(1, &app->normalsAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->normalsAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // [Texture] Diffuse
    glGenTextures(1, &app->diffuseAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->diffuseAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // [Texture] Depth
    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->positionAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->normalsAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->diffuseAttachmentHandle, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachmentHandle, 0);

    GLenum drawBuffersGBuffer[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(ARRAY_COUNT(drawBuffersGBuffer), drawBuffersGBuffer);

    GLenum frameBufferStatusGBuffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (frameBufferStatusGBuffer != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (frameBufferStatusGBuffer)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                          ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:              ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:      ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                        ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:           ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;

        default: ELOG("Unknown framebuffer status error"); break;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // [Framebuffer] FBuffer
    glGenTextures(1, &app->finalRenderAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->finalRenderAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &app->fBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->finalRenderAttachmentHandle, 0);

    GLenum drawBuffersFBuffer[] = { GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(ARRAY_COUNT(drawBuffersFBuffer), drawBuffersFBuffer);

    GLenum frameBufferStatusFBuffer = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (frameBufferStatusFBuffer != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (frameBufferStatusFBuffer)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                          ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:              ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:      ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                        ELOG("Framebuffer status error: GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:             ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:           ELOG("Framebuffer status error: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;

        default: ELOG("Unknown framebuffer status error"); break;
        }
    }

    // [Texture] Reflection color
    glGenTextures(1, &app->waterReflectionAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->waterReflectionAttachmentHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // [Texture] Reflection depth
    glGenTextures(1, &app->waterReflectionDepthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->waterReflectionDepthAttachmentHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    app->currentFBOAttachmentType = FBOAttachmentType::FINAL;
    app->mode = Mode_Deferred;

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

}

void Gui(App* app)
{
    static bool p_open = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &p_open, window_flags);
    ImGui::PopStyleVar();

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::Begin("Menu");
    ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
    if (ImGui::CollapsingHeader("Entities"))
    {
        std::string name;
        for (u32 i = 0; i < app->entities.size(); ++i)
        {
            Entity& e = app->entities[i];
            name = ("Entity " + std::to_string(i));
            if (ImGui::TreeNode(name.c_str()))
            {
                // Position edit
                ImGui::DragFloat3("Position", (float*)&e.position, 0.01F);
                ImGui::Spacing();

                // Rotation edit
                ImGui::DragFloat3("Rotation", (float*)&e.rotation, 0.01F);
                ImGui::Spacing();

                // Scale edit
                ImGui::DragFloat3("Scale", (float*)&e.scale, 0.01F);
                ImGui::Spacing();

                ImGui::TreePop();
            }
        }
    }
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
    if (ImGui::CollapsingHeader("Render"))
    {
        const char* items[] = { "Forward", "Deferred"};
        static const char* curr = items[1];
        if (ImGui::BeginCombo("##combo", curr))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                bool is_selected = (curr == items[n]);
                if (ImGui::Selectable(items[n], is_selected))
                    curr = items[n];
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }

                if (strcmp(curr, items[0]) == 0)
                    app->mode = Mode::Mode_TexturedMesh;
                if (strcmp(curr, items[1]) == 0)
                    app->mode = Mode::Mode_Deferred;
            }
            ImGui::EndCombo();
        }
        const char* items2[] = { "Position", "Normals", "Diffuse", "Depth", "Final" };
        static const char* curr2 = items2[4];
        if (curr == "Deferred" && ImGui::BeginCombo("##combo2", curr2))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items2); n++)
            {
                bool is_selected = (curr2 == items2[n]);
                if (ImGui::Selectable(items2[n], is_selected))
                    curr2 = items2[n];
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }

                if (strcmp(curr2, items2[0]) == 0)
                    app->currentFBOAttachmentType = FBOAttachmentType::POSITION;
                if (strcmp(curr2, items2[1]) == 0)
                    app->currentFBOAttachmentType = FBOAttachmentType::NORMALS;
                if (strcmp(curr2, items2[2]) == 0)
                    app->currentFBOAttachmentType = FBOAttachmentType::DIFFUSE;
                if (strcmp(curr2, items2[3]) == 0)
                    app->currentFBOAttachmentType = FBOAttachmentType::DEPTH;
                if (strcmp(curr2, items2[4]) == 0)
                    app->currentFBOAttachmentType = FBOAttachmentType::FINAL;
            }
            ImGui::EndCombo();
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

    ImGui::End(); // End menu

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });

    ImGui::Begin("Scene");
    ImVec2 size = ImGui::GetContentRegionAvail();
    GLuint currentAttachment = 0;
    switch (app->mode)
    {
    case Mode::Mode_TexturedMesh:
        currentAttachment = app->forwardRenderAttachmentHandle;
        break;
    case Mode::Mode_Deferred:
        switch (app->currentFBOAttachmentType)
        {
        case FBOAttachmentType::POSITION:
        {
            currentAttachment = app->positionAttachmentHandle;
        }
        break;

        case FBOAttachmentType::NORMALS:
        {
            currentAttachment = app->normalsAttachmentHandle;
        }
        break;

        case FBOAttachmentType::DIFFUSE:
        {
            currentAttachment = app->diffuseAttachmentHandle;
        }
        break;

        case FBOAttachmentType::DEPTH:
        {
            currentAttachment = app->depthAttachmentHandle;
        }
        break;

        case FBOAttachmentType::FINAL:
        {
            currentAttachment = app->finalRenderAttachmentHandle;
        }
        break;

        default:
        {} break;
        }
        break;
    default:
    {} break;
    }

    ImGui::Image((ImTextureID)currentAttachment, size, { 0, 1 }, { 1, 0 });
    app->isFocused = ImGui::IsWindowFocused();
    ImGui::End(); // End scene

    ImGui::PopStyleVar();

    ImGui::End(); // End dockspace
}

void Update(App* app)
{

    HandleInput(app);

    app->projectionMat = GetProjectionMatrix(app->cam);
    app->viewMat = GetViewMatrix(app->cam);

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
        PushFloat(app->uniformBuffer, light.intensity);
        PushVec3(app->uniformBuffer, light.position);
        PushFloat(app->uniformBuffer, light.radius);
    }

    app->globalParamsSize = app->uniformBuffer.head - app->globalParamsOffset;

    // Local parameters
    for (u32 i = 0; i < app->entities.size(); ++i)
    {
        AlignHead(app->uniformBuffer, app->uniformBlockAlignment);

        Entity& ref = app->entities[i];

        glm::mat4 world = MatrixFromPositionRotationScale(ref.position, ref.rotation, ref.scale);
        glm::mat4 worldViewProjectionMatrix = app->projectionMat * app->viewMat * world;

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

    switch (app->mode)
    {
        case Mode_TexturedQuad:
            {
                glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glViewport(0, 0, app->displaySize.x, app->displaySize.y);

                glEnable(GL_DEPTH_TEST);

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
            glBindFramebuffer(GL_FRAMEBUFFER, app->forwardFrameBuffer);

                glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glViewport(0, 0, app->displaySize.x, app->displaySize.y);

                glEnable(GL_DEPTH_TEST);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
                glUseProgram(texturedMeshProgram.handle);

                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

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
                        bool hasTex = submeshMaterial.albedoTextureIdx < UINT32_MAX && submeshMaterial.albedoTextureIdx != 0 ? true : false;

                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, app->textures[hasTex ? submeshMaterial.albedoTextureIdx : app->whiteTexIdx].handle);
                        glUniform1i(app->texturedMeshProgram_uTexture, 0);
                        glUniform3f(app->texturedMeshProgram_uColor, (hasTex) ? 1.0F : submeshMaterial.albedo.r, (hasTex) ? 1.0F : submeshMaterial.albedo.g, (hasTex) ? 1.0F : submeshMaterial.albedo.b);

                        Submesh& submesh = mesh.submeshes[i];
                        glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                    }
                }
                RenderQuad(app);
                glUseProgram(0);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            break;
        case Mode_Deferred:
            {
            glClearColor(0.f, 0.f, 0.f, 1.0f);

            /* First pass (geometry) */

            glBindFramebuffer(GL_FRAMEBUFFER, app->gBuffer);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            GLenum drawBuffersGBuffer[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
            glDrawBuffers(ARRAY_COUNT(drawBuffersGBuffer), drawBuffersGBuffer);

            glViewport(0, 0, app->displaySize.x, app->displaySize.y);

            glEnable(GL_DEPTH_TEST);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glDepthMask(GL_TRUE);

            Program& deferredGeometryPassProgram = app->programs[app->deferredGeometryPassProgramIdx];
            glUseProgram(deferredGeometryPassProgram.handle);

            for (const Entity& entity : app->entities)
            {
                Model& model = app->models[entity.modelIdx];
                Mesh& mesh = app->meshes[model.meshIdx];

                glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->uniformBuffer.handle, entity.localParamsOffset, entity.localParamsSize);

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVAO(mesh, i, deferredGeometryPassProgram);
                    glBindVertexArray(vao);

                    u32 submesh_material_index = model.materialIdx[i];
                    Material& submesh_material = app->materials[submesh_material_index];
                    bool hasTex = submesh_material.albedoTextureIdx < UINT32_MAX && submesh_material.albedoTextureIdx != 0 ? true : false;

                    glActiveTexture(GL_TEXTURE0);

                    glBindTexture(GL_TEXTURE_2D, app->textures[(hasTex) ? submesh_material.albedoTextureIdx : app->whiteTexIdx].handle);
                    glUniform1i(app->deferredGeometryProgram_uTexture, 0);
                    glUniform3f(app->deferredGeometryProgram_uColor, (hasTex) ? 1.0F : submesh_material.albedo.r, (hasTex) ? 1.0F : submesh_material.albedo.g, (hasTex) ? 1.0F : submesh_material.albedo.b);

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);

                    glBindVertexArray(0);
                }
            }
            glUseProgram(0);

            glDepthMask(GL_FALSE);

            Program& skyBoxProgram = app->programs[app->skyBox];
            glUseProgram(skyBoxProgram.handle);
            RenderSkybox(app);

            glUseProgram(0);
            glDepthMask(GL_TRUE);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            /* Second pass (lighting) */

            glBindFramebuffer(GL_FRAMEBUFFER, app->fBuffer);

            glClearColor(0.f, 0.f, 0.f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            GLenum drawBuffersFBuffer[] = { GL_COLOR_ATTACHMENT3 };
            glDrawBuffers(ARRAY_COUNT(drawBuffersFBuffer), drawBuffersFBuffer);

            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);

            //glDepthMask(GL_FALSE);

            Program& deferredLightingPassProgram = app->programs[app->deferredLightingPassProgramIdx];
            glUseProgram(deferredLightingPassProgram.handle);

            glUniform1i(app->deferredLightingProgram_uGPosition, 1);
            glUniform1i(app->deferredLightingProgram_uGNormals, 2);
            glUniform1i(app->deferredLightingProgram_uGDiffuse, 3);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, app->normalsAttachmentHandle);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, app->diffuseAttachmentHandle);

            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->uniformBuffer.handle, app->globalParamsOffset, app->globalParamsSize);

            RenderQuad(app);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, app->gBuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, app->fBuffer);

            glBlitFramebuffer(0, 0, app->displaySize.x, app->displaySize.x, 0, 0, app->displaySize.x, app->displaySize.x, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

            glUseProgram(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            break;
        default:
            break;
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

void RenderQuad(App* app)
{
    if (app->quadVAO == 0)
    {
        float vertices[] = {
        -1.0F, 1.0F, 0.0F, 0.0F, 1.0F,
        -1.0F, -1.0F, 0.0F, 0.0F, 0.0F,
        1.0F, 1.0F, 0.0F, 1.0F, 1.0F,
        1.0F, -1.0F, 0.0F, 1.0F, 0.0F,
        };

        glGenVertexArrays(1, &app->quadVAO);
        glGenBuffers(1, &app->quadVBO);

        glBindVertexArray(app->quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, app->quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    glBindVertexArray(app->quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);


}

void SetCamera(Camera& cam)
{
    cam.position = vec3(0.F, 10.F, 10.F);
    cam.front = vec3(0.F, 0.F, -1.F);
    cam.up = vec3(0.F, 1.F, 0.F);
    cam.worldUp = cam.up;

    cam.yaw = -90.F;
    cam.pitch = 0.F;

    cam.speed = 0.25F;
}

void SetAspectRatio(Camera& cam, float dsX, float dsY)
{
    cam.aspectRatio = dsX / dsY;
}

glm::mat4 GetViewMatrix(Camera& cam)
{
    return glm::lookAt(cam.position, cam.position + cam.front, cam.up);
}

glm::mat4 GetProjectionMatrix(Camera& cam)
{
    return glm::perspective(glm::radians(cam.fov), cam.aspectRatio, cam.nearPlane, cam.farPlane);
}

void HandleInput(App* app)
{
    if (app->input.keys[K_W] == BUTTON_PRESSED)app->cam.position += (app->cam.front * app->cam.speed);      // Forward
    if(app->input.keys[K_A] == BUTTON_PRESSED)app->cam.position -= (app->cam.right * app->cam.speed);       // Left
    if(app->input.keys[K_S] == BUTTON_PRESSED)app->cam.position -= (app->cam.front * app->cam.speed);       // Backward
    if(app->input.keys[K_D] == BUTTON_PRESSED)app->cam.position += (app->cam.right * app->cam.speed);       // Right
    if(app->input.keys[K_X] == BUTTON_PRESSED)app->cam.position += (app->cam.up * app->cam.speed);          // Up
    if(app->input.keys[K_C] == BUTTON_PRESSED)app->cam.position -= (app->cam.up * app->cam.speed);          // Down

    if (app->input.mouseButtons[LEFT] == BUTTON_PRESSED)
    {
        float xOS = app->input.mouseDelta.x * 0.15F; // 0.1F is the sensitivity
        float yOS = app->input.mouseDelta.y * 0.15F;

        if (app->isFocused)
        {
            app->cam.yaw += xOS;
            app->cam.pitch += yOS;

            app->cam.pitch = (app->cam.pitch > 89.F) ? 89.F : app->cam.pitch;
            app->cam.pitch = (app->cam.pitch < -89.F) ? -89.F : app->cam.pitch;

            glm::vec3 newFront = vec3(
                cos(glm::radians(app->cam.yaw)) * cos(glm::radians(app->cam.pitch)),
                sin(glm::radians(app->cam.pitch)),
                sin(glm::radians(app->cam.yaw)) * cos(glm::radians(app->cam.pitch)));

            app->cam.front = glm::normalize(newFront);
            app->cam.right = glm::normalize(glm::cross(app->cam.front, app->cam.worldUp));
            app->cam.up = glm::normalize(glm::cross(app->cam.right, app->cam.front));
        }
    }
    SetAspectRatio(app->cam, (float)app->displaySize.x, (float)app->displaySize.y);
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

void LoadSphere(App* app)
{
    glGenVertexArrays(1, &app->sphereVAO);

    unsigned int vbo, ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
    {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            positions.push_back(glm::vec3(xPos, yPos, zPos));
            uv.push_back(glm::vec2(xSegment, ySegment));
            normals.push_back(glm::vec3(xPos, yPos, zPos));
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
    {
        if (!oddRow)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                indices.push_back(y * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else
        {
            for (int x = X_SEGMENTS; x >= 0; --x)
            {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y * (X_SEGMENTS + 1) + x);
            }
        }

        oddRow = !oddRow;
    }

    app->sphereIdxCount = indices.size();

    std::vector<float> data;
    for (std::size_t i = 0; i < positions.size(); ++i)
    {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);

        if (uv.size() > 0)
        {
            data.push_back(uv[i].x);
            data.push_back(uv[i].y);
        }

        if (normals.size() > 0)
        {
            data.push_back(normals[i].x);
            data.push_back(normals[i].y);
            data.push_back(normals[i].z);
        }
    }

    glBindVertexArray(app->sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    float stride = (3 + 2 + 3) * sizeof(float);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
}

void RenderSphere(App* app)
{
    glBindVertexArray(app->sphereVAO);

    glDrawElements(GL_TRIANGLE_STRIP, app->sphereIdxCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

void RenderSkybox(App* app)
{
    if (app->SKyboxVAO == 0)
    {
        float skyboxVertices[] = {
            // positions          
            -1000.0f,  1000.0f, -1000.0f,
            -1000.0f, -1000.0f, -1000.0f,
             1000.0f, -1000.0f, -1000.0f,
             1000.0f, -1000.0f, -1000.0f,
             1000.0f,  1000.0f, -1000.0f,
            -1000.0f,  1000.0f, -1000.0f,
        
            -1000.0f, -1000.0f,  1000.0f,
            -1000.0f, -1000.0f, -1000.0f,
            -1000.0f,  1000.0f, -1000.0f,
            -1000.0f,  1000.0f, -1000.0f,
            -1000.0f,  1000.0f,  1000.0f,
            -1000.0f, -1000.0f,  1000.0f,

             1000.0f, -1000.0f, -1000.0f,
             1000.0f, -1000.0f,  1000.0f,
             1000.0f,  1000.0f,  1000.0f,
             1000.0f,  1000.0f,  1000.0f,
             1000.0f,  1000.0f, -1000.0f,
             1000.0f, -1000.0f, -1000.0f,
              
            -1000.0f, -1000.0f,  1000.0f,
            -1000.0f,  1000.0f,  1000.0f,
             1000.0f,  1000.0f,  1000.0f,
             1000.0f,  1000.0f,  1000.0f,
             1000.0f, -1000.0f,  1000.0f,
            -1000.0f, -1000.0f,  1000.0f,
               
            -1000.0f,  1000.0f, -1000.0f,
             1000.0f,  1000.0f, -1000.0f,
             1000.0f,  1000.0f,  1000.0f,
             1000.0f,  1000.0f,  1000.0f,
            -1000.0f,  1000.0f,  1000.0f,
            -1000.0f,  1000.0f, -1000.0f,
                              
            -1000.0f, -1000.0f, -1000.0f,
            -1000.0f, -1000.0f,  1000.0f,
             1000.0f, -1000.0f, -1000.0f,
             1000.0f, -1000.0f, -1000.0f,
            -1000.0f, -1000.0f,  1000.0f,
             1000.0f, -1000.0f,  1000.0f
        };

                // skybox VAO
        glGenVertexArrays(1, &app->SKyboxVAO);
        glGenBuffers(1, &app->SkyboxVBO);
        glBindVertexArray(app->SKyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, app->SkyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        //Unbinding
        glBindVertexArray(0);
    }
    glDepthFunc(GL_LEQUAL);
    glBindVertexArray(app->SKyboxVAO);
    glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubeMapId);
    
    Program& skyBoxProgram = app->programs[app->skyBox];
    i32 projLoc = glGetUniformLocation(skyBoxProgram.handle, "projection");
    i32 viewLoc = glGetUniformLocation(skyBoxProgram.handle, "view");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &app->projectionMat[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &app->viewMat[0][0]);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);

    glDepthFunc(GL_LESS);


}

void CreateCubeMap(App* app, Image pixels[6])
{
    glGenTextures(1, &app->cubeMapId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, app->cubeMapId);

    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
            app->displaySize.x, app->displaySize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels[i].pixels);

        FreeImage(pixels[i]);

    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


//void HDRImage(const char* filename)
//{
//    //load
//    int w = 0;
//    int h = 0;
//    int comp = 0;
//    float* hdrData = nullptr;
//    if (stbi_is_hdr(filename))
//    {
//        stbi_set_flip_vertically_on_load(true);
//        hdrData = stbi_loadf(filename, &w, &h, &comp, 0);
//    }
//
//    //unload
//    if (hdrData != nullptr)
//    {
//        stbi_image_free(hdrData);
//        hdrData = nullptr;
//    }
//}

//void BakeCubemap(App* app)
//{
//    /* OpenGLState glState;
//     glState.faceCulling = false;
//     glState.apply();
//     
//     
//     */
//    Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
//    glUseProgram(programTexturedGeometry.handle);
//
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, )
//    
//
//}
