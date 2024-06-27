#include <RootTask.h>

#include <rio.h>
#include <gfx/rio_Window.h>

static const rio::InitializeArg cInitializeArg = {
    .window = {
        .width = 800,
        .height = 800,
#if RIO_IS_WIN
        .resizable = false,
        .gl_major = 3,
        .gl_minor = 3
#endif // RIO_IS_WIN
    }
};

int main()
{
    // Initialize RIO with root task
    if (!rio::Initialize<RootTask>(cInitializeArg))
        return -1;

    char* isServerOnly = getenv("SERVER_ONLY");

    // do not present to window when server only
    // may avoid an issue where, when window hasn't been updated
    // for a while (which it won't be if SERVER_ONLY is enabled
    // , since the program blocks on accept() all day)...
    // , the program may hang next time it calls swapBuffers()
    // due to a call to wl_display_dispatch_queue_pending() hanging
    if (isServerOnly)
    {
        rio::Window* window = rio::Window::instance();
        // Main loop
        while (window->isRunning())
        {
            // Update the task manager
            rio::TaskMgr::instance()->calc();

            // Render
            //lyr::Renderer::instance()->render();

            // Swap the front and back buffers
            //window->swapBuffers();
        }
    } else {
        // Main loop
        rio::EnterMainLoop();
    }

    // Exit RIO
    rio::Exit();

    return 0;
}
