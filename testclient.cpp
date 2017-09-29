#include "testclient.h"
#include <swift/error_code.hpp>
#include <swift/net/sdp/sdp.hpp>
#include <swift/net/rtsp/rtsp_client_session.hpp>
#include <swift/net/rtsp/rtsp_session.hpp>
#include <swift/net/hlp_core/hlp_www_authentication.hpp>
#include <swift/crypto.hpp>
#include <swift/text/algorithm.hpp>

#define CONSOLE_DEBUG 0

#define LOCAL_ "192.168.0.122"
#define DEFAULT_MULITCAST_ADDRESS "232.0.0.1"

#define PROTOCOL_HEADER "rtsp://"
// TODO : 这个路径应该是解析SDP 获得的，目前是暂时用宏去拿SETUP路径 以后去完成..
#define SETUP_URL_TAIL "/trackID=1"
#define HIK_DEVICE_TAIL "/Streaming/Channels/101"
#define MULITCAST_URI "rtsp://192.168.0.236:554/Streaming/Channels/101?transportmode=mcast&profile=Profile_1"

using namespace std;
using namespace swift::net::rtsp;
void MyAuthenCtxt::fillRequest(swift::net::rtsp::request_ptr req)
{
    SWIFT_ASSERT(req);

    swift::octets authen;
    if(m_mode == swift::net::http::authenication_basic)
    {
        authen = "Basic ";
        swift::octets src = m_username + ":" + m_pwd;
        authen += swift::crypto::base64::encode(swift::const_buffer(src.c_str(), src.length()));
    }
    req->set_header("Authorization", authen);
}

////////////////////////////////////////////////////////////////////////////////
/// \brief asyncRequestCtxt 异步回调情境基类
/// \param myClient 上下文凭据

asyncRequestCtxt::asyncRequestCtxt(MyRtspClient &myClient)
    : m_myClient(myClient)
    , m_bAccessdenied(false)
{
#if CONSOLE_DEBUG
    cout << "new async context : 0x" << this << endl;
#endif
}

void asyncRequestCtxt::onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req)
{
}

void asyncRequestCtxt::onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess)
{
}

void asyncRequestCtxt::on_holder_destroy()
{
#if CONSOLE_DEBUG
    cout << "destroy async context : 0x" << this << endl;
#endif
    delete this;
}

////////////////////////////////////////////////////////////////////////////////
/// \brief optCtxt [OPTION]查看服务端提供的可用服务

optCtxt::optCtxt(MyRtspClient &myClient)
    : asyncRequestCtxt(myClient)
{

}

void optCtxt::onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess)
{
    cout << "OPTION isn't necessary step :-)" << endl;
}

////////////////////////////////////////////////////////////////////////////////
/// \brief descCtext [DESCRIBE] 能力描述

descCtext::descCtext(MyRtspClient &myClient)
    : asyncRequestCtxt(myClient)
{

}

void descCtext::onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req)
{
    QString header = "[DESCRIBE] Response :";
    if (resp->is_success()) {
        ///DESCRIBE消息回复成功 ,创建SETUP请求
        header += "成功";
        //这里设置监听端口
        swift::net::rtsp::transport_value trans("RTP/AVP;multicast;client_port=12000");
        m_myClient.m_client.setup("rtsp://192.168.0.236:554/Streaming/Channels/101/trackID=1?transportmode=mcast&profile=Profile_1", trans, new setupCtxt(m_myClient));
    } else if (resp->get_status() == swift::net::http::HTTP_UNAUTHORIZED && !m_bAccessdenied) {
        header += "未注册";
        m_bAccessdenied = true;
        m_myClient.sendSafeAuthenticationRequest(req, resp, this);
    }
    std::string _resp = resp->to_string();
    emit m_myClient.getClientResp(header, _resp.c_str());
}
////////////////////////////////////////////////////////////////////////////////
/// \brief setupCtxt [SETUP] 确认状态

setupCtxt::setupCtxt(MyRtspClient &myClient)
    : asyncRequestCtxt(myClient)
{

}

void setupCtxt::onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess)
{
    QString header = "[SETUP] Response:";
    if (!resp->is_success()) {
        header += resp->get_status();
        return;
    }

    std::string _resp = resp->to_string();
    emit m_myClient.getClientResp(header, _resp.c_str());

    swift::net::rtsp::transport_value ttrans("RTP/AVP;multicast;client_port=10001-10002");

    m_myClient.m_session->setup("rtsp://192.168.0.236:554/Streaming/Channels/101/trackID=1?transportmode=mcast&profile=Profile_1", ttrans, new sessionSetupCtxt(m_myClient));

    ///SETUP消息回复成功, 启用RTP 接收数据
#if 0
    swift::net::rtsp::request_ptr playReq = sess->make_play_request();
    playReq->set_context(new playCtxt(m_myClient));
    sess->send_request(playReq);
#endif
}

////////////////////////////////////////////////////////////////////////////////
/// \brief session内重新开始setup
sessionSetupCtxt::sessionSetupCtxt(MyRtspClient &myClient)
    : asyncRequestCtxt(myClient)
{

}

