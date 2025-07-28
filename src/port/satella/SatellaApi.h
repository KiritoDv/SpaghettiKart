#pragma once

#include "nlohmann/json.hpp"
#include <cpr/response.h>
#include <functional>
#include <memory>
#include "SatellaTypes.h"

using json = nlohmann::json;

template<typename T>
using Callback = std::function<void(const T&)>;

typedef Callback<SatellaResponse> DefaultCallback;

class SatellaApi {
public:
    SatellaApi() = default;
    ~SatellaApi() = default;

    void LinkAccount(std::string linkCode, DeviceType device, DefaultCallback callback);
    void SyncUser(DefaultCallback callback);

    void DownloadAvatar(const User& user);

    void GetFriends(DefaultCallback callback);
    void SearchFriends(const std::string& query, Callback<std::vector<User>> callback);
    void AddFriend(const std::string& friendId, DefaultCallback callback);
    void RemoveFriend(const std::string& friendId, DefaultCallback callback);
    void ModifyFriendRequest(const std::string& friendId, bool accept, DefaultCallback callback);

    void LoadSession();

    std::shared_ptr<User> GetUser() const {
        return user;
    }

    std::shared_ptr<AuthSession> GetSession() const {
        return session;
    }

    std::shared_ptr<std::vector<User>> GetFriends() const {
        return friends;
    }

private:
    void SaveSession();

protected:
    std::shared_ptr<User> user;
    std::shared_ptr<AuthSession> session;
    std::shared_ptr<std::vector<User>> friends;
};