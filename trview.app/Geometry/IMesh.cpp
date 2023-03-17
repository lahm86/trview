#include "IMesh.h"
#include <random>

using namespace DirectX::SimpleMath;

namespace trview
{
    namespace
    {
        const uint16_t Texture_Mask = 0x7fff;

        Vector3 calculate_normal(const Vector3* const vertices)
        {
            auto first = vertices[1] - vertices[0];
            auto second = vertices[2] - vertices[0];
            first.Normalize();
            second.Normalize();
            return first.Cross(second);
        }

        std::shared_ptr<IMesh> create_sprite_mesh(const IMesh::Source& source,
            uint8_t x,
            uint8_t y,
            uint16_t w,
            uint16_t h,
            uint32_t tile,
            int16_t left,
            int16_t right,
            int16_t top,
            int16_t bottom,
            Matrix& scale,
            Vector3& offset,
            SpriteOffsetMode offset_mode)
        {
            // Calculate UVs.
            float u = static_cast<float>(x) / 256.0f;
            float v = static_cast<float>(y) / 256.0f;
            float width = static_cast<float>((w - 255) / 256) / 256.0f;
            float height = static_cast<float>((h - 255) / 256) / 256.0f;

            // Generate quad.
            using namespace DirectX::SimpleMath;
            std::vector<MeshVertex> vertices
            {
                { Vector3(-0.5f, -0.5f, 0), Vector3::Zero, Vector2(u, v + height), Color(1,1,1,1)  },
                { Vector3(0.5f, -0.5f, 0), Vector3::Zero, Vector2(u + width, v + height), Color(1,1,1,1) },
                { Vector3(-0.5f, 0.5f, 0), Vector3::Zero, Vector2(u, v), Color(1,1,1,1) },
                { Vector3(0.5f, 0.5f, 0), Vector3::Zero, Vector2(u + width, v), Color(1,1,1,1) },
            };

            std::vector<TransparentTriangle> transparent_triangles
            {
                { vertices[0].pos, vertices[1].pos, vertices[2].pos, vertices[0].uv, vertices[1].uv, vertices[2].uv, tile, TransparentTriangle::Mode::Normal },
                { vertices[2].pos, vertices[1].pos, vertices[3].pos, vertices[2].uv, vertices[1].uv, vertices[3].uv, tile, TransparentTriangle::Mode::Normal },
            };

            std::vector<Triangle> collision_triangles;

            float object_width = static_cast<float>(right - left) / trlevel::Scale_X;
            float object_height = static_cast<float>(bottom - top) / trlevel::Scale_Y;
            scale = Matrix::CreateScale(object_width, object_height, 1);

            if (offset_mode == SpriteOffsetMode::RoomSprite)
            {
                offset = Vector3(0, (1 - object_height) * 0.5f, 0);
            }
            else
            {
                offset = Vector3(0, object_height / -2.0f, 0);
            }

            return source(std::vector<MeshVertex>(), std::vector<std::vector<uint32_t>>(), std::vector<uint32_t>(), transparent_triangles, collision_triangles);
        }
    }

    IMesh::~IMesh()
    {
    }

    std::shared_ptr<IMesh> create_mesh(const trlevel::tr_mesh& mesh, const IMesh::Source& source, const ILevelTextureStorage& texture_storage, bool transparent_collision)
    {
        std::vector<std::vector<uint32_t>> indices(texture_storage.num_tiles());
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> untextured_indices;
        std::vector<TransparentTriangle> transparent_triangles;
        std::vector<Triangle> collision_triangles;

        process_textured_rectangles(mesh.textured_rectangles, mesh.vertices, texture_storage, vertices, indices, transparent_triangles, collision_triangles, transparent_collision);
        process_textured_triangles(mesh.textured_triangles, mesh.vertices, texture_storage, vertices, indices, transparent_triangles, collision_triangles, transparent_collision);
        process_coloured_rectangles(mesh.coloured_rectangles, mesh.vertices, texture_storage, vertices, untextured_indices, collision_triangles);
        process_coloured_triangles(mesh.coloured_triangles, mesh.vertices, texture_storage, vertices, untextured_indices, collision_triangles);

        return source(vertices, indices, untextured_indices, transparent_triangles, collision_triangles);
    }

