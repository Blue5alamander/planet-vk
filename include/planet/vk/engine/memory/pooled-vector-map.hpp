#pragma once


#include <felspar/coro/generator.hpp>
#include <map>
#include <utility>
#include <vector>


namespace planet::vk::engine::memory {


    /// ## Pooled map of vectors
    /**
     * This is intended for use in shaders where we often need to store a number
     * of items to be drawn against a key (i.e. a texture). Instead of having
     * each draw use its own texture, we want to pool them together here.
     *
     * When the pool is cleared it should first remove any keys whose vectors
     * are empty -- the assumption is that these relate to textures that are no
     * longer relevant. Any key whose vector has data in gets that vector
     * cleared. That way textures that are used in most frames will end up
     * requiring few, if any, allocations.
     */
    template<typename Map>
    class pooled_vector_map {
      public:
        using key_type = typename Map::key_type;
        using vector_type = typename Map::mapped_type;
        using value_type = typename vector_type::value_type;
        using iteration_value_type = std::pair<key_type const &, vector_type &>;


        /// ### Queries
        bool empty() const noexcept { return storage.empty(); }
        bool contains(key_type const &key) const noexcept {
            return storage.find(key) != storage.end();
        }
        std::size_t non_empty_count() const noexcept {
            std::size_t count = 0;
            for (auto const &[key, vec] : storage) {
                if (not vec.empty()) { ++count; }
            }
            return count;
        }


        /// #### Iteration over non-empty vectors
        felspar::coro::generator<iteration_value_type> non_empty_vectors()
        /**
         * Returns a generator that yields references to key-value pairs where
         * the vector is not empty. Empty vectors in the map are skipped.
         */
        {
            for (auto &[key, vec] : storage) {
                if (not vec.empty()) { co_yield {key, vec}; }
            }
        }


        /// ### Mutation
        void push_back(key_type const &key, value_type value) {
            storage[key].push_back(std::move(value));
        }

        template<typename... Args>
        void emplace_back(key_type const &key, Args... args) {
            storage[key].emplace_back(std::forward<Args>(args)...);
        }

        /// #### Clear the underlying map
        void clear() {
            auto it = storage.begin();
            while (it != storage.end()) {
                if (it->second.empty()) {
                    it = storage.erase(it);
                } else {
                    it->second.clear();
                    ++it;
                }
            }
        }


      private:
        Map storage;
    };


}
