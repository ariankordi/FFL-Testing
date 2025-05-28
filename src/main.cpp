#include <RootTask.h>

#include <rio.h>
#include <gfx/rio_Window.h>

#include <string> // for parseArgv

#ifdef __EMSCRIPTEN__
    #include <emscripten/emscripten.h>
    #include <gfx/lyr/rio_Renderer.h>
    #include <gfx/rio_Window.h>
    #include <task/rio_TaskMgr.h>
#endif

static rio::InitializeArg initializeArg = {
    .window = {
        // square window
        .width = 600,
        .height = 600,
#if RIO_IS_WIN
        .resizable = true
#endif // RIO_IS_WIN
    },
    .primitive_renderer = {},
};


#ifdef __EMSCRIPTEN__
rio::Window* window = nullptr; // set in main()
void mainLoop()
{
    RIO_ASSERT(window != nullptr);
    // Main loop iteration
    // Update the task manager
    rio::TaskMgr::instance()->calc();

    // Render
    rio::lyr::Renderer::instance()->render();

    // Swap the front and back buffers
    window->swapBuffers();
}
#else
#include <csignal>
void handleSignal(int signum)
{
    if (signum == SIGINT)
    {
        RIO_LOG("SIGINT received, exiting RIO...");

        if (RootTask::sServerOnlyFlag != NULL)
        {
            rio::Exit(); // PROBABLY unsafe - returns right back to process
            return;
        }
#ifndef RIO_NO_GLFW_CALLS
        GLFWwindow* glfwWindow = rio::Window::instance()->getNativeWindow().getGLFWwindow();
        glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
#endif // RIO_NO_GLFW_CALLS
    }
}
#endif

#ifdef USE_SENTRY_DSN
#include <sentry.h>

void initializeSentry()
{
    // Set up Sentry.
#ifndef SENTRY_RELEASE
    #define SENTRY_RELEASE "(SENTRY_RELEASE not set)"
#endif
    RIO_LOG("Initializing Sentry with DSN: %s, release: %s\n", USE_SENTRY_DSN, SENTRY_RELEASE);

    sentry_options_t *options = sentry_options_new();
#if RIO_DEBUG
      sentry_options_set_debug(options, 1);
#endif
    sentry_options_set_dsn(options, USE_SENTRY_DSN);
    sentry_options_set_release(options, SENTRY_RELEASE);
    const int result = sentry_init(options);
    RIO_LOG("sentry_init result: %d\n", result);
}
#endif // USE_SENTRY_DSN

void parseArgv(int argc, char* argv[]); // forward decl

int main(int argc, char* argv[])
{
    // parse and pass arguments to RootTask
    parseArgv(argc, argv);

#ifdef __EMSCRIPTEN__
    window = rio::Window::instance(); // used in mainLoop
    emscripten_set_main_loop(mainLoop, 0, 1);
#endif

    // Initialize RIO, make window...
    if (!rio::Initialize<RootTask>(initializeArg))
        return -1; // 255

#ifdef USE_SENTRY_DSN
    initializeSentry();
#endif // USE_SENTRY_DSN

#ifndef __EMSCRIPTEN__
    // do not draw/present to window when server only

    // creates an invisible window whose buffers are
    // never swapped, and the event loop will block on
    // accept() all day causing the process to "sleep"

    if (RootTask::sServerOnlyFlag)
    {
        rio::Window* window = rio::Window::instance();

        // run event loop without lyr/swapBuffers
        while (window->isRunning())
            rio::TaskMgr::instance()->calc(); // event loop
            //window->swapBuffers(); // Not updating screen.

    }
    else
    {
        // Register SIGINT handler.
        std::signal(SIGINT, handleSignal);

        // Main loop
        rio::EnterMainLoop();
    }
    // !window->isRunning():

    // Exit RIO
    rio::Exit();
#endif // __EMSCRIPTEN__

#ifdef USE_SENTRY_DSN
    // make sure everything flushes
    sentry_shutdown();
#endif

    return 0; // EXIT_SUCCESS
}

// Parses command line arguments
// and sets values in RootTask.
void parseArgv(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i]; // use std::string for ==

        if (arg == "--server" || arg == "-s")
            RootTask::sServerOnlyFlag = argv[i];
            // note that if you do have a window and the
            // program blocks on accept(), next time it
            // calls swapBuffers() it may hang due on a
            // call to wl_display_dispatch_queue_pending()

        //else if (arg == "--no-spin" || arg == "-n")
        //    RootTask::sNoSpinFlag = argv[i];
        else if (
            (arg == "--port" || arg == "-p")
            && i + 1 < argc // get next argument
        )
            RootTask::sServerPort = argv[++i];

        else if ((arg == "--resource-path")
                && i + 1 < argc)
            RootTask::sResourceSearchPath = argv[++i];
        else if ((arg == "--resource-high")
                && i + 1 < argc)
            RootTask::sResourceHighPath = argv[++i];

        else if (arg == "--help" || arg == "-h")
        {
            RIO_LOG("Usage: %s [options]\n\n", argv[0]);
            //RIO_LOG("--no-spin, -n = Disable rotation or spinning for on-screen heads (demo only).");
            RIO_LOG("Options:\n");

            // server options
            RIO_LOG("  --server, -s = Run as a standalone server. Avoids maintaining window and uses minimal resources (pauses on accept())\n");
            RIO_LOG("  --port, -p <port> = Set TCP server port (host is always localhost)\n");

            // resource options
            RIO_LOG("  --resource-path <directory> = Set search path for FFLResMiddle.dat/FFLResHigh.dat.\n");
            RIO_LOG("  --resource-high <file> = Set path for high resource file (e.g., AFLResHigh_2_3.dat)\n");

            // show this help message
            RIO_LOG("  --help, -h = Show this help message.\n");
            exit(0);
        }
        else
            RIO_LOG("Unknown argument: \"%s\", ignoring.\n\n", argv[i]);
    }
}