    std::shared_ptr<IMesh> create_cube_mesh(const IMesh::Source& source)
    {
        const std::vector<MeshVertex> vertices
        {
            // Body:
            // + y
            { { -0.5, 0.5f, -0.5 }, Vector3::Down, { 0, 0 }, { 1.0f, 1.0f, 1.0f } },       // 2
            { { 0.5, 0.5f, -0.5 }, Vector3::Down, { 1, 0 }, { 1.0f, 1.0f, 1.0f } },        // 1
            { { 0.5, 0.5f, 0.5 }, Vector3::Down, { 1, 1 }, { 1.0f, 1.0f, 1.0f } },         // 0
            { { -0.5, 0.5f, 0.5 }, Vector3::Down, { 0, 1 }, { 1.0f, 1.0f, 1.0f } },        // 3

            // +x
            { { 0.5, -0.5f, -0.5 }, Vector3::Left, { 0, 0 }, { 1.0f, 1.0f, 1.0f } },       // 5
            { { 0.5, -0.5f, 0.5 }, Vector3::Left, { 1, 0 }, { 1.0f, 1.0f, 1.0f } },        // 4
            { { 0.5, 0.5f, 0.5 }, Vector3::Left, { 1, 1 }, { 1.0f, 1.0f, 1.0f } },         // 0
            { { 0.5, 0.5f, -0.5 }, Vector3::Left, { 0, 1 }, { 1.0f, 1.0f, 1.0f } },        // 1

            // -x 
            { { -0.5, 0.5f, -0.5 }, Vector3::Right, { 1, 1 }, { 1.0f, 1.0f, 1.0f } },       // 2
            { { -0.5, 0.5f, 0.5 }, Vector3::Right, { 0, 1 }, { 1.0f, 1.0f, 1.0f } },        // 3
            { { -0.5, -0.5f, 0.5 }, Vector3::Right, { 0, 0 }, { 1.0f, 1.0f, 1.0f } },       // 7
            { { -0.5, -0.5f, -0.5 }, Vector3::Right, { 1, 0 }, { 1.0f, 1.0f, 1.0f } },      // 6

            // +z
            { { 0.5, 0.5f, 0.5 }, Vector3::Forward, { 0, 1 }, { 1.0f, 1.0f, 1.0f } },         // 0
            { { 0.5, -0.5f, 0.5 }, Vector3::Forward, { 0, 0 }, { 1.0f, 1.0f, 1.0f } },        // 4
            { { -0.5, 0.5f, 0.5 }, Vector3::Forward, { 1, 1 }, { 1.0f, 1.0f, 1.0f } },        // 3
            { { -0.5, -0.5f, 0.5 }, Vector3::Forward, { 1, 0 }, { 1.0f, 1.0f, 1.0f } },       // 7

            // -z
            { { 0.5, -0.5f, -0.5 }, Vector3::Backward, { 1, 0 }, { 1.0f, 1.0f, 1.0f } },       // 5
            { { 0.5, 0.5f, -0.5 }, Vector3::Backward, { 1, 1 }, { 1.0f, 1.0f, 1.0f } },        // 1
            { { -0.5, 0.5f, -0.5 }, Vector3::Backward, { 0, 1 }, { 1.0f, 1.0f, 1.0f } },       // 2
            { { -0.5, -0.5f, -0.5 }, Vector3::Backward, { 0, 0 }, { 1.0f, 1.0f, 1.0f } },      // 6

            // -y
            { { 0.5, -0.5f, 0.5 }, Vector3::Up, { 1, 0 }, { 1.0f, 1.0f, 1.0f } },        // 4
            { { 0.5, -0.5f, -0.5 }, Vector3::Up, { 1, 1 }, { 1.0f, 1.0f, 1.0f } },       // 5
            { { -0.5, -0.5f, -0.5 }, Vector3::Up, { 0, 1 }, { 1.0f, 1.0f, 1.0f } },      // 6
            { { -0.5, -0.5f, 0.5 }, Vector3::Up, { 0, 0 }, { 1.0f, 1.0f, 1.0f } },        // 7
        };

        const std::vector<uint32_t> indices
        {
            0,  1,  2,  2,  3,  0,  // +y
            4,  5,  6,  6,  7,  4,  // +x
            8,  9,  10, 10, 11, 8,  // -x
            12, 13, 14, 13, 15, 14, // +z
            16, 17, 18, 18, 19, 16, // -z
            20, 21, 22, 22, 23, 20  // -y
        };

        return source(vertices, std::vector<std::vector<uint32_t>>(), indices, std::vector<TransparentTriangle>(), {});
    }

