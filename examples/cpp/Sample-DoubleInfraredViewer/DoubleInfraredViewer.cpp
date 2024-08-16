#include "window.hpp"  
#include "libobsensor/hpp/Pipeline.hpp"  
#include "libobsensor/hpp/Error.hpp"  
#define STB_IMAGE_WRITE_IMPLEMENTATION 
#include "stb_image_write.h"  // Include the stb_image_write header  
#include <fstream> // for file operations  
#include <iostream> // for output  

std::shared_ptr<ob::Context>  ctx;
std::shared_ptr<ob::Device>   device;
std::recursive_mutex          deviceMutex;
std::shared_ptr<ob::Pipeline> pipeline;
// ob::Pipeline pipeline;

int32_t ir_exposure_value_ = 18000;
int32_t ir_gain_value_ = 15000;

void printfIrExposuer()
{
    int32_t irExposure = device->getIntProperty(OB_PROP_IR_EXPOSURE_INT);
    std::cerr << "IR exposuer value is " << irExposure << std::endl;
}

void printfIrGain()
{
    int32_t irGain = device->getIntProperty(OB_PROP_IR_GAIN_INT);
    std::cerr << "IR Gain value is " << irGain << std::endl;

}

void setIrExposuerGain(int32_t export_value, int32_t gain_value) {
  std::unique_lock<std::recursive_mutex> lk(deviceMutex);
  if (device) {
    try {
      // 首先关掉 AE
      device->setBoolProperty(OB_PROP_IR_AUTO_EXPOSURE_BOOL, false);
      device->setIntProperty(OB_PROP_IR_EXPOSURE_INT, export_value);
      device->setIntProperty(OB_PROP_IR_GAIN_INT, gain_value);
    } catch (ob::Error &e) {
      std::cerr << "IR setIrExposuerGain is not supported." << std::endl;
    }
  }
}

void switchLaserOff() {
    std::unique_lock<std::recursive_mutex> lk(deviceMutex);
    if(device) {
        try {
            if(device->isPropertySupported(OB_PROP_LASER_BOOL, OB_PERMISSION_READ)) {
                // bool value = device->getBoolProperty(OB_PROP_LASER_BOOL);
                bool value = true;
                if(device->isPropertySupported(OB_PROP_LASER_BOOL, OB_PERMISSION_WRITE)) {
                    device->setBoolProperty(OB_PROP_LASER_BOOL, !value);

                    // if(!value) {
                    //     std::cout << "laser turn on!" << std::endl;
                    // }
                    // else {
                    //     std::cout << "laser turn off!" << std::endl;
                    // }
                }
            }
            else {
                std::cerr << "Laser switch property is not supported." << std::endl;
            }
        }
        catch(ob::Error &e) {
            std::cerr << "Laser switch property is not supported." << std::endl;
        }
    }
}

void switchLDPOff() {
    std::unique_lock<std::recursive_mutex> lk(deviceMutex);
    if(device) {
        try {
            if(device->isPropertySupported(OB_PROP_LDP_BOOL, OB_PERMISSION_READ)) {
                // bool value = device->getBoolProperty(OB_PROP_LDP_BOOL);
                bool value = true;
                if(device->isPropertySupported(OB_PROP_LDP_BOOL, OB_PERMISSION_WRITE)) {
                    device->setBoolProperty(OB_PROP_LDP_BOOL, !value);

                    // if(!value) {
                    //     std::cout << "LDP turn on!" << std::endl;
                    // }
                    // else {
                    //     std::cout << "LDP turn off!" << std::endl;
                    // }
                    // std::cout << "Attention: For some models, it is require to restart depth stream after turn on/of LDP. Input \"stream\" command "
                    //              "to restart stream!"
                    //           << std::endl;
                }
            }
            else {
                std::cerr << "LDP switch property is not supported." << std::endl;
            }
        }
        catch(ob::Error &e) {
            std::cerr << "LDP switch property is not supported." << std::endl;
        }
    }
}

// Device connection callback
void handleDeviceConnected(std::shared_ptr<ob::DeviceList> devices) {
    // Get the number of connected devices
    if(devices->deviceCount() == 0) {
        return;
    }

    const auto deviceCount = devices->deviceCount();
    for(uint32_t i = 0; i < deviceCount; i++) {
        std::string deviceSN = devices->serialNumber(i);
        std::cout << "Found device connected, SN: " << deviceSN << std::endl;
    }

    std::unique_lock<std::recursive_mutex> lk(deviceMutex);
    if(!device) {
        // open default device (device index=0)
        device   = devices->getDevice(0);
        pipeline = std::make_shared<ob::Pipeline>(device);
        // pipeline = ob::Pipeline(device);
        std::cout << "Open device success, SN: " << devices->serialNumber(0) << std::endl;

        // // try to switch depth work mode
        // switchDepthWorkMode();

        // // try turn off hardware disparity to depth converter (switch to software d2d)
        // turnOffHwD2d();

        // // set depth unit
        // setDepthUnit();

        // // set depth value range
        // setDepthValueRange();

        // // set depth soft filter
        // setDepthSoftFilter();

        // start stream
        // startStream();
    }
}

