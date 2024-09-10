#pragma once


#include <planet/vk/colour.hpp>
#include <planet/vk/engine/forward.hpp>

#include <felspar/coro/task.hpp>


namespace planet::vk::engine {


    /// ## Draw a blank screen
    /**
     * This will last for a whole cycle of the render loop (currently 3 frames)
     * and will allow all frames queued by an old render loop to be retired
     * before it is destroyed and replaced with a new one
     */
    felspar::coro::task<void> blank(app &, renderer &, colour const &);


}
