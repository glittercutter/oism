#include "../OISMHandler.h"
#include "../OISMLog.h"
#include "../OISMSimpleSerializer.h"

#include "TestUtils.h"

#include <iostream>
#include <string>


int main(int argc, char** argv)
{
    // Set logger output
    oism::Logger::callback = [](const std::string& msg){std::cout<<"oism | "<<msg<<std::endl;};

    // Create handler & load mapping
    oism::Handler* input = new oism::Handler(testutils::createWindow());
    input->serialize<oism::SimpleSerializer>("../");

    // ===============
    // Callbacks
    // ===============
    
    // Callback are disabled when the share pointer is destroyed;
    oism::Bind::CallbackSharedPtr exit = input->callback("quit",
        [](){testutils::isRunning = false;});

    {
        // Callback is disable at the end of the scope
        auto tmp = input->callback("disabled",
            [](){std::cout<<"disabled!"<<std::endl;});
    }

    // Helper container to create and destroy callbacks
    oism::CallbackList cbList(input);
    cbList.add("toggle_exclusive", [&](){input->setExclusive(!input->isExclusive());});

    // =======
    // Bindings
    // =======

    // Binding are use if we need to check a value continuously.
    // They represent any device in the form of -1.0 to 1.0,
    // the value farthest from zero win.
    oism::Bind* walk = input->getBinding("walk");

    while (testutils::isRunning)
    {
        input->update();
        std::cout<<"walk value="<<walk->getValue();
        std::cout<<"                      \r"<<std::flush; // Keep the same output line
    }
    
    // Cleanup
    delete input;
    testutils::destroyWindow();

    std::cout << std::endl << "Terminated normally" << std::endl;
    return 0;
}
