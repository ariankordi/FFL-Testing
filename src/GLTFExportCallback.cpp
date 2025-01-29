#ifndef NO_GLTF

#include "GLTFExportCallback.h"
#include "nn/ffl/FFLDrawParam.h"
#include <cstring>
#include <limits>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <misc/rio_MemUtil.h>

// Include stb_image_write implementation
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "for_gltf/stb_image_write.h"

// Defined in IMPLEMENTATION sector in tinygltf.h but needed here
namespace tinygltf {
    std::string base64_encode(unsigned char const*, unsigned int len);
    std::string base64_decode(std::string const& encoded_string);
}

GLTFExportCallback::GLTFExportCallback()
{
    mpCharModel = nullptr;
}

// Static function implementations
/*
void GLTFExportCallback::ApplyAlphaTestFunc(void* pObj, bool enable, rio::Graphics::CompareFunc func, float ref)
{
}

void GLTFExportCallback::SetMatrixFunc(void* pObj, const rio::BaseMtx44f* matrix)
{
}
*/

void GLTFExportCallback::DrawFunc(void* pObj, const FFLDrawParam* drawParam)
{
    GLTFExportCallback* self = static_cast<GLTFExportCallback*>(pObj);
    self->Draw(*drawParam);
}

FFLShaderCallback GLTFExportCallback::GetShaderCallback()
{
    FFLShaderCallback callback;
    // Zero out the structure.
    rio::MemUtil::set(&callback, 0, sizeof(FFLShaderCallback));

    callback.pObj = this;

    // Only called during FFLInitCharModelGPUStep.
    //callback.pApplyAlphaTestFunc = &GLTFExportCallback::ApplyAlphaTestFunc;
    //callback.pSetMatrixFunc = &GLTFExportCallback::SetMatrixFunc;

    callback.pDrawFunc = &GLTFExportCallback::DrawFunc;

    return callback;
}

// Static helper functions
void GLTFExportCallback::UnpackNormal_10_10_10_2(uint32_t packed, float& x, float& y, float& z)
{
    // Extract each 10-bit component, sign-extended
    int32_t nx = ((int32_t)(packed << 22)) >> 22;
    int32_t ny = ((int32_t)(packed << 12)) >> 22;
    int32_t nz = ((int32_t)(packed << 2))  >> 22;
    // int32_t nw = (packed >> 30) & 0x03; // The 2-bit w component, not used for normals

    // Convert to float in range [-1, 1]
    x = nx / 511.0f;
    y = ny / 511.0f;
    z = nz / 511.0f;
}

int GLTFExportCallback::MapPrimitiveMode(rio::Drawer::PrimitiveMode mode)
{
    switch (mode)
    {
    case rio::Drawer::POINTS:
        return TINYGLTF_MODE_POINTS;
    case rio::Drawer::LINES:
        return TINYGLTF_MODE_LINE;
    case rio::Drawer::LINE_STRIP:
        return TINYGLTF_MODE_LINE_STRIP;
    case rio::Drawer::TRIANGLES:
        return TINYGLTF_MODE_TRIANGLES;
    case rio::Drawer::TRIANGLE_STRIP:
        return TINYGLTF_MODE_TRIANGLE_STRIP;
    case rio::Drawer::TRIANGLE_FAN:
        return TINYGLTF_MODE_TRIANGLE_FAN;
    default:
        return TINYGLTF_MODE_TRIANGLES;
    }
}


bool GLTFExportCallback::ExtractTextureToRGBA(rio::Texture2D* texture, std::vector<unsigned char>& rgbaData, int* width, int* height)
{
#if !RIO_IS_WIN
    #pragma warning("GLTFExportCallback::ExtractTextureToRGBA does not work on non-Windows right now.")
    RIO_ASSERT("GLTFExportCallback::ExtractTextureToRGBA does not work on non-Windows right now.");
    return false;
#else
    if (!texture) return false;

    // Use texture width, height, and internal format
    *width = texture->getWidth();
    *height = texture->getHeight();
    rio::TextureFormat internalFormat = texture->getTextureFormat();

    // Determine format based on internal format
    int channels = rio::TextureFormatUtil::getPixelByteSize(internalFormat);
    GLenum format = texture->getNativeTexture().surface.nativeFormat.format;

    // Allocate buffer
    std::vector<unsigned char> imageData(*width * *height * channels);

    // Create and bind a framebuffer
    GLuint framebuffer;
    RIO_GL_CALL(glGenFramebuffers(1, &framebuffer));
    RIO_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer));

    // Attach the texture to the framebuffer
    RIO_GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->getNativeTextureHandle(), 0));

    // Check if the framebuffer is complete
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        RIO_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        RIO_GL_CALL(glDeleteFramebuffers(1, &framebuffer));
        return false;
    }

    // Read pixels from the framebuffer
    RIO_GL_CALL(glReadPixels(0, 0, *width, *height, format, GL_UNSIGNED_BYTE, imageData.data()));

    // Unbind and delete the framebuffer
    RIO_GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    RIO_GL_CALL(glDeleteFramebuffers(1, &framebuffer));

    // Unbind texture
    RIO_GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));

    // Convert to RGBA and apply modulate mode and base color (pColorR)
    rgbaData.resize(*width * *height * 4); // Always RGBA

    for (int i = 0; i < *width * *height; ++i)
    {
        unsigned char r = imageData[i * channels + 0];
        rgbaData[i * 4 + 0] = r;
        rgbaData[i * 4 + 1] = (channels > 1) ? imageData[i * channels + 1] : r;
        rgbaData[i * 4 + 2] = (channels > 2) ? imageData[i * channels + 2] : r;
        rgbaData[i * 4 + 3] = (channels > 3) ? imageData[i * channels + 3] : 255;
    }
    return true;
#endif
}

