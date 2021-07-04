#pragma once
#include <skepu3/backend/logging/termcolor.h>
#include <iostream>
#include <functional>
using namespace termcolor;

enum LogType 
{
    INFO,
    WARN,
    ERROR,
    DEBUG,
    TODO,
    NOTE
};

struct LOG
{
    std::function<void(void)> header;
    std::function<void(void)> footer = [] () {
        std::cout << termcolor::reset; 
    };

    LOG(LogType type) {
        switch (type)
        {
        case INFO:
            header = []() {
                std::cout << termcolor::on_green 
                          << termcolor::bold 
                          << "INFO:"
                          << termcolor::reset
                          << " "
                          << termcolor::green;
            };
            break;
        case WARN:
            header = []() {
                std::cout << termcolor::on_yellow 
                            << termcolor::bold 
                            << "WARNING:"
                            << termcolor::reset
                            << " "
                            << termcolor::yellow;
            };
            break;
        case ERROR:
            header = []() {
                std::cout << termcolor::on_red 
                            << termcolor::bold 
                            << "ERROR:"
                            << termcolor::reset
                            << " "
                            << termcolor::red;
            };
            break;
        case TODO:
            header = []() {
                std::cout << termcolor::on_bright_cyan 
                            << termcolor::bold 
                            << "TODO:"
                            << termcolor::reset
                            << " "
                            << termcolor::cyan;
            };
            break;
        case NOTE:
            /* code */
            break;

        default:
            break;
        }

    }

    template<typename T>
    std::ostream& operator<<(T something) &&
    { 
        header();
        std::cout << something;
        return std::cout; 
    }

    ~LOG() 
    {
        footer();
    }
};