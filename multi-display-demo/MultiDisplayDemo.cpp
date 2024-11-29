
#define LOG_TAG "MultiDisplayDemo"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <hardware/gralloc.h>
#include <ui/GraphicBuffer.h>
#include <ui/DisplayState.h>
#include <utils/Log.h>

#include "MultiDisplayNS.h"
#include "DisplayDevice.h"

using namespace android;

const uint32_t BUFFER_WIDTH = 1080;
const uint32_t BUFFER_HEIGHT = 1920;

int mLayerStack = 0;
std::vector<DisplayDevice> mDisplayDevices;

void fillRGBA8Buffer(uint8_t* img, int width, int height, int stride, int r, int g, int b) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t* pixel = img + (4 * (y*stride + x));
            pixel[0] = r;
            pixel[1] = g;
            pixel[2] = b;
            pixel[3] = 0;
        }
    }
}

class RenderThread : public Thread
{
public:
    RenderThread(const sp<MultiDisplayNS> &wb)
            : Thread(false), mSurface(wb) {}
    virtual void onFirstRef() {
        run("RenderThread", PRIORITY_DISPLAY);
    }
    virtual bool threadLoop();
private:
    wp<MultiDisplayNS> mSurface;
};


bool RenderThread::threadLoop() {
    sp<MultiDisplayNS> nativeSurface = mSurface.promote();
    if (nativeSurface.get() == NULL) {
        return false;
    }
    
    status_t err = NO_ERROR;
    static int countFrame = 0;
    
    sp<IGraphicBufferProducer> igbProducer;
    nativeSurface->getProducer(igbProducer);
    IGraphicBufferProducer::QueueBufferOutput qbOutput;
    igbProducer->connect(new StubProducerListener, NATIVE_WINDOW_API_CPU, false, &qbOutput);
    igbProducer->setMaxDequeuedBufferCount(3);
    nativeSurface->setBufferSize(BUFFER_WIDTH, BUFFER_HEIGHT);
    
    int slot;
    sp<Fence> fence;
    sp<GraphicBuffer> buf;

    // 1. dequeue buffer
    igbProducer->dequeueBuffer(&slot, &fence, BUFFER_WIDTH, BUFFER_HEIGHT,
                                          PIXEL_FORMAT_RGBA_8888, GRALLOC_USAGE_SW_WRITE_OFTEN,
                                          nullptr, nullptr);
    igbProducer->requestBuffer(slot, &buf);

    int waitResult = fence->waitForever("wait_release_fence");
    if (waitResult != OK) {
        ALOGE("wait_release_fence: Fence::wait returned an error: %d", waitResult);
        return false;
    }

    // 2. fill the buffer with color
    uint8_t* img = nullptr;
    err = buf->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)(&img));
    if (err != NO_ERROR) {
        ALOGE("error: lock failed: %s (%d)", strerror(-err), -err);
        return false;
    }

    countFrame = (countFrame+1)%3;
    fillRGBA8Buffer(img, BUFFER_WIDTH, BUFFER_HEIGHT, buf->getStride(),
                    countFrame == 0 ? 255 : 0,
                    countFrame == 1 ? 255 : 0,
                    countFrame == 2 ? 255 : 0);

    err = buf->unlock();
    if (err != NO_ERROR) {
        ALOGE("error: unlock failed: %s (%d)", strerror(-err), -err);
        return false;
    }

    // 3. queue the buffer to display
    IGraphicBufferProducer::QueueBufferInput input(systemTime(), true /* autotimestamp */,
                                                   HAL_DATASPACE_UNKNOWN, {},
                                                   NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW, 0,
                                                   Fence::NO_FENCE);
    igbProducer->queueBuffer(slot, input, &qbOutput);

    sleep(1);

    return true; // loop until we need to quit
}

size_t getDisplayInfos() {
    const std::vector<PhysicalDisplayId> ids = SurfaceComposerClient::getPhysicalDisplayIds();
    const size_t size = ids.size();
    if (size == 0) {
        fprintf(stderr, "ERROR: there is no available display\n");
        return 0;
    }

    for (int i = 0; i < size; ++i) {
        const sp<IBinder> token = SurfaceComposerClient::getPhysicalDisplayToken(ids[i]);
        if (token == nullptr) {
            fprintf(stderr, "ERROR: no display\n");
            continue;
        }
        ui::DisplayState displayState;
        status_t err = SurfaceComposerClient::getDisplayState(token, &displayState);
        if (err != NO_ERROR) {
            fprintf(stderr, "ERROR: unable to get display state\n");
            continue;
        }

        ui::DisplayMode displayMode;
        const status_t error =
            SurfaceComposerClient::getActiveDisplayMode(token, &displayMode);
        if (error != NO_ERROR)
            continue;

        DisplayDevice display { displayState.layerStack
                              , displayMode.resolution.width
                              , displayMode.resolution.height
                              , displayMode.refreshRate };
        mDisplayDevices.push_back(display);
    }
    return size;
}

void dumpDisplayInfos() {
    fprintf(stderr, "Display Devices: size=%zu\n", mDisplayDevices.size());
    int i = 0;
    for(auto display : mDisplayDevices) {
        fprintf(stderr, "\t#%d display layerStack=%u activeMode={ %dx%d@%.2ffps }\n"
                                , i++
                                , display.layerStack.id
                                , display.width
                                , display.height
                                , display.refreshRate);
    }
}

DisplayDevice& getOtherDisplay(DisplayDevice& current) {
    for(auto &display : mDisplayDevices) {
        if(display.layerStack != current.layerStack)
            return display;
    }
    return current;
}

static void usage(const char *me)
{
    fprintf(stderr, "\nusage: \t%s [options]\n"
                    "\t----------------------------------------------------------------------------------\n"
                    "\t[-h] help\n"
                    "\t[-l] list displays\n"
                    "\t[-d] layer stack(In case of multi-display, show surface on the specified displays)\n"
                    "\n\tWhen demo is running, you can input a char to control surface\n"
                    "\t\ts: show surface on diffrent displays\n"
                    "\t----------------------------------------------------------------------------------\n",
                    me);
    exit(1);
}

void parseOptions(int argc, char **argv) {
    const char *me = argv[0];
    int res;
    while((res = getopt(argc, argv, "d:lh")) >= 0) {
        switch(res) {
            case 'd':
                mLayerStack = atoi(optarg);
                break;
            case 'l':
                dumpDisplayInfos();
                exit(0);
                break;
            case 'h':
            default:
            {
                usage(me);
            }
        }
    }
}

int main(int argc, char ** argv) {
    getDisplayInfos();
    parseOptions(argc, argv);

    sp<MultiDisplayNS> nativeSurface(new MultiDisplayNS(String8("MultiDisplayDemo"), mLayerStack));
    sp<RenderThread> renderThread = new RenderThread(nativeSurface);

    bool quit = false;
    while(renderThread->isRunning() && !quit) {
        char input = getchar();
        if(input == 'q')
            quit = true;
        if(input == 's') { // show surface on diffrent displays
            DisplayDevice& other = getOtherDisplay(nativeSurface->getCurrentDisplay());
            nativeSurface->showSurfaceOnOtherDisplay(other);
        }
    }
    renderThread->requestExitAndWait();

    return 0;
}
