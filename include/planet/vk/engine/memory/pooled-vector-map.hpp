#pragma once


#include <map>
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
    template<typename K, typename V>
    class pooled_vector_map {
        std::map<K, std::vector<V>> storage;


      public:
        /// ### Queries
        bool empty() const noexcept { return storage.empty(); }
        bool contains(K const &key) const noexcept {
            return storage.find(key) != storage.end();
        }

        std::vector<V> const &get(K const &key) const noexcept {
            static std::vector<V> const empty;
            auto const it = storage.find(key);
            return it != storage.end() ? it->second : empty;
        }


        /// ### Mutation
        void push_back(K const &key, V value) {
            storage[key].push_back(std::move(value));
        }

        template<typename... Args>
        void emplace_back(K const &key, Args... args) {
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
    };


}
