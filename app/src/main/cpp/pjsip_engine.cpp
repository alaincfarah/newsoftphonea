#include "pjsip_engine.h"

#include <android/log.h>

namespace {
    constexpr char kTag[] = "PjsipEngine";
    PjsipEngine *g_engine = nullptr;

#if PJSIP_AVAILABLE
    std::string PjToString(const pj_str_t &value) {
        if (value.ptr == nullptr || value.slen == 0) {
            return "";
        }
        return std::string(value.ptr, value.slen);
    }

    pjsua_transport_type GetTransportType(int transportType) {
        switch (transportType) {
            case 1:
                return PJSIP_TRANSPORT_TCP;
            case 2:
                return PJSIP_TRANSPORT_TLS;
            default:
                return PJSIP_TRANSPORT_UDP;
        }
    }

    void OnIncomingCall(pjsua_acc_id, pjsua_call_id callId, pjsip_rx_data *) {
        if (!g_engine) return;
        pjsua_call_info info;
        pjsua_call_get_info(callId, &info);
        std::string remoteInfo = PjToString(info.remote_info);
        std::string displayName = PjToString(info.remote_contact);
        if (g_engine->incomingCallCallback_) {
            g_engine->incomingCallCallback_(callId, remoteInfo, displayName);
        }
    }

    void OnCallState(pjsua_call_id callId, pjsip_event *) {
        if (!g_engine) return;
        pjsua_call_info info;
        pjsua_call_get_info(callId, &info);
        if (g_engine->callStateCallback_) {
            g_engine->callStateCallback_(callId, static_cast<int>(info.state), info.last_status);
        }
    }

    void OnCallMediaState(pjsua_call_id callId) {
        if (!g_engine) return;
        pjsua_call_info info;
        pjsua_call_get_info(callId, &info);
        bool active = info.media_status == PJSUA_CALL_MEDIA_ACTIVE;
        if (g_engine->callMediaCallback_) {
            g_engine->callMediaCallback_(callId, active);
        }
    }

    void OnRegState(pjsua_acc_id accId) {
        if (!g_engine) return;
        pjsua_acc_info info;
        pjsua_acc_get_info(accId, &info);
        bool registered = info.status / 100 == 2;
        if (g_engine->registrationCallback_) {
            g_engine->registrationCallback_(registered, info.status);
        }
    }
#endif
}

PjsipEngine::PjsipEngine()
    : incomingCallCallback_(nullptr),
      callStateCallback_(nullptr),
      callMediaCallback_(nullptr),
      registrationCallback_(nullptr)
#if PJSIP_AVAILABLE
      , accountId_(PJSUA_INVALID_ID),
      recorderId_(PJSUA_INVALID_ID)
#endif
{
    g_engine = this;
}

PjsipEngine::~PjsipEngine() {
    g_engine = nullptr;
#if PJSIP_AVAILABLE
    pjsua_destroy();
#endif
}

bool PjsipEngine::Init(const std::string &logPath, int sipPort, int audioPort, int transportType) {
#if PJSIP_AVAILABLE
    pj_status_t status = pjsua_create();
    if (status != PJ_SUCCESS) return false;

    pjsua_config cfg;
    pjsua_config_default(&cfg);
    cfg.cb.on_incoming_call = &OnIncomingCall;
    cfg.cb.on_call_state = &OnCallState;
    cfg.cb.on_call_media_state = &OnCallMediaState;
    cfg.cb.on_reg_state = &OnRegState;

    pjsua_logging_config logCfg;
    pjsua_logging_config_default(&logCfg);
    logCfg.log_filename = pj_str(logPath.c_str());

    pjsua_media_config mediaCfg;
    pjsua_media_config_default(&mediaCfg);
    mediaCfg.snd_clock_rate = 16000;
    mediaCfg.clock_rate = 16000;
    mediaCfg.audio_frame_ptime = 20;
    mediaCfg.snd_auto_close_time = 0;
    mediaCfg.rtp_port = audioPort;
    mediaCfg.rtp_port_range = 200;

    status = pjsua_init(&cfg, &logCfg, &mediaCfg);
    if (status != PJ_SUCCESS) return false;

    pjsua_transport_config transportCfg;
    pjsua_transport_config_default(&transportCfg);
    transportCfg.port = sipPort;
    status = pjsua_transport_create(GetTransportType(transportType), &transportCfg, nullptr);
    if (status != PJ_SUCCESS) return false;

    status = pjsua_start();
    return status == PJ_SUCCESS;
#else
    (void)logPath;
    (void)sipPort;
    (void)audioPort;
    (void)transportType;
    __android_log_print(ANDROID_LOG_WARN, kTag, "PJSIP not available; Init skipped");
    return false;
#endif
}

