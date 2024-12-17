#include <iostream>

extern "C" float SinIntegral(float A, float B, float e);
extern "C" char* Translation(long x);

int main()
{
    int prog;
    while (true)
    {
        std::cout << "Input program code:\n 1 -> Calculate integral\n 2 -> Translation\n-1 -> Exit\n";
        std::cin >> prog;
        switch (prog)
        {
        case 1:
            std::cout << "Enter A, B and e: ";
            float A, B, e;
            std::cin >> A >> B >> e;

            std::cout << "Calculated integral: " << SinIntegral(A, B, e) << "\n\n";
            break;
        case 2:
            long x;
            std::cout << "Enter x: ";
            std::cin >> x;

            std::cout << "Translationed number: " << Translation(x) << "\n\n";
            break;
        default:
            std::cout << "Exit\n";
            return 0;
        }
    }
}