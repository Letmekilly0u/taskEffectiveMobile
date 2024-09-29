#include <iostream>
#include <inttypes.h>
#include "drawBmp.h"
#include <algorithm> 

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage:start /d <exe filepath> drawBmp.exe <name.bmp" << "\n";
        return 1;
    }
    else
    {
        BMP bmp(argv[1]);
    }  
    system("pause");
}