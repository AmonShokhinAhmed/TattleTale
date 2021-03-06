#include "shared/kernels/resourcekernels/relationship.hpp"
#include <assert.h>
#include <iostream>
#include <fmt/core.h>
#include "shared/actor.hpp"

namespace tattletale
{
    Relationship::Relationship(RelationshipType type, size_t id, size_t tick, Actor *owner, Actor *target, std::vector<Kernel *> reasons, float value)
        : Resource(
              fmt::format("{}", type),
              Relationship::positive_name_variants_[static_cast<int>(type)],
              Relationship::negative_name_variants_[static_cast<int>(type)],
              id,
              tick,
              owner,
              reasons,
              value,
              KernelType::kRelationship,
              Verb("felt", "feeling", "feel")),
          target_(target),
          type_(type){};

    RelationshipType Relationship::StringToRelationshipType(std::string relationship_string)
    {
        if (relationship_string == "love")
            return RelationshipType::kLove;
        if (relationship_string == "attraction")
            return RelationshipType::kAttraction;
        if (relationship_string == "friendship")
            return RelationshipType::kFriendship;
        if (relationship_string == "anger")
            return RelationshipType::kAnger;
        if (relationship_string == "protective")
            return RelationshipType::kProtective;

        TATTLETALE_ERROR_PRINT(true, "Invalid Relationship string was passed");
        return RelationshipType::kLast;
    }

    std::string Relationship::GetAdjective() const
    {
        if ((type_ == RelationshipType::kProtective) || (type_ == RelationshipType::kAnger && value_ < 0))
        {
            return Resource::GetAdjective();
        }
        else
        {
            if (abs(value_) < 0.3f)
            {
                return "slight";
            }
            else if (abs(value_) < 0.6f)
            {
                return "";
            }
            else if (abs(value_) < 1.0f)
            {
                return "a lot of";
            }
            return "all encompassing";
        }
    }

    std::string Relationship::GetDefaultDescription() const
    {
        std::string default_description = Resource::GetDefaultDescription();
        return fmt::format("{} {}", default_description, *target_);
    }
    std::string Relationship::GetDetailedDescription() const
    {
        std::string detailed_description = Resource::GetDetailedDescription();
        return fmt::format("{} targeting {}", detailed_description, *target_);
    }
    std::string Relationship::GetPassiveDescription() const
    {
        std::string passive_description = Resource::GetPassiveDescription();
        return fmt::format("{} {}", passive_description, *target_);
    }
    std::string Relationship::GetActiveDescription() const
    {
        std::string active_description = Resource::GetActiveDescription();
        return fmt::format("{} {}",active_description, *target_);
    }

    std::vector<Actor *> Relationship::GetAllParticipants() const
    {
        std::vector<Actor *> participants;
        participants.push_back(owner_);
        participants.push_back(target_);
        return participants;
    }

    float Relationship::CalculateChanceInfluence(const Interaction *interaction) const
    {
        const auto &participants = interaction->GetAllParticipants();
        int participant_index = -1;
        for (size_t i = 0; i < participants.size(); ++i)
        {
            if (participants[i]->id_ == target_->id_)
            {
                participant_index = i;
            }
        }
        // if not found or owner is meant
        if (participant_index <= 0)
        {
            return 0;
        }
        return (interaction->GetTendency()->relationships[participant_index - 1][static_cast<int>(type_)] * value_);
    }
    bool Relationship::IsSameSpecificType(Kernel *other) const
    {
        if (!IsSameKernelType(other))
        {
            return false;
        }
        return (dynamic_cast<Relationship *>(other)->type_ == type_);
    }

    RelationshipType Relationship::GetType() const
    {
        return type_;
    }
} // namespace tattletale