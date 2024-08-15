#include "window.hpp"  
#include "libobsensor/hpp/Pipeline.hpp"  
#include "libobsensor/hpp/Error.hpp"  
#define STB_IMAGE_WRITE_IMPLEMENTATION 
#include "stb_image_write.h"  // Include the stb_image_write header  
#include <fstream> // for file operations  
#include <iostream> // for output  

int main(int argc, char **argv) try {  
    // Create a pipeline with default device  
    ob::Pipeline pipe;  

    // Configure which streams to enable or disable for the Pipeline by creating a Config  
    std::shared_ptr<ob::Config> config = std::make_shared<ob::Config>();  

    // Get the ir_left camera configuration list  
    auto irLeftProfiles = pipe.getStreamProfileList(OB_SENSOR_IR_LEFT);  

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
    auto irRightProfiles = pipe.getStreamProfileList(OB_SENSOR_IR_RIGHT);  

    // Open the default profile of IR_RIGHT Sensor  
    try {  
        auto irRightProfile = irRightProfiles->getProfile(OB_PROFILE_DEFAULT);  
        config->enableStream(irRightProfile->as<ob::VideoStreamProfile>());  
    }  
    catch (...) {  
        std::cout << "IR_Right stream not found!" << std::endl;  
    }  

    // Start the pipeline with config  
    pipe.start(config);  

    // Create a window for rendering  
    Window app("DoubleInfraredViewer", 1280, 800, RENDER_ONE_ROW);  

    bool saved = false; // Flag to check if the image has been saved  
    
    int frameCount = 0;  
    while (app) {  
        // Wait for frameset  
        auto frameSet = pipe.waitForFrames(100);  
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

        // Save the leftFrame image only once  
        if (!saved) {  
            // Prepare to save leftFrame data  
            auto frameData = leftFrame->metadata();   
            auto width = 1280;
            auto height = 800;
            size_t frameSize = leftFrame->metadataSize();  

            // Save the image as JPEG using stb_image_write  
            if (stbi_write_jpg("left_frame_image.jpg", width, height, 1, frameData, 100)) {  
                std::cout << "Saved left frame image to left_frame_image.jpg" << std::endl;  
                saved = true; // Mark as saved  
            } else {  
                std::cerr << "Failed to save the image" << std::endl;  
            }  
            break; // Exit the loop after saving the image  
        }  
    }  

    // Stop the pipeline  
    pipe.stop();  
    return 0;  

} catch (ob::Error &e) {  
    std::cerr << "function:" << e.getName() << "\nargs:" << e.getArgs() << "\nmessage:" << e.getMessage() << "\ntype:" << e.getExceptionType() << std::endl;  
    exit(EXIT_FAILURE);  
}