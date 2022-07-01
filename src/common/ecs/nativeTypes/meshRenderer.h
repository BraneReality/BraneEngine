//
// Created by eli on 2/9/2022.
//

#pragma once
#include <ecs/core/component.h>
#include <utility/inlineArray.h>


class MeshRendererComponent : public NativeComponent<MeshRendererComponent>
{
	REGISTER_MEMBERS_2("Mesh Renderer", mesh, materials);
public:
	uint32_t mesh;
	inlineUIntArray materials; //TODO replace with buffer thing cache friend stuff
};