void sessionSetupCtxt::onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess)
{
    QString header = "[SETUP] Response:";
    if (!(resp->is_success())) {
        header += resp->get_status();
    }
    header += "Success";
    std::string _resp = resp->to_string();
    emit m_myClient.getClientResp(header, _resp.c_str());

    ///SETUP消息回复成功, 启用RTP 接收数据
    //m_myClient.m_receiver.start_receive();
    m_myClient.m_socket.open();

    m_myClient.m_socket.join_group("232.0.0.1");

    emit m_myClient.getClientResp("**************************\n下个回复成功即开始接收RTP数据", "**************************\n");
    swift::net::rtsp::range_value range("npt", "0.000", "");
    m_myClient.m_session->play(range);
#if 0
    swift::net::rtsp::request_ptr playReq = sess->make_play_request();
    playReq->set_context(new playCtxt(m_myClient));
    sess->send_request(playReq);
#endif
}

////////////////////////////////////////////////////////////////////////////////
/// \brief playCtxt [PLAY] 接收数据

playCtxt::playCtxt(MyRtspClient &myClient)
    : asyncRequestCtxt(myClient)
{

}

void playCtxt::onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess)
{
    QString header = "[PLAY] Response:";
    if (!resp->is_success())
        header += "失败";
    header += "成功";
    std::string _resp = resp->to_string();
    emit m_myClient.getClientResp(header, _resp.c_str());
#if 0
    //m_myClient.m_receiver.start_receive();
    //swift::net::rtsp::request_ptr optionsReq = sess->make_request(m_OPTIONS);
    //sess->send_request(optionsReq);
#endif
}

////////////////////////////////////////////////////////////////////////////////
/// \brief pauseCtxt [PAUSE] 暂停接收

pauseCtxt::pauseCtxt(MyRtspClient &myClient)
    : asyncRequestCtxt(myClient)
{

}

void pauseCtxt::onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess)
{

}

////////////////////////////////////////////////////////////////////////////////
/// \brief teardownCtxt [TEARDOWN] 终止会话

teardownCtxt::teardownCtxt(MyRtspClient &myClient)
    : asyncRequestCtxt(myClient)
{

}

void teardownCtxt::onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess)
{

}

////////////////////////////////////////////////////////////////////////////////
/// \brief myRtspClient 通信客户端
/// \param engine rtp 接收和客户端的通信线程
/// \param info  设备参数信息

MyRtspClient::MyRtspClient(swift::engine &engine, MyDeviceInfo *info, QObject *parent)
    : QObject(parent)
    , m_info(info)
    // 组播模式地址
    , m_url(MULITCAST_URI)
    // 单播模式地址
    //, m_url(PROTOCOL_HEADER + m_info->m_deviceIp + ":" + m_info->m_serverPort + HIK_DEVICE_TAIL)
    , m_setupUrl(m_url + SETUP_URL_TAIL)
    , m_client(swift::net::ip::endpoint(info->m_deviceIp.toStdString().c_str(),info->m_serverPort), *this, engine)
    , m_receiver(swift::net::ip::endpoint("232.0.0.1", 10001), *this, engine)
                     //swift::net::ip::any_address_v4, 5000), *this, engine)
    , m_socket(engine)
{

}

void MyRtspClient::start()
{
    ///rtsp://192.168.0.236:554/Streaming/Channels/101?transportmode=mcast&profile=Profile_1
    ///rtsp://192.168.0.236:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1
    ///rtsp://192.168.0.63:554/Streaming/Channels/101   application/sdp
    swift::net::rtsp::rtsp_url url("rtsp://192.168.0.236:554/Streaming/Channels/101?transportmode=mcast&profile=Profile_1");
    m_client.describe(url, "application/sdp", new descCtext(*this));
#if 0
    //swift::net::rtsp::transport_value trans("RTP/AVP;unicast;client_port=4000-4001");
    //m_client.setup("rtsp://192.168.0.63:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1", trans, new setupCtxt(*this));
#endif
}

void MyRtspClient::stop()
{
    if (m_session)
        m_session->teardown();
}

void MyRtspClient::sendSafeAuthenticationRequest(const swift::net::rtsp::request_ptr req, const swift::net::rtsp::response_ptr resp, swift::context *ctxt)
{
    authentication::auth_ctxt_ptr auth = authentication::make_authentication_context(resp, authentication::auth_mode_basic);
    //TODO : 使用新www-authon模块
    if (auth) {
        auth->set_authentication("admin", "bq111111");
        auth->fill_request(req);
        req->remove_header(swift::net::rtsp::h_CSeq);
    }
    if (ctxt)
        m_client.send_request(req);
}

void MyRtspClient::on_rtp_packet(swift::net::rtp::rtp_packet &packet, const swift::net::ip::endpoint &remoteEP, swift::net::rtp::receiver &owner)
{

    QString _resp = "[Length:]" + QString::number(packet.get_payload_length()) + "[Sequence:]" + QString::number(packet.get_sequence());
    emit getClientResp("", _resp);
#if CONSOLE_DEBUG
    cout << __func__ << "receive : [pt][" << packet.get_payload_type() << "]"
         << "[pl][" << packet.get_payload_length() << "]"
         << "[timestamp][" << packet.get_timestamp() << "]"
         << "[sequence][" << packet.get_sequence() << "]" << endl;
#endif
}

