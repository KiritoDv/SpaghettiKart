#pragma once

#include "nlohmann/json.hpp"
#include <cpr/response.h>
#include <functional>
#include <memory>
#include "SatellaTypes.h"
#include "controller_pak/SatellaPak.h"

using json = nlohmann::json;

template<typename T>
using Callback = std::function<void(const T&)>;

typedef Callback<SatellaResponse> DefaultCallback;

class SatellaApi {
public:
    SatellaApi() = default;
    ~SatellaApi() = default;

    std::string GetAuthURL();
    void LinkAccount(std::string linkCode, DeviceType device, DefaultCallback callback);
    void SyncUser(DefaultCallback callback);
    void Logout(DefaultCallback callback);
    void DownloadAvatar(const User& user);

    void GetFriends(DefaultCallback callback);
    void SearchFriends(const std::string& query, Callback<std::vector<User>> callback);
    void AddFriend(User& user, DefaultCallback callback);
    void RemoveFriend(const std::string& friendId, DefaultCallback callback);
    void ModifyFriendRequest(const std::string& friendId, bool accept, DefaultCallback callback);

    void ListPaks(DefaultCallback callback);
    void CreatePak(DefaultCallback callback);
    void UploadPak(const std::string& pakId, DefaultCallback callback);
    void UpdatePak(const VirtualControllerPak& pak, DefaultCallback callback);
    void InsertPak(const std::string& pakId, DefaultCallback callback);
    void DeletePak(const std::string& pakId, DefaultCallback callback);

    void LoadSession();
    void SaveSession();

    void EjectPak() {
        this->currentPak = nullptr;
    }

    std::shared_ptr<User> GetUser() const {
        return user;
    }

    std::shared_ptr<AuthSession> GetSession() const {
        return session;
    }

    std::shared_ptr<std::vector<User>> GetFriends() const {
        return friends;
    }

    std::shared_ptr<SatellaPakData> GetCurrentPak() const {
        return currentPak;
    }

    std::shared_ptr<std::vector<VirtualControllerPak>> GetPaks() const {
        return paks;
    }

protected:
    std::shared_ptr<User> user;
    std::shared_ptr<AuthSession> session;
    std::shared_ptr<std::vector<User>> friends;

    std::shared_ptr<SatellaPakData> currentPak;
    std::shared_ptr<std::vector<VirtualControllerPak>> paks;
};