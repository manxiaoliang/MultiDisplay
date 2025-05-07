#define LOG_TAG "VirtualDisplayView"

#include <android-base/properties.h>
#include "VirtualDisplayView.h"

namespace android {

VirtualDisplayView::VirtualDisplayView(uint32_t srcDisplayLayerStack,uint32_t desDisplayLayerStack) 
    : mSrcDisplayLayerStack(srcDisplayLayerStack),mDesDisplayLayerStack(desDisplayLayerStack) {}

VirtualDisplayView::~VirtualDisplayView() {
    SurfaceComposerClient::destroyVirtualDisplay(mVirtualDisplay);
}

void VirtualDisplayView::onFirstRef() {



    const auto ids = SurfaceComposerClient::getPhysicalDisplayIds();
    mMainDisplay = SurfaceComposerClient::getPhysicalDisplayToken(ids.front());
    SurfaceComposerClient::getDisplayState(mMainDisplay, &mMainDisplayState);
    SurfaceComposerClient::getActiveDisplayMode(mMainDisplay, &mMainDisplayMode);

    // mVirtualDisplayWidth  = mMainDisplayMode.resolution.getWidth()/2;
    // mVirtualDisplayHeight = mMainDisplayMode.resolution.getHeight()/2;

    mVirtualDisplayWidth  = 1600;
    mVirtualDisplayHeight = 900;
    

    // 创建Surface用于显示虚拟屏幕的内容
    mSurfaceControl = SurfaceComposerClient::getDefault()->createSurface(String8("VirtualDisplay-Surface"), 
                                                                mVirtualDisplayWidth, 
                                                                mVirtualDisplayHeight,
                                                                PIXEL_FORMAT_RGBA_8888,
                                                                ISurfaceComposerClient::eFXSurfaceBufferState,
                                                                /*parent*/ nullptr);

    SurfaceComposerClient::Transaction()
            .setLayer(mSurfaceControl, std::numeric_limits<int32_t>::max())
            // .setLayerStack(mSurfaceControl, ui::DEFAULT_LAYER_STACK)
            .setLayerStack(mSurfaceControl, ui::LayerStack::fromValue(mDesDisplayLayerStack))
            .setPosition(mSurfaceControl, 0, 0)
            .show(mSurfaceControl)
            .apply();

    mVirtualDisplay =
            SurfaceComposerClient::createVirtualDisplay(std::string("Super-VirtualDisplay"), false /*secure*/);
    SurfaceComposerClient::Transaction t;
    t.setDisplaySurface(mVirtualDisplay, mSurfaceControl->getIGraphicBufferProducer());
    t.setDisplayLayerStack(mVirtualDisplay, ui::LayerStack::fromValue(mSrcDisplayLayerStack));
    t.setDisplayProjection(mVirtualDisplay, ui::ROTATION_0,
                        Rect(mMainDisplayState.layerStackSpaceRect), 
                        Rect(0, 0, mVirtualDisplayWidth, mVirtualDisplayHeight));
    t.apply(true);
    
}

} // namespace android
