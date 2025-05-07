#define LOG_TAG "VirtualDisplay"

#include "VirtualDisplayView.h"

using namespace android;

class VirtualDisplayThread : public Thread
{
public:
    VirtualDisplayThread(const sp<VirtualDisplayView> &wb)
            : Thread(false), mVirtualDisplay(wb) {}
    virtual void onFirstRef() {
        run("RenderThread", PRIORITY_DISPLAY);
    }
    virtual bool threadLoop();
private:
    wp<VirtualDisplayView> mVirtualDisplay;
};


bool VirtualDisplayThread::threadLoop() {
 
    return true; // loop until we need to quit
}

static void usage(const char *me)
{
    fprintf(stderr, "\nusage: \t%s [options]\n"
                    "\t------------- options ----------------\n"
                    "\t[-s] source display layerstack\n"
                    "\t[-d] destination display layerstack\n"
                    "\t[ps] VirtualDisplayDemo -s 0 -d 2\n"
                    "\n"
                    "\t q to exit VirtualDisplayDemo\n"
                    "\t--------------------------------------\n",
                    me);
    exit(1);
}

int mSrcDisplay = 0;
int mDesDisplay = 2;

void parseOptions(int argc, char **argv) {
    const char *me = argv[0];
    int res;
    while((res = getopt(argc, argv, "s:d:h")) >= 0) {
        switch(res) {
            case 's':
                mSrcDisplay = atoi(optarg);
                printf("opt is s, oprarg is: %s\n", optarg);
                break;
            case 'd':
                mDesDisplay = atoi(optarg);
                printf("opt is t, oprarg is: %s\n", optarg);
                break;
            case 'h':
            default:
            {
                usage(me);
            }
        }
    }
}

int main(int argc, char **argv) {
    if(argc == 1) usage(argv[0]);
    parseOptions(argc, argv);

    ALOGI("mSrcDisplayLayerStack: %d, mDesDisplayLayerStack: %d", mSrcDisplay,mDesDisplay);
    printf("mSrcDisplayLayerStack: %d, mDesDisplayLayerStack: %d", mSrcDisplay,mDesDisplay);
    
    sp<VirtualDisplayView> virtualDisplayView(new VirtualDisplayView(mSrcDisplay,mDesDisplay));
    sp<VirtualDisplayThread> virtualDisplayThread = new VirtualDisplayThread(virtualDisplayView);

    bool quit = false;
    while(virtualDisplayThread->isRunning() && !quit) {
        char input = getchar();
        if(input == 'q')
            quit = true;
    }
    virtualDisplayThread->requestExitAndWait();

    return 0;
}
