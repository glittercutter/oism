#include "../OISMHandler.h"
#include "../OISMSimpleSerializer.h"

#include "TestUtils.h"

#include <iostream>
#include <string>
#include <chrono>


int main(int argc, char** argv)
{
    auto input = new oism::Handler(testutils::createWindow());
    input->serialize<oism::SimpleSerializer>("../");

    auto cbSharedPointer = input->callback("quit", [](){testutils::isRunning = false;});
    auto walkBinding = input->getBinding("walk");

    // Dummy bindings
    auto test0Binding = input->getBinding("test0");
    auto test1Binding = input->getBinding("test1");
    auto test2Binding = input->getBinding("test2");
    auto test3Binding = input->getBinding("test3");
    auto test4Binding = input->getBinding("test4");
    auto test5Binding = input->getBinding("test5");
    auto test6Binding = input->getBinding("test6");
    auto test7Binding = input->getBinding("test7");
    auto test8Binding = input->getBinding("test8");
    auto test9Binding = input->getBinding("test9");

    using namespace std::chrono;

    auto startTime = high_resolution_clock::now();
    unsigned iter = 0;

    while (testutils::isRunning)
    {
        input->update();
        ++iter;
    }

    auto diff = high_resolution_clock::now() - startTime;
    std::cout << duration_cast<nanoseconds>(diff).count() / (double)iter << " nanoseconds / iteration" << std::endl;
    
    // Cleanup
    delete input;
    testutils::destroyWindow();

    std::cout << std::endl << "Terminated normally" << std::endl;
    return 0;
}
