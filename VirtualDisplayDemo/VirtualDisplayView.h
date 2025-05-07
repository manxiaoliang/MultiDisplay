
#ifndef SURFACE_WRAPPER_H
#define SURFACE_WRAPPER_H

#include <android/gui/ISurfaceComposerClient.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <gui/BLASTBufferQueue.h>
// #include <gui/Surface.h>
// #include <gui/SurfaceComposerClient.h>
#include <hardware/gralloc.h>
#include <ui/DisplayState.h>
#include <ui/GraphicBuffer.h>
#include <ui/DisplayState.h>
#include <utils/Log.h>

#include <gui/BLASTBufferQueue.h>
#include <gui/IGraphicBufferProducer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>
#include <system/window.h>
#include <utils/RefBase.h>

namespace android {

class VirtualDisplayView : public RefBase {
public:
VirtualDisplayView(uint32_t srcDisplayLayerStack, uint32_t desDisplayLayerStack);
    virtual ~VirtualDisplayView();

    virtual void onFirstRef();

private:
    DISALLOW_COPY_AND_ASSIGN(VirtualDisplayView);

    ui::DisplayState mMainDisplayState;
    ui::DisplayMode mMainDisplayMode;
    sp<IBinder> mMainDisplay;
    sp<IBinder> mVirtualDisplay;
    sp<SurfaceControl> mSurfaceControl;
    int mSrcDisplayLayerStack;
    int mDesDisplayLayerStack;

    uint32_t mVirtualDisplayWidth;
    uint32_t mVirtualDisplayHeight;
};

} // namespace android

#endif //
