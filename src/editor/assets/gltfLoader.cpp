//
// Created by wirewhiz on 1/19/22.
//

#include "gltfLoader.h"
#include <filesystem>
#include <iostream>
#include "runtime/runtime.h"

bool GLTFLoader::loadGltfFromFile(const std::filesystem::path& gltfFilename)
{
    std::ifstream jsonFile(gltfFilename, std::ios::binary);
    if(!jsonFile.is_open())
        return false;
    try
    {
        jsonFile >> _data;
    }
    catch(const std::exception& e)
    {
        Runtime::error("Problem reading gltf: " + (std::string)e.what());
        return false;
    }

    jsonFile.close();

    std::filesystem::path binFilename = gltfFilename.parent_path() / _data["buffers"][0]["uri"].asString();

    std::ifstream binFile = std::ifstream(
        binFilename, std::ios::binary | std::ios::ate); // File will be closed when this object is destroyed.
    if(!binFile.is_open())
    {
        Runtime::error("Could not find bin file for gltf: " + binFilename.string());
        return false;
    }

    auto binLength = binFile.tellg();
    binFile.seekg(0);
    _bin.resize(binLength);
    binFile.read((char*)_bin.data(), binLength);
    binFile.close();
    return true;
}

bool GLTFLoader::loadGlbFromFile(const std::filesystem::path& glbFilename)
{
    std::ifstream binFile =
        std::ifstream(glbFilename, std::ios::binary); // File will be closed when this object is destroyed.
    if(!binFile.is_open())
        return false;

    binFile.seekg(12); // Skip past the 12 byte header, to the json header
    uint32_t jsonLength;
    binFile.read((char*)&jsonLength, sizeof(uint32_t));

    std::string jsonStr;
    jsonStr.resize(jsonLength);
    binFile.seekg(20);
    binFile.read(jsonStr.data(), jsonLength);

    Json::Reader reader;
    if(!reader.parse(jsonStr, _data))
    {
        std::cerr << "Problem parsing assetData: " << jsonStr << std::endl;
        return false;
    }

    uint32_t binLength;
    binFile.read((char*)&binLength, sizeof(binLength));
    binFile.seekg(sizeof(uint32_t), std::ios_base::cur); // skip chunk type
    _bin.resize(binLength);
    binFile.read((char*)_bin.data(), binLength);
    return true;
}

bool GLTFLoader::loadGltfFromString(const std::string& gltf, const std::string& bin)
{
    Json::Reader reader;
    if(!reader.parse(gltf, _data))
    {
        std::cerr << "Problem parsing assetData: " << gltf << std::endl;
        return false;
    }

    _bin.resize(bin.size());
    std::memcpy(_bin.data(), bin.data(), bin.size());
    return true;
}

bool GLTFLoader::loadGlbFromString(const std::string& glb)
{
    std::stringstream binFile(glb);

    binFile.seekg(12); // Skip past the 12 byte header, to the json header
    uint32_t jsonLength;
    binFile.read((char*)&jsonLength, sizeof(uint32_t));

    std::string jsonStr;
    jsonStr.resize(jsonLength);
    binFile.seekg(20);
    binFile.read(jsonStr.data(), jsonLength);

    Json::Reader reader;
    if(!reader.parse(jsonStr, _data))
    {
        std::cerr << "Problem parsing assetData: " << jsonStr << std::endl;
        return false;
    }

    uint32_t binLength;
    binFile.read((char*)&binLength, sizeof(binLength));
    binFile.seekg(sizeof(uint32_t), std::ios_base::cur); // skip chunk type
    _bin.resize(binLength);
    binFile.read((char*)_bin.data(), binLength);
    return true;
}

GLTFLoader::~GLTFLoader() {}

#include <unordered_set>

void GLTFLoader::printInfo()
{
    std::cout << "meshes: " << _data["meshes"].size() << std::endl;
    size_t primitives = 0;
    size_t verts = 0;
    std::unordered_set<size_t> accessors;
    for(Json::Value& mesh : _data["meshes"])
    {
        primitives += mesh["primitives"].size();
        for(Json::Value& primitive : mesh["primitives"])
        {
            int position = primitive["attributes"]["POSITION"].asInt();
            if(!accessors.count(position))
            {
                accessors.insert(position);
                verts += _data["accessors"][position]["count"].asLargestUInt();
            }
        }
    }
    std::cout << "primitives: " << primitives << std::endl;
    std::cout << "vertices: " << verts << std::endl;
}