int PjsipEngine::CreateAccount(const std::string &username,
                               const std::string &password,
                               const std::string &domain,
                               const std::string &proxy) {
#if PJSIP_AVAILABLE
    pjsua_acc_config accCfg;
    pjsua_acc_config_default(&accCfg);
    std::string id = "sip:" + username + "@" + domain;
    std::string regUri = "sip:" + domain;
    accCfg.id = pj_str(id.c_str());
    accCfg.reg_uri = pj_str(regUri.c_str());
    accCfg.cred_count = 1;
    accCfg.cred_info[0].realm = pj_str("*");
    accCfg.cred_info[0].scheme = pj_str("digest");
    accCfg.cred_info[0].username = pj_str(username.c_str());
    accCfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    accCfg.cred_info[0].data = pj_str(password.c_str());
    if (!proxy.empty()) {
        accCfg.proxy_cnt = 1;
        accCfg.proxy[0] = pj_str(proxy.c_str());
    }

    pjsua_acc_id accId = PJSUA_INVALID_ID;
    pj_status_t status = pjsua_acc_add(&accCfg, PJ_TRUE, &accId);
    if (status != PJ_SUCCESS) return -1;
    accountId_ = accId;
    return accId;
#else
    (void)username;
    (void)password;
    (void)domain;
    (void)proxy;
    return -1;
#endif
}

bool PjsipEngine::RegisterAccount(int accountId, bool enable) {
#if PJSIP_AVAILABLE
    return pjsua_acc_set_registration(accountId, enable ? PJ_TRUE : PJ_FALSE) == PJ_SUCCESS;
#else
    (void)accountId;
    (void)enable;
    return false;
#endif
}

int PjsipEngine::MakeCall(int accountId, const std::string &targetUri) {
#if PJSIP_AVAILABLE
    pjsua_call_id callId = PJSUA_INVALID_ID;
    pj_status_t status = pjsua_call_make_call(
        accountId,
        pj_str(targetUri.c_str()),
        0,
        nullptr,
        nullptr,
        &callId);
    return status == PJ_SUCCESS ? callId : -1;
#else
    (void)accountId;
    (void)targetUri;
    return -1;
#endif
}

bool PjsipEngine::AnswerCall(int callId, int statusCode) {
#if PJSIP_AVAILABLE
    return pjsua_call_answer(callId, statusCode, nullptr, nullptr) == PJ_SUCCESS;
#else
    (void)callId;
    (void)statusCode;
    return false;
#endif
}

bool PjsipEngine::HangupCall(int callId) {
#if PJSIP_AVAILABLE
    return pjsua_call_hangup(callId, 0, nullptr, nullptr) == PJ_SUCCESS;
#else
    (void)callId;
    return false;
#endif
}

bool PjsipEngine::HoldCall(int callId, bool hold) {
#if PJSIP_AVAILABLE
    if (hold) {
        return pjsua_call_set_hold(callId, nullptr) == PJ_SUCCESS;
    }
    return pjsua_call_reinvite(callId, PJSUA_CALL_REINVITE_UNHOLD, nullptr) == PJ_SUCCESS;
#else
    (void)callId;
    (void)hold;
    return false;
#endif
}

