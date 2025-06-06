#include "Compass.h"
#include <trview.app/Camera/ICamera.h>
#include <trview.app/Geometry/MeshVertex.h>
#include <trview.graphics/ISprite.h>
#include <trview.graphics/RenderTargetStore.h>
#include <trview.graphics/ViewportStore.h>
#include <trview.common/Maths.h>

namespace trview
{
    using namespace graphics;
    using namespace DirectX;
    using namespace DirectX::SimpleMath;

    namespace
    {
        const float View_Size = 200;
        const float Nodule_Size = 0.05f;

        const std::unordered_map<Compass::Axis, std::string> axis_type_names
        {
            { Compass::Axis::Pos_X, "+X" },
            { Compass::Axis::Pos_Y, "+Y" },
            { Compass::Axis::Pos_Z, "+Z" },
            { Compass::Axis::Neg_X, "-X" },
            { Compass::Axis::Neg_Y, "-Y" },
            { Compass::Axis::Neg_Z, "-Z" },
        };

        const std::unordered_map<Compass::Axis, BoundingBox> nodule_boxes
        {
            { Compass::Axis::Pos_X, BoundingBox(Vector3(0.5f, 0, 0), Vector3(Nodule_Size)) },
            { Compass::Axis::Pos_Y, BoundingBox(Vector3(0, 0.5f, 0), Vector3(Nodule_Size)) },
            { Compass::Axis::Pos_Z, BoundingBox(Vector3(0, 0, 0.5f), Vector3(Nodule_Size)) },
            { Compass::Axis::Neg_X, BoundingBox(Vector3(-0.5f, 0, 0), Vector3(Nodule_Size)) },
            { Compass::Axis::Neg_Y, BoundingBox(Vector3(0, -0.5f, 0), Vector3(Nodule_Size)) },
            { Compass::Axis::Neg_Z, BoundingBox(Vector3(0, 0, -0.5f), Vector3(Nodule_Size)) },
        };
    }

    ICompass::~ICompass()
    {
    }

    Compass::Compass(const std::shared_ptr<graphics::IDevice>& device, const graphics::ISprite::Source& sprite_source, const graphics::IRenderTarget::SizeSource& render_target_source, const IMesh::Source& mesh_source)
        : _device(device), _mesh_camera(Size(View_Size, View_Size)), _mesh(create_cube_mesh(mesh_source)), _sprite(sprite_source(Size(View_Size, View_Size))),
        _render_target(render_target_source(static_cast<uint32_t>(View_Size), static_cast<uint32_t>(View_Size), IRenderTarget::DepthStencilMode::Enabled))
    {
    }

    void Compass::render(const ICamera& camera)
    {
        if (!_visible)
        {
            return;
        }

        auto context = _device->context();

        {
            RenderTargetStore rs_store(context);
            ViewportStore vp_store(context);
            _render_target->apply();
            _render_target->clear(Color(0.0f, 0.0f, 0.0f, 0.0f));

            // Have a camera that looks at the compass and match rotation to the real camera
            _mesh_camera.set_target(Vector3::Zero);
            _mesh_camera.set_zoom(2.0f);
            _mesh_camera.set_rotation_pitch(camera.rotation_pitch());
            _mesh_camera.set_rotation_yaw(camera.rotation_yaw());

            const auto view_projection = _mesh_camera.view_projection();
            const float thickness = 0.015f;
            const auto scale = Matrix::CreateScale(thickness, 1.0f, thickness);

            _mesh->render(scale * view_projection, Color(1.0f, 0.0f, 0.0f));
            _mesh->render(scale * Matrix::CreateRotationZ(maths::HalfPi) * view_projection, Color(0.0f, 1.0f, 0.0f));
            _mesh->render(scale * Matrix::CreateRotationX(maths::HalfPi) * view_projection, Color(0.0f, 0.0f, 1.0f));

            // Nodules for each direction - they can be clicked.
            const auto nodule_scale = Matrix::CreateScale(0.05f);
            // Y
            _mesh->render(nodule_scale * Matrix::CreateTranslation(0, 0.5f, 0) * view_projection, Color(1.0f, 0.0f, 0.0f));
            _mesh->render(nodule_scale * Matrix::CreateTranslation(0, -0.5f, 0) * view_projection, Color(1.0f, 0.0f, 0.0f));
            // X
            _mesh->render(nodule_scale * Matrix::CreateTranslation(0.5f, 0, 0) * view_projection, Color(0.0f, 1.0f, 0.0f));
            _mesh->render(nodule_scale * Matrix::CreateTranslation(-0.5f, 0, 0) * view_projection, Color(0.0f, 1.0f, 0.0f));
            // Z
            _mesh->render(nodule_scale * Matrix::CreateTranslation(0, 0, 0.5f) * view_projection, Color(0.0f, 0.0f, 1.0f));
            _mesh->render(nodule_scale * Matrix::CreateTranslation(0, 0, -0.5f) * view_projection, Color(0.0f, 0.0f, 1.0f));
        }

        auto screen_size = camera.view_size();
        _sprite->set_host_size(screen_size);
        _sprite->render(_render_target->texture(), screen_size.width - View_Size, screen_size.height - View_Size, View_Size, View_Size);
    }

    bool Compass::pick(const Point& mouse_position, const Size& screen_size, Axis& axis)
    {
        if (!_visible)
        {
            return false;
        }

        // Convert the mouse position into coordinates of the compass window.
        const auto view_top_left = Point(screen_size.width - View_Size, screen_size.height - View_Size);
        const auto view_pos = mouse_position - view_top_left;
        const auto view_size = Size(View_Size, View_Size);

        if (!view_pos.is_between(Point(), Point(View_Size, View_Size)))
        {
            return false;
        }

        const Vector3 position = _mesh_camera.position();
        auto world = Matrix::CreateTranslation(position);

        Vector3 direction = XMVector3Unproject(Vector3(view_pos.x, view_pos.y, 1), 0, 0, view_size.width, view_size.height, 0, 1.0f, _mesh_camera.projection(), _mesh_camera.view(), world);
        direction.Normalize();

        bool hit = false;
        float nearest = 0;
        for (const auto& box : nodule_boxes)
        {
            float box_distance = 0;
            if (box.second.Intersects(position, direction, box_distance) && (!hit || box_distance < nearest))
            {
                nearest = box_distance;
                axis = box.first;
                hit = true;
            }
        }

        return hit;
    }

    void Compass::set_visible(bool value)
    {
        _visible = value;
    }

    std::string axis_name(Compass::Axis axis)
    {
        auto name = axis_type_names.find(axis);
        if (name == axis_type_names.end())
        {
            return "Unknown";
        }
        return name->second;
    }

    void align_camera_to_axis(ICamera& camera, Compass::Axis axis)
    {
        float yaw = camera.rotation_yaw();
        float pitch = camera.rotation_pitch();

        switch (axis)
        {
        case Compass::Axis::Pos_X:
            yaw = maths::HalfPi;
            pitch = 0;
            break;
        case Compass::Axis::Pos_Y:
            pitch = -maths::HalfPi;
            break;
        case Compass::Axis::Pos_Z:
            yaw = 0;
            pitch = 0;
            break;
        case Compass::Axis::Neg_X:
            yaw = -maths::HalfPi;
            pitch = 0;
            break;
        case Compass::Axis::Neg_Y:
            pitch = maths::HalfPi;
            break;
        case Compass::Axis::Neg_Z:
            yaw = maths::Pi;
            pitch = 0;
            break;
        }

        camera.rotate_to_yaw(yaw);
        camera.rotate_to_pitch(pitch);
    }
}
