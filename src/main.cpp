#include "defs/backend/engine.h"

#ifdef __EMSCRIPTEN__
#define SDL_MAIN_HANDLED
#endif
#include <SDL3/SDL_main.h>
#include <cstdlib>

auto main(void) noexcept -> int {
    backend::Engine engine{};

    engine.create();
    engine.run();
    engine.destroy();

    return EXIT_SUCCESS;
}
