#include <iostream>
#include <string>

#include "../OISMHandler.h"
#include "../OISMSimpleSerializer.h"

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Need window handle" << std::endl;
        return 1;
    }
    int whnd = std::stod(argv[1]);

    auto& inputHandler = oism::Handler::getInstance();
    inputHandler.init<oism::SimpleSerializer>(whnd, "../");

    bool run = true;

    auto quitCb = OISM_NEW_CALLBACK("quit", CT_ON_POSITIVE,
        [&](){run = false;});

    while (run)
    {
        inputHandler.update();
    }

    return 0;
}
