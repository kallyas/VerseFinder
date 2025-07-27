#include "VerseFinderApp.h"

int main(int, char**) {
    VerseFinderApp app;
    if (!app.init()) return 1;
    app.run();
    return 0;
}