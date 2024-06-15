#include <RootTask.h>

#include <rio.h>

static const rio::InitializeArg cInitializeArg = {
    .window = {
        .width = 800,
        .height = 800,
#if RIO_IS_WIN
        .resizable = false
#endif // RIO_IS_WIN
    }
};

int main()
{
    // Initialize RIO with root task
    if (!rio::Initialize<RootTask>(cInitializeArg))
        return -1;

    // Main loop
    rio::EnterMainLoop();

    // Exit RIO
    rio::Exit();

    return 0;
}
