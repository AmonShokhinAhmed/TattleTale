#ifndef TALE_GLOBALS_INTERACTIONSTORE_H
#define TALE_GLOBALS_INTERACTIONSTORE_H

#include <string>
#include <map>
#include <memory>
#include "tale/kernels/interactions/interaction.hpp"
#include "tale/kernels/interactions/interactionprototype.hpp"
#include "tale/kernels/interactions/interactionrequirement.hpp"
#include "tale/kernels/interactions/interactiontendency.hpp"
#include "tale/globals/random.hpp"
#include "tale/globals/chronicle.hpp"
#include "tale/talecore.hpp"
#include <nlohmann/json.hpp>
#include <fmt/core.h>

namespace tale
{
    class Actor;
    /**
     * @brief Stores Prototypes for all possible interactions.
     *
     * An Interaction should only be created through this class, so it's field are correctly filled.
     * Also contains helper function to get appropriate Interaction options for each situation
     */
    class InteractionStore
    {
    public:
        /**
         * @brief Default constructor populates catalogue.
         *
         * Reads from tale/resources/interactioncatalogue.json to get the data to build up the interaction prototype catalogue.
         *
         * @param random Reference to the Random object used by the simulation.
         */
        InteractionStore(Random &random, Chronicle &chronicle);
        /**
         * @brief Returns a random interactionPrototype index from the catalogue.
         *
         * @return A random interactionPrototype index
         */
        uint32_t GetRandomInteractionPrototypeIndex() const;
        /**
         * @brief Returns the name of the Interaction that would be created with the passed index.
         *
         * @param prototype_index The index of the corresponding InteractionPrototype.
         * @return The name of the Interaction.
         */
        std::string GetInteractionName(size_t prototype_index) const;
        /**
         * @brief Returns the participation count for a catalogued InteractionPrototype.
         *
         * Participation count is needed to know how many \link Actor Actors \endlink are necessary for creation of an
         * Interaction.
         * This will fail if the index is invalid.
         * @param prototype_index Index of the queried InteractionPrototype.
         * @return The participation count
         */
        const size_t &GetParticipantCount(size_t prototype_index) const;
        /**
         * @brief Returns the wealth effects for a catalogued InteractionPrototype.
         *
         * Should only really be needed for testing purposes
         * @param prototype_index Index of the queried InteractionPrototype.
         * @return The wealth effects
         */
        const std::vector<float> &GetWealthEffects(size_t prototype_index) const;
        /**
         * @brief Returns the emotion effects for a catalogued InteractionPrototype.
         *
         * Should only really be needed for testing purposes
         * @param prototype_index Index of the queried InteractionPrototype.
         * @return The emotion effects
         */
        const std::vector<std::map<EmotionType, float>> &GetEmotionEffects(size_t prototype_index) const;
        /**
         * @brief Returns the relationship effects for a catalogued InteractionPrototype.
         *
         * Should only really be needed for testing purposes
         * @param prototype_index Index of the queried InteractionPrototype.
         * @return The relationship effects
         */
        const std::vector<std::map<size_t, std::map<RelationshipType, float>>> &GetRelationshipEffects(size_t prototype_index) const;
        /**
         * @brief Creates an interaction from a catalogued InteractionPrototype.
         *
         * This is the only way an Interaction should be created. This ensures that the member variables of the Interaction is properly
         * populated.
         * @param prototype_index Index of the InteractionPrototype that should be used as a template
         * @param tick Tick this interaction was created
         * @param participants Vectors holding every Actor that was involved in the Interaction
         * @param reasons Vectors holding every Kernel responsible for the creation.
         * @return A pointer to the created Interaction
         */
        std::weak_ptr<Interaction> CreateInteraction(size_t prototype_index, size_t tick, std::vector<std::weak_ptr<Kernel>> reasons, std::vector<std::weak_ptr<Actor>> participants);
        /**
         * @brief Gettter for a Reference to the  Requirement catalogue.
         *
         * @return A Reference to the catalogue for the \link Requirement Requirements \endlink.
         */
        const std::vector<Requirement> &GetRequirementCatalogue() const;
        /**
         * @brief Gettter for a Reference to the Tendency catalogue.
         *
         * @return A Reference to the catalogue for the \link Tendency Tendencies \endlink.
         */
        const std::vector<Tendency> &GetTendencyCatalogue() const;