    std::shared_ptr<IMesh> create_sphere_mesh(const IMesh::Source& source, uint32_t stacks, uint32_t slices)
    {
        constexpr float Pi2 = 6.283185307f;
        // Rotation around X per stack
        const float per_stack = Pi2 / stacks;
        // Rotation areound Y per slice
        const float per_slice = Pi2 / slices;
        const Matrix slice_rotation = Matrix::CreateRotationY(per_slice);

        std::vector<MeshVertex> points;

        const Vector3 top(0, -0.5f, 0);
        points.push_back(MeshVertex{ top, Vector3::Down, Vector2::Zero, Colour::White });

        for (uint32_t stack = 0; stack < stacks; ++stack)
        {
            Vector3 point = Vector3::Transform(top, Matrix::CreateRotationX(per_stack * (stack + 1)));
            for (uint32_t slice = 0; slice < slices; ++slice)
            {
                Vector3 normal = point;
                normal.Normalize();
                points.push_back({ point, normal, Vector2::Zero, Colour::White });
                point = Vector3::Transform(point, slice_rotation);
            }
        }

        std::vector<uint32_t> indices;
        for (uint32_t slice = 0; slice < slices; ++slice)
        {
            indices.push_back(0);
            indices.push_back(slice + 1);
            indices.push_back(slice == slices - 1 ? 1 : slice + 2);
        }

        for (uint32_t stack = 0; stack < stacks; ++stack)
        {
            uint32_t b = 1 + stack * slices;
            for (uint32_t slice = 0; slice < slices; ++slice)
            {
                const uint32_t top_right = (slice == slices - 1 ? 0 : slice + 1);
                const uint32_t bottom_right = (slice == slices - 1 ? 0 : slice + 1);

                indices.push_back(b + slice);
                indices.push_back(b + slice + slices);
                indices.push_back(b + slices + bottom_right);
                indices.push_back(b + slice);
                indices.push_back(b + slices + bottom_right);
                indices.push_back(b + top_right);
            }
        }

        return source(points, std::vector<std::vector<uint32_t>>(), indices, std::vector<TransparentTriangle>(), std::vector<Triangle>());
    }

    std::shared_ptr<IMesh> create_sprite_mesh(const IMesh::Source& source, const std::optional<trlevel::tr_sprite_texture>& sprite, Matrix& scale, Vector3& offset, SpriteOffsetMode offset_mode)
    {
        if (sprite)
        {
            const auto& s = sprite.value();
            return create_sprite_mesh(source, s.x, s.y, s.Width, s.Height, s.Tile, s.LeftSide, s.RightSide, s.TopSide, s.BottomSide, scale, offset, offset_mode);
        }
        return create_sprite_mesh(source, 0, 0, 0, 0, TransparentTriangle::Untextured, -256, 256, -256, 256, scale, offset, offset_mode);
    }

    std::shared_ptr<IMesh> create_sprite_mesh(const IMesh::Source& source, const std::optional<trlevel::tr_sprite_texture>& sprite, Matrix& scale, Matrix& offset, SpriteOffsetMode offset_mode)
    {
        Vector3 offset_vector;
        auto mesh = create_sprite_mesh(source, sprite, scale, offset_vector, offset_mode);
        offset = Matrix::CreateTranslation(offset_vector);
        return mesh;
    }

