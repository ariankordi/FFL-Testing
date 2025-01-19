#include <nn/ffl.h>
#include <nn/ffl/FFLiCharModel.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <array>
#include <cmath>        // For powf and sqrtf
#include <cstring>      // For memcpy
#include <cstdint>      // For fixed-width integer types
#include <iostream>     // For std::ostream

// Include TinyGLTF
#include "for_gltf/tiny_gltf.h"

class GLTFExportCallback
{
public:
    /**
     * @brief Constructor initializes the GLTFExportCallback instance.
     */
    GLTFExportCallback();

    /**
     * @brief Retrieves the shader callback structure.
     *
     * @return FFLShaderCallback Structure containing callback function pointers.
     */
    FFLShaderCallback GetShaderCallback();

    /**
     * @brief Processes a drawing parameter to collect mesh data.
     *
     * @param drawParam The drawing parameters containing mesh and rendering information.
     */
    void Draw(const FFLDrawParam& drawParam);

    /**
     * @brief Sets the character model for exporting additional masks and related data.
     *
     * @param pCharModel Pointer to the character model.
     */
    void SetCharModel(FFLCharModel* pCharModel)
    {
        mpCharModel = pCharModel;
    }

    /**
     * @brief Exports the collected model data to a GLTF file.
     *
     * @param filename The path to the output GLTF file.
     */
    void ExportModelToFile(const std::string& filename)
    {
        // Call the internal export method with filename and no output stream
        ExportModelInternal(filename, nullptr);
    }

    /**
     * @brief Exports the collected model data to an output stream.
     *
     * @param outStream The output stream to write the GLTF data to.
     * @return true If the export was successful.
     * @return false If the export failed.
     */
    bool ExportModelToStream(std::ostream* outStream)
    {
        // Call the internal export method with no filename and the provided output stream
        return ExportModelInternal("", outStream);
    }

    // Data structures to collect the mesh data
    struct MeshData
    {
        std::vector<float> positions;        ///< Vertex positions (x, y, z)
        std::vector<float> normals;          ///< Vertex normals (x, y, z)
        std::vector<float> texcoords;        ///< Vertex texture coordinates (u, v)
        std::vector<float> tangents;         ///< Vertex tangents (x, y, z, w)
        std::vector<uint8_t> colors;         ///< Vertex colors (r, g, b, a)
        std::vector<uint16_t> indices;       ///< Mesh indices
        rio::Drawer::PrimitiveMode primitiveType; ///< Primitive mode (e.g., TRIANGLES)

        // Modulate mode and related data
        FFLModulateMode modulateMode;        ///< Modulation mode
        FFLModulateType modulateType;        ///< Modulation type
        std::array<float, 4> colorR;         ///< RGBA color factor
        rio::Texture2D* texture;             ///< Pointer to the associated texture
        FFLCullMode cullMode;                ///< Culling mode
    };

private:
    // Static callback functions matching FFLShaderCallback's function pointer types

    /**
     * @brief Static callback to apply alpha testing. Only used in FFLInitCharModelGPUStep (mask drawing), so it is ignored.
     */
    //static void ApplyAlphaTestFunc(void* pObj, bool enable, rio::Graphics::CompareFunc func, float ref);

    /**
     * @brief Static callback to set matrix. Only used in FFLInitCharModelGPUStep (faceline/mask texture 2D planes drawing), so it is ignored.
     *
     * @param pObj Pointer to the GLTFExportCallback instance.
     * @param matrix The transformation matrix to set.
     */
    //static void SetMatrixFunc(void* pObj, const rio::BaseMtx44f* matrix);

    /**
     * @brief Static callback to handle drawing commands.
     *
     * @param pObj Pointer to the GLTFExportCallback instance.
     * @param drawParam The drawing parameters containing mesh and rendering information.
     */
    static void DrawFunc(void* pObj, const FFLDrawParam* drawParam);

    /**
     * @brief Internal method to export the collected model data to a GLTF file or stream.
     *
     * @param filename The path to the output GLTF file. If empty, export to stream.
     * @param outStream The output stream to write the GLTF data to. If nullptr, export to file.
     * @return true If the export was successful.
     * @return false If the export failed.
     */
    bool ExportModelInternal(const std::string& filename, std::ostream* outStream);

