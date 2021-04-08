#pragma once

#include <external/boost/di.hpp>
#include "Level.h"
#include "TypeNameLookup.h"

#include "Resources/resource.h"
#include "Resources/ResourceHelper.h"

namespace trview
{
    auto register_app_elements_module() noexcept
    {
        using namespace boost;
        using namespace graphics;
        return di::make_injector(
            di::bind<ITypeNameLookup>.to(
                []()
                {
                    Resource type_list = get_resource_memory(IDR_TYPE_NAMES, L"TEXT");
                    return std::make_shared<TypeNameLookup>(std::string(type_list.data, type_list.data + type_list.size));
                }),
            di::bind<ILevel::Source>.to(
                [](const auto& injector) -> ILevel::Source
                {
                    return [&](auto&& level)
                    {
                        auto texture_storage = injector.create<ILevelTextureStorage::Source>()(*level);
                        auto mesh_storage = injector.create<IMeshStorage::Source>()(*level, *texture_storage);
                        return std::make_unique<Level>(
                            injector.create<std::shared_ptr<IDevice>>(),
                            injector.create<std::shared_ptr<IShaderStorage>>(),
                            std::move(level),
                            std::move(texture_storage),
                            std::move(mesh_storage),
                            injector.create<std::unique_ptr<ITransparencyBuffer>>(),
                            injector.create<std::unique_ptr<ISelectionRenderer>>(),
                            injector.create<std::shared_ptr<ITypeNameLookup>>(),
                            injector.create<IMesh::Source>(),
                            injector.create<IMesh::TransparentSource>());
                    };
                })
        );
    }
}