// Device disconnect callback
void handleDeviceDisconnected(std::shared_ptr<ob::DeviceList> disconnectList) {
    std::string currentDevSn = "";
    {
        std::unique_lock<std::recursive_mutex> lk(deviceMutex);
        if(device) {
            std::shared_ptr<ob::DeviceInfo> devInfo = device->getDeviceInfo();
            currentDevSn                            = devInfo->serialNumber();
        }
    }
    const auto deviceCount = disconnectList->deviceCount();
    for(uint32_t i = 0; i < deviceCount; i++) {
        std::string deviceSN = disconnectList->serialNumber(i);
        std::cout << "Device disconnected, SN: " << deviceSN << std::endl;
        if(currentDevSn == deviceSN) {
            device.reset();    // release device
            pipeline.reset();  // release pipeline
            std::cout << "Current device disconnected" << std::endl;
        }
    }
}

int main(int argc, char **argv) try {  

        // Set log severity, disable log, please set OB_LOG_SEVERITY_OFF.
    ob::Context::setLoggerSeverity(OB_LOG_SEVERITY_ERROR);

    // Create ob:Context.
    ctx = std::make_shared<ob::Context>();

    // Register device callback
    ctx->setDeviceChangedCallback([](std::shared_ptr<ob::DeviceList> removedList, std::shared_ptr<ob::DeviceList> addedList) {
        handleDeviceDisconnected(removedList);
        handleDeviceConnected(addedList);
    });

    // Query the list of connected devices.
    std::shared_ptr<ob::DeviceList> devices = ctx->queryDeviceList();

    // Handle connected devices（and open one device）
    handleDeviceConnected(devices);

    if(!device) {
        std::cout << "Waiting for connect device...";
        while(!device) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // // Create a pipeline with default device  
    // ob::Pipeline pipeline;  

    // Configure which streams to enable or disable for the Pipeline by creating a Config  
    std::shared_ptr<ob::Config> config = std::make_shared<ob::Config>();  

    // Get the ir_left camera configuration list  
    auto irLeftProfiles = pipeline->getStreamProfileList(OB_SENSOR_IR_LEFT);  

    if (irLeftProfiles == nullptr) {  
        std::cerr << "The obtained IR_Left resolution list is NULL. For monocular structured light devices, try opening the IR data stream using the IR example. " << std::endl;  
        return 0;  
    }  

    // Open the default profile of IR_LEFT Sensor  
    try {  
        auto irLeftProfile = irLeftProfiles->getProfile(OB_PROFILE_DEFAULT);  
        config->enableStream(irLeftProfile->as<ob::VideoStreamProfile>());  
    }  
    catch (...) {  
        std::cout << "IR_Left stream not found!" << std::endl;  
    }  

    // Get the ir_right camera configuration list  
    auto irRightProfiles = pipeline->getStreamProfileList(OB_SENSOR_IR_RIGHT);  

    // Open the default profile of IR_RIGHT Sensor  
    try {  
        auto irRightProfile = irRightProfiles->getProfile(OB_PROFILE_DEFAULT);  
        config->enableStream(irRightProfile->as<ob::VideoStreamProfile>());  
    }  
    catch (...) {  
        std::cout << "IR_Right stream not found!" << std::endl;  
    }  

    // Start the pipeline with config  
    pipeline->start(config);  

    // Create a window for rendering  
    Window app("DoubleInfraredViewer", 1280, 800, RENDER_ONE_ROW);  
    bool saved = false; // Flag to check if the image has been saved  
    int frameCount = 0; 
    while (app) {  
        // Wait for frameset  
        setIrExposuerGain(ir_exposure_value_,ir_gain_value_);

        switchLaserOff();
        switchLDPOff();
        
        // printfIrExposuer();
        // printfIrGain();

        auto frameSet = pipeline->waitForFrames(100);  
        if (frameSet == nullptr) {  
            continue;  
        }  

        // Filter the first 5 frames of data, and save it after the data is stable  
        if(frameCount < 5) {  
            frameCount++;  
            continue;  
        }  

        // Get the data of left and right IR  
        auto leftFrame = frameSet->getFrame(OB_FRAME_IR_LEFT);  
        auto rightFrame = frameSet->getFrame(OB_FRAME_IR_RIGHT);  
        if (leftFrame == nullptr || rightFrame == nullptr) {  
            std::cout << "left ir frame or right ir frame is null." << std::endl;  
            continue;  
        }  
        
        // Render the frames in the window  
        app.addToRender({leftFrame, rightFrame});  

        // // Save the leftFrame image only once  
        // if (!saved) {  
        //     // Prepare to save leftFrame data  
        //     auto frameData = leftFrame->data();   
        //     auto width = 1280;
        //     auto height = 800;
        //     size_t frameSize = leftFrame->dataSize();  

        //     // Save the image as JPEG using stb_image_write  
        //     if (stbi_write_jpg("left_frame_image.jpg", width, height, 1, frameData, 100)) {  
        //         std::cout << "Saved left frame image to left_frame_image.jpg" << std::endl;  
        //         saved = true; // Mark as saved  
        //     } else {  
        //         std::cerr << "Failed to save the image" << std::endl;  
        //     }  
        //     break; // Exit the loop after saving the image  
        // }  
    }  

    // Stop the pipeline  
    pipeline->stop();  
    return 0;  

} catch (ob::Error &e) {  
    std::cerr << "function:" << e.getName() << "\nargs:" << e.getArgs() << "\nmessage:" << e.getMessage() << "\ntype:" << e.getExceptionType() << std::endl;  
    exit(EXIT_FAILURE);  
}