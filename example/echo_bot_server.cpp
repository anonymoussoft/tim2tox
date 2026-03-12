// V2TIM-based echo bot server
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>

#include <V2TIMManager.h>
#include <V2TIMListener.h>
#include <V2TIMMessage.h>
#include <V2TIMCallback.h>
#include <V2TIMFriendship.h>
#include <V2TIMFriendshipManager.h>

class SDKListener : public V2TIMSDKListener {
public:
    void OnConnectSuccess() override {
        printf("Server: Online\n");
        fflush(stdout);
    }
    void OnConnectFailed(int error_code, const V2TIMString &error_message) override {
        printf("Server: Connect failed %d: %s\n", error_code, error_message.CString());
        fflush(stdout);
    }
    void OnUserStatusChanged(const V2TIMUserStatusVector &userStatusList) override {
        for (size_t i = 0; i < userStatusList.Size(); ++i) {
            const auto &s = userStatusList[i];
            printf("Server: User %s status changed to %d\n", s.userID.CString(), (int)s.statusType);
        }
        fflush(stdout);
    }
};

class SimpleMsgListener : public V2TIMSimpleMsgListener {
public:
    void OnRecvC2CTextMessage(const V2TIMString &msgID, const V2TIMUserFullInfo &sender,
                              const V2TIMString &text) override {
        // Echo back
        struct NoopSendCb : public V2TIMSendCallback {
            void OnSuccess(const V2TIMMessage&) override {}
            void OnError(int, const V2TIMString&) override {}
            void OnProgress(uint32_t) override {}
        } cb;
        V2TIMManager::GetInstance()->SendC2CTextMessage(text, sender.userID, &cb);
        static unsigned long long counter = 0;
        counter++;
        printf("Message echoed back to friend %s (total echoes: %llu)\n", sender.userID.CString(), counter);
        fflush(stdout);
    }
    void OnRecvC2CCustomMessage(const V2TIMString &msgID, const V2TIMUserFullInfo &sender,
                                const V2TIMBuffer &customData) override {
        // Echo back custom as text-friendly fallback
        struct NoopSendCb : public V2TIMSendCallback {
            void OnSuccess(const V2TIMMessage&) override {}
            void OnError(int, const V2TIMString&) override {}
            void OnProgress(uint32_t) override {}
        } cb;
        V2TIMManager::GetInstance()->SendC2CCustomMessage(customData, sender.userID, &cb);
    }
};

class FriendListener : public V2TIMFriendshipListener {
public:
    void OnFriendApplicationListAdded(const V2TIMFriendApplicationVector &applicationList) override {
        auto *fm = V2TIMManager::GetInstance()->GetFriendshipManager();
        for (size_t i = 0; i < applicationList.Size(); ++i) {
            const auto &app = applicationList[i];
            printf("Server: Received friend application from %s, accepting...\n", app.userID.CString());
            struct AcceptCb : public V2TIMValueCallback<V2TIMFriendOperationResult> {
                void OnSuccess(const V2TIMFriendOperationResult& r) override {
                    printf("Server: Accepted friend %s (code=%d info=%s)\n", r.userID.CString(), r.resultCode, r.resultInfo.CString());
                    fflush(stdout);
                }
                void OnError(int code, const V2TIMString& msg) override {
                    printf("Server: Accept friend error %d: %s\n", code, msg.CString());
                    fflush(stdout);
                }
            } cb;
            fm->AcceptFriendApplication(app, V2TIM_FRIEND_ACCEPT_AGREE_AND_ADD, &cb);
        }
    }
};

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    SDKListener sdkListener;
    SimpleMsgListener simpleListener;
    FriendListener friendListener;

    // Init SDK
    V2TIMSDKConfig cfg;
    printf("server: before AddSDKListener\n");
    V2TIMManager::GetInstance()->AddSDKListener(&sdkListener);
    printf("server: after AddSDKListener\n");
    // Register friendship listener to auto-accept friend requests
    V2TIMManager::GetInstance()->GetFriendshipManager()->AddFriendListener(&friendListener);
    bool ok = V2TIMManager::GetInstance()->InitSDK(0, cfg);
    printf("server: after InitSDK=%d\n", ok ? 1 : 0);
    if (!ok) {
        fprintf(stderr, "InitSDK failed\n");
        return 1;
    }

    // Login
    struct NoopCb : public V2TIMCallback {
        void OnSuccess() override {}
        void OnError(int, const V2TIMString&) override {}
    } loginCb;
    printf("server: before Login\n");
    V2TIMManager::GetInstance()->Login("EchoBotServer", "dummy_sig", &loginCb);
    printf("server: after Login\n");

    // Print user ID (mapped underlying address via V2TIM)
    std::string user_id = V2TIMManager::GetInstance()->GetLoginUser().CString();
    printf("=== Echo Bot Server ===\n");
    printf("User ID: %s\n", user_id.c_str());
    printf("Status: Echoing your messages\n");
    printf("=======================\n");
    fflush(stdout);

    // Register simple message listener
    V2TIMManager::GetInstance()->AddSimpleMsgListener(&simpleListener);

    printf("Server starting...\n");
    printf("Press Ctrl+C to stop\n\n");

    // Run indefinitely
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return 0;
}

