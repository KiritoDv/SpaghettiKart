#pragma once

#include "nlohmann/json.hpp"
#include <cpr/response.h>
#include <functional>
#include <memory>
#include "SatellaTypes.h"

using json = nlohmann::json;

typedef std::function<void(const SatellaResponse&)> Callback;

class SatellaApi {
public:
    SatellaApi() = default;
    ~SatellaApi() = default;

    void LinkAccount(std::string linkCode, DeviceType device, Callback callback);
    void SyncUser(Callback callback);
    void LoadSession();

    std::shared_ptr<User> GetUser() const {
        return user;
    }

private:
    void SaveSession();

protected:
    std::shared_ptr<AuthSession> session;
    std::shared_ptr<User> user;
};