bool PjsipEngine::MuteCall(int callId, bool mute) {
#if PJSIP_AVAILABLE
    pjsua_call_info info;
    pjsua_call_get_info(callId, &info);
    if (mute) {
        return pjsua_conf_disconnect(0, info.conf_slot) == PJ_SUCCESS;
    }
    return pjsua_conf_connect(0, info.conf_slot) == PJ_SUCCESS;
#else
    (void)callId;
    (void)mute;
    return false;
#endif
}

bool PjsipEngine::BlindTransfer(int callId, const std::string &targetUri) {
#if PJSIP_AVAILABLE
    return pjsua_call_xfer(callId, pj_str(targetUri.c_str()), nullptr) == PJ_SUCCESS;
#else
    (void)callId;
    (void)targetUri;
    return false;
#endif
}

bool PjsipEngine::AttendedTransfer(int callId, int replaceCallId) {
#if PJSIP_AVAILABLE
    return pjsua_call_xfer_replaces(callId, replaceCallId, 0, nullptr) == PJ_SUCCESS;
#else
    (void)callId;
    (void)replaceCallId;
    return false;
#endif
}

bool PjsipEngine::StartRecording(int callId, const std::string &filePath) {
#if PJSIP_AVAILABLE
    if (recorderId_ != PJSUA_INVALID_ID) {
        pjsua_recorder_destroy(recorderId_);
        recorderId_ = PJSUA_INVALID_ID;
    }

    pj_status_t status = pjsua_recorder_create(
        pj_str(filePath.c_str()),
        0,
        nullptr,
        0,
        0,
        &recorderId_);
    if (status != PJ_SUCCESS) {
        recorderId_ = PJSUA_INVALID_ID;
        return false;
    }

    pjsua_call_info info;
    pjsua_call_get_info(callId, &info);
    pjsua_conf_port_id recPort = pjsua_recorder_get_conf_port(recorderId_);
    return pjsua_conf_connect(info.conf_slot, recPort) == PJ_SUCCESS;
#else
    (void)callId;
    (void)filePath;
    return false;
#endif
}

bool PjsipEngine::StopRecording(int callId) {
#if PJSIP_AVAILABLE
    if (recorderId_ == PJSUA_INVALID_ID) return false;
    pjsua_call_info info;
    pjsua_call_get_info(callId, &info);
    pjsua_conf_port_id recPort = pjsua_recorder_get_conf_port(recorderId_);
    pjsua_conf_disconnect(info.conf_slot, recPort);
    pjsua_recorder_destroy(recorderId_);
    recorderId_ = PJSUA_INVALID_ID;
    return true;
#else
    (void)callId;
    return false;
#endif
}

bool PjsipEngine::SetCodecPriorities(const std::vector<CodecPriority> &codecs) {
#if PJSIP_AVAILABLE
    for (const auto &codec : codecs) {
        pjsua_codec_set_priority(pj_str(codec.name.c_str()), codec.priority);
    }
    return true;
#else
    (void)codecs;
    return false;
#endif
}

bool PjsipEngine::HandleNetworkChange() {
#if PJSIP_AVAILABLE
    pj_status_t status = pjsua_handle_ip_change(nullptr);
    if (accountId_ != PJSUA_INVALID_ID) {
        pjsua_acc_set_registration(accountId_, PJ_TRUE);
    }
    return status == PJ_SUCCESS;
#else
    return false;
#endif
}

void PjsipEngine::SetIncomingCallCallback(void (*callback)(int, const std::string &, const std::string &)) {
    incomingCallCallback_ = callback;
}

void PjsipEngine::SetCallStateCallback(void (*callback)(int, int, int)) {
    callStateCallback_ = callback;
}

void PjsipEngine::SetCallMediaCallback(void (*callback)(int, bool)) {
    callMediaCallback_ = callback;
}

void PjsipEngine::SetRegistrationCallback(void (*callback)(bool, int)) {
    registrationCallback_ = callback;
}
