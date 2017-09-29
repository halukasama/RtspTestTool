#ifndef __TEST_CLIENT_H__
#define __TEST_CLIENT_H__

#include <swift/net/rtsp/rtsp.hpp>
#include <swift/net/rtp/rtp.hpp>
#include <swift/utils/context.hpp>
#include <swift/net/http/http_authenication.hpp>
#include <swift/net/socket/ip/udp.hpp>
#include <iostream>
#include <QObject>

/*!
 * MyDeviceInfo, 用于存放路径,监听端口等简单的用户输入配置
 * 当无配置或缺少必要配置时，提供默认配置
 * deviceip:		摄像机地址
 * serverprot: 		rtsp服务端口
 * username 		用户名
 * password			密码
 * listenport		本地监听端口
 */

struct MyDeviceInfo
{
    QString m_deviceIp = "192.168.0.236";
    unsigned int m_serverPort = 554;
    QString m_username = "admin";
    QString m_password = "bq111111";
    unsigned int m_listenPort = 43966;
};

/*! MyAuthenCtxt
 * 针对需要RTSP认证的设备 创建新的请求消息
 * 因http中的请求与rtsp的请求消息内容有差别
 * 所以需要适当修改初始化，以及填充请求等功能
 */

class MyAuthenCtxt;
typedef swift::shared_ptr<MyAuthenCtxt> MyAuthenCtxtptr;

class MyAuthenication : public swift::net::http::authenication
{
public:
    static MyAuthenCtxtptr makeAuthenContext(swift::net::rtsp::response_ptr resp, swift::net::http::_authenication_mode prior_mode);
    static bool initAuthenCtxt(MyAuthenCtxtptr ctxt, const swift::net::hlp_core::const_header_ptr& authen_header,
                                         swift::net::http::_authenication_mode prior_mode);

};

class MyAuthenCtxt : public swift::net::http::authen_ctxt
{
public:
    void fillRequest(swift::net::rtsp::request_ptr req);

    friend class MyAuthenication;
};

/*!
 *  为对应的请求添加context
 *  异步回调需要发出请求时传入相应的凭据(这里保存一个客户端的引用)
 *  当接收回复消息时应该清楚是哪个客户端发送请求产生的回调
 */

////////////////////////////////////////////////////////////////////////////////
/// \brief The asyncRequestCtxt class
///
class MyRtspClient;

class asyncRequestCtxt : public swift::context
{
public:
    asyncRequestCtxt(MyRtspClient &myClient);

    virtual void onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req);
    virtual void onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess);

    // context interface
protected:
    void on_holder_destroy();

protected:
    MyRtspClient &m_myClient;
    bool m_bAccessdenied;
};

class optCtxt : public asyncRequestCtxt
{
public:
    optCtxt(MyRtspClient &myClient);

    virtual void onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess);
};

class descCtext : public asyncRequestCtxt
{
public:
    descCtext(MyRtspClient &myClient);

    virtual void onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req);
};

class setupCtxt : public asyncRequestCtxt
{
public:
    setupCtxt(MyRtspClient &myClient);

    virtual void onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess);
};

class sessionSetupCtxt : public asyncRequestCtxt
{
public:
    sessionSetupCtxt(MyRtspClient &myClient);

    virtual void onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess);
};

class playCtxt : public asyncRequestCtxt
{
public:
    playCtxt(MyRtspClient &myClient);

    virtual void onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess);
};

class pauseCtxt : public asyncRequestCtxt
{
public:
    pauseCtxt(MyRtspClient &myClient);

    virtual void onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess);
};

class teardownCtxt : public asyncRequestCtxt
{
public:
    teardownCtxt(MyRtspClient &myClient);

    virtual void onResponse(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess);
};

////////////////////////////////////////////////////////////////////////////////

/*!
 * 	实现rtsp客户端,和rtp的接收端，即完善相应回调;
 *
 *  |->	发送请求<—————————————————————————————————————填写下一个的请求
 *  |		|-->等待回复											|
 *  |			|--->获得回复										|
 *  |					|--->执行回调clientCallBack函数			|
 *  |								|							|
 *  |								|--->派发给对应context回调		|
 *  |											根据协议流程—————	|
 *  |											失败	|	|成功
 *  |	根据回复信息处理失败的消息<—————————————————————|	|——————————>接收RTP数据（成功）
 *  |		可继续处理|		|不可继续处理
 *  创建发送请求<—————|		|—————————————————————————————————>返回错误信息(失败)
 *  		\
 *  		 \--- 这里注意一定要将通信序号加1
 */

