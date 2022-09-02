#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "../src/WindowInfoLoader.h"

#include <png.h>

static void write_image(const char *filename, int width, int height, int stride,
        bool y_invert, png_bytep data)
{
    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        fprintf(stderr, "failed to open output file\n");
        exit(EXIT_FAILURE);
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info  = png_create_info_struct(png);

    png_init_io(png, f);

    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_set_bgr(png);

    png_write_info(png, info);

    for (size_t i = 0; i < (size_t)height; ++i) {
        png_bytep row;
        if (y_invert) {
            row = data + (height - i - 1) * stride;
        } else {
            row = data + i * stride;
        }
        png_write_row(png, row);
    }

    png_write_end(png, NULL);

    png_destroy_write_struct(&png, &info);

    fclose(f);
}

class WindowLoader : public BufferHandler
{
private:
    virtual void onInited();
    virtual void bufferCallback(std::shared_ptr<WindowInfo> info) override;

    WindowInfoLoader *m_loader;
    int m_count = 0;

    int m_key = 0;

public:
    WindowLoader(int argc, char **argv);
    ~WindowLoader();

    int run(int key);
};

WindowLoader::WindowLoader(int argc, char **argv)
{
    m_loader = WindowInfoLoader::get(argc, argv);
    m_loader->addBufferHandler(this);
}

WindowLoader::~WindowLoader() {}

int WindowLoader::run(int key)
{
    m_key = key;
    return m_loader->run();
}

void WindowLoader::onInited()
{
    std::cout << "ut-gfx-demo:onInited window_screenshot key " << m_key << std::endl;
    switch (m_key) {
        case 0: {
            m_count = 0;
            m_loader->screenshot(1);
        } break;
        case 1: {
            m_count = 0;

            std::map<int, std::shared_ptr<WindowInfo>> windows =
                    m_loader->getWindowInfos();

            std::map<int, std::shared_ptr<WindowInfo>>::iterator it = windows.begin();
            for (it = windows.begin(); it != windows.end(); ++it) {
                m_loader->captureWindow(it->second);
            }
        } break;
        case 2: {
            m_count = 0;
            m_loader->startRecording();
        } break;
        default:
            break;
    }
}

void WindowLoader::bufferCallback(std::shared_ptr<WindowInfo> info)
{
    std::cout << "ut-gfx-demo:window_screenshot windowId " << info->windowId
              << " resourceName " << info->resourceName << " size " << info->x << "x"
              << info->y << " " << info->width << "x" << info->height << std::endl;
    std::string name("wayland-screenshot_");
    name += std::to_string(m_count++);
    name += ".png";
    write_image(name.c_str(), info->width, info->height, info->width * 4, false,
            (png_bytep)info->data);
}

void print_help(void)
{
    const char usage[] =
            "Usage: windowloader [OPTIONS] ...\n"
            "Get screen data or window data.\n"
            "  -o         screen shot one frame and quit\n"
            "  -r         record screen\n"
            "  -w         shot all windows and quit\n"
            "  -h         print help message and quit\n";
    std::cout << usage << std::endl;
}

int main(int argc, char **argv)
{
    int key              = 0;
    if (argc > 1) {
        if (strcmp(argv[1], "-o") == 0) {
            key = 0;
        } else if (strcmp(argv[1], "-w") == 0) {
            key = 1;
        } else if (strcmp(argv[1], "-r") == 0) {
            key = 2;
        } else {
            print_help();
            return 0;
        }
        std::cout << "ut-gfx-demo:window_screenshot 1 " << argv[1] << " key " << key
                  << std::endl;
    } else {
        print_help();
        return 0;
    }
    WindowLoader *loader = new WindowLoader(argc, argv);
    return loader->run(key);
}
