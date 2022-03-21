#ifndef TALE_TRAIT_H
#define TALE_TRAIT_H

#include "shared/kernels/kernel.hpp"

namespace tattletale
{
    /**
     * @brief Represents the Traits an Actor has.
     *
     */
    class Trait : public Kernel
    {
    public:
        std::string GetDefaultDescription() const override;
        std::string GetPassiveDescription() const override;
        std::string GetActiveDescription() const override;

    private:
        Trait(std::string name, size_t id, size_t tick, Actor *owner, std::vector<Kernel *> reasons);
        friend class Chronicle;
    };

} // namespace tattletale
#endif // TALE_TRAIT_H