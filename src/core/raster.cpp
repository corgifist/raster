#include "raster.h"
#include "app/app.h"

int main(int argc, char** argv) {
    Raster::App::Initialize();
    Raster::App::RenderLoop();
    Raster::App::Terminate();
    return 0;
}