// V2TIM-based echo bot client
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>

#include <V2TIMManager.h>
#include <V2TIMListener.h>
#include <V2TIMMessage.h>
#include <V2TIMCallback.h>
#include <V2TIMFriendshipManager.h>

static std::atomic<int> g_connected{0};
static std::atomic<int> g_echo_ok{0};
static std::string g_expected;
static std::atomic<int> g_custom_ok{0};
static std::vector<uint8_t> g_custom_expected;
static std::string g_target_user;
static std::atomic<int> g_target_online{0};

class SDKListener : public V2TIMSDKListener {
public:
    void OnConnectSuccess() override {
        g_connected.store(1);
        printf("Client: Online\n");
        fflush(stdout);
    }
    void OnConnectFailed(int error_code, const V2TIMString &error_message) override {
        printf("Client: Connect failed %d: %s\n", error_code, error_message.CString());
        fflush(stdout);
    }
    void OnUserStatusChanged(const V2TIMUserStatusVector& userStatusList) override {
        for (size_t i = 0; i < userStatusList.Size(); ++i) {
            const auto& s = userStatusList[i];
            if (!g_target_user.empty() && s.userID == g_target_user.c_str()) {
                if (s.statusType == V2TIM_USER_STATUS_ONLINE) {
                    g_target_online.store(1);
                }
            }
        }
    }
};

class SimpleMsgListener : public V2TIMSimpleMsgListener {
public:
    void OnRecvC2CTextMessage(const V2TIMString &msgID, const V2TIMUserFullInfo &sender,
                              const V2TIMString &text) override {
        if (!g_expected.empty() && text == V2TIMString(g_expected.c_str())) {
            g_echo_ok.store(1);
        }
        printf("Echo received from friend %s: %s\n", sender.userID.CString(), text.CString());
        fflush(stdout);
    }
    void OnRecvC2CCustomMessage(const V2TIMString &msgID, const V2TIMUserFullInfo &sender,
                                const V2TIMBuffer &customData) override {
        if (!g_custom_expected.empty() &&
            customData.Size() == g_custom_expected.size() &&
            memcmp(customData.Data(), g_custom_expected.data(), customData.Size()) == 0) {
            g_custom_ok.store(1);
        }
        printf("Custom echo received from friend %s: %zu bytes\n", sender.userID.CString(), (size_t)customData.Size());
        fflush(stdout);
    }
};

