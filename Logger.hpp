//
// Created by Carlo Ronconi on 15/07/23.
//

#ifndef DRONE_DELIVERY_LOGGER_HPP
#define DRONE_DELIVERY_LOGGER_HPP
#include <iostream>
#include <map>
#include <iomanip>

class Logger {
    std::ostream& stream;
    long lastLogTime;
    long logTime;

public:
    explicit Logger(std::ostream &stream, long logTime = 1.0) :
    stream(stream), lastLogTime(time(nullptr)), logTime(logTime) {}

    template<class T>
    void log(const std::map<std::string, T>& info,  std::function<std::string(T)> writer, int width = 20) {
        long currTime = time(nullptr);
        if (currTime - lastLogTime < logTime) return;
        lastLogTime = currTime;

        for (const auto& element : info) {
            stream << element.first << std::setw(width);
        }
        stream << "\n";

        for (const auto& element : info) {
            stream << writer(element.second) << std::setw(width);
        }
        stream << "\n";
    }
};

#endif //DRONE_DELIVERY_LOGGER_HPP
