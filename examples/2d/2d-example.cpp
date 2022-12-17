#include <planet/vk/engine2d.hpp>


int main(int const argc, char const *argv[]) {
    planet::vk::engine2d::app app{argc, argv, "2d example"};
    planet::vk::engine2d::renderer renderer{app};

    bool done = false;
    while (not done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) { done = true; }
            if (event.type == SDL_KEYDOWN
                && event.key.keysym.sym == SDLK_ESCAPE) {
                done = true;
            }
            if (event.type == SDL_WINDOWEVENT
                && event.window.event == SDL_WINDOWEVENT_CLOSE
                && event.window.windowID == SDL_GetWindowID(app.window.get())) {
                done = true;
            }
        }

        // Get an image from the swap chain
        uint32_t img_index = 0;
        planet::vk::worked(vkAcquireNextImageKHR(
                app.device.get(), renderer.swapchain.get(),
                std::numeric_limits<uint64_t>::max(),
                renderer.img_avail_semaphore.get(), VK_NULL_HANDLE,
                &img_index));

        // We need to wait for the image before we can run the commands to draw
        // to it, and signal the render finished one when we're done
        std::array<VkSemaphore, 1> const wait_semaphores = {
                renderer.img_avail_semaphore.get()};
        std::array<VkSemaphore, 1> const signal_semaphores = {
                renderer.render_finished_semaphore.get()};
        std::array<VkPipelineStageFlags, 1> const wait_stages = {
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
        std::array const fences{renderer.fence.get()};

        planet::vk::worked(
                vkResetFences(app.device.get(), fences.size(), fences.data()));

        std::array command_buffer{renderer.command_buffers[img_index].get()};
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = wait_semaphores.size();
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();
        submit_info.commandBufferCount = command_buffer.size();
        submit_info.pCommandBuffers = command_buffer.data();
        submit_info.signalSemaphoreCount = signal_semaphores.size();
        submit_info.pSignalSemaphores = signal_semaphores.data();
        planet::vk::worked(vkQueueSubmit(
                app.device.graphics_queue, 1, &submit_info,
                renderer.fence.get()));

        // Finally, present the updated image in the swap chain
        std::array<VkSwapchainKHR, 1> present_chain = {
                renderer.swapchain.get()};
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = signal_semaphores.size();
        present_info.pWaitSemaphores = signal_semaphores.data();
        present_info.swapchainCount = present_chain.size();
        present_info.pSwapchains = present_chain.data();
        present_info.pImageIndices = &img_index;
        planet::vk::worked(
                vkQueuePresentKHR(app.device.present_queue, &present_info));

        // Wait for the frame to finish
        planet::vk::worked(vkWaitForFences(
                app.device.get(), fences.size(), fences.data(), true,
                std::numeric_limits<uint64_t>::max()));
    }

    return 0;
}
