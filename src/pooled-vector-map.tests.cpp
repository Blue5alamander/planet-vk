#include <planet/vk/engine/memory/pooled-vector-map.hpp>

#include <felspar/memory/small_vector.hpp>
#include <felspar/test.hpp>

#include <unordered_map>


namespace {


    auto const s = felspar::testsuite("pooled-vector-map");


    auto const simple = s.test(
            "insert and retrieve",
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.emplace_back(1, 2.0f);
                check(m.non_empty_count()) == 1u;
                auto gen = m.non_empty_vectors();
                auto [key, vec] = *gen.next();
                check(key) == 1;
                check(vec.size()) == 2u;
                check(vec[0]) == 1.0f;
                check(vec[1]) == 2.0f;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<std::unordered_map<
                        int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.emplace_back(1, 2.0f);
                check(m.non_empty_count()) == 1u;
                auto gen = m.non_empty_vectors();
                auto [key, vec] = *gen.next();
                check(key) == 1;
                check(vec.size()) == 2u;
                check(vec[0]) == 1.0f;
                check(vec[1]) == 2.0f;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::unordered_map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.emplace_back(1, 2.0f);
                check(m.non_empty_count()) == 1u;
                auto gen = m.non_empty_vectors();
                auto [key, vec] = *gen.next();
                check(key) == 1;
                check(vec.size()) == 2u;
                check(vec[0]) == 1.0f;
                check(vec[1]) == 2.0f;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.emplace_back(1, 2.0f);
                check(m.non_empty_count()) == 1u;
                auto gen = m.non_empty_vectors();
                auto [key, vec] = *gen.next();
                check(key) == 1;
                check(vec.size()) == 2u;
                check(vec[0]) == 1.0f;
                check(vec[1]) == 2.0f;
            });


    auto const capacity = s.test(
            "clear preserves capacity",
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(1, 2.0f);
                check(m.non_empty_count()) == 1u;
                m.clear();
                check(m.non_empty_count()) == 0u;
                // First clear - vectors emptied, keys remain
                auto gen = m.non_empty_vectors();
                check(gen.next()).is_falsey(); // No non-empty vectors to iterate
                check(m.contains(1)) == true;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<std::unordered_map<
                        int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(1, 2.0f);
                check(m.non_empty_count()) == 1u;
                m.clear();
                check(m.non_empty_count()) == 0u;
                // First clear - vectors emptied, keys remain
                auto gen = m.non_empty_vectors();
                check(gen.next()).is_falsey(); // No non-empty vectors to iterate
                check(m.contains(1)) == true;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::unordered_map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(1, 2.0f);
                check(m.non_empty_count()) == 1u;
                m.clear();
                check(m.non_empty_count()) == 0u;
                // First clear - vectors emptied, keys remain
                auto gen = m.non_empty_vectors();
                check(gen.next()).is_falsey(); // No non-empty vectors to iterate
                check(m.contains(1)) == true;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(1, 2.0f);
                check(m.non_empty_count()) == 1u;
                m.clear();
                check(m.non_empty_count()) == 0u;
                // First clear - vectors emptied, keys remain
                auto gen = m.non_empty_vectors();
                check(gen.next()).is_falsey(); // No non-empty vectors to iterate
                check(m.contains(1)) == true;
            });


    auto const remove_empty = s.test(
            "clear removes empty keys",
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                check(m.non_empty_count()) == 1u;
                m.clear(); // First clear - vector emptied, key remains
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == true;
                m.clear(); // Second clear - empty key removed
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == false;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<std::unordered_map<
                        int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                check(m.non_empty_count()) == 1u;
                m.clear(); // First clear - vector emptied, key remains
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == true;
                m.clear(); // Second clear - empty key removed
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == false;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::unordered_map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                check(m.non_empty_count()) == 1u;
                m.clear(); // First clear - vector emptied, key remains
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == true;
                m.clear(); // Second clear - empty key removed
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == false;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                check(m.non_empty_count()) == 1u;
                m.clear(); // First clear - vector emptied, key remains
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == true;
                m.clear(); // Second clear - empty key removed
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == false;
            });


    auto const multiple = s.test(
            "multiple keys",
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.emplace_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;
                std::size_t count = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    if (key == 1) {
                        check(vec[0]) == 1.0f;
                    } else {
                        check(key) == 2;
                        check(vec[0]) == 2.0f;
                    }
                    ++count;
                }
                check(count) == 2u;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<std::unordered_map<
                        int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.emplace_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;
                std::size_t count = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    if (key == 1) {
                        check(vec[0]) == 1.0f;
                    } else {
                        check(key) == 2;
                        check(vec[0]) == 2.0f;
                    }
                    ++count;
                }
                check(count) == 2u;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::unordered_map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.emplace_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;
                std::size_t count = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    if (key == 1) {
                        check(vec[0]) == 1.0f;
                    } else {
                        check(key) == 2;
                        check(vec[0]) == 2.0f;
                    }
                    ++count;
                }
                check(count) == 2u;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.emplace_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;
                std::size_t count = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    if (key == 1) {
                        check(vec[0]) == 1.0f;
                    } else {
                        check(key) == 2;
                        check(vec[0]) == 2.0f;
                    }
                    ++count;
                }
                check(count) == 2u;
            });


    auto const empty_map = s.test(
            "clear empty map",
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, std::vector<float>>>
                        m;
                check(m.non_empty_count()) == 0u;
                m.clear();
                check(m.non_empty_count()) == 0u;
                check(m.empty()) == true;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<std::unordered_map<
                        int, felspar::memory::small_vector<float>>>
                        m;
                check(m.non_empty_count()) == 0u;
                m.clear();
                check(m.non_empty_count()) == 0u;
                check(m.empty()) == true;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::unordered_map<int, std::vector<float>>>
                        m;
                check(m.non_empty_count()) == 0u;
                m.clear();
                check(m.non_empty_count()) == 0u;
                check(m.empty()) == true;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, felspar::memory::small_vector<float>>>
                        m;
                check(m.non_empty_count()) == 0u;
                m.clear();
                check(m.non_empty_count()) == 0u;
                check(m.empty()) == true;
            });


    auto const iterate_all = s.test(
            "iterate over non-empty vectors",
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                m.push_back(3, 3.0f);
                check(m.non_empty_count()) == 3u;

                std::size_t count = 0;
                float sum = 0.0f;
                int key_sum = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    sum += vec[0];
                    key_sum += key;
                    ++count;
                }
                check(count) == 3u;
                check(sum) == 6.0f;
                check(key_sum) == 6;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<std::unordered_map<
                        int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                m.push_back(3, 3.0f);
                check(m.non_empty_count()) == 3u;

                std::size_t count = 0;
                float sum = 0.0f;
                int key_sum = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    sum += vec[0];
                    key_sum += key;
                    ++count;
                }
                check(count) == 3u;
                check(sum) == 6.0f;
                check(key_sum) == 6;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::unordered_map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                m.push_back(3, 3.0f);
                check(m.non_empty_count()) == 3u;

                std::size_t count = 0;
                float sum = 0.0f;
                int key_sum = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    sum += vec[0];
                    key_sum += key;
                    ++count;
                }
                check(count) == 3u;
                check(sum) == 6.0f;
                check(key_sum) == 6;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                m.push_back(3, 3.0f);
                check(m.non_empty_count()) == 3u;

                std::size_t count = 0;
                float sum = 0.0f;
                int key_sum = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    sum += vec[0];
                    key_sum += key;
                    ++count;
                }
                check(count) == 3u;
                check(sum) == 6.0f;
                check(key_sum) == 6;
            });


    auto const iterate_skip_empty = s.test(
            "iteration skips empty vectors",
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                m.push_back(3, 3.0f);
                check(m.non_empty_count()) == 3u;
                m.clear(); // First clear - vectors emptied, keys remain
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == true;
                check(m.contains(2)) == true;
                check(m.contains(3)) == true;
                // Push new values into different keys
                m.push_back(4, 4.0f);
                m.push_back(5, 5.0f);
                check(m.non_empty_count()) == 2u;

                std::size_t count = 0;
                int key_sum = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    key_sum += key;
                    ++count;
                }
                check(count) == 2u; // Only keys 4 and 5 should be iterated
                check(key_sum) == 9; // 4 + 5
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<std::unordered_map<
                        int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                m.push_back(3, 3.0f);
                check(m.non_empty_count()) == 3u;
                m.clear(); // First clear - vectors emptied, keys remain
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == true;
                check(m.contains(2)) == true;
                check(m.contains(3)) == true;
                // Push new values into different keys
                m.push_back(4, 4.0f);
                m.push_back(5, 5.0f);
                check(m.non_empty_count()) == 2u;

                std::size_t count = 0;
                int key_sum = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    key_sum += key;
                    ++count;
                }
                check(count) == 2u; // Only keys 4 and 5 should be iterated
                check(key_sum) == 9; // 4 + 5
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::unordered_map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                m.push_back(3, 3.0f);
                check(m.non_empty_count()) == 3u;
                m.clear(); // First clear - vectors emptied, keys remain
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == true;
                check(m.contains(2)) == true;
                check(m.contains(3)) == true;
                // Push new values into different keys
                m.push_back(4, 4.0f);
                m.push_back(5, 5.0f);
                check(m.non_empty_count()) == 2u;

                std::size_t count = 0;
                int key_sum = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    key_sum += key;
                    ++count;
                }
                check(count) == 2u; // Only keys 4 and 5 should be iterated
                check(key_sum) == 9; // 4 + 5
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                m.push_back(3, 3.0f);
                check(m.non_empty_count()) == 3u;
                m.clear(); // First clear - vectors emptied, keys remain
                check(m.non_empty_count()) == 0u;
                check(m.contains(1)) == true;
                check(m.contains(2)) == true;
                check(m.contains(3)) == true;
                // Push new values into different keys
                m.push_back(4, 4.0f);
                m.push_back(5, 5.0f);
                check(m.non_empty_count()) == 2u;

                std::size_t count = 0;
                int key_sum = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    check(vec.size()) == 1u;
                    key_sum += key;
                    ++count;
                }
                check(count) == 2u; // Only keys 4 and 5 should be iterated
                check(key_sum) == 9; // 4 + 5
            });


    auto const iterate_empty_map = s.test(
            "iteration on empty map yields nothing",
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, std::vector<float>>>
                        m;
                check(m.non_empty_count()) == 0u;

                bool yielded = false;
                for ([[maybe_unused]] auto [key, vec] : m.non_empty_vectors()) {
                    yielded = true;
                }
                check(yielded) == false;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<std::unordered_map<
                        int, felspar::memory::small_vector<float>>>
                        m;
                check(m.non_empty_count()) == 0u;

                bool yielded = false;
                for ([[maybe_unused]] auto [key, vec] : m.non_empty_vectors()) {
                    yielded = true;
                }
                check(yielded) == false;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::unordered_map<int, std::vector<float>>>
                        m;
                check(m.non_empty_count()) == 0u;

                bool yielded = false;
                for ([[maybe_unused]] auto [key, vec] : m.non_empty_vectors()) {
                    yielded = true;
                }
                check(yielded) == false;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, felspar::memory::small_vector<float>>>
                        m;
                check(m.non_empty_count()) == 0u;

                bool yielded = false;
                for ([[maybe_unused]] auto [key, vec] : m.non_empty_vectors()) {
                    yielded = true;
                }
                check(yielded) == false;
            });


    auto const iterate_after_double_clear = s.test(
            "iteration after double clear yields nothing",
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;
                m.clear(); // First clear - vectors emptied, keys remain
                check(m.non_empty_count()) == 0u;
                m.clear(); // Second clear - empty keys removed
                check(m.non_empty_count()) == 0u;

                bool yielded = false;
                for ([[maybe_unused]] auto [key, vec] : m.non_empty_vectors()) {
                    yielded = true;
                }
                check(yielded) == false;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<std::unordered_map<
                        int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;
                m.clear(); // First clear - vectors emptied, keys remain
                check(m.non_empty_count()) == 0u;
                m.clear(); // Second clear - empty keys removed
                check(m.non_empty_count()) == 0u;

                bool yielded = false;
                for ([[maybe_unused]] auto [key, vec] : m.non_empty_vectors()) {
                    yielded = true;
                }
                check(yielded) == false;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::unordered_map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;
                m.clear(); // First clear - vectors emptied, keys remain
                check(m.non_empty_count()) == 0u;
                m.clear(); // Second clear - empty keys removed
                check(m.non_empty_count()) == 0u;

                bool yielded = false;
                for ([[maybe_unused]] auto [key, vec] : m.non_empty_vectors()) {
                    yielded = true;
                }
                check(yielded) == false;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;
                m.clear(); // First clear - vectors emptied, keys remain
                check(m.non_empty_count()) == 0u;
                m.clear(); // Second clear - empty keys removed
                check(m.non_empty_count()) == 0u;

                bool yielded = false;
                for ([[maybe_unused]] auto [key, vec] : m.non_empty_vectors()) {
                    yielded = true;
                }
                check(yielded) == false;
            });


    auto const iterate_next_api = s.test(
            "iteration using next() API",
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;

                auto gen = m.non_empty_vectors();
                auto [key1, vec1] = *gen.next();
                check(key1) == 1;
                check(vec1[0]) == 1.0f;

                auto [key2, vec2] = *gen.next();
                check(key2) == 2;
                check(vec2.size()) == 1u;
                check(vec2[0]) == 2.0f;

                check(gen.next()).is_falsey(); // No more values
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<std::unordered_map<
                        int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;

                auto gen = m.non_empty_vectors();
                std::map<int, float> results;
                for (auto [key, vec] : gen) {
                    check(vec.size()) == 1u;
                    results[key] = vec[0];
                }
                check(results.size()) == 2u;
                check(results[1]) == 1.0f;
                check(results[2]) == 2.0f;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::unordered_map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;

                auto gen = m.non_empty_vectors();
                std::map<int, float> results;
                for (auto [key, vec] : gen) {
                    check(vec.size()) == 1u;
                    results[key] = vec[0];
                }
                check(results.size()) == 2u;
                check(results[1]) == 1.0f;
                check(results[2]) == 2.0f;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(2, 2.0f);
                check(m.non_empty_count()) == 2u;

                auto gen = m.non_empty_vectors();
                auto [key1, vec1] = *gen.next();
                check(key1) == 1;
                check(vec1[0]) == 1.0f;

                auto [key2, vec2] = *gen.next();
                check(key2) == 2;
                check(vec2.size()) == 1u;
                check(vec2[0]) == 2.0f;

                check(gen.next()).is_falsey(); // No more values
            });


    auto const iterate_multiple_values_per_key = s.test(
            "iteration with multiple values per key",
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(1, 1.5f);
                m.push_back(1, 2.0f);
                m.push_back(2, 3.0f);
                check(m.non_empty_count()) == 2u;

                std::size_t count = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    ++count;
                    if (key == 1) {
                        check(vec.size()) == 3u;
                    } else {
                        check(key) == 2;
                        check(vec.size()) == 1u;
                    }
                }
                check(count) == 2u;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<std::unordered_map<
                        int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(1, 1.5f);
                m.push_back(1, 2.0f);
                m.push_back(2, 3.0f);
                check(m.non_empty_count()) == 2u;

                std::size_t count = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    ++count;
                    if (key == 1) {
                        check(vec.size()) == 3u;
                    } else {
                        check(key) == 2;
                        check(vec.size()) == 1u;
                    }
                }
                check(count) == 2u;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::unordered_map<int, std::vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(1, 1.5f);
                m.push_back(1, 2.0f);
                m.push_back(2, 3.0f);
                check(m.non_empty_count()) == 2u;

                std::size_t count = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    ++count;
                    if (key == 1) {
                        check(vec.size()) == 3u;
                    } else {
                        check(key) == 2;
                        check(vec.size()) == 1u;
                    }
                }
                check(count) == 2u;
            },
            [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<
                        std::map<int, felspar::memory::small_vector<float>>>
                        m;
                m.push_back(1, 1.0f);
                m.push_back(1, 1.5f);
                m.push_back(1, 2.0f);
                m.push_back(2, 3.0f);
                check(m.non_empty_count()) == 2u;

                std::size_t count = 0;
                for (auto [key, vec] : m.non_empty_vectors()) {
                    ++count;
                    if (key == 1) {
                        check(vec.size()) == 3u;
                    } else {
                        check(key) == 2;
                        check(vec.size()) == 1u;
                    }
                }
                check(count) == 2u;
            });


}
