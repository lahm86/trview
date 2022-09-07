#include "Item.h"

namespace trview
{
    Item::Item(uint32_t number, uint32_t room, uint32_t type_id, const std::string& type, int32_t ocb, uint16_t flags, const std::vector<std::weak_ptr<ITrigger>>& triggers, const DirectX::SimpleMath::Vector3& position)
        : _number(number), _room(room), _type_id(type_id), _type(type), _ocb(ocb), _flags(flags), _triggers(triggers), _position(position)
    {
    }

    uint32_t Item::number() const
    {
        return _number;
    }

    uint32_t Item::room() const
    {
        return _room;
    }

    uint32_t Item::type_id() const
    {
        return _type_id;
    }

    std::string Item::type() const
    {
        return _type;
    }

    int32_t Item::ocb() const
    {
        return _ocb;
    }

    uint16_t Item::activation_flags() const
    {
        return (_flags & 0x3E00) >> 9;
    }

    bool Item::clear_body_flag() const
    {
        return (_flags & 0x8000) != 0;
    }

    bool Item::invisible_flag() const
    {
        return (_flags & 0x100) != 0;
    }

    std::vector<std::weak_ptr<ITrigger>> Item::triggers() const
    {
        return _triggers;
    }

    DirectX::SimpleMath::Vector3 Item::position() const
    {
        return _position;
    }

    bool Item::visible() const
    {
        return _visible;
    }

    void Item::set_visible(bool value)
    {
        _visible = value;
    }

    bool is_mutant_egg(const Item& item)
    {
        return is_mutant_egg(item.type_id());
    }

    bool is_mutant_egg(uint32_t type_id)
    {
        return equals_any(type_id, 163, 181);
    }

    uint16_t mutant_egg_contents(const Item& item)
    {
        return mutant_egg_contents(item.activation_flags());
    }

    uint16_t mutant_egg_contents(uint16_t flags)
    {
        switch (flags)
        {
        case 1:
            return 21; // Shooter
        case 2:
            return 23; // Centaur
        case 4:
            return 34; // Torso
        case 8:
            return 22; // Mutant
        }
        return 20; // Winged
    }
}
