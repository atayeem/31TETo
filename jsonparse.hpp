#pragma once

#include "cJSON.h"
#include <filesystem>

class JSON {
private:
    cJSON *obj;
public:
    JSON(std::filesystem::path p);
};