    private:
        /**
         * @brief Holds a reference to the Random object of the simulation.
         */
        Random &random_;
        /**
         * @brief Holds a reference to the Chronicle object of the simulation.
         */
        Chronicle &chronicle_;
        /**
         * @brief Holds all available \link InteractionPrototype InteractionPrototypes \endlink.
         */
        std::vector<InteractionPrototype> prototype_catalogue_;
        /**
         * @brief Holds the hard \link Requirement Requirements \endlink for all available Interaction prototypes.
         */
        std::vector<Requirement> requirements_catalogue_;
        /**
         * @brief Holds the hard \link Requirement Requirements \endlink for all available Interaction prototypes.
         */
        std::vector<Tendency> tendencies_catalogue_;
        /**
         * @brief Path to the json file where Interaction prototypes are defined.
         */
        std::string prototype_json_path_ = "tale/resources/interactioncatalogue.json";

        const std::string name_key_ = "name";
        const std::string participant_count_key_ = "participant_count";
        const std::string wealth_key_ = "wealth";
        const std::string emotion_key_ = "emotions";
        const std::vector<std::string> emotion_type_keys_ = {"happy", "calm", "satisfied", "brave", "extroverted"};
        const std::vector<std::string> context_type_keys_ = {"course", "freetime"};
        const std::string relationship_key_ = "relationships";
        const std::string participant_key_ = "participant";
        const std::string changes_key_ = "changes";
        const std::vector<std::string> relationship_type_keys_ = {"love", "attraction", "friendship", "anger", "protective"};
        const std::string context_key_ = "context";
        const std::string group_size_key_ = "group_size";
        bool ReadPrototypeJSON(nlohmann::json json, size_t participant_count, std::string error_preamble, InteractionPrototype &out_prototype);
        bool ReadRequirementJSON(nlohmann::json json, std::string error_preamble, Requirement &out_requirement);
        bool ReadTendencyJSON(nlohmann::json json, size_t participant_count, std::string error_preamble, Tendency &out_tendency);

        template <typename T1, nlohmann::detail::value_t T2>
        bool ReadJsonValueFromDictionary(T1 &out_value, nlohmann::json json, std::string key, bool required, std::string error_preamble = "")
        {
            if (!json.contains(key))
            {
                if (required)
                {
                    TALE_ERROR_PRINT(fmt::format("{}JSON DID NOT CONTAIN KEY {}", error_preamble, key));
                    return false;
                }
            }
            else
            {
                if (json[key].type() == T2)
                {
                    out_value = json[key];
                }
                else
                {
                    TALE_ERROR_PRINT(fmt::format("{}VALUE FOR KEY {} WAS OF AN INCORRECT TYPE", error_preamble, key));
                    return false;
                }
            }
            return true;
        }

        template <typename T1, nlohmann::detail::value_t T2>
        bool ReadJsonValueFromArray(T1 &out_value, nlohmann::json json, size_t index, bool required, std::string error_preamble = "")
        {
            if (json.size() <= index)
            {
                if (required)
                {
                    TALE_ERROR_PRINT(fmt::format("{}JSON DID NOT CONTAIN INDEX {}", error_preamble, index));
                    return false;
                }
            }
            else
            {
                if (json[index].type() == T2)
                {
                    out_value = json[index];
                }
                else
                {
                    TALE_ERROR_PRINT(fmt::format("{}VALUE FOR INDEX {} WAS OF AN INCORRECT TYPE", error_preamble, index));
                    return false;
                }
            }
            return true;
        }

        bool CheckCorrectValueRange(float value, std::string error_preamble = "");
    };
} // namespace tale
#endif // TALE_GLOBALS_INTERACTIONSTORE_H