    // Helper functions for processing and exporting model data

    /**
     * @brief Unpacks a 10_10_10_2 packed normal into separate float components.
     *
     * @param packed The packed normal data.
     * @param x Reference to store the unpacked x component.
     * @param y Reference to store the unpacked y component.
     * @param z Reference to store the unpacked z component.
     */
    static void UnpackNormal_10_10_10_2(uint32_t packed, float& x, float& y, float& z);

    /**
     * @brief Maps the internal primitive mode to TinyGLTF's primitive mode.
     *
     * @param mode The internal primitive mode.
     * @return int The corresponding TinyGLTF primitive mode.
     */
    static int MapPrimitiveMode(rio::Drawer::PrimitiveMode mode);

    /**
     * @brief Converts an sRGB color value to linear space.
     *
     * @param color The sRGB color component.
     * @return float The linear color component.
     */
    static float SRGBToLinear(float color);

    /**
     * @brief Extracts texture data from a Texture2D object and converts it to RGBA format.
     *
     * @param texture Pointer to the Texture2D object.
     * @param rgbaData Vector to store the extracted RGBA data.
     * @param width Pointer to store the texture width.
     * @param height Pointer to store the texture height.
     * @return true If the extraction was successful.
     * @return false If the extraction failed.
     */
    static bool ExtractTextureToRGBA(rio::Texture2D* texture, std::vector<unsigned char>& rgbaData, int* width, int* height);

    /**
     * @brief Encodes RGBA data into PNG format using stb_image_write.
     *
     * @param rgbaData The RGBA pixel data to encode.
     * @param width The width of the image.
     * @param height The height of the image.
     * @param pngData Vector to store the encoded PNG data.
     * @return true If encoding was successful.
     * @return false If encoding failed.
     */
    static bool EncodeRGBADataToPNG(const std::vector<unsigned char>& rgbaData, int width, int height, std::vector<unsigned char>& pngData);

    /**
     * @brief Converts an FFLVec3 structure to a tinygltf::Value array.
     *
     * @param vec The FFLVec3 structure containing x, y, z components.
     * @return tinygltf::Value The converted tinygltf::Value array.
     */
    tinygltf::Value FFLVec3ToGltfValue(const FFLVec3& vec);

    /**
     * @brief Processes mesh attributes and adds them to the GLTF primitive.
     *
     * @param meshData The mesh data to process.
     * @param model The GLTF model being constructed.
     * @param bufferData The buffer data being accumulated.
     * @param bufferSize The current size of the buffer.
     * @param primitive The GLTF primitive to which attributes will be added.
     */
    void ProcessMeshAttributes(MeshData& meshData, tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize, tinygltf::Primitive& primitive);

    /**
     * @brief Adds a vertex attribute to the GLTF buffer and primitive.
     *
     * @tparam T The data type of the vertex attribute.
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
    void AddAttributeToBuffer(const std::vector<T>& data, tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize, tinygltf::Primitive& primitive, const std::string& attributeName, int type, int componentType, size_t byteSize, bool normalized = false);

    /**
     * @brief Adds index data to the GLTF buffer and primitive.
     *
     * @param indices The index data.
     * @param model The GLTF model being constructed.
     * @param bufferData The buffer data being accumulated.
     * @param bufferSize The current size of the buffer.
     * @param primitive The GLTF primitive to which the indices will be added.
     */
    void AddIndicesToBuffer(const std::vector<uint16_t>& indices, tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize, tinygltf::Primitive& primitive);

    /**
     * @brief Calculates and assigns the minimum and maximum values for an accessor.
     *
     * @tparam T The data type of the vertex attribute.
     * @param data The vertex attribute data.
     * @param accessor The accessor to update with min and max values.
     */
    template <typename T>
    void CalculateAccessorMinMax(const std::vector<T>& data, tinygltf::Accessor& accessor);

    /**
     * @brief Adds modulation parameters to the primitive's extras for custom usage.
     *
     * @param meshData The mesh data containing modulation parameters.
     * @param primitive The GLTF primitive to which extras will be added.
     */
    void AddPrimitiveExtras(MeshData& meshData, tinygltf::Primitive& primitive);