    void process_textured_rectangles(
        const std::vector<trlevel::tr4_mesh_face4>& rectangles,
        const std::vector<trlevel::tr_vertex>& input_vertices,
        const ILevelTextureStorage& texture_storage,
        std::vector<MeshVertex>& output_vertices,
        std::vector<std::vector<uint32_t>>& output_indices,
        std::vector<TransparentTriangle>& transparent_triangles,
        std::vector<Triangle>& collision_triangles,
        bool transparent_collision)
    {
        using namespace trlevel;

        uint16_t previous_texture = 0;
        for (const auto& rect : rectangles)
        {
            std::array<Vector3, 4> verts;
            for (int i = 0; i < 4; ++i)
            {
                verts[i] = convert_vertex(input_vertices[rect.vertices[i]]);
            }

            uint16_t texture = rect.texture & Texture_Mask;
            if (texture >= texture_storage.num_object_textures())
            {
                texture = previous_texture;
            }
            previous_texture = texture;

            std::array<Vector2, 4> uvs;
            for (auto i = 0u; i < uvs.size(); ++i)
            {
                uvs[i] = texture_storage.uv(texture, i);
            }

            const bool double_sided = rect.texture & 0x8000;

            TransparentTriangle::Mode transparency_mode;
            if (determine_transparency(texture_storage.attribute(texture), rect.effects, transparency_mode))
            {
                transparent_triangles.emplace_back(verts[0], verts[1], verts[2], uvs[0], uvs[1], uvs[2], texture_storage.tile(texture), transparency_mode);
                transparent_triangles.emplace_back(verts[2], verts[3], verts[0], uvs[2], uvs[3], uvs[0], texture_storage.tile(texture), transparency_mode);
                if (transparent_collision)
                {
                    collision_triangles.emplace_back(verts[0], verts[1], verts[2]);
                    collision_triangles.emplace_back(verts[2], verts[3], verts[0]);
                }

                if (double_sided)
                {

                    transparent_triangles.emplace_back(verts[2], verts[1], verts[0], uvs[2], uvs[1], uvs[0], texture_storage.tile(texture), transparency_mode);
                    transparent_triangles.emplace_back(verts[0], verts[3], verts[2], uvs[0], uvs[3], uvs[2], texture_storage.tile(texture), transparency_mode);
                    if (transparent_collision)
                    {
                        collision_triangles.emplace_back(verts[2], verts[1], verts[0]);
                        collision_triangles.emplace_back(verts[0], verts[3], verts[2]);
                    }
                }
                continue;
            }

            const uint32_t base = static_cast<uint32_t>(output_vertices.size());
            const auto normal = calculate_normal(&verts[0]);
            for (int i = 0; i < 4; ++i)
            {
                output_vertices.push_back({ verts[i], normal, uvs[i], Color(1,1,1,1) });
            }

            auto& tex_indices = output_indices[texture_storage.tile(texture)];
            tex_indices.push_back(base);
            tex_indices.push_back(base + 1);
            tex_indices.push_back(base + 2);
            tex_indices.push_back(base + 2);
            tex_indices.push_back(base + 3);
            tex_indices.push_back(base + 0);

            if (double_sided)
            {
                tex_indices.push_back(base + 2);
                tex_indices.push_back(base + 1);
                tex_indices.push_back(base);
                tex_indices.push_back(base);
                tex_indices.push_back(base + 3);
                tex_indices.push_back(base + 2);
            }

            collision_triangles.emplace_back(verts[0], verts[1], verts[2]);
            collision_triangles.emplace_back(verts[2], verts[3], verts[0]);
            if (double_sided)
            {
                collision_triangles.emplace_back(verts[2], verts[1], verts[0]);
                collision_triangles.emplace_back(verts[0], verts[3], verts[2]);
            }
        }
    }

    void process_textured_triangles(
        const std::vector<trlevel::tr4_mesh_face3>& triangles,
        const std::vector<trlevel::tr_vertex>& input_vertices,
        const ILevelTextureStorage& texture_storage,
        std::vector<MeshVertex>& output_vertices,
        std::vector<std::vector<uint32_t>>& output_indices,
        std::vector<TransparentTriangle>& transparent_triangles,
        std::vector<Triangle>& collision_triangles,
        bool transparent_collision)
    {
        uint16_t previous_texture = 0;
        for (const auto& tri : triangles)
        {
            std::array<Vector3, 3> verts;
            for (int i = 0; i < 3; ++i)
            {
                verts[i] = convert_vertex(input_vertices[tri.vertices[i]]);
            }

            uint16_t texture = tri.texture & Texture_Mask;
            if (texture >= texture_storage.num_object_textures())
            {
                texture = previous_texture;
            }
            previous_texture = texture;

            std::array<Vector2, 3> uvs;
            for (auto i = 0u; i < uvs.size(); ++i)
            {
                uvs[i] = texture_storage.uv(texture, i);
            }

            const bool double_sided = tri.texture & 0x8000;

            TransparentTriangle::Mode transparency_mode;
            if (determine_transparency(texture_storage.attribute(texture), tri.effects, transparency_mode))
            {
                transparent_triangles.emplace_back(verts[0], verts[1], verts[2], uvs[0], uvs[1], uvs[2], texture_storage.tile(texture), transparency_mode);
                if (transparent_collision)
                {
                    collision_triangles.emplace_back(verts[0], verts[1], verts[2]);
                }
                if (double_sided)
                {
                    transparent_triangles.emplace_back(verts[2], verts[1], verts[0], uvs[2], uvs[1], uvs[0], texture_storage.tile(texture), transparency_mode);
                    if (transparent_collision)
                    {
                        collision_triangles.emplace_back(verts[2], verts[1], verts[0]);
                    }
                }
                continue;
            }

            const uint32_t base = static_cast<uint32_t>(output_vertices.size());
            const auto normal = calculate_normal(&verts[0]);
            for (int i = 0; i < 3; ++i)
            {
                output_vertices.push_back({ verts[i], normal, uvs[i], Color(1,1,1,1) });
            }

            auto& tex_indices = output_indices[texture_storage.tile(texture)];
            tex_indices.push_back(base);
            tex_indices.push_back(base + 1);
            tex_indices.push_back(base + 2);
            if (double_sided)
            {
                tex_indices.push_back(base + 2);
                tex_indices.push_back(base + 1);
                tex_indices.push_back(base);
            }

            collision_triangles.emplace_back(verts[0], verts[1], verts[2]);
            if (double_sided)
            {
                collision_triangles.emplace_back(verts[2], verts[1], verts[0]);
            }
        }
    }