void GLTFLoader::printPositions(int meshIndex, int primitiveIndex)
{
    Json::Value& primitive = _data["meshes"][meshIndex]["primitives"][primitiveIndex];
    Json::Value& positionAccessor = _data["accessors"][primitive["attributes"]["POSITION"].asInt()];
    Json::Value& bufferView = _data["bufferViews"][positionAccessor["bufferView"].asInt()];

    float* buffer = (float*)(_bin.data() + bufferView["byteOffset"].asInt());

    for(int i = 0; i < positionAccessor["count"].asInt(); ++i)
    {
        std::cout << "(" << buffer[i * 3] << ", " << buffer[i * 3 + 1] << ", " << buffer[i * 3 + 2] << ")" << std::endl;
    }

    std::cout << "vertices: " << positionAccessor["count"].asInt() << std::endl;
}

std::vector<uint16_t> GLTFLoader::readShortScalarBuffer(uint32_t accessorIndex)
{
    Json::Value& accessor = _data["accessors"][accessorIndex];
    if(accessor["type"].asString() != "SCALAR" || accessor["componentType"].asUInt() != 5123)
        throw std::runtime_error("Mismatched accessor type for reading Scalar");

    Json::Value& bufferView = _data["bufferViews"][accessor["bufferView"].asUInt()];
    uint32_t count = accessor["count"].asUInt();
    uint32_t offset = bufferView["byteOffset"].asUInt() + accessor["byteOffset"].asUInt();

    uint32_t stride = bufferView.get("byteStride", sizeof(uint16_t)).asUInt();
    std::vector<uint16_t> buffer(count);
    for(uint32_t i = 0; i < count; ++i)
        buffer[i] = *(uint16_t*)&_bin[offset + stride * i];
    return buffer;
}

std::vector<uint32_t> GLTFLoader::readScalarBuffer(uint32_t accessorIndex)
{
    Json::Value& accessor = _data["accessors"][accessorIndex];
    if(accessor["type"].asString() != "SCALAR")
        throw std::runtime_error("Mismatched accessor type for reading Scalar");

    Json::Value& bufferView = _data["bufferViews"][accessor["bufferView"].asUInt()];
    uint32_t count = accessor["count"].asUInt();
    uint32_t offset = bufferView["byteOffset"].asUInt() + accessor["byteOffset"].asUInt();

    // UNSIGNED_SHORT
    if(accessor["componentType"].asUInt() == 5123)
    {
        uint32_t stride = bufferView.get("byteStride", sizeof(uint16_t)).asUInt();
        std::vector<uint32_t> buffer(count);
        for(uint32_t i = 0; i < count; ++i)
            buffer[i] = *(uint16_t*)&_bin[offset + stride * i];
        return buffer;
    }
    // UNSIGNED_INT
    if(accessor["componentType"].asUInt() == 5125)
    {
        uint32_t stride = bufferView.get("byteStride", sizeof(uint32_t)).asUInt();
        std::vector<uint32_t> buffer(count);
        for(uint32_t i = 0; i < count; ++i)
            buffer[i] = *(uint32_t*)&_bin[offset + stride * i];
        return buffer;
    }
    throw std::runtime_error("Unknown accessor component type");
}

std::vector<glm::vec2> GLTFLoader::readVec2Buffer(uint32_t accessorIndex)
{
    Json::Value& accessor = _data["accessors"][accessorIndex];
    if(accessor["componentType"].asUInt() != 5126 || accessor["type"].asString() != "VEC2")
        throw std::runtime_error("Mismatched accessor values for reading Vec2");

    Json::Value& bufferView = _data["bufferViews"][accessor["bufferView"].asUInt()];
    uint32_t count = accessor["count"].asUInt();
    uint32_t stride = bufferView.get("byteStride", sizeof(float) * 2).asUInt();
    uint32_t offset = bufferView["byteOffset"].asUInt() + accessor["byteOffset"].asUInt();

    std::vector<glm::vec2> buffer(count);
    for(uint32_t i = 0; i < count; ++i)
    {
        float* ittr = (float*)&_bin[offset + stride * i];
        buffer[i].x = ittr[0];
        buffer[i].y = ittr[1];
    }

    return buffer;
}