    /**
     * @brief Processes a texture by extracting, encoding, and adding it to the GLTF model.
     *
     * @param meshData The mesh data containing the texture information.
     * @param model The GLTF model being constructed.
     * @param bufferData The buffer data being accumulated.
     * @param bufferSize The current size of the buffer.
     */
    void ProcessTexture(MeshData& meshData, tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize);

    /**
     * @brief Applies modulation to the RGBA data based on the modulate mode.
     *
     * @param meshData The mesh data containing modulation parameters.
     * @param rgbaData The RGBA texture data to modify.
     */
    void ApplyModulateMode(MeshData& meshData, std::vector<unsigned char>& rgbaData);

    /**
     * @brief Adds a PNG-encoded image to the GLTF model's buffer and returns its index.
     *
     * @param model The GLTF model being constructed.
     * @param bufferData The buffer data being accumulated.
     * @param bufferSize The current size of the buffer.
     * @param pngData The PNG-encoded image data.
     * @param imageName The name to assign to the image.
     * @return int The index of the added image in the model's images array.
     */
    int AddImageToModel(tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize, const std::vector<unsigned char>& pngData, const std::string& imageName);

    /**
     * @brief Adds a texture to the GLTF model using the provided image index.
     *
     * @param model The GLTF model being constructed.
     * @param imageIndex The index of the image to use for the texture.
     * @return int The index of the added texture in the model's textures array.
     */
    int AddTextureToModel(tinygltf::Model& model, int imageIndex);

    /**
     * @brief Assigns a material to the primitive based on modulation mode and texture presence.
     *
     * @param meshData The mesh data containing material parameters.
     * @param model The GLTF model being constructed.
     * @param primitive The GLTF primitive to which the material will be assigned.
     * @param meshIndex The index of the current mesh for naming purposes.
     */
    void AssignMaterialToPrimitive(MeshData& meshData, tinygltf::Model& model, tinygltf::Primitive& primitive, size_t meshIndex);

    /**
     * @brief Assigns a material to the primitive when no texture is present.
     *
     * @param meshData The mesh data containing material parameters.
     * @param model The GLTF model being constructed.
     * @param primitive The GLTF primitive to which the material will be assigned.
     * @param meshIndex The index of the current mesh for naming purposes.
     */
    void AssignMaterialWithoutTexture(MeshData& meshData, tinygltf::Model& model, tinygltf::Primitive& primitive, size_t meshIndex);

    /**
     * @brief Adds a node to the default scene in the GLTF model.
     *
     * @param model The GLTF model being constructed.
     * @param nodeIndex The index of the node to add.
     */
    void AddNodeToScene(tinygltf::Model& model, int nodeIndex);

    /**
     * @brief Handles additional mask textures associated with the character model.
     *
     * @param model The GLTF model being constructed.
     * @param bufferData The buffer data being accumulated.
     * @param bufferSize The current size of the buffer.
     */
    void HandleAdditionalMaskTextures(tinygltf::Model& model, std::vector<unsigned char>& bufferData, size_t& bufferSize);

    /**
     * @brief Includes character model information such as FFLiCharInfo and FFLPartsTransform into the GLTF model's extras.
     *
     * @param model The GLTF model being constructed.
     */
    void IncludeCharacterModelInfo(tinygltf::Model& model);

    /**
     * @brief Writes the GLTF model to a file or output stream.
     *
     * @param model The GLTF model to write.
     * @param filename The filename to write to. If empty, the model will not be written to a file.
     * @param outStream The output stream to write to. If nullptr, the model will not be written to a stream.
     * @return true If writing was successful.
     * @return false If writing failed.
     */
    bool WriteModelToFileOrStream(const tinygltf::Model& model, const std::string& filename, std::ostream* outStream);

    FFLCharModel* mpCharModel;                ///< Pointer to the character model

    std::vector<MeshData> mMeshes;            ///< Collection of meshes to be exported

    // Texture management
    std::unordered_map<rio::Texture2D*, int> mTextureMap; ///< Maps Texture2D pointers to GLTF texture indices
};
