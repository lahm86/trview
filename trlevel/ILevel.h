#pragma once

#include <cstdint>
#include "trtypes.h"

namespace trlevel
{
    // Interface that defines a level.
    struct ILevel
    {
        virtual ~ILevel() = 0;

        // Gets the number of textiles in the level.
        // Returns: The number of textiles.
        virtual uint32_t num_textiles() const = 0;

        // Gets the 8 bit textile with the specified index.
        // Returns: The textile for this index.
        virtual tr_textile8 get_textile8(uint32_t index) const = 0;

        // Gets the 16 bit textile with the specified index.
        // Returns: The textile for this index.
        virtual tr_textile16 get_textile16(uint32_t index) const = 0;
    };
}