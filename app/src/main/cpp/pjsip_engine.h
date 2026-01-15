#pragma once

#include <cstring>
#include <string>
#include <vector>

#if __has_include(<pjsua-lib/pjsua.h>)
#define PJSIP_AVAILABLE 1
#include <pjsua-lib/pjsua.h>
#else
#define PJSIP_AVAILABLE 0
using pjsua_call_id = int;
using pjsua_acc_id = int;
using pjsua_conf_port_id = int;
using pj_status_t = int;
struct pj_str_t { const char *ptr; int slen; };
struct pjsua_call_info {};
struct pjsua_acc_config {};
struct pjsua_transport_config {};
struct pjsua_config {};
struct pjsua_logging_config {};
struct pjsua_media_config {};
struct pjsua_call_setting {};
struct pjsua_msg_data {};
#define PJ_SUCCESS 0
#define PJ_EINVAL 1
inline pj_str_t pj_str(const char *text) { return pj_str_t{text, static_cast<int>(strlen(text))}; }
#endif

struct CodecPriority {
    std::string name;
    int priority;
};

class PjsipEngine {
public:
    PjsipEngine();
    ~PjsipEngine();

    bool Init(const std::string &logPath, int sipPort, int audioPort, int transportType);
    int CreateAccount(const std::string &username,
                      const std::string &password,
                      const std::string &domain,
                      const std::string &proxy);
    bool RegisterAccount(int accountId, bool enable);
    int MakeCall(int accountId, const std::string &targetUri);
    bool AnswerCall(int callId, int statusCode);
    bool HangupCall(int callId);
    bool HoldCall(int callId, bool hold);
    bool MuteCall(int callId, bool mute);
    bool BlindTransfer(int callId, const std::string &targetUri);
    bool AttendedTransfer(int callId, int replaceCallId);
    bool StartRecording(int callId, const std::string &filePath);
    bool StopRecording(int callId);
    bool SetCodecPriorities(const std::vector<CodecPriority> &codecs);
    bool HandleNetworkChange();

    void SetIncomingCallCallback(void (*callback)(int, const std::string &, const std::string &));
    void SetCallStateCallback(void (*callback)(int, int, int));
    void SetCallMediaCallback(void (*callback)(int, bool));
    void SetRegistrationCallback(void (*callback)(bool, int));

private:
    void (*incomingCallCallback_)(int, const std::string &, const std::string &);
    void (*callStateCallback_)(int, int, int);
    void (*callMediaCallback_)(int, bool);
    void (*registrationCallback_)(bool, int);

#if PJSIP_AVAILABLE
    pjsua_acc_id accountId_;
    int recorderId_;
#endif
};
