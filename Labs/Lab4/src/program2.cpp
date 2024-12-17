#include <iostream>
#include <dlfcn.h>

int main()
{
    int prog = 1;
    int real = 1;
    void *lib = nullptr;

    typedef float (*IntFunc)(float, float, float);
    typedef char* (*TranslationFunc)(long);

    IntFunc SinIntegral;
    TranslationFunc Translation;

    // Initial library load
    lib = dlopen("./lib_pr2_1.so", RTLD_LAZY);
    if (!lib)
    {
        std::cerr << "Error loading initial library: " << dlerror() << std::endl;
        return 1;
    }
    std::cout << "Library is loaded\n";

    SinIntegral = (IntFunc)dlsym(lib, "SinIntegral");
    Translation = (TranslationFunc)dlsym(lib, "Translation");
    if (!SinIntegral || !Translation)
    {
        std::cerr << "Failed to load symbols: " << dlerror() << std::endl;
        dlclose(lib);
        return 1;
    }

    while (true)
    {
        std::cout << "Input program code:\n 0 -> Library switch\n 1 -> Calculate integral\n 2 -> Translation\n-1 -> Exit\n";
        std::cin >> prog;
        switch (prog)
        {
        case 0:
            dlclose(lib); // Close the current library
            if (real == 1)
            {
                lib = dlopen("./lib_pr2_2.so", RTLD_LAZY);
                real = 2;
            }
            else
            {
                lib = dlopen("./lib_pr2_1.so", RTLD_LAZY);
                real = 1;
            }

            if (!lib)
            { // Check for dlopen errors
                std::cerr << "Error loading library: " << dlerror() << std::endl;
                return 1;
            }
            //system("clear");
            std::cout << "Library switched succesfully!\n";

            // Reload symbols
            SinIntegral = (IntFunc)dlsym(lib, "SinIntegral");
            Translation = (TranslationFunc)dlsym(lib, "Translation");
            if (!SinIntegral || !Translation)
            {
                std::cerr << "Failed to load symbols: " << dlerror() << std::endl;
                dlclose(lib);
                return 1;
            }
            break;
        case 1:
            //system("clear");
            float A, B, e;
            std::cout << "Enter A, B and e: ";
            std::cin >> A >> B >> e;

            if (real == 1)
                std::cout << "Counting integral with rectangles\n";
            else
                std::cout << "Counting integral with trapeze\n";
            std::cout << "Integral: " << SinIntegral(A, B, e) << "\n\n";
            break;
        case 2:
            //system("clear");
            long x;
            std::cout << "Enter x: ";
            std::cin >> x;

            if (real == 1)
                std::cout << "Translationing to binary\n";
            else
                std::cout << "Translationing to trinity\n";
            std::cout << "Result is: " << Translation(x) << "\n\n";
            break;
        default:
            std::cout << "Exit\n";
            dlclose(lib);
            return 0;
        }
    }
}