bool GLTFExportCallback::EncodeRGBADataToPNG(const std::vector<unsigned char>& rgbaData, int width, int height, std::vector<unsigned char>& pngData)
{
    const int channels = 4;
    // Flip the image vertically
    /*
    std::vector<unsigned char> flippedImage(width * height * channels);
    for (int y = 0; y < height; ++y)
    {
        memcpy(&flippedImage[y * width * channels],
               &rgbaData[(height - 1 - y) * width * channels],
               width * channels);
    }
    */

    int pngSize = 0;
    unsigned char* pngBuffer = stbi_write_png_to_mem(rgbaData.data(), width * channels, width, height, channels, &pngSize);
    if (!pngBuffer)
    {
        return false;
    }

    pngData.assign(pngBuffer, pngBuffer + pngSize);
    free(pngBuffer);

    return true;
}

// Function to convert sRGB value to linear for baseColorFactor
float GLTFExportCallback::SRGBToLinear(float color)
{
    if (color <= 0.04045f)
        return color / 12.92f;
    else
        return powf((color + 0.055f) / 1.055f, 2.4f);
}

// Helper function to convert FFLVec3 to tinygltf::Value array
tinygltf::Value GLTFExportCallback::FFLVec3ToGltfValue(const FFLVec3& vec)
{
    return tinygltf::Value(std::vector<tinygltf::Value>{
        tinygltf::Value(vec.x),
        tinygltf::Value(vec.y),
        tinygltf::Value(vec.z)
    });
}
/*
// Helper function to convert FFLColor to tinygltf::Value array
tinygltf::Value FFLColorToGltfValue(const FFLColor& color)
{
    return tinygltf::Value(std::vector<tinygltf::Value>{
        tinygltf::Value(color.r),
        tinygltf::Value(color.g),
        tinygltf::Value(color.b),
        tinygltf::Value(color.a)
    });
}
*/
void GLTFExportCallback::Draw(const FFLDrawParam& drawParam)
{
    if (drawParam.primitiveParam.pIndexBuffer != nullptr)
    {
        MeshData meshData;
        meshData.modulateMode = drawParam.modulateParam.mode;
        meshData.modulateType = drawParam.modulateParam.type;
        meshData.cullMode = drawParam.cullMode;
        meshData.texture = nullptr;
        meshData.colorR = {1.0f, 1.0f, 1.0f, 1.0f}; // Default RGBA
        if (drawParam.modulateParam.pColorR)
        {
            // pColorR is RGBA
            meshData.colorR = {
                drawParam.modulateParam.pColorR->r,
                drawParam.modulateParam.pColorR->g,
                drawParam.modulateParam.pColorR->b,
                drawParam.modulateParam.pColorR->a
            };
        }

        // Handle modulate modes
        switch (meshData.modulateMode)
        {
        case FFL_MODULATE_MODE_CONSTANT:
            break;
        case FFL_MODULATE_MODE_TEXTURE_DIRECT:

        // FFL_MODULATE_MODE_RGB_LAYERED (2) is ONLY
        // used when drawing mask, leaving it unused

        case FFL_MODULATE_MODE_ALPHA:
        case FFL_MODULATE_MODE_LUMINANCE_ALPHA:
        case FFL_MODULATE_MODE_ALPHA_OPA:
            meshData.texture = const_cast<rio::Texture2D*>(drawParam.modulateParam.pTexture2D);
            break;
        default:
            // Ignore other modes for now
            return;
        }

        // Collect indices
        uint32_t indexCount = drawParam.primitiveParam.indexCount;
        const uint16_t* indices = static_cast<const uint16_t*>(drawParam.primitiveParam.pIndexBuffer);
        meshData.indices.assign(indices, indices + indexCount);

        // When FFL set the cull mode to front... that means
        // that X is flipped so we have to reverse face winding
        // (glTF does not support front face culling)
        if (drawParam.cullMode == FFL_CULL_MODE_FRONT)
            for (uint32_t i = 0; i < indexCount; i += 3)
                // Swap the first and last indices of each triangle to reverse winding
                std::swap(meshData.indices[i], meshData.indices[i + 2]);

        // Get the maximum vertex index to know how many vertices we need
        uint16_t maxIndex = 0;
        for (uint32_t i = 0; i < indexCount; ++i)
            if (indices[i] > maxIndex)
                maxIndex = indices[i];
        uint32_t vertexCount = maxIndex + 1;

        // Initialize per-vertex arrays
        meshData.positions.resize(vertexCount * 3, 0.0f);
        meshData.normals.resize(vertexCount * 3, 0.0f);
        meshData.texcoords.resize(vertexCount * 2, 0.0f);
        meshData.tangents.resize(vertexCount * 4, 0.0f);
        meshData.colors.resize(vertexCount * 4, 0);

        // Now process attribute buffers
        for (int type = FFL_ATTRIBUTE_BUFFER_TYPE_POSITION; type <= FFL_ATTRIBUTE_BUFFER_TYPE_COLOR; ++type)
        {
            const FFLAttributeBuffer& buffer = drawParam.attributeBufferParam.attributeBuffers[type];
            void* ptr = buffer.ptr;

            if (ptr)
            {
                uint32_t stride = buffer.stride;
                uint32_t size = buffer.size;

                if (stride < 1 && type != FFL_ATTRIBUTE_BUFFER_TYPE_COLOR)
                {
                    RIO_LOG("Error: Stride is 0 and this is not the color attribute.\n");
                    continue;
                }
                else if (stride > 0)
                {
                    uint32_t numElements = size / stride;

                    if (numElements < vertexCount)
                    {
                        // Handle error
                        RIO_LOG("Error: Not enough elements in attribute buffer.\n");
                        continue;
                    }

                }

                uint8_t* dataPtr = static_cast<uint8_t*>(ptr);
                for (uint32_t i = 0; i < vertexCount; ++i)
                {
                    uint8_t* elemPtr = dataPtr + i * stride;

                    switch (type)
                    {
                    case FFL_ATTRIBUTE_BUFFER_TYPE_POSITION:
                    {
                        float* position = reinterpret_cast<float*>(elemPtr);
                        meshData.positions[i * 3 + 0] = position[0];
                        meshData.positions[i * 3 + 1] = position[1];
                        meshData.positions[i * 3 + 2] = position[2];
                        break;
                    }
                    case FFL_ATTRIBUTE_BUFFER_TYPE_TEXCOORD:
                    {
                        float* texcoord = reinterpret_cast<float*>(elemPtr);
                        meshData.texcoords[i * 2 + 0] = texcoord[0];
                        meshData.texcoords[i * 2 + 1] = texcoord[1];
                        break;
                    }
                    case FFL_ATTRIBUTE_BUFFER_TYPE_NORMAL:
                    {
                        uint32_t packedNormal = *reinterpret_cast<uint32_t*>(elemPtr);
                        float x, y, z;
                        UnpackNormal_10_10_10_2(packedNormal, x, y, z);
                        meshData.normals[i * 3 + 0] = x;
                        meshData.normals[i * 3 + 1] = y;
                        meshData.normals[i * 3 + 2] = z;
                        break;
                    }
                    case FFL_ATTRIBUTE_BUFFER_TYPE_TANGENT:
                    {
                        int8_t* tangent = reinterpret_cast<int8_t*>(elemPtr);
                        if (tangent[3] == 0)
                            continue;
                        float x = tangent[0] / 127.0f;
                        float y = tangent[1] / 127.0f;
                        float z = tangent[2] / 127.0f;
                        float w = tangent[3] / 127.0f; // Usually, w is 1.0 or -1.0
                        // has to be normalized ourselves
                        // otherwise we see: Only (u)byte and (u)short accessors can be normalized

                        // Calculate magnitude of the tangent vector (x, y, z)
                        float magnitude = sqrtf(x * x + y * y + z * z);

                        // Normalize if needed
                        if (magnitude != 0.0f)
                        {
                            x /= magnitude;
                            y /= magnitude;
                            z /= magnitude;
                        }

                        // Assign back to meshData
                        meshData.tangents[i * 4 + 0] = x;
                        meshData.tangents[i * 4 + 1] = y;
                        meshData.tangents[i * 4 + 2] = z;
                        meshData.tangents[i * 4 + 3] = w; // w is not part of the normalization
                        break;
                    }
                    case FFL_ATTRIBUTE_BUFFER_TYPE_COLOR:
                    {
                        // default if there is no color
                        if (stride < 1)
                        {
                            meshData.colors[i * 4 + 0] = 1;
                            meshData.colors[i * 4 + 1] = 1;
                            meshData.colors[i * 4 + 2] = 0;
                            // set A (rim width) to 1, which is
                            // default for the normal type otherwise
                            meshData.colors[i * 4 + 3] = 1;
                        }
                        // Store color as R8G8B8A8_UNORM
                        uint8_t* color = reinterpret_cast<uint8_t*>(elemPtr);
#ifdef TRY_COLOR_AS_FLOATS
                        meshData.colors[i * 4 + 0] = color[0] / 255.0f;
                        meshData.colors[i * 4 + 1] = color[1] / 255.0f;
                        meshData.colors[i * 4 + 2] = color[2] / 255.0f;
                        meshData.colors[i * 4 + 3] = color[3] / 255.0f;
#else
                        meshData.colors[i * 4 + 0] = color[0];
                        meshData.colors[i * 4 + 1] = color[1];
                        meshData.colors[i * 4 + 2] = color[2];
                        meshData.colors[i * 4 + 3] = color[3];
#endif
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
        }

        meshData.primitiveType = static_cast<rio::Drawer::PrimitiveMode>(drawParam.primitiveParam.primitiveType);

        // Store the meshData
        mMeshes.push_back(meshData);
    }
}

bool GLTFExportCallback::ExportModelInternal(const std::string& filename, std::ostream* outStream)
{
    tinygltf::Model model;

    // Set asset information
    model.asset.version = "2.0";
    model.asset.generator = "https://mii-unsecure.ariankordi.net (or if this site does not exist anymore: https://github.com/ariankordi)";

    // Initialize buffer and associated data structures
    tinygltf::Buffer buffer;
    std::vector<unsigned char> bufferData;
    size_t bufferSize = 0;

    // Prepare materials container
    std::vector<tinygltf::Material> materials;

    // Process each mesh and construct GLTF components
    for (size_t meshIndex = 0; meshIndex < mMeshes.size(); ++meshIndex)
    {
        MeshData& meshData = mMeshes[meshIndex];

        // Create a new GLTF mesh and primitive
        tinygltf::Mesh gltfMesh;
        tinygltf::Primitive primitive;

        // Process mesh attributes and add to primitive
        ProcessMeshAttributes(meshData, model, bufferData, bufferSize, primitive);

        // Set primitive mode based on the mesh's primitive type
        primitive.mode = MapPrimitiveMode(meshData.primitiveType);

        // Add modulate parameters to this primitive's extras for custom usage
        AddPrimitiveExtras(meshData, primitive);

        // Handle texture assignment and processing
        if (meshData.texture != nullptr)
        {
            ProcessTexture(meshData, model, bufferData, bufferSize);
            AssignMaterialToPrimitive(meshData, model, primitive, meshIndex);
        }
        else
        {
            // Assign material without texture
            AssignMaterialWithoutTexture(meshData, model, primitive, meshIndex);
        }

        // Material has been added to the model

        // Add the primitive to the mesh
        gltfMesh.primitives.push_back(primitive);

        // Add the mesh to the model
        model.meshes.push_back(gltfMesh);
        int meshIndexInModel = static_cast<int>(model.meshes.size() - 1);

        // Create a node for the mesh
        tinygltf::Node node;
        node.mesh = meshIndexInModel;
        int nodeIndex = static_cast<int>(model.nodes.size());
        model.nodes.push_back(node);

        // Add node to the default scene
        AddNodeToScene(model, nodeIndex);
    }

    // Handle additional mask textures if present
    HandleAdditionalMaskTextures(model, bufferData, bufferSize);

    // Assign buffer data to the model
    buffer.data = bufferData;
    model.buffers.push_back(buffer);

    // Include character model information in the GLTF extras if available
    IncludeCharacterModelInfo(model);

    // Write the model to a GLTF file or output stream
    return WriteModelToFileOrStream(model, filename, outStream);
}

//------------------------ Helper Functions for ExportModelInternal ------------------------

/**
 * @brief Processes mesh attributes such as positions, normals, texcoords, etc., and adds them to the GLTF primitive.
 *
 * @param meshData The mesh data to process.
 * @param model The GLTF model being constructed.
 * @param bufferData The buffer data being accumulated.
 * @param bufferSize The current size of the buffer.
 * @param primitive The GLTF primitive to which attributes will be added.
 */
void GLTFExportCallback::ProcessMeshAttributes(MeshData& meshData, tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize, tinygltf::Primitive& primitive)
{
    // Process positions
    AddAttributeToBuffer(meshData.positions, model, bufferData, bufferSize, primitive, "POSITION", TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT, 3 * sizeof(float));

    // Process texture coordinates if available and texture is present
    if (!meshData.texcoords.empty() && meshData.texture != nullptr)
    {
        AddAttributeToBuffer(meshData.texcoords, model, bufferData, bufferSize, primitive, "TEXCOORD_0", TINYGLTF_TYPE_VEC2, TINYGLTF_COMPONENT_TYPE_FLOAT, 2 * sizeof(float));
    }

    // Process normals if available
    if (!meshData.normals.empty())
    {
        AddAttributeToBuffer(meshData.normals, model, bufferData, bufferSize, primitive, "NORMAL", TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT, 3 * sizeof(float));
    }

    // Process tangents if available
    if (!meshData.tangents.empty() && meshData.tangents[3] != 0.0f)
    {
        AddAttributeToBuffer(meshData.tangents, model, bufferData, bufferSize, primitive, "TANGENT", TINYGLTF_TYPE_VEC4,
                             TINYGLTF_COMPONENT_TYPE_FLOAT, 4 * sizeof(float)); // pre normalized
    }

    // Process colors if available
    if (!meshData.colors.empty())
    {
        // Not named COLOR_0 because it is not actually the color
        AddAttributeToBuffer(meshData.colors, model, bufferData, bufferSize, primitive, "_COLOR", TINYGLTF_TYPE_VEC4,
                             TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, 4 * sizeof(uint8_t), true);
    }

    // Process indices
    AddIndicesToBuffer(meshData.indices, model, bufferData, bufferSize, primitive);
}

/**
 * @brief Adds a vertex attribute to the GLTF buffer and primitive.
 *
 * @param data The vertex attribute data.
 * @param model The GLTF model being constructed.
 * @param bufferData The buffer data being accumulated.
 * @param bufferSize The current size of the buffer.
 * @param primitive The GLTF primitive to which the attribute will be added.
 * @param attributeName The name of the attribute (e.g., "POSITION", "NORMAL").
 * @param type The GLTF type of the attribute (e.g., TINYGLTF_TYPE_VEC3).
 * @param componentType The GLTF component type (e.g., TINYGLTF_COMPONENT_TYPE_FLOAT).
 * @param byteSize The size in bytes of each attribute element.
 * @param normalized Whether the attribute data should be normalized.
 */
template <typename T>
void GLTFExportCallback::AddAttributeToBuffer(const std::vector<T>& data, tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize, tinygltf::Primitive& primitive, const std::string& attributeName, int type, int componentType, size_t byteSize, bool normalized)
{
    // Align buffer to the component size for optimal access
    size_t alignment = sizeof(T);
    size_t padding = (alignment - (bufferSize % alignment)) % alignment;
    bufferData.insert(bufferData.end(), padding, 0);
    bufferSize += padding;

    // Create and configure a BufferView for this attribute
    tinygltf::BufferView bufferView;
    bufferView.buffer = 0;
    bufferView.byteOffset = bufferSize;
    bufferView.byteLength = data.size() * sizeof(T);
    bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    model.bufferViews.push_back(bufferView);

    // Append the attribute data to the buffer
    const unsigned char* dataPtr = reinterpret_cast<const unsigned char*>(data.data());
    bufferData.insert(bufferData.end(), dataPtr, dataPtr + data.size() * sizeof(T));
    bufferSize += data.size() * sizeof(T);

    // Create and configure an Accessor for this attribute
    tinygltf::Accessor accessor;
    accessor.bufferView = static_cast<int>(model.bufferViews.size() - 1);
    accessor.byteOffset = 0;
    accessor.componentType = componentType;
    accessor.count = data.size() / (byteSize / sizeof(T));
    accessor.type = type;
    accessor.normalized = normalized;

    // Calculate min and max values for position attribute
    if (attributeName == "POSITION")
        CalculateAccessorMinMax(data, accessor);

    model.accessors.push_back(accessor);
    int accessorIndex = static_cast<int>(model.accessors.size() - 1);

    // Assign the accessor to the primitive's attributes
    primitive.attributes[attributeName] = accessorIndex;
}

/**
 * @brief Adds index data to the GLTF buffer and primitive.
 *
 * @param indices The index data.
 * @param model The GLTF model being constructed.
 * @param bufferData The buffer data being accumulated.
 * @param bufferSize The current size of the buffer.
 * @param primitive The GLTF primitive to which the indices will be added.
 */
void GLTFExportCallback::AddIndicesToBuffer(const std::vector<uint16_t>& indices, tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize, tinygltf::Primitive& primitive)
{
    // Align buffer to 2 bytes for unsigned short indices
    size_t alignment = 2;
    size_t padding = (alignment - (bufferSize % alignment)) % alignment;
    bufferData.insert(bufferData.end(), padding, 0);
    bufferSize += padding;

    // Create and configure a BufferView for the indices
    tinygltf::BufferView indexBufferView;
    indexBufferView.buffer = 0;
    indexBufferView.byteOffset = bufferSize;
    indexBufferView.byteLength = indices.size() * sizeof(uint16_t);
    indexBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
    model.bufferViews.push_back(indexBufferView);

    // Append the index data to the buffer
    const unsigned char* indexPtr = reinterpret_cast<const unsigned char*>(indices.data());
    bufferData.insert(bufferData.end(), indexPtr, indexPtr + indices.size() * sizeof(uint16_t));
    bufferSize += indices.size() * sizeof(uint16_t);

    // Create and configure an Accessor for the indices
    tinygltf::Accessor accessor;
    accessor.bufferView = static_cast<int>(model.bufferViews.size() - 1);
    accessor.byteOffset = 0;
    accessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
    accessor.count = indices.size();
    accessor.type = TINYGLTF_TYPE_SCALAR;

    model.accessors.push_back(accessor);
    int accessorIndex = static_cast<int>(model.accessors.size() - 1);

    // Assign the accessor to the primitive's indices
    primitive.indices = accessorIndex;
}

/**
 * @brief Calculates and assigns the minimum and maximum values for an accessor.
 *
 * @param data The vertex attribute data.
 * @param accessor The accessor to update with min and max values.
 */
template <typename T>
void GLTFExportCallback::CalculateAccessorMinMax(const std::vector<T>& data, tinygltf::Accessor& accessor)
{
    if (data.empty()) return;

    float minVals[3] = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    float maxVals[3] = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
    for (size_t i = 0; i < data.size(); i += 3)
    {
        float x = data[i + 0];
        float y = data[i + 1];
        float z = data[i + 2];

        if (x < minVals[0]) minVals[0] = x;
        if (y < minVals[1]) minVals[1] = y;
        if (z < minVals[2]) minVals[2] = z;

        if (x > maxVals[0]) maxVals[0] = x;
        if (y > maxVals[1]) maxVals[1] = y;
        if (z > maxVals[2]) maxVals[2] = z;
    }

    accessor.minValues = { minVals[0], minVals[1], minVals[2] };
    accessor.maxValues = { maxVals[0], maxVals[1], maxVals[2] };
}

/**
 * @brief Adds modulate parameters to the primitive's extras.
 *
 * @param meshData The mesh data containing modulate parameters.
 * @param primitive The GLTF primitive to which extras will be added.
 */
void GLTFExportCallback::AddPrimitiveExtras(MeshData& meshData, tinygltf::Primitive& primitive)
{
    FFLModulateMode modulateMode = meshData.modulateMode;
    // Set modulate mode. NOTE that since all textures
    // have been converted from R8/R8G8 to RGBA8,
    // the modulate mode should be clamped to texture direct
    modulateMode = std::min(modulateMode, FFL_MODULATE_MODE_TEXTURE_DIRECT);

    tinygltf::Value modulateModeValue(modulateMode);
    tinygltf::Value modulateTypeValue(meshData.modulateType);
    // set "modulateColor" as colorR
    tinygltf::Value modulateColor(std::vector<tinygltf::Value>{
        tinygltf::Value(meshData.colorR[0]),
        tinygltf::Value(meshData.colorR[1]),
        tinygltf::Value(meshData.colorR[2]),
        tinygltf::Value(meshData.colorR[3])
    });
    //FFLCullMode cullModeWithoutFront = meshData.cullMode;
    // we do not want the client to apply front face culling
    // because we already reversed the triangle winding
    // effectively "simulating" it
    /*
    if (cullModeWithoutFront == FFL_CULL_MODE_FRONT)
        cullModeWithoutFront = FFL_CULL_MODE_BACK;
    tinygltf::Value cullModeValue(cullModeWithoutFront);
    */

    primitive.extras = tinygltf::Value(tinygltf::Value::Object{
        {"modulateMode", modulateModeValue},
        {"modulateType", modulateTypeValue},
        {"modulateColor", modulateColor},
        //{"cullMode", cullModeValue}
    });
}

/**
 * @brief Processes a texture by extracting it, encoding to PNG, and adding it to the GLTF model.
 *
 * @param meshData The mesh data containing the texture.
 * @param model The GLTF model being constructed.
 * @param bufferData The buffer data being accumulated.
 * @param bufferSize The current size of the buffer.
 */
void GLTFExportCallback::ProcessTexture(MeshData& meshData, tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize)
{
    // Check if texture is already loaded
    if (mTextureMap.find(meshData.texture) == mTextureMap.end())
    {
        // Extract texture data to RGBA
        std::vector<unsigned char> rgbaData;
        int texWidth, texHeight;
        bool success = ExtractTextureToRGBA(meshData.texture, rgbaData, &texWidth, &texHeight);
        if (!success)
        {
            RIO_LOG("Error: Failed to extract texture.\n");
            return; // Skip this texture
        }

        // Modify texture data based on modulate mode
        ApplyModulateMode(meshData, rgbaData);

        // Encode RGBA data to PNG
        std::vector<unsigned char> pngData;
        success = EncodeRGBADataToPNG(rgbaData, texWidth, texHeight, pngData);
        if (!success)
        {
            RIO_LOG("Error: Failed to encode texture to PNG.\n");
            return; // Skip this texture
        }

        // Add the PNG data to the buffer and create a BufferView
        int imageIndex = AddImageToModel(model, bufferData, bufferSize, pngData, "Texture_" + std::to_string(mTextureMap.size()));

        // Create a GLTF Texture and Sampler
        int textureIndex = AddTextureToModel(model, imageIndex);

        // Map the texture to avoid duplicate entries
        mTextureMap[meshData.texture] = textureIndex;
    }
}

/**
 * @brief Applies modulation to the RGBA data based on the modulate mode.
 *
 * @param meshData The mesh data containing modulation parameters.
 * @param rgbaData The RGBA texture data to modify.
 */
void GLTFExportCallback::ApplyModulateMode(MeshData& meshData, std::vector<unsigned char>& rgbaData)
{
    switch (meshData.modulateMode)
    {
    case 3: // FFL_MODULATE_MODE_ALPHA
    {
        // Apply per-pixel operations to modify RGB based on alpha
        for (size_t i = 0; i < rgbaData.size(); i += 4)
        {
            float textureR = rgbaData[i] / 255.0f;
            rgbaData[i]     = static_cast<unsigned char>(meshData.colorR[0] * textureR * 255.0f);
            rgbaData[i + 1] = static_cast<unsigned char>(meshData.colorR[1] * textureR * 255.0f);
            rgbaData[i + 2] = static_cast<unsigned char>(meshData.colorR[2] * textureR * 255.0f);
            rgbaData[i + 3] = static_cast<unsigned char>(textureR * 255.0f);
        }
        break;
    }
    case 4: // FFL_MODULATE_MODE_LUMINANCE_ALPHA
    {
        // Apply per-pixel operations to modify RGB based on luminance and alpha
        for (size_t i = 0; i < rgbaData.size(); i += 4)
        {
            float textureG = rgbaData[i + 1] / 255.0f;
            float textureR = rgbaData[i] / 255.0f;
            rgbaData[i]     = static_cast<unsigned char>(meshData.colorR[0] * textureG * 255.0f);
            rgbaData[i + 1] = static_cast<unsigned char>(meshData.colorR[1] * textureG * 255.0f);
            rgbaData[i + 2] = static_cast<unsigned char>(meshData.colorR[2] * textureG * 255.0f);
            rgbaData[i + 3] = static_cast<unsigned char>(textureR * 255.0f);
        }
        break;
    }
    case 5: // FFL_MODULATE_MODE_ALPHA_OPA
    {
        // Apply per-pixel operations to modify RGB based on alpha and set alpha to opaque
        for (size_t i = 0; i < rgbaData.size(); i += 4)
        {
            float textureR = rgbaData[i] / 255.0f;
            rgbaData[i]     = static_cast<unsigned char>(meshData.colorR[0] * textureR * 255.0f);
            rgbaData[i + 1] = static_cast<unsigned char>(meshData.colorR[1] * textureR * 255.0f);
            rgbaData[i + 2] = static_cast<unsigned char>(meshData.colorR[2] * textureR * 255.0f);
            rgbaData[i + 3] = 255; // Set alpha to opaque
        }
        break;
    }
    default:
        break; // Other modes do not require modification here
    }
}

/**
 * @brief Adds an image to the GLTF model's buffer and returns its index.
 *
 * @param model The GLTF model being constructed.
 * @param bufferData The buffer data being accumulated.
 * @param bufferSize The current size of the buffer.
 * @param pngData The PNG-encoded image data.
 * @param imageName The name to assign to the image.
 * @return The index of the added image in the model's images array.
 */
int GLTFExportCallback::AddImageToModel(tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize, const std::vector<unsigned char>& pngData, const std::string& imageName)
{
    tinygltf::Image image;
    image.name = imageName;
    image.mimeType = "image/png";
    image.image = pngData;

    // Align buffer to 4 bytes for consistency
    size_t alignment = 4;
    size_t padding = (alignment - (bufferSize % alignment)) % alignment;
    bufferData.insert(bufferData.end(), padding, 0);
    bufferSize += padding;

    // Create and configure a BufferView for the image
    tinygltf::BufferView imageBufferView;
    imageBufferView.buffer = 0;
    imageBufferView.byteOffset = bufferSize;
    imageBufferView.byteLength = pngData.size();
    // Do NOT set target for image bufferViews
    // imageBufferView.target remains 0 (NONE)
    model.bufferViews.push_back(imageBufferView);

    // Append image data to the buffer
    bufferData.insert(bufferData.end(), pngData.begin(), pngData.end());
    bufferSize += pngData.size();

    // Assign the BufferView to the image
    image.bufferView = static_cast<int>(model.bufferViews.size() - 1);

    // Add image to the model's images array
    model.images.push_back(image);
    int imageIndex = static_cast<int>(model.images.size() - 1);

    return imageIndex;
}

/**
 * @brief Adds a texture to the GLTF model using the provided image index.
 *
 * @param model The GLTF model being constructed.
 * @param imageIndex The index of the image to use for the texture.
 * @return The index of the added texture in the model's textures array.
 */
int GLTFExportCallback::AddTextureToModel(tinygltf::Model& model, int imageIndex)
{
    tinygltf::Texture gltfTexture;
    gltfTexture.source = imageIndex;

    // Create and configure a Sampler for the texture
    tinygltf::Sampler sampler;
    sampler.wrapS = TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT;
    sampler.wrapT = TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT;
    sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
    sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
    model.samplers.push_back(sampler);
    gltfTexture.sampler = static_cast<int>(model.samplers.size() - 1);

    // Add texture to the model's textures array
    model.textures.push_back(gltfTexture);
    int textureIndex = static_cast<int>(model.textures.size() - 1);

    return textureIndex;
}

/**
 * @brief Assigns a material to the primitive based on modulation mode and texture presence.
 *
 * @param meshData The mesh data containing material parameters.
 * @param model The GLTF model being constructed.
 * @param primitive The GLTF primitive to which the material will be assigned.
 * @param meshIndex The index of the current mesh.
 */
void GLTFExportCallback::AssignMaterialToPrimitive(MeshData& meshData, tinygltf::Model& model, tinygltf::Primitive& primitive, size_t meshIndex)
{
    tinygltf::Material material;
    material.name = "Material_" + std::to_string(meshIndex);

    if (meshData.modulateMode == 0) // FFL_MODULATE_MODE_CONSTANT
    {
        // Mode 0: Set base color using pColorR
        material.pbrMetallicRoughness.baseColorFactor = {
            SRGBToLinear(meshData.colorR[0]),
            SRGBToLinear(meshData.colorR[1]),
            SRGBToLinear(meshData.colorR[2]),
            SRGBToLinear(meshData.colorR[3])
        };
        material.alphaMode = "OPAQUE";
    }
    else
    {
        // Apply modulation modes with textures
        switch (meshData.modulateMode)
        {
        case 1: // FFL_MODULATE_MODE_TEXTURE_DIRECT
        {
            // Mode 1: Use texture directly
            int mappedTextureIndex = mTextureMap[meshData.texture];
            material.pbrMetallicRoughness.baseColorTexture.index = mappedTextureIndex;
            material.pbrMetallicRoughness.baseColorTexture.texCoord = 0;
            material.alphaMode = "MASK";
            break;
        }
        case 3: // FFL_MODULATE_MODE_ALPHA
        {
            // Mode 3: Texture modified by pColorR
            int mappedTextureIndex = mTextureMap[meshData.texture];
            material.pbrMetallicRoughness.baseColorTexture.index = mappedTextureIndex;
            material.pbrMetallicRoughness.baseColorTexture.texCoord = 0;
            material.alphaMode = "BLEND";
            break;
        }
        case 4: // FFL_MODULATE_MODE_LUMINANCE_ALPHA
        {
            // Mode 4: Set base color using pColorR and use texture's alpha for masking
            int mappedTextureIndex = mTextureMap[meshData.texture];
            material.pbrMetallicRoughness.baseColorTexture.index = mappedTextureIndex;
            material.pbrMetallicRoughness.baseColorTexture.texCoord = 0;
            material.alphaMode = "BLEND";
            break;
        }
        case 5: // FFL_MODULATE_MODE_ALPHA_OPA
        {
            // Mode 5: Set base color using pColorR and use texture's alpha for blending
            int mappedTextureIndex = mTextureMap[meshData.texture];
            material.pbrMetallicRoughness.baseColorTexture.index = mappedTextureIndex;
            material.pbrMetallicRoughness.baseColorTexture.texCoord = 0;
            material.alphaMode = "OPAQUE";
            break;
        }
        default:
            break;
        }
    }

    // Set the doubleSided property based on the culling mode
    switch (meshData.cullMode)
    {
        case FFL_CULL_MODE_NONE:
            material.doubleSided = true;
            break;
        case FFL_CULL_MODE_BACK:
        case FFL_CULL_MODE_FRONT:
            // we should have converted from
            // front culling to back culling earlier
            // by reversing triangle windings
        default:
            material.doubleSided = false;
            break;
    }

    // Add the material to the model's materials array
    model.materials.push_back(material);
    int materialIndex = static_cast<int>(model.materials.size() - 1);

    // Assign the material to the primitive
    primitive.material = materialIndex;
}

/**
 * @brief Assigns a material to the primitive when no texture is present.
 *
 * @param meshData The mesh data containing material parameters.
 * @param model The GLTF model being constructed.
 * @param primitive The GLTF primitive to which the material will be assigned.
 * @param meshIndex The index of the current mesh.
 */
void GLTFExportCallback::AssignMaterialWithoutTexture(MeshData& meshData, tinygltf::Model& model, tinygltf::Primitive& primitive, size_t meshIndex)
{
    tinygltf::Material material;
    material.name = "Material_" + std::to_string(meshIndex);

    // Default handling for other modulation modes without texture
    material.pbrMetallicRoughness.baseColorFactor = {
        SRGBToLinear(meshData.colorR[0]),
        SRGBToLinear(meshData.colorR[1]),
        SRGBToLinear(meshData.colorR[2]),
        SRGBToLinear(meshData.colorR[3])
    };
    material.alphaMode = "OPAQUE";

    // Set the doubleSided property based on the culling mode
    switch (meshData.cullMode)
    {
        case FFL_CULL_MODE_NONE:
            material.doubleSided = true;
            break;
        case FFL_CULL_MODE_BACK:
        case FFL_CULL_MODE_FRONT:
        default:
            material.doubleSided = false;
            break;
    }

    // Add the material to the model's materials array
    model.materials.push_back(material);
    int materialIndex = static_cast<int>(model.materials.size() - 1);

    // Assign the material to the primitive
    primitive.material = materialIndex;
}

/**
 * @brief Adds a node to the default scene in the GLTF model.
 *
 * @param model The GLTF model being constructed.
 * @param nodeIndex The index of the node to add.
 */
void GLTFExportCallback::AddNodeToScene(tinygltf::Model& model, int nodeIndex)
{
    if (model.scenes.empty())
    {
        tinygltf::Scene scene;
        scene.nodes.push_back(nodeIndex);
        model.scenes.push_back(scene);
        model.defaultScene = 0;
    }
    else
    {
        model.scenes[model.defaultScene].nodes.push_back(nodeIndex);
    }
}

/**
 * @brief Handles additional mask textures associated with the character model.
 *
 * @param model The GLTF model being constructed.
 * @param bufferData The buffer data being accumulated.
 * @param bufferSize The current size of the buffer.
 */
void GLTFExportCallback::HandleAdditionalMaskTextures(tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize)
{
    if (mpCharModel != nullptr)
    {
        FFLiCharModel* pCharModel = reinterpret_cast<FFLiCharModel*>(mpCharModel);
        for (int expr = 0; expr < FFL_EXPRESSION_MAX; ++expr)
        {
            if (expr == pCharModel->expression)
                continue;

            FFLiRenderTexture* renderTexture = pCharModel->maskTextures.pRenderTextures[expr];
            if (renderTexture == nullptr || renderTexture->pTexture2D == nullptr)
                continue;

            rio::Texture2D* texture = renderTexture->pTexture2D;

            // Check if texture is already exported
            if (mTextureMap.find(texture) != mTextureMap.end())
                continue;

            // Extract texture to RGBA
            std::vector<unsigned char> rgbaData;
            int texWidth, texHeight;
            bool success = ExtractTextureToRGBA(texture, rgbaData, &texWidth, &texHeight);
            if (!success)
            {
                RIO_LOG("Failed to extract mask texture for expression %d\n", expr);
                continue;
            }

            // Encode RGBA data to PNG
            std::vector<unsigned char> pngData;
            if (!EncodeRGBADataToPNG(rgbaData, texWidth, texHeight, pngData))
            {
                RIO_LOG("Failed to encode mask texture to PNG for expression %d\n", expr);
                continue;
            }

            // Add the PNG data to the buffer and create a BufferView
            int imageIndex = AddImageToModel(model, bufferData, bufferSize, pngData, "MaskTexture_" + std::to_string(expr));

            // Create a GLTF Texture and Sampler for the mask texture
            int textureIndex = AddTextureToModel(model, imageIndex);
            model.textures.back().name = "MaskTexture_" + std::to_string(expr);

            // Map the texture to avoid duplicate entries
            mTextureMap[texture] = textureIndex;
        }
    }
}

/**
 * @brief Includes character model information such as FFLiCharInfo and FFLPartsTransform into the GLTF model's extras.
 *
 * @param model The GLTF model being constructed.
 */
void GLTFExportCallback::IncludeCharacterModelInfo(tinygltf::Model& model)
{
    if (mpCharModel != nullptr)
    {
        FFLiCharModel* pCharModel = reinterpret_cast<FFLiCharModel*>(mpCharModel);
        FFLiCharInfo* pCharInfo = &pCharModel->charInfo;

        // Encode FFLiCharInfo as base64
        const std::string charInfoB64 = tinygltf::base64_encode(reinterpret_cast<unsigned char const*>(pCharInfo), sizeof(FFLiCharInfo));

        // Encode FFLPartsTransform
        FFLPartsTransform partsTransform;
        FFLGetPartsTransform(&partsTransform, mpCharModel);

        tinygltf::Value::Object transformObj;
        transformObj["hatTranslate"] = FFLVec3ToGltfValue(partsTransform.hatTranslate);
        transformObj["headFrontRotate"] = FFLVec3ToGltfValue(partsTransform.headFrontRotate);
        transformObj["headFrontTranslate"] = FFLVec3ToGltfValue(partsTransform.headFrontTranslate);
        transformObj["headSideRotate"] = FFLVec3ToGltfValue(partsTransform.headSideRotate);
        transformObj["headSideTranslate"] = FFLVec3ToGltfValue(partsTransform.headSideTranslate);
        transformObj["headTopRotate"] = FFLVec3ToGltfValue(partsTransform.headTopRotate);
        transformObj["headTopTranslate"] = FFLVec3ToGltfValue(partsTransform.headTopTranslate);

        // Include body build and height, u32 cast to s32
        tinygltf::Value buildValue = tinygltf::Value(*reinterpret_cast<s32*>(&pCharInfo->build));
        tinygltf::Value heightValue = tinygltf::Value(*reinterpret_cast<s32*>(&pCharInfo->height));

        // Add charInfo and partsTransform to the model's extras
        model.asset.extras = tinygltf::Value(tinygltf::Value::Object{
            {"build", buildValue},
            {"height", heightValue},
            // FFLiCharInfo value, not parsable or used by anything
            {"charInfo", tinygltf::Value(charInfoB64)},
            {"partsTransform", tinygltf::Value(transformObj)}
        });
    }
}

/**
 * @brief Writes the GLTF model to a file or output stream.
 *
 * @param model The GLTF model to write.
 * @param filename The filename to write to. If empty, the model will not be written to a file.
 * @param outStream The output stream to write to. If nullptr, the model will not be written to a stream.
 * @return true if writing was successful, false otherwise.
 */
bool GLTFExportCallback::WriteModelToFileOrStream(const tinygltf::Model& model, const std::string& filename, std::ostream* outStream)
{
    tinygltf::TinyGLTF gltfContext;
    bool ret = false;

    if (!filename.empty())
    {
        // Write as a pretty-printed GLTF file for human readability
        ret = gltfContext.WriteGltfSceneToFile(&model, filename, true, false, true, false);
        // Parameters:
        // prettyPrint = true: Makes the file human-readable (useful for debugging).
        // writeBinary = false: Writes as a GLTF file (.gltf).
    }
    else if (outStream != nullptr)
    {
        // Write as a binary stream
        ret = gltfContext.WriteGltfSceneToStream(&model, *outStream, false, true);
        // Parameters:
        // prettyPrint = false: No need for human readability in a binary stream.
        // writeBinary = true: Writes in binary format to the stream.
    }
    else
    {
        return false; // No output method specified
    }

    if (!ret)
    {
        // Handle error if writing failed
        RIO_LOG("Failed to write glTF file.\n");
    }
    return true;
}

#endif // NO_GLTF