    void process_coloured_rectangles(
        const std::vector<trlevel::tr_face4>& rectangles,
        const std::vector<trlevel::tr_vertex>& input_vertices,
        const ILevelTextureStorage& texture_storage,
        std::vector<MeshVertex>& output_vertices,
        std::vector<uint32_t>& output_indices,
        std::vector<Triangle>& collision_triangles)
    {
        for (const auto& rect : rectangles)
        {
            const uint16_t texture = rect.texture & 0x7fff;
            const bool double_sided = rect.texture & 0x8000;

            std::array<Vector3, 4> verts;
            for (int i = 0; i < 4; ++i)
            {
                verts[i] = convert_vertex(input_vertices[rect.vertices[i]]);
            }

            const uint32_t base = static_cast<uint32_t>(output_vertices.size());
            const auto normal = calculate_normal(&verts[0]);
            for (int i = 0; i < 4; ++i)
            {
                output_vertices.push_back({ verts[i], normal, Vector2::Zero, texture_storage.palette_from_texture(texture) });
            }

            output_indices.push_back(base);
            output_indices.push_back(base + 1);
            output_indices.push_back(base + 2);
            output_indices.push_back(base + 2);
            output_indices.push_back(base + 3);
            output_indices.push_back(base + 0);
            if (double_sided)
            {
                output_indices.push_back(base + 2);
                output_indices.push_back(base + 1);
                output_indices.push_back(base);
                output_indices.push_back(base);
                output_indices.push_back(base + 3);
                output_indices.push_back(base + 2);
            }

            collision_triangles.emplace_back(verts[0], verts[1], verts[2]);
            collision_triangles.emplace_back(verts[2], verts[3], verts[0]);
            if (double_sided)
            {
                collision_triangles.emplace_back(verts[2], verts[1], verts[0]);
                collision_triangles.emplace_back(verts[0], verts[3], verts[2]);
            }
        }
    }

    void process_coloured_triangles(
        const std::vector<trlevel::tr_face3>& triangles,
        const std::vector<trlevel::tr_vertex>& input_vertices,
        const ILevelTextureStorage& texture_storage,
        std::vector<MeshVertex>& output_vertices,
        std::vector<uint32_t>& output_indices,
        std::vector<Triangle>& collision_triangles)
    {
        for (const auto& tri : triangles)
        {
            const uint16_t texture = tri.texture & 0x7fff;
            const bool double_sided = tri.texture & 0x8000;

            std::array<Vector3, 3> verts;
            for (int i = 0; i < 3; ++i)
            {
                verts[i] = convert_vertex(input_vertices[tri.vertices[i]]);
            }

            const uint32_t base = static_cast<uint32_t>(output_vertices.size());
            const auto normal = calculate_normal(&verts[0]);
            for (int i = 0; i < 3; ++i)
            {
                output_vertices.push_back({ verts[i], normal, Vector2::Zero, texture_storage.palette_from_texture(texture) });
            }

            output_indices.push_back(base);
            output_indices.push_back(base + 1);
            output_indices.push_back(base + 2);
            collision_triangles.emplace_back(verts[0], verts[1], verts[2]);

            if (double_sided)
            {
                output_indices.push_back(base + 2);
                output_indices.push_back(base + 1);
                output_indices.push_back(base);
                collision_triangles.emplace_back(verts[2], verts[1], verts[0]);
            }
        }
    }

    Vector3 convert_vertex(const trlevel::tr_vertex& vertex)
    {
        return Vector3(vertex.x / trlevel::Scale_X, vertex.y / trlevel::Scale_Y, vertex.z / trlevel::Scale_Z);
    }
}
