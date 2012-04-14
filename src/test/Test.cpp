#include "../OISMHandler.h"
#include "../OISMLog.h"
#include "../OISMSimpleSerializer.h"

#include "TestUtils.h"

#include <iostream>
#include <string>


int main(int argc, char** argv)
{
    oism::Logger::callback = [](const std::string& msg){std::cout<<"oism | "<<msg<<std::endl;};

    auto input = new oism::Handler(testutils::createWindow());
    input->serialize<oism::SimpleSerializer>("../");

    // Callbacks are disabled when the shared pointer goes out of scope
    auto cbSharedPointer = input->callback("quit", [](){testutils::isRunning = false;});

    {
        // Callback is disabled at the end of this scope
        auto cbOutOfScope = input->callback("disabled",
            [](){
                std::cout << "this should be disabled!" << std::endl;
            });
    }

    auto cbToggleExclusive = input->callback("toggle_exclusive", [&]()
    {
        input->setExclusive(!input->getExclusive());
    });

    auto walkBinding = input->getBinding("walk");

    while (testutils::isRunning)
    {
        input->update();
        std::cout << "walk value=" << walkBinding->getValue();
        std::cout << "                      \r" << std::flush;
    }
    
    // Cleanup
    delete input;
    testutils::destroyWindow();

    std::cout << std::endl << "Terminated normally" << std::endl;
    return 0;
}
