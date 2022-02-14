#include "tale/kernels/relationship.hpp"

#include <assert.h>
#include <iostream>
#include <fmt/core.h>

namespace tale
{
    Relationship::Relationship(std::string name, size_t tick, std::vector<std::weak_ptr<Kernel>> reasons, float value) : Kernel(name, tick, reasons), value_(value){};

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
        assert(false); // invalid string was passed
        return RelationshipType::kNone;
    }
    std::string Relationship::RelationshipTypeToString(RelationshipType relationship_type)
    {
        switch (relationship_type)
        {
        case RelationshipType::kNone:
            assert(false); // invalid enum was passed
            return "none";
            break;
        case RelationshipType::kLove:
            return "love";
            break;
        case RelationshipType::kAttraction:
            return "attraction";
            break;
        case RelationshipType::kFriendship:
            return "friendship";
            break;
        case RelationshipType::kAnger:
            return "anger";
            break;
        case RelationshipType::kProtective:
            return "protective";
            break;
        }
        return "none";
    }
    float Relationship::GetValue() const
    {
        return value_;
    }
    std::string Relationship::ToString()
    {
        return fmt::format("{} with value: {}", name_, value_);
    }
} // namespace tale