////////////////////////////////////////////////////////////////////////////////

class MyRtspClient : public QObject, public swift::net::rtsp::client_callback, public swift::net::rtp::receiver_callback
{
    Q_OBJECT
public:
    MyRtspClient(swift::engine &engine, MyDeviceInfo *info, QObject *parent = 0);
    void start();
    void stop();
public:
    swift::net::rtsp::client m_client;
    swift::net::rtsp::client_session_ptr m_session;
    swift::net::rtp::receiver m_receiver;
    swift::net::ip::udp::socket m_socket;

    MyDeviceInfo *m_info;
    QString m_url;
    QString m_setupUrl;
signals:
    void getClientResp(const QString &header, const QString &body);

    /// 发送RTSP 安全认证请求
public:
    /**
     * 发送此请求进行安全认证
     * 要在context 保存的客户端中调用(当回复消息头是401 "未认证"时)
     * @param [in]
     * @param [in]
     * @param [in]
     */
    void sendSafeAuthenticationRequest(swift::net::rtsp::request_ptr req, swift::net::rtsp::response_ptr resp, swift::context *ctxt);
private:
    /**
     * 收到RTP数据包
     *
     * @param [in] packet RTP数据包
     * @param [in] remote_endpoint RTP数据发送方地址
     * @param [in] owner 发出此回调的receiver实例
     */
    virtual void on_rtp_packet(swift::net::rtp::rtp_packet &packet, const swift::net::ip::endpoint &remoteEP, swift::net::rtp::receiver &owner);

    /// 会话外消息回调
private:
    /**
     * 收到会话外响应
     *
     * @param [in] resp 收到的响应消息
     * @param [in] req  与响应对应的请求消息
     */
    virtual void on_response(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client &owner);

    /**
     * 收到会话外请求
     *
     * @param [in] req 收到的请求消息
     */
    virtual swift::net::rtsp::response_ptr on_request(swift::net::rtsp::request_ptr req, swift::net::rtsp::client &owner);

    /**
     * 发送请求超时
     *
     * @param [in] req  请求消息
     * @param [in] millisec 超时时间
     */
    virtual void on_request_timeout(swift::net::rtsp::request_ptr req, unsigned int millisec, swift::net::rtsp::client &owner);

    /**
     * 请求消息发送失败
     *
     * @param [in] req  请求消息
     * @param [in] ec   错误码
     */
    virtual void on_request_send_error(swift::net::rtsp::request_ptr req, const swift::error_code &ec, swift::net::rtsp::client &owner);

    /// 会话相关回调
private:
    /**
     * 会话建立
     *
     * @param [in] sess 新建立的会话
     * @param [in] req  用于建立会话的Setup请求
     * @param [in] resp 与请求消息对应的响应消息
     */
    virtual void on_session_setup(swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::request_ptr req, swift::net::rtsp::response_ptr resp, swift::net::rtsp::client &owner);

    /**
     * 收到会话内响应
     *
     * @param [in] resp 收到的响应消息
     * @param [in] req  与响应对应的请求消息
     * @param [in] sess 消息所属会话
     */
    virtual void on_session_response(swift::net::rtsp::response_ptr resp, swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::client &owner);

    /**
     * 收到会话内请求
     *
     * @param [in] req 收到的请求消息
     * @param [in] sess 消息所属会话
     */
    virtual swift::net::rtsp::response_ptr on_session_request(swift::net::rtsp::request_ptr req, swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::client &owner);

    /**
     * 发送请求超时
     *
     * @param [in] req  请求消息
     * @param [in] millisec 超时时间
     */
    virtual void on_session_request_timeout(swift::net::rtsp::request_ptr req, unsigned int millisec, swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::client &owner);

    /**
     * 请求消息发送失败
     *
     * @param [in] req  请求消息
     * @param [in] ec   错误码
     */
    virtual void on_session_request_send_error(swift::net::rtsp::request_ptr req, const swift::error_code &ec, swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::client &owner);
    /**
     * 会话终止
     *
     * @param [in] sess 终止的会话
     */
    virtual void on_session_teardown(swift::net::rtsp::client_session_ptr sess, swift::net::rtsp::client &owner );

    /**
     * 不匹配的回复
     * @brief on_not_matched_response
     * @param resp
     * @param owner
     */
    virtual void on_not_matched_response(swift::net::rtsp::response_ptr resp, swift::net::rtsp::client &owner);
};
#endif /// __TEST_CLIENT_H__
