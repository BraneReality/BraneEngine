//
// Created by eli on 2/1/2022.
//

#ifndef BRANEENGINE_ASSEMBLYBUILDER_H
#define BRANEENGINE_ASSEMBLYBUILDER_H

#include "assets/assembly.h"
#include "assets/types/meshAsset.h"
#include "gltfLoader.h"

class MaterialAsset;

class AssemblyBuilder
{
  public:
    struct AssemblyAssets
    {
        std::unique_ptr<Assembly> assembly;
        std::vector<std::unique_ptr<MeshAsset>> meshes;
    };

    static AssemblyBuilder::AssemblyAssets
    buildAssembly(const std::string& name, GLTFLoader& loader, MaterialAsset* defaultMaterial);
};

#endif // BRANEENGINE_ASSEMBLYBUILDER_H