std::vector<glm::vec3> GLTFLoader::readVec3Buffer(uint32_t accessorIndex)
{
    Json::Value& accessor = _data["accessors"][accessorIndex];
    std::string type = accessor["type"].asString();
    // We can read vec4 values as vec 3 as the stride will account for the unread value.
    if(accessor["componentType"].asUInt() != 5126 || !(type == "VEC3" || type == "VEC4"))
        throw std::runtime_error("Mismatched accessor values for reading Vec3");

    Json::Value& bufferView = _data["bufferViews"][accessor["bufferView"].asUInt()];
    uint32_t count = accessor["count"].asUInt();
    uint32_t stride = bufferView.get("byteStride", sizeof(float) * 3).asUInt();
    uint32_t offset = bufferView["byteOffset"].asUInt() + accessor["byteOffset"].asUInt();

    std::vector<glm::vec3> buffer(count);
    for(uint32_t i = 0; i < count; ++i)
    {
        float* ittr = (float*)&_bin[offset + stride * i];
        buffer[i].x = ittr[0];
        buffer[i].y = ittr[1];
        buffer[i].z = ittr[2];
    }

    return buffer;
}

std::vector<glm::vec4> GLTFLoader::readVec4Buffer(uint32_t accessorIndex)
{
    Json::Value& accessor = _data["accessors"][accessorIndex];
    std::string type = accessor["type"].asString();
    // We can read vec4 values as vec 3 as the stride will account for the unread value.
    if(accessor["componentType"].asUInt() != 5126 || !(type == "VEC4"))
        throw std::runtime_error("Mismatched accessor values for reading Vec3");

    Json::Value& bufferView = _data["bufferViews"][accessor["bufferView"].asUInt()];
    uint32_t count = accessor["count"].asUInt();
    uint32_t stride = bufferView.get("byteStride", sizeof(float) * 4).asUInt();
    uint32_t offset = bufferView["byteOffset"].asUInt() + accessor["byteOffset"].asUInt();

    std::vector<glm::vec4> buffer(count);
    for(uint32_t i = 0; i < count; ++i)
    {
        float* ittr = (float*)&_bin[offset + stride * i];
        buffer[i].x = ittr[0];
        buffer[i].y = ittr[1];
        buffer[i].z = ittr[2];
        buffer[i].w = ittr[3];
    }

    return buffer;
}

std::vector<MeshAsset*> GLTFLoader::extractAllMeshes()
{
    std::vector<MeshAsset*> meshAssets;
    for(auto& meshData : _data["meshes"])
    {
        MeshAsset* mesh = new MeshAsset();
        mesh->name = meshData["name"].asString();

        for(auto& primitive : meshData["primitives"])
        {
            auto positions = readVec3Buffer(primitive["attributes"]["POSITION"].asUInt());
            size_t pIndex;
            auto indexBufferType = accessorComponentType(primitive["indices"].asUInt());
            if(indexBufferType == 5123) // UNSIGNED_SHORT
                pIndex = mesh->addPrimitive(readShortScalarBuffer(primitive["indices"].asUInt()),
                                            static_cast<uint32_t>(positions.size()));
            else // UNSIGNED_INT
                pIndex = mesh->addPrimitive(readScalarBuffer(primitive["indices"].asUInt()),
                                            static_cast<uint32_t>(positions.size()));
            mesh->addAttribute(pIndex, "POSITION", positions);

            if(primitive["attributes"].isMember("NORMAL"))
            {
                auto v = readVec3Buffer(primitive["attributes"]["NORMAL"].asUInt());
                mesh->addAttribute(pIndex, "NORMAL", v);
            }

            if(primitive["attributes"].isMember("TANGENT"))
            {
                auto v = readVec4Buffer(primitive["attributes"]["TANGENT"].asUInt());
                mesh->addAttribute(pIndex, "TANGENT", v);
            }

            // TODO  make it so that we automatically detect all texcoords
            if(primitive["attributes"].isMember("TEXCOORD_0"))
            {
                auto v = readVec2Buffer(primitive["attributes"]["TEXCOORD_0"].asUInt());
                mesh->addAttribute(pIndex, "TEXCOORD_0", v);
            }

            // TODO: Remove vertices unused by indices array, since primitives reuse buffers
        }

        meshAssets.push_back(mesh);
    }
    return meshAssets;
}

Json::Value& GLTFLoader::nodes()
{
    return _data["nodes"];
}

Json::Value& GLTFLoader::data()
{
    return _data;
}

bool GLTFLoader::loadFromFile(const std::filesystem::path& filename)
{
    std::string ext = filename.extension().string();
    if(ext == ".gltf")
        return loadGltfFromFile(filename);
    if(ext == ".glb")
        return loadGltfFromFile(filename);
    Runtime::error("Unrecognised file postfix: " + ext);
    return false;
}

uint32_t GLTFLoader::accessorComponentType(uint32_t accessorIndex)
{
    Json::Value& accessor = _data["accessors"][accessorIndex];
    return accessor["componentType"].asUInt();
}
