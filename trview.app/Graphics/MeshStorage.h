#pragma once

#include <unordered_map>
#include <cstdint>
#include <memory>

#include <trlevel/ILevel.h>

#include "IMeshStorage.h"
#include <trview.app/Geometry/Mesh.h>
#include <trview.graphics/IDevice.h>

namespace trview
{
    struct ILevelTextureStorage;

    class MeshStorage final : public IMeshStorage
    {
    public:
        explicit MeshStorage(const std::shared_ptr<graphics::IDevice>& device, const trlevel::ILevel& level, const ILevelTextureStorage& texture_storage);

        virtual ~MeshStorage() = default;

        virtual Mesh* mesh(uint32_t mesh_pointer) const override;
    private:
        mutable std::unordered_map<uint32_t, std::unique_ptr<Mesh>> _meshes;
    };
}
