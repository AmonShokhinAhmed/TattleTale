#ifndef TALE_TRAIT_H
#define TALE_TRAIT_H

#include "tale/kernels/kernel.hpp"

namespace tale
{
    /**
     * @brief Represents the Traits an Actor has.
     * 
     */
    class Trait : Kernel
    {
    public:
        Trait(std::vector<std::weak_ptr<Kernel>> reasons);
        std::string ToString();
    };

} // namespace tale
#endif // TALE_TRAIT_H