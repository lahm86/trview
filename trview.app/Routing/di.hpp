#pragma once

#include <external/boost/di.hpp>
#include "Route.h"
#include "Waypoint.h"
#include "Actions.h"

namespace trview
{
    auto register_app_routing_module() noexcept
    {
        using namespace boost;
        return di::make_injector(
            di::bind<IActions>.to(
                []()
                {
                    Resource type_list = get_resource_memory(IDR_ACTIONS, L"TEXT");
                    return std::make_shared<Actions>(std::string(type_list.data, type_list.data + type_list.size));
                }),
            di::bind<IWaypoint::Source>.to(
                [](const auto& injector) -> IWaypoint::Source
                {
                    const auto mesh = create_cube_mesh(injector.create<IMesh::Source>());
                    return [=](auto&&... args)
                    {
                        return std::make_unique<Waypoint>(mesh, args...);
                    };
                }),
            di::bind<IRoute>.to<Route>(),
            di::bind<IRoute::Source>.to(
                [](const auto& injector) -> IRoute::Source
                {
                    return [&]()
                    {
                        return injector.create<std::unique_ptr<IRoute>>();
                    };
                })
        );
    }
}

