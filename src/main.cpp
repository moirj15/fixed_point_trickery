#include <iostream>
#include <SDL2/SDL.h>
import renderer_init;

int main(int argc, char **argv)
{
  fpt::Window window = fpt::OpenWindow(1920, 1080, "fpt");

  fpt::VulkanApi vk     = fpt::InitVulkanApi(window);
  SDL_Event      e;
  bool           quit = false;
  while (!quit) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = true;
      } else if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
          // m_stopRendering = true;
        } else if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
          // m_stopRendering = false;
        }
      }

      // ImGui_ImplSDL2_ProcessEvent(&e);
    }
  }

  fpt::Destroy(vk);
  return 0;
}
