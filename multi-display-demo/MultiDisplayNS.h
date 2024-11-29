
#ifndef SURFACE_WRAPPER_H
#define SURFACE_WRAPPER_H

#include <gui/BLASTBufferQueue.h>
#include <gui/IGraphicBufferProducer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>
#include <system/window.h>
#include <utils/RefBase.h>

#include "DisplayDevice.h"

namespace android {

class MultiDisplayNS : public RefBase {
public:
    MultiDisplayNS(const String8& name, uint32_t layerStack=0);
    virtual ~MultiDisplayNS() {}

    virtual void onFirstRef();

    void getProducer(sp<IGraphicBufferProducer>& producer);
    void setBufferSize(uint32_t w, uint32_t h) { mBufferWidth=w; mBufferHeight=h; }
    void updateDisplay(DisplayDevice& other);
    void showSurfaceOnOtherDisplay(DisplayDevice& other);
    DisplayDevice& getCurrentDisplay() { return mCurrentDisplay; }
    ui::Size limitSurfaceSize(int width, int height) const;

private:
    DISALLOW_COPY_AND_ASSIGN(MultiDisplayNS);

    sp<BLASTBufferQueue> mBlastBufferQueue;
    sp<SurfaceControl> mSurface;
    sp<SurfaceComposerClient> mComposerClient;
    DisplayDevice mCurrentDisplay;
    uint32_t mBufferWidth;
    uint32_t mBufferHeight;
    String8 mName;
};

} // namespace android

#endif // SURFACE_WRAPPER_H
