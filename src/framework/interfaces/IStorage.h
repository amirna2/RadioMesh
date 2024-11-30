#pragma once

#include <vector>
#include <common/inc/Options.h>

class IStorage {
public:
    virtual ~IStorage() = default;

    // Core operations
    virtual int read(const std::string& key, std::vector<byte>& data) = 0;
    virtual int write(const std::string& key, const std::vector<byte>& data) = 0;
    virtual int remove(const std::string& key) = 0;
    virtual bool exists(const std::string& key) = 0;

    // Storage management
    virtual int begin() = 0;
    virtual int end() = 0;
    virtual int clear() = 0;
    virtual size_t available() = 0;
    virtual bool isFull() = 0;
};
