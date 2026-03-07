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
     *
     * TODO How can we use this with `felspar::memory::small_vector`? Maybe `V`
     * should be the underlying vector type and not the value type for the
     * vector? If we're doing that, maybe there should be only one type and
     * that's the `map` type and we get the other types from that?
     */
    template<
            typename K,
            typename V,
            template<typename, typename...> typename Vector = std::vector,
            template<typename, typename, typename...> typename Map = std::map>
    class pooled_vector_map {
        Map<K, Vector<V>> storage;


      public:
        using iteration_value_type = std::pair<const K &, Vector<V> &>;


        /// ### Queries
        bool empty() const noexcept { return storage.empty(); }
        bool contains(K const &key) const noexcept {
            return storage.find(key) != storage.end();
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
