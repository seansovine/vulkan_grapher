#ifndef SHADER_UTIL_H_
#define SHADER_UTIL_H_

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<char> loadShader(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::stringstream error{};
        error << "Unable to open SPIR-V file: " << filePath << std::endl;
        error << "Did you run compile.sh yet?" << std::endl;
        throw std::runtime_error(error.str());
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

#endif // SHADER_UTIL_H_
