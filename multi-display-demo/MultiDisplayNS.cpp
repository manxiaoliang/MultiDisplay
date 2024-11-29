

#define LOG_TAG "MultiDisplayNS"

#include <android-base/properties.h>
// #include <gui/ISurfaceComposerClient.h>
#include <gui/Surface.h>
#include <ui/DisplayState.h>
#include <utils/Log.h>

#include "MultiDisplayNS.h"

namespace android {

MultiDisplayNS::MultiDisplayNS(const String8& name, uint32_t layerStack) 
    : mCurrentDisplay{ui::LayerStack::fromValue(layerStack),0,0,0}, mName(name) {}

void MultiDisplayNS::onFirstRef() {
    mComposerClient = new SurfaceComposerClient;
    status_t err = mComposerClient->initCheck();
    if (err != NO_ERROR) {
        ALOGD("SurfaceComposerClient::initCheck error: %#x\n", err);
        return;
    }
    // get ID for any displays
    const std::vector<PhysicalDisplayId> ids = SurfaceComposerClient::getPhysicalDisplayIds();
    if (ids.empty()) {
        ALOGE("Failed to get ID for any displays\n");
        return;
    }

        // get display information
    sp<IBinder> displayToken = nullptr;
    for (const auto id : ids) {
        displayToken = SurfaceComposerClient::getPhysicalDisplayToken(id);
        if (displayToken != nullptr) {
            ui::DisplayState ds;
            err = SurfaceComposerClient::getDisplayState(displayToken, &ds);
            if(err == OK && ds.layerStack == mCurrentDisplay.layerStack) {
                break;
            }
        }
    }

    // default -- display 0 is used
    if (displayToken == nullptr) {
        displayToken = SurfaceComposerClient::getPhysicalDisplayToken(ids.front());
        // mLayerStack = 0;
        mCurrentDisplay.layerStack = ui::LayerStack::fromValue(0);
        if (displayToken == nullptr) {
            ALOGE("Failed to getPhysicalDisplayToken");
            return;
        }
    }

    ui::DisplayMode displayMode;
    const status_t error =
            SurfaceComposerClient::getActiveDisplayMode(displayToken, &displayMode);
    if (error != NO_ERROR)
        return;

    ui::Size resolution = displayMode.resolution;
    resolution = limitSurfaceSize(resolution.width, resolution.height);
    // create the native surface
    mSurface = mComposerClient->createSurface(mName, resolution.getWidth(), 
                                                     resolution.getHeight(), PIXEL_FORMAT_RGBA_8888,
                                                     ISurfaceComposerClient::eFXSurfaceBufferState,
                                                     /*parent*/ nullptr);

    SurfaceComposerClient::Transaction{}
            .setLayer(mSurface, std::numeric_limits<int32_t>::max())
            .setBackgroundColor(mSurface, half3{0, 0, 0}, 1.0f, ui::Dataspace::UNKNOWN) // black background
            .setAlpha(mSurface, 1.0f)
            .setLayerStack(mSurface, mCurrentDisplay.layerStack)
            .show(mSurface)
            .apply();

    mCurrentDisplay.width = resolution.getWidth();
    mCurrentDisplay.height = resolution.getHeight();

    // create BLASTBufferQueue instance 
    mBlastBufferQueue = new BLASTBufferQueue("DemoBBQ", mSurface, 
                                             resolution.getWidth(), resolution.getHeight(),
                                             PIXEL_FORMAT_RGBA_8888);
}


ui::Size MultiDisplayNS::limitSurfaceSize(int width, int height) const {
    ui::Size limited(width, height);
    bool wasLimited = false;
    const float aspectRatio = float(width) / float(height);

    int maxWidth = android::base::GetIntProperty("ro.surface_flinger.max_graphics_width", 0);
    int maxHeight = android::base::GetIntProperty("ro.surface_flinger.max_graphics_height", 0);

    if (maxWidth != 0 && width > maxWidth) {
        limited.height = maxWidth / aspectRatio;
        limited.width = maxWidth;
        wasLimited = true;
    }
    if (maxHeight != 0 && limited.height > maxHeight) {
        limited.height = maxHeight;
        limited.width = maxHeight * aspectRatio;
        wasLimited = true;
    }
    SLOGV_IF(wasLimited, "Surface size has been limited to [%dx%d] from [%dx%d]",
             limited.width, limited.height, width, height);
    return limited;
}

void MultiDisplayNS::getProducer(sp<IGraphicBufferProducer>& producer) {
    producer = mBlastBufferQueue->getIGraphicBufferProducer();
}

void MultiDisplayNS::updateDisplay(DisplayDevice& other) {
    SurfaceComposerClient::Transaction{}
            .setLayerStack(mSurface, other.layerStack)
            .setPosition(mSurface, 0, 0)
            .apply();

    memcpy(&mCurrentDisplay, &other, sizeof(mCurrentDisplay));
}

void MultiDisplayNS::showSurfaceOnOtherDisplay(DisplayDevice& other) {
    sp<SurfaceControl> mirrorSurface = mComposerClient->mirrorSurface(mSurface.get());
    if(mirrorSurface == nullptr) {
        fprintf(stderr, "ERROR: mirrorSurface fail!\n");
        return;
    }
    // must create a parent surface for mirrorSurface
    sp<SurfaceControl> parentSurface = mComposerClient->createSurface(String8("MirrorParent")
                                                                , 0 /*width*/, 0 /*height*/ 
                                                                , PIXEL_FORMAT_RGBA_8888
                                                                , ISurfaceComposerClient::eFXSurfaceContainer
                                                                , nullptr/*parent*/);

    SurfaceComposerClient::Transaction{}
            .setLayer(parentSurface, std::numeric_limits<int32_t>::max())
            .setLayerStack(parentSurface, other.layerStack)
            .show(parentSurface)
            .apply();

    SurfaceComposerClient::Transaction{}
            .reparent(mirrorSurface, parentSurface)
            .setLayer(mirrorSurface, std::numeric_limits<int32_t>::max())
            .setLayerStack(mirrorSurface, other.layerStack)
            .show(mirrorSurface)
            .apply();

    int count = 10;
    while(count-- > 0) {
        fprintf(stderr, "show surface on diffrent displays!\n");
        sleep(1);
    }
}

} // namespace android
