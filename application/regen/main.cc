/*
 * Copyright 2026 wtcat
 */

#define STB_IMAGE_IMPLEMENTATION
#include <thirdparty/stb/stb_image.h>

#include "application/regen/resource_file.h"

#include "base/bind.h"
#include "base/at_exit.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/linked_list.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/threading/thread.h"
#include "base/command_line.h"

#include "thirdparty/concurrentqueue/concurrentqueue.h"
#include "thirdparty/lz4/lib/lz4.h"


class RGBConvertor {
public:
    RGBConvertor(const std::string& s) : name_(s) {}
    bool ReadFrom(const FilePath& path, int format, ) {
        int width, height, channels;
        uint8_t* px = stbi_load(path.AsUTF8Unsafe().c_str(), &width, &height, &channels, 0);
        if (!px) {
            printf("stb error: %s", stbi_failure_reason());
            return false;
        }

        /*
         * 
         */
        Convert(px, w, h, channels, );

        stbi_image_free(px);
    }

protected:
    virtual size_t Convert(const uint8_t* px, int w, int h, int channels, int format, uint8_t* outpx) {
        if (channels == 4) {
            /* With alpha channel */

        }
    }
    virtual bool Compress(const uint8_t* src, size_t size, uint8_t *dst, size_t *dst_size) {
        int max_size = LZ4_compressBound(size);
        if (*dst_size < max_size)
            return false;

        int ret = LZ4_compress_default(src, dst, size, *dst_size);
        if (ret <= 0)
            return false;

        *dst_size = ret;
        return true;
    }

    const std::string& name() const {
        return name_;
    }

private:
    std::string name_;
};

class ResourceFile {

};

int main(int argc, char* argv[]) {
    base::AtExitManager atexit;

    if (CommandLine::Init(argc, argv)) {
        CommandLine* cmdline = CommandLine::ForCurrentProcess();
        if (cmdline->HasSwitch("help")) {
            printf(
                "lvsim "
                "[--dir=path]"
                "[--hres=value]"
                "[--vres=value]\n"

                "Options:\n"
                "  --dir      The root directory of input files\n"
                "  --hres     The horizontal resolution of window\n"
                "  --vres     The vertical resolution of window\n"
            );

            return 0;
        }

        FilePath dir(L"source");
        int hres = 480;
        int vres = 480;

        if (cmdline->HasSwitch("dir"))
            dir = cmdline->GetSwitchValuePath("dir");

        if (!file_util::PathExists(dir)) {
            printf("Not found path(%s)\n", dir.AsUTF8Unsafe().c_str());
            return -1;
        }

        if (cmdline->HasSwitch("hres"))
            hres = std::stoi(cmdline->GetSwitchValueASCII("hres"));

        if (cmdline->HasSwitch("vres"))
            vres = std::stoi(cmdline->GetSwitchValueASCII("vres"));

    }

    return 0;
}
