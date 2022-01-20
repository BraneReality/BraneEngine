//
// Created by wirewhiz on 1/19/22.
//

#include <iostream>
#include "gltfLoader.h"

bool gltfLoader::loadGltfFromFile(const std::string& gltfFilename)
{
    std::ifstream jsonFile(gltfFilename, std::ios::binary);
    if(!jsonFile.is_open())
        return false;
    try{
        jsonFile >> _json;
    }
    catch(const std::exception& e){
        std::cerr << "Json parsing error: " << e.what() << std::endl;
        return false;
    }

    jsonFile.close();

	std::string binFilename = _json["buffers"][0]["uri"].asString();

    std::ifstream binFile = std::ifstream(binFilename, std::ios::binary | std::ios::ate); // File will be closed when this object is destroyed.
    if(!binFile.is_open())
        return false;

	size_t binLength = binFile.tellg();
	binFile.seekg(0);
	_bin.resize(binLength);
    binFile.read((char*)_bin.data(), binLength);
	binFile.close();
    return true;
}

bool gltfLoader::loadGlbFromFile(const std::string& glbFilename)
{
	std::ifstream binFile = std::ifstream(glbFilename, std::ios::binary); // File will be closed when this object is destroyed.
	if(!binFile.is_open())
		return false;

	binFile.seekg(12); //Skip past the 12 byte header, to the json header
	uint32_t jsonLength;
	binFile.read((char*)&jsonLength, sizeof(uint32_t));

	std::string jsonStr;
	jsonStr.resize(jsonLength);
	binFile.seekg(20);
	binFile.read(jsonStr.data(), jsonLength);

	Json::Reader reader;
	if(!reader.parse(jsonStr, _json))
	{
		std::cerr << "Problem parsing assetData: " << jsonStr << std::endl;
		return false;
	}

	uint32_t binLength;
	binFile.read((char*)&binLength, sizeof(binLength));
	binFile.seekg(sizeof(uint32_t), std::ios_base::cur); //skip chunk type
	_bin.resize(binLength);
	binFile.read((char*)_bin.data(), binLength);
	return true;
}


bool gltfLoader::loadGltfFromString(const std::string& gltf, const std::string& bin)
{

	Json::Reader reader;
	if(!reader.parse(gltf, _json))
	{
		std::cerr << "Problem parsing assetData: " << gltf << std::endl;
		return false;
	}

	_bin.resize(bin.size());
	std::memcpy(_bin.data(), bin.data(), bin.size());
	return true;
}

bool gltfLoader::loadGlbFromString(const std::string& glb)
{
	std::stringstream binFile(glb);

	binFile.seekg(12); //Skip past the 12 byte header, to the json header
	uint32_t jsonLength;
	binFile.read((char*)&jsonLength, sizeof(uint32_t));

	std::string jsonStr;
	jsonStr.resize(jsonLength);
	binFile.seekg(20);
	binFile.read(jsonStr.data(), jsonLength);

	Json::Reader reader;
	if(!reader.parse(jsonStr, _json))
	{
		std::cerr << "Problem parsing assetData: " << jsonStr << std::endl;
		return false;
	}

	uint32_t binLength;
	binFile.read((char*)&binLength, sizeof(binLength));
	binFile.seekg(sizeof(uint32_t), std::ios_base::cur); //skip chunk type
	_bin.resize(binLength);
	binFile.read((char*)_bin.data(), binLength);
	return true;
}
gltfLoader::~gltfLoader()
{

}
#include <unordered_set>

void gltfLoader::printInfo()
{
    std::cout << "meshes: " << _json["meshes"].size() << std::endl;
    size_t primitives = 0;
    size_t verts = 0;
    std::unordered_set<size_t> accessors;
    for (Json::Value& mesh : _json["meshes"])
    {
        primitives += mesh["primitives"].size();
        for(Json::Value& primitive : mesh["primitives"])
        {
            int position = primitive["attributes"]["POSITION"].asInt();
            if(!accessors.count(position))
            {
                accessors.insert(position);
                verts += _json["accessors"][position]["count"].asLargestUInt();
            }
        }
    }
    std::cout << "primitives: " << primitives << std::endl;
    std::cout << "vertices: " << verts << std::endl;

}

void gltfLoader::printPositions(int meshIndex, int primitiveIndex)
{
	Json::Value& primitive = _json["meshes"][meshIndex]["primitives"][primitiveIndex];
	Json::Value& positionAccessor = _json["accessors"][primitive["attributes"]["POSITION"].asInt()];
	Json::Value& bufferView = _json["bufferViews"][positionAccessor["bufferView"].asInt()];

	float* buffer = (float*)(_bin.data() + bufferView["byteOffset"].asInt());

	for (int i = 0; i < positionAccessor["count"].asInt(); ++i)
	{
	    std::cout << "(" << buffer[i*3] << ", " << buffer[i*3 + 1] << ", " << buffer[i*3 + 2] << ")" << std::endl;
	}

	std::cout << "vertices: " << positionAccessor["count"].asInt() << std::endl;
}

