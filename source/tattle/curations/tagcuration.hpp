#ifndef TATTLE_CURATIONS_TAGCURATION_H
#define TATTLE_CURATIONS_TAGCURATION_H

#include "tattle/curations/curation.hpp"

namespace tattletale
{
    class TagCuration : public Curation
    {
    public:
        TagCuration(size_t max_chain_size);
        float CalculateScore(const std::vector<Kernel *> &chain) const override;
        Kernel *GetFirstNoteworthyEvent(const std::vector<Kernel *> &chain) const override;
        Kernel *GetSecondNoteworthyEvent(const std::vector<Kernel *> &chain) const override;
    };
} // namespace tattletale
#endif // TATTLE_CURATIONS_TAGCURATION_H