

#ifndef DISPLAY_DEVICE_H
#define DISPLAY_DEVICE_H

namespace android {

struct DisplayDevice {
    ui::LayerStack layerStack;
    //  ui::LayerStack layerStack = ui::LayerStack::fromValue(std::rand());
    int32_t width;
    int32_t height;
    float refreshRate;
};

} // namespace android

#endif // DISPLAY_DEVICE_H