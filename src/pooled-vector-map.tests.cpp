#include <planet/vk/engine/memory/pooled-vector-map.hpp>

#include <felspar/test.hpp>


namespace {


    auto const s = felspar::testsuite("pooled-vector-map");


    auto const simple = s.test("insert and retrieve", [](auto check) {
        planet::vk::engine::memory::pooled_vector_map<int, float> m;
        m.push_back(1, 1.0f);
        m.emplace_back(1, 2.0f);
        check(m.get(1).size()) == 2u;
        check(m.get(1)[0]) == 1.0f;
        check(m.get(1)[1]) == 2.0f;
    });


    auto const capacity = s.test("clear preserves capacity", [](auto check) {
        planet::vk::engine::memory::pooled_vector_map<int, float> m;
        m.push_back(1, 1.0f);
        m.push_back(1, 2.0f);
        auto const cap = m.get(1).capacity();
        m.clear();
        check(m.get(1).empty()) == true;
        check(m.get(1).capacity()) == cap;
    });


    auto const remove_empty =
            s.test("clear removes empty keys", [](auto check) {
                planet::vk::engine::memory::pooled_vector_map<int, float> m;
                m.push_back(1, 1.0f);
                m.clear(); // First clear - vector emptied, key remains
                check(m.contains(1)) == true;
                m.clear(); // Second clear - empty key removed
                check(m.contains(1)) == false;
            });


    auto const multiple = s.test("multiple keys", [](auto check) {
        planet::vk::engine::memory::pooled_vector_map<int, float> m;
        m.push_back(1, 1.0f);
        m.emplace_back(2, 2.0f);
        check(m.get(1).size()) == 1u;
        check(m.get(2).size()) == 1u;
        check(m.get(1)[0]) == 1.0f;
        check(m.get(2)[0]) == 2.0f;
    });


    auto const empty_map = s.test("clear empty map", [](auto check) {
        planet::vk::engine::memory::pooled_vector_map<int, float> m;
        m.clear();
        check(m.empty()) == true;
    });


}
