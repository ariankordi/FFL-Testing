#include <RootTask.h>

#include <rio.h>

#ifdef __EMSCRIPTEN__
    #include <emscripten/emscripten.h>
    #include <gfx/lyr/rio_Renderer.h>
    #include <gfx/rio_Window.h>
    #include <task/rio_TaskMgr.h>
#endif

static const rio::InitializeArg cInitializeArg = {
    .window = {
#if RIO_IS_CAFE
        .width = 1280,
        .height = 720,
#else
        .width = 600,
        .height = 600,
#endif // RIO_IS_CAFE
#if RIO_IS_WIN
        .resizable = true
#endif // RIO_IS_WIN
    }
};


#ifdef __EMSCRIPTEN__
rio::Window* window;
void mainLoop()
{
    // Main loop iteration
    // Update the task manager
    rio::TaskMgr::instance()->calc();

    // Render
    rio::lyr::Renderer::instance()->render();

    // Swap the front and back buffers
    window->swapBuffers();
}
#endif

int main()
{
    // Initialize RIO with root task
    if (!rio::Initialize<RootTask>(cInitializeArg))
        return -1;
#ifdef __EMSCRIPTEN__
    // Get window instance
    window = rio::Window::instance();
    emscripten_set_main_loop(mainLoop, 0, 1);
#else
    // Main loop
    rio::EnterMainLoop();

    // Exit RIO
    rio::Exit();
#endif // __EMSCRIPTEN__
    return 0;
}