void MyRtspClient::on_response(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client &owner)
{
#if CONSOLE_DEBUG
    cout << "[CLIENT CALLBACK]" << __func__ << endl;
    cout << resp->to_string() << endl;
#endif
    asyncRequestCtxt *ctxt = dynamic_cast<asyncRequestCtxt*>(req->get_context());

    if (ctxt)
        ctxt->onResponse(resp, req);
}

swift::net::rtsp::response_ptr MyRtspClient::on_request(swift::net::rtsp::request_ptr req, swift::net::rtsp::client &owner)
{
#if CONSOLE_DEBUG
    cout << "[CLIENT CALLBACK]" << __func__ << endl;
    cout << req->to_string() << endl;
#endif
    return swift::net::rtsp::response_ptr();
}

void MyRtspClient::on_request_timeout(swift::net::rtsp::request_ptr req, unsigned int millisec, swift::net::rtsp::client &owner)
{
#if CONSOLE_DEBUG
    cout << "[CLIENT CALLBACK]" << __func__ << endl;
    cout << req->to_string() << endl;
#endif
}

void MyRtspClient::on_request_send_error(swift::net::rtsp::request_ptr req, const swift::error_code &ec, swift::net::rtsp::client &owner)
{
#if CONSOLE_DEBUG
    cout << "[CLIENT CALLBACK]" << __func__ << endl;
    cout << req->to_string() << endl;
#endif
}

void MyRtspClient::on_session_setup(swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::request_ptr req, swift::net::rtsp::response_ptr resp, swift::net::rtsp::client &owner)
{
    swift::octets sessid = sess->get_id();
    QString header = "[On_Session_SETUP]: Session ID: " + QString::number(sessid.to_int());
    // 这里的回复是一定要成功吗?
    if (!resp->is_success()) {
        header += "成功";
    } else {
        header += "失败";
    }

    std::string _resp = resp->to_string();

    emit getClientResp(header, _resp.c_str());
#if CONSOLE_DEBUG
    cout << "[CLIENT CALLBACK]" << __func__ << endl;
    cout << "[session][" << sess->get_id() << endl;
#endif
    m_session = sess;
}

void MyRtspClient::on_session_response(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::client &owner)
{
    ///每次当前Session回复消息时都会回调这里
#if 0
    swift::octets sessid = sess->get_id();
    QString header = "[On_Session_Response]: Session ID:" + QString::number(sessid.to_int());
    std::string _resp = resp->to_string();
    emit getClientResp(header, _resp.c_str());
#endif
#if CONSOLE_DEBUG
    cout << "[CLIENT CALLBACK]" << __func__ << endl;
    cout << "[session][" << sess->get_id() << endl;
    cout << resp->to_string() << endl;
#endif
    asyncRequestCtxt *ctxt = dynamic_cast<asyncRequestCtxt*>(req->get_context());
    if(ctxt)
        ctxt->onResponse(resp, req, sess);
}

swift::net::rtsp::response_ptr MyRtspClient::on_session_request(swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::client &owner)
{
    /// 客户端发送请求后调用
#if CONSOLE_DEBUG
    cout << "[CLIENT CALLBACK]" << __func__ << endl;
    cout << "[session][" << sess->get_id() << endl;
    cout << req->to_string() << endl;
#endif
    return swift::net::rtsp::response_ptr();
}

void MyRtspClient::on_session_request_timeout(swift::net::rtsp::request_ptr req, unsigned int millisec, swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::client &owner)
{
    swift::octets sessid = sess->get_id();
    QString header = "[On_Session_timeout]: Session ID:" + QString::number(sessid.to_int());
    QString _resp = QString::number(millisec) + "毫秒 无响应";
    emit getClientResp(header, _resp);
#if CONSOLE_DEBUG
    cout << "[CLIENT CALLBACK]" << __func__ << endl;
    cout << "[session][" << sess->get_id() << endl;
    cout << req->to_string() << endl;
#endif
}

void MyRtspClient::on_session_request_send_error(swift::net::rtsp::request_ptr req, const swift::error_code &ec, swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::client &owner)
{
#if CONSOLE_DEBUG
    cout << "[CLIENT CALLBACK]" << __func__ << endl;
    cout << "[session][" << sess->get_id() << endl;
    cout << req->to_string() << endl;
#endif
}

void MyRtspClient::on_session_teardown(swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::client &owner)
{
    swift::octets sessid = sess->get_id();
    QString header = "[On_Session_Teardown]: Session ID:" + QString::number(sessid.to_int());
    QString _resp = "终止当前会话..........";
    emit getClientResp(header, _resp);
#if CONSOLE_DEBUG
    cout << "[CLIENT CALLBACK]" << __func__ << endl;
    cout << "[session][" << sess->get_id() << endl;
#endif
}

void MyRtspClient::on_not_matched_response(swift::net::rtsp::response_ptr resp, swift::net::rtsp::client &owner)
{
}
