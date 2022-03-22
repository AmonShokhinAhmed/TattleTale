#ifndef TALE_KERNELS_RESOURCEKERNELS_RELATIONSHIP_H
#define TALE_KERNELS_RESOURCEKERNELS_RELATIONSHIP_H

#include "shared/kernels/resourcekernels/resource.hpp"

namespace tattletale
{
    enum class RelationshipType
    {
        kLove,
        kAttraction,
        kFriendship,
        kAnger,
        kProtective,
        kLast
    };
    /**
     * @brief Represents a Relationship an Actor has with another Actor
     *
     * Will be instantiated inside a two dimensional map connecting different other \link Actor Actors \endlink to
     * \link RelationshipType RelationshipTypes \endlink and corresponding \link Relationship Relationships \endlink for each Actor.
     *
     */
    class Relationship : public Resource
    {
    public:
        /**
         * @brief Converts a string to an RelationshipType.
         *
         * Uses the same string values RelationshipTypeToString returns.
         * So valid strings are: love", "attraction", "friendship", "anger" and "protective".
         * Everything else just return RelationshipType::kLast. (But an assert will crash the Application in Debug mode.)
         *
         * @param relationship_string The string we want to convert.
         * @return The corresponding RelationshipType value.
         */
        static RelationshipType StringToRelationshipType(std::string relationship_string);
        /**
         * @brief Converts an RelationshipType to a string for easy printing.
         *
         * Note that RelationshipType::kLast will assert false and crash in Debug mode.
         *
         * @param relationship_type The RelationshipType we want to convert.
         * @return The corresponding string.
         */
        static std::string RelationshipTypeToString(RelationshipType relationship_type);

        std::string GetDefaultDescription() const override;
        std::string GetDetailedDescription() const override;
        std::string GetPassiveDescription() const override;
        std::string GetActiveDescription() const override;

    private:
        /**
         * @brief Constructor initializing base Resource class and type_ member.
         *
         * This should only ever be called through the Chronicle.
         *
         * @param type What RelationshipType this Relationship is of.
         * @param id The index this Kernel holds in the Chronicle.
         * @param tick In which tick this Relationship was created.
         * @param owner The Actor which owns this Resource.
         * @param reasons What other \link Kernel Kernels \endlink led to this Relationship and its value.
         * @param value Value of the Relationship between -1.0 and 1.0.
         */
        Relationship(RelationshipType type, size_t id, size_t tick, Actor *owner, Actor *target, std::vector<Kernel *> reasons, float value);
        std::string GetAdjective() const override;
        /**
         * @brief The RelationshipType of this Relationship.
         */
        RelationshipType type_;
        const static inline std::vector<std::string> positive_name_variants_ =
            {"love for",
             "attraction for",
             "friendship for",
             "anger for",
             "protective of"};
        const static inline std::vector<std::string> negative_name_variants_ =
            {"hate for",
             "disgust for",
             "animosity for",
             "comfortable with",
             "power over for"};
        Actor *target_;
        friend class Chronicle;
    };

} // namespace tattletale
#endif // TALE_KERNELS_RESOURCEKERNELS_RELATIONSHIP_H