static bool wait_connected(uint32_t timeout_seconds) {
    for (uint32_t i = 0; i < timeout_seconds * 10; ++i) {
        if (g_connected.load()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

static bool wait_echo(uint32_t timeout_seconds) {
    for (uint32_t i = 0; i < timeout_seconds * 10; ++i) {
        if (g_echo_ok.load()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

static bool wait_custom(uint32_t timeout_seconds) {
    for (uint32_t i = 0; i < timeout_seconds * 10; ++i) {
        if (g_custom_ok.load()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

int main(int argc, char** argv) {
    SDKListener sdkListener;
    SimpleMsgListener simpleListener;

    // Init SDK
    V2TIMSDKConfig cfg;
    V2TIMManager::GetInstance()->AddSDKListener(&sdkListener);
    bool ok = V2TIMManager::GetInstance()->InitSDK(0, cfg);
    if (!ok) {
        fprintf(stderr, "InitSDK failed\n");
        return 1;
    }

    // Login
    struct NoopCb : public V2TIMCallback {
        void OnSuccess() override {}
        void OnError(int, const V2TIMString&) override {}
    } loginCb;
    V2TIMManager::GetInstance()->Login("EchoBotClient", "dummy_sig", &loginCb);
    V2TIMManager::GetInstance()->AddSimpleMsgListener(&simpleListener);

    // Print own ID (mapped underlying address via V2TIM)
    std::string self_user_id = V2TIMManager::GetInstance()->GetLoginUser().CString();
    printf("=== Echo Bot Client ===\n");
    printf("User ID: %s\n", self_user_id.c_str());
    printf("Status: Ready to test echo server\n");
    printf("=======================\n");
    fflush(stdout);

    // Non-interactive mode: echo_bot_client --auto <server_user_id> <message>
    if (argc >= 3 && strcmp(argv[1], "--auto") == 0) {
        const char* server_user_id = argv[2];
        const char* msg = (argc >= 4) ? argv[3] : "ping";
        bool run_extended = (argc >= 5 && strcmp(argv[4], "--extended") == 0);

        printf("Non-interactive mode: adding friend and sending message...\n");
        if (!wait_connected(120)) {
            printf("Client did not come online in time.\n");
            V2TIMManager::GetInstance()->UnInitSDK();
            return 2;
        }

        // Add friend (server)
        V2TIMFriendAddApplication app;
        app.userID = server_user_id;
        app.addWording = "Hello from echo bot client!";
        g_target_user = server_user_id;
        V2TIMFriendshipManager* fm = V2TIMManager::GetInstance()->GetFriendshipManager();
        std::atomic<bool> add_done{false};
        V2TIMFriendOperationResult add_res;
        class AddCb : public V2TIMValueCallback<V2TIMFriendOperationResult> {
        public:
            std::atomic<bool>* done;
            V2TIMFriendOperationResult* out;
            void OnSuccess(const V2TIMFriendOperationResult& value) override { *out = value; done->store(true); }
            void OnError(int code, const V2TIMString& desc) override { out->resultCode = code; out->resultInfo = desc; done->store(true); }
        } add_cb;
        add_cb.done = &add_done;
        add_cb.out = &add_res;
        fm->AddFriend(app, &add_cb);
        for (int i = 0; i < 200 && !add_done.load(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Wait for peer to come online (via V2TIM SDK listener)
        g_target_online.store(0);
        for (int i = 0; i < 240 && !g_target_online.load(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Send message
        g_echo_ok.store(0);
        g_expected = msg;
        // No direct tox usage. We will rely on send retries below.
        class SendCb : public V2TIMSendCallback {
        public:
            std::atomic<bool>* ok;
            void OnSuccess(const V2TIMMessage&) override { ok->store(true); }
            void OnError(int code, const V2TIMString& msg) override { ok->store(false); printf("Send failed %d: %s\n", code, msg.CString()); }
            void OnProgress(uint32_t) override {}
        } send_cb;
        std::atomic<bool> send_ok{false};
        send_cb.ok = &send_ok;
        // Retry send for up to ~60 seconds until success (server may need to accept friend).
        for (int i = 0; i < 60; ++i) {
            send_ok.store(false);
            (void)V2TIMManager::GetInstance()->SendC2CTextMessage(msg, server_user_id, &send_cb);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (send_ok.load()) break;
        }
        if (!wait_echo(60)) {
            printf("Did not receive expected echo for basic message within timeout.\n");
            V2TIMManager::GetInstance()->UnInitSDK();
            return 6;
        }

        // Always run extended tests unless explicitly disabled via env
        bool do_extended = true;
        if (const char* env = getenv("TIM2TOX_SKIP_EXTENDED")) {
            if (strcmp(env, "1") == 0 || strcasecmp(env, "true") == 0) do_extended = false;
        }
        if (do_extended || run_extended) {
            printf("Running extended verification suite...\n");
            // Unicode text
            {
                const char* unicode_msg = "Hello/你好/Привет/😀";
                g_expected = unicode_msg;
                g_echo_ok.store(0);
                V2TIMManager::GetInstance()->SendC2CTextMessage(unicode_msg, server_user_id, &send_cb);
                if (!wait_echo(60)) {
                    printf("Extended test failed: Unicode message not echoed.\n");
                    V2TIMManager::GetInstance()->UnInitSDK();
                    return 7;
                }
            }
            // Long text
            {
                std::string long_msg(1350, 'L'); // near tox text limit
                g_expected = long_msg;
                g_echo_ok.store(0);
                V2TIMManager::GetInstance()->SendC2CTextMessage(long_msg.c_str(), server_user_id, &send_cb);
                if (!wait_echo(90)) {
                    printf("Extended test failed: Long message not echoed.\n");
                    V2TIMManager::GetInstance()->UnInitSDK();
                    return 9;
                }
            }
            // Burst messages
            for (int i = 0; i < 5; ++i) {
                char burst[128];
                snprintf(burst, sizeof(burst), "burst_%d_%ld", i, (long)time(NULL));
                g_expected = burst;
                g_echo_ok.store(0);
                V2TIMManager::GetInstance()->SendC2CTextMessage(burst, server_user_id, &send_cb);
                if (!wait_echo(60)) {
                    printf("Extended test failed: Burst message %d not echoed.\n", i);
                    V2TIMManager::GetInstance()->UnInitSDK();
                    return 10;
                }
            }
            // Custom message
            {
                const char* payload = "custom:ping";
                g_custom_expected.assign(payload, payload + strlen(payload));
                g_custom_ok.store(0);
                V2TIMBuffer buf(reinterpret_cast<const uint8_t*>(g_custom_expected.data()), g_custom_expected.size());
                V2TIMManager::GetInstance()->SendC2CCustomMessage(buf, server_user_id, &send_cb);
                if (!wait_custom(60)) {
                    printf("Extended test failed: Custom message not echoed.\n");
                    V2TIMManager::GetInstance()->UnInitSDK();
                    return 11;
                }
            }
            // Set/Get self profile
            {
                V2TIMUserFullInfo info;
                info.nickName = "EchoClientNick";
                info.selfSignature = "EchoClientSignature";
                struct SetCb : public V2TIMCallback {
                    std::atomic<bool>* done;
                    int* code;
                    void OnSuccess() override { *code = 0; done->store(true); }
                    void OnError(int c, const V2TIMString&) override { *code = c; done->store(true); }
                } set_cb;
                std::atomic<bool> set_done{false};
                int set_code = -1;
                set_cb.done = &set_done;
                set_cb.code = &set_code;
                V2TIMManager::GetInstance()->SetSelfInfo(info, &set_cb);
                for (int i = 0; i < 60 && !set_done.load(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (set_code != 0) {
                    printf("Extended test failed: SetSelfInfo error=%d\n", set_code);
                    V2TIMManager::GetInstance()->UnInitSDK();
                    return 12;
                }
                V2TIMStringVector ids;
                ids.PushBack(V2TIMManager::GetInstance()->GetLoginUser());
                struct GetCb : public V2TIMValueCallback<V2TIMUserFullInfoVector> {
                    std::atomic<bool>* done;
                    V2TIMUserFullInfoVector* out;
                    int* code;
                    V2TIMString* desc;
                    void OnSuccess(const V2TIMUserFullInfoVector& v) override { *out = v; *code = 0; done->store(true); }
                    void OnError(int c, const V2TIMString& d) override { *code = c; *desc = d; done->store(true); }
                } get_cb;
                std::atomic<bool> get_done{false};
                int get_code = -1;
                V2TIMString get_desc;
                V2TIMUserFullInfoVector infos;
                get_cb.done = &get_done;
                get_cb.out = &infos;
                get_cb.code = &get_code;
                get_cb.desc = &get_desc;
                V2TIMManager::GetInstance()->GetUsersInfo(ids, &get_cb);
                for (int i = 0; i < 60 && !get_done.load(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (get_code != 0 || infos.Size() == 0 || infos[0].nickName != "EchoClientNick") {
                    printf("Extended test failed: GetUsersInfo mismatch or error=%d\n", get_code);
                    V2TIMManager::GetInstance()->UnInitSDK();
                    return 13;
                }
            }
            // CheckFriend and GetFriendList
            {
                V2TIMStringVector ids;
                ids.PushBack(server_user_id);
                struct CheckCb : public V2TIMValueCallback<V2TIMFriendCheckResultVector> {
                    std::atomic<bool>* done;
                    V2TIMFriendCheckResultVector* out;
                    void OnSuccess(const V2TIMFriendCheckResultVector& v) override { *out = v; done->store(true); }
                    void OnError(int, const V2TIMString&) override { done->store(true); }
                } check_cb;
                std::atomic<bool> check_done{false};
                V2TIMFriendCheckResultVector check_res;
                check_cb.done = &check_done;
                check_cb.out = &check_res;
                V2TIMManager::GetInstance()->GetFriendshipManager()->CheckFriend(ids, V2TIM_FRIEND_TYPE_SINGLE, &check_cb);
                for (int i = 0; i < 60 && !check_done.load(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (check_res.Size() == 0 || check_res[0].relationType == V2TIM_FRIEND_RELATION_TYPE_NONE) {
                    printf("Extended test failed: CheckFriend indicates not friends.\n");
                    V2TIMManager::GetInstance()->UnInitSDK();
                    return 14;
                }
                struct ListCb : public V2TIMValueCallback<V2TIMFriendInfoVector> {
                    std::atomic<bool>* done;
                    V2TIMFriendInfoVector* out;
                    void OnSuccess(const V2TIMFriendInfoVector& v) override { *out = v; done->store(true); }
                    void OnError(int, const V2TIMString&) override { done->store(true); }
                } list_cb;
                std::atomic<bool> list_done{false};
                V2TIMFriendInfoVector friends;
                list_cb.done = &list_done;
                list_cb.out = &friends;
                V2TIMManager::GetInstance()->GetFriendshipManager()->GetFriendList(&list_cb);
                for (int i = 0; i < 60 && !list_done.load(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (friends.Size() == 0) {
                    printf("Extended test warning: Friend list empty.\n");
                }
            }
            printf("Extended verification suite passed.\n");
        }

        V2TIMManager::GetInstance()->UnInitSDK();
        return 0;
    }

    // Interactive mode not needed for tests
    printf("Client stopped.\n");
    V2TIMManager::GetInstance()->UnInitSDK();
    return 0;
}

