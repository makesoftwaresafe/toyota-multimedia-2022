/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsocks5socketengine_p.h"

#ifndef QT_NO_SOCKS5

#include "qtcpsocket.h"
#include "qudpsocket.h"
#include "qtcpserver.h"
#include "qdebug.h"
#include "qhash.h"
#include "qqueue.h"
#include "qelapsedtimer.h"
#include "qmutex.h"
#include "qthread.h"
#include "qcoreapplication.h"
#include "qurl.h"
#include "qauthenticator.h"
#include <qendian.h>
#include <qnetworkinterface.h>
#include "FjInspiriumNetworkLogger.h"

QT_BEGIN_NAMESPACE

static const int MaxWriteBufferSize = 128*1024;

//#define QSOCKS5SOCKETLAYER_DEBUG

#define MAX_DATA_DUMP 256
#if !defined(Q_OS_WINCE)
#define SOCKS5_BLOCKING_BIND_TIMEOUT 5000
#else
#define SOCKS5_BLOCKING_BIND_TIMEOUT 10000
#endif

#define Q_INIT_CHECK(returnValue) do { \
    if (!d->data) { \
        return returnValue; \
    } } while (0)

#define S5_VERSION_5 0x05
#define S5_CONNECT 0x01
#define S5_BIND 0x02
#define S5_UDP_ASSOCIATE 0x03
#define S5_IP_V4 0x01
#define S5_DOMAINNAME 0x03
#define S5_IP_V6 0x04
#define S5_SUCCESS 0x00
#define S5_R_ERROR_SOCKS_FAILURE 0x01
#define S5_R_ERROR_CON_NOT_ALLOWED 0x02
#define S5_R_ERROR_NET_UNREACH 0x03
#define S5_R_ERROR_HOST_UNREACH 0x04
#define S5_R_ERROR_CONN_REFUSED 0x05
#define S5_R_ERROR_TTL 0x06
#define S5_R_ERROR_CMD_NOT_SUPPORTED 0x07
#define S5_R_ERROR_ADD_TYPE_NOT_SUPORTED 0x08

#define S5_AUTHMETHOD_NONE 0x00
#define S5_AUTHMETHOD_PASSWORD 0x02
#define S5_AUTHMETHOD_NOTACCEPTABLE 0xFF

#define S5_PASSWORDAUTH_VERSION 0x01

#ifdef QSOCKS5SOCKETLAYER_DEBUG
#  define QSOCKS5_Q_DEBUG qDebug() << this
#  define QSOCKS5_D_DEBUG qDebug() << q_ptr
#  define QSOCKS5_DEBUG qDebug() << "[QSocks5]"
static QString s5StateToString(QSocks5SocketEnginePrivate::Socks5State s)
{
    switch (s) {
    case QSocks5SocketEnginePrivate::Uninitialized: return QLatin1String("Uninitialized");
    case QSocks5SocketEnginePrivate::ConnectError: return QLatin1String("ConnectError");
    case QSocks5SocketEnginePrivate::AuthenticationMethodsSent: return QLatin1String("AuthenticationMethodsSent");
    case QSocks5SocketEnginePrivate::Authenticating: return QLatin1String("Authenticating");
    case QSocks5SocketEnginePrivate::AuthenticatingError: return QLatin1String("AuthenticatingError");
    case QSocks5SocketEnginePrivate::RequestMethodSent: return QLatin1String("RequestMethodSent");
    case QSocks5SocketEnginePrivate::RequestError: return QLatin1String("RequestError");
    case QSocks5SocketEnginePrivate::Connected: return QLatin1String("Connected");
    case QSocks5SocketEnginePrivate::UdpAssociateSuccess: return QLatin1String("UdpAssociateSuccess");
    case QSocks5SocketEnginePrivate::BindSuccess: return QLatin1String("BindSuccess");
    case QSocks5SocketEnginePrivate::ControlSocketError: return QLatin1String("ControlSocketError");
    case QSocks5SocketEnginePrivate::SocksError: return QLatin1String("SocksError");
    case QSocks5SocketEnginePrivate::HostNameLookupError: return QLatin1String("HostNameLookupError");
    default: break;
    }
    return QLatin1String("unknown state");
}

static QString dump(const QByteArray &buf)
{
    QString data;
    for (int i = 0; i < qMin<int>(MAX_DATA_DUMP, buf.size()); ++i) {
        if (i) data += QLatin1Char(' ');
        uint val = (unsigned char)buf.at(i);
       // data += QString("0x%1").arg(val, 3, 16, QLatin1Char('0'));
        data += QString::number(val);
    }
    if (buf.size() > MAX_DATA_DUMP)
        data += QLatin1String(" ...");

    return QString::fromLatin1("size: %1 data: { %2 }").arg(buf.size()).arg(data);
}

#else
#  define QSOCKS5_DEBUG if (0) qDebug()
#  define QSOCKS5_Q_DEBUG if (0) qDebug()
#  define QSOCKS5_D_DEBUG if (0) qDebug()

static inline QString s5StateToString(QSocks5SocketEnginePrivate::Socks5State) { return QString(); }
static inline QString dump(const QByteArray &) { return QString(); }
#endif

/*
   inserts the host address in buf at pos and updates pos.
   if the func fails the data in buf and the vallue of pos is undefined
*/
static bool qt_socks5_set_host_address_and_port(const QHostAddress &address, quint16 port, QByteArray *pBuf)
{
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("setting [%s : %i]", address.toString().toUtf8().constData(), port);
#endif

    union {
        quint16 port;
        quint32 ipv4;
        QIPv6Address ipv6;
        char ptr;
    } data;

    // add address
    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        data.ipv4 = qToBigEndian<quint32>(address.toIPv4Address());
        pBuf->append(S5_IP_V4);
        pBuf->append(QByteArray::fromRawData(&data.ptr, sizeof data.ipv4));
    } else if (address.protocol() == QAbstractSocket::IPv6Protocol) {
        data.ipv6 = address.toIPv6Address();
        pBuf->append(S5_IP_V6);
        pBuf->append(QByteArray::fromRawData(&data.ptr, sizeof data.ipv6));
    } else {
        return false;
    }

    // add port
    data.port = qToBigEndian<quint16>(port);
    pBuf->append(QByteArray::fromRawData(&data.ptr, sizeof data.port));
    return true;
}

/*
   like above, but for a hostname
*/
static bool qt_socks5_set_host_name_and_port(const QString &hostname, quint16 port, QByteArray *pBuf)
{
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("setting [%s:%i]", hostname.toUtf8().constData(), port);
#endif

    QByteArray encodedHostName = QUrl::toAce(hostname);
    QByteArray &buf = *pBuf;

    if (encodedHostName.length() > 255)
        return false;

    buf.append(S5_DOMAINNAME);
    buf.append(uchar(encodedHostName.length()));
    buf.append(encodedHostName);

    // add port
    union {
        quint16 port;
        char ptr;
    } data;
    data.port = qToBigEndian<quint16>(port);
    buf.append(QByteArray::fromRawData(&data.ptr, sizeof data.port));

    return true;
}


/*
   retrives the host address in buf at pos and updates pos.
   return 1 if OK, 0 if need more data, -1 if error
   if the func fails the value of the address and the pos is undefined
*/
static int qt_socks5_get_host_address_and_port(const QByteArray &buf, QHostAddress *pAddress, quint16 *pPort, int *pPos)
{
    int ret = -1;
    int pos = *pPos;
    const unsigned char *pBuf = reinterpret_cast<const unsigned char*>(buf.constData());
    QHostAddress address;
    quint16 port = 0;

    if (buf.size() - pos < 1) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("need more data address/port");
#endif
        return 0;
    }
    if (pBuf[pos] == S5_IP_V4) {
        pos++;
        if (buf.size() - pos < 4) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("need more data for ip4 address");
#endif
            return 0;
        }
        address.setAddress(qFromBigEndian<quint32>(&pBuf[pos]));
        pos += 4;
        ret = 1;
    } else if (pBuf[pos] == S5_IP_V6) {
        pos++;
        if (buf.size() - pos < 16) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("need more data for ip6 address");
#endif
            return 0;
        }
        QIPv6Address add;
        for (int i = 0; i < 16; ++i)
            add[i] = buf[pos++];
        address.setAddress(add);
        ret = 1;
    } else if (pBuf[pos] == S5_DOMAINNAME){
        // just skip it
        pos++;
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("skipping hostname of len %u", uint(pBuf[pos]));
#endif
        pos += uchar(pBuf[pos]);
    } else {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("invalid address type %d", (int)pBuf[pos]);
#endif
        ret = -1;
    }

    if (ret == 1) {
        if (buf.size() - pos < 2) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("need more data for port");
#endif
            return 0;
        }
        port = qFromBigEndian<quint16>(&pBuf[pos]);
        pos += 2;
    }

    if (ret == 1) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("got [%s : %i]", address.toString().toUtf8().constData(), port);
#endif
        *pAddress = address;
        *pPort = port;
        *pPos = pos;
    }

    return ret;
}

/*
   Returns the difference between msecs and elapsed. If msecs is -1,
   however, -1 is returned.
*/
static int qt_timeout_value(int msecs, int elapsed)
{
    if (msecs == -1)
        return -1;

    int timeout = msecs - elapsed;
    return timeout < 0 ? 0 : timeout;
}

struct QSocks5Data
{
    QTcpSocket *controlSocket;
    QSocks5Authenticator *authenticator;
};

struct QSocks5ConnectData : public QSocks5Data
{
    QByteArray readBuffer;
};

struct QSocks5BindData : public QSocks5Data
{
    QHostAddress localAddress;
    quint16 localPort;
    QHostAddress peerAddress;
    quint16 peerPort;
    QElapsedTimer timeStamp;
};

struct QSocks5RevivedDatagram
{
    QByteArray data;
    QHostAddress address;
    quint16 port;
};

#ifndef QT_NO_UDPSOCKET
struct QSocks5UdpAssociateData : public QSocks5Data
{
    QUdpSocket *udpSocket;
    QHostAddress associateAddress;
    quint16 associatePort;
    QQueue<QSocks5RevivedDatagram> pendingDatagrams;
};
#endif

// needs to be thread safe
class QSocks5BindStore : public QObject
{
public:
    QSocks5BindStore();
    ~QSocks5BindStore();

    void add(qintptr socketDescriptor, QSocks5BindData *bindData);
    bool contains(qintptr socketDescriptor);
    QSocks5BindData *retrieve(qintptr socketDescriptor);

protected:
    void timerEvent(QTimerEvent * event);

    QMutex mutex;
    int sweepTimerId;
    //socket descriptor, data, timestamp
    QHash<int, QSocks5BindData *> store;
};

Q_GLOBAL_STATIC(QSocks5BindStore, socks5BindStore)

QSocks5BindStore::QSocks5BindStore()
    : mutex(QMutex::Recursive)
    , sweepTimerId(-1)
{
    QCoreApplication *app = QCoreApplication::instance();
    if (app && app->thread() != thread())
        moveToThread(app->thread());
}

QSocks5BindStore::~QSocks5BindStore()
{
}

void QSocks5BindStore::add(qintptr socketDescriptor, QSocks5BindData *bindData)
{
    QMutexLocker lock(&mutex);
    if (store.contains(socketDescriptor)) {
        // qDebug() << "delete it";
    }
    bindData->timeStamp.start();
    store.insert(socketDescriptor, bindData);
    // start sweep timer if not started
    if (sweepTimerId == -1)
        sweepTimerId = startTimer(60000);
}

bool QSocks5BindStore::contains(qintptr socketDescriptor)
{
    QMutexLocker lock(&mutex);
    return store.contains(socketDescriptor);
}

QSocks5BindData *QSocks5BindStore::retrieve(qintptr socketDescriptor)
{
    QMutexLocker lock(&mutex);
    if (!store.contains(socketDescriptor))
        return 0;
    QSocks5BindData *bindData = store.take(socketDescriptor);
    if (bindData) {
        if (bindData->controlSocket->thread() != QThread::currentThread()) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("Can not access socks5 bind data from different thread");
#endif
            return 0;
        }
    } else {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("__ERROR__ binddata == 0");
#endif
    }
    // stop the sweep timer if not needed
    if (store.isEmpty()) {
        killTimer(sweepTimerId);
        sweepTimerId = -1;
    }
    return bindData;
}

void QSocks5BindStore::timerEvent(QTimerEvent * event)
{
    QMutexLocker lock(&mutex);
    if (event->timerId() == sweepTimerId) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("QSocks5BindStore performing sweep");
#endif
        QMutableHashIterator<int, QSocks5BindData *> it(store);
        while (it.hasNext()) {
            it.next();
            if (it.value()->timeStamp.hasExpired(350000)) {
#ifdef FJ_INSP_NETWORKLOG
                DEBUG_SOCKET_QDEBUG2("QSocks5BindStore removing JJJJ");
#endif
                it.remove();
            }
        }
    }
}

QSocks5Authenticator::QSocks5Authenticator()
{
}

QSocks5Authenticator::~QSocks5Authenticator()
{
}

char QSocks5Authenticator::methodId()
{
    return 0x00;
}

bool QSocks5Authenticator::beginAuthenticate(QTcpSocket *socket, bool *completed)
{
    Q_UNUSED(socket);
    *completed = true;
    return true;
}

bool QSocks5Authenticator::continueAuthenticate(QTcpSocket *socket, bool *completed)
{
    Q_UNUSED(socket);
    *completed = true;
    return true;
}

bool QSocks5Authenticator::seal(const QByteArray buf, QByteArray *sealedBuf)
{
    *sealedBuf = buf;
    return true;
}

bool QSocks5Authenticator::unSeal(const QByteArray sealedBuf, QByteArray *buf)
{
    *buf = sealedBuf;
    return true;
}

bool QSocks5Authenticator::unSeal(QTcpSocket *sealedSocket, QByteArray *buf)
{
    return unSeal(sealedSocket->readAll(), buf);
}

QSocks5PasswordAuthenticator::QSocks5PasswordAuthenticator(const QString &userName, const QString &password)
{
    this->userName = userName;
    this->password = password;
}

char QSocks5PasswordAuthenticator::methodId()
{
    return 0x02;
}

bool QSocks5PasswordAuthenticator::beginAuthenticate(QTcpSocket *socket, bool *completed)
{
    *completed = false;
    QByteArray uname = userName.toLatin1();
    QByteArray passwd = password.toLatin1();
    QByteArray dataBuf(3 + uname.size() + passwd.size(), 0);
    char *buf = dataBuf.data();
    int pos = 0;
    buf[pos++] = S5_PASSWORDAUTH_VERSION;
    buf[pos++] = uname.size();
    memcpy(&buf[pos], uname.data(), uname.size());
    pos += uname.size();
    buf[pos++] = passwd.size();
    memcpy(&buf[pos], passwd.data(), passwd.size());
    return socket->write(dataBuf) == dataBuf.size();
}

bool QSocks5PasswordAuthenticator::continueAuthenticate(QTcpSocket *socket, bool *completed)
{
    *completed = false;

    if (socket->bytesAvailable() < 2)
        return true;

    QByteArray buf = socket->read(2);
    if (buf.at(0) == S5_PASSWORDAUTH_VERSION && buf.at(1) == 0x00) {
        *completed = true;
        return true;
    }

    // must disconnect
    socket->close();
    return false;
}

QString QSocks5PasswordAuthenticator::errorString()
{
    return QLatin1String("Socks5 user name or password incorrect");
}



QSocks5SocketEnginePrivate::QSocks5SocketEnginePrivate()
    : socks5State(Uninitialized)
    , readNotificationEnabled(false)
    , writeNotificationEnabled(false)
    , exceptNotificationEnabled(false)
    , socketDescriptor(-1)
    , data(0)
    , connectData(0)
#ifndef QT_NO_UDPSOCKET
    , udpData(0)
#endif
    , bindData(0)
    , readNotificationActivated(false)
    , writeNotificationActivated(false)
    , readNotificationPending(false)
    , writeNotificationPending(false)
    , connectionNotificationPending(false)
{
    mode = NoMode;
}

QSocks5SocketEnginePrivate::~QSocks5SocketEnginePrivate()
{
}

void QSocks5SocketEnginePrivate::initialize(Socks5Mode socks5Mode)
{
    Q_Q(QSocks5SocketEngine);

    mode = socks5Mode;
    if (mode == ConnectMode) {
        connectData = new QSocks5ConnectData;
        data = connectData;
#ifndef QT_NO_UDPSOCKET
    } else if (mode == UdpAssociateMode) {
        udpData = new QSocks5UdpAssociateData;
        data = udpData;
        udpData->udpSocket = new QUdpSocket(q);
#ifndef QT_NO_BEARERMANAGEMENT
        udpData->udpSocket->setProperty("_q_networksession", q->property("_q_networksession"));
#endif
        udpData->udpSocket->setProxy(QNetworkProxy::NoProxy);
        QObject::connect(udpData->udpSocket, SIGNAL(readyRead()),
                         q, SLOT(_q_udpSocketReadNotification()),
                         Qt::DirectConnection);
#endif // QT_NO_UDPSOCKET
    } else if (mode == BindMode) {
        bindData = new QSocks5BindData;
        data = bindData;
    }

    data->controlSocket = new QTcpSocket(q);
#ifndef QT_NO_BEARERMANAGEMENT
    data->controlSocket->setProperty("_q_networksession", q->property("_q_networksession"));
#endif
    data->controlSocket->setProxy(QNetworkProxy::NoProxy);
    QObject::connect(data->controlSocket, SIGNAL(connected()), q, SLOT(_q_controlSocketConnected()),
                     Qt::DirectConnection);
    QObject::connect(data->controlSocket, SIGNAL(readyRead()), q, SLOT(_q_controlSocketReadNotification()),
                     Qt::DirectConnection);
    QObject::connect(data->controlSocket, SIGNAL(bytesWritten(qint64)), q, SLOT(_q_controlSocketBytesWritten()),
                     Qt::DirectConnection);
    QObject::connect(data->controlSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                     q, SLOT(_q_controlSocketError(QAbstractSocket::SocketError)),
                     Qt::DirectConnection);
    QObject::connect(data->controlSocket, SIGNAL(disconnected()), q, SLOT(_q_controlSocketDisconnected()),
                     Qt::DirectConnection);
    QObject::connect(data->controlSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                     q, SLOT(_q_controlSocketStateChanged(QAbstractSocket::SocketState)),
                     Qt::DirectConnection);

    if (!proxyInfo.user().isEmpty() || !proxyInfo.password().isEmpty()) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("using username/password authentication; user = %s", proxyInfo.user().toUtf8().constData());
#endif
        data->authenticator = new QSocks5PasswordAuthenticator(proxyInfo.user(), proxyInfo.password());
    } else {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("not using authentication");
#endif
        data->authenticator = new QSocks5Authenticator();
    }
}

void QSocks5SocketEnginePrivate::setErrorState(Socks5State state, const QString &extraMessage)
{
    Q_Q(QSocks5SocketEngine);

    switch (state) {
    case Uninitialized:
    case Authenticating:
    case AuthenticationMethodsSent:
    case RequestMethodSent:
    case Connected:
    case UdpAssociateSuccess:
    case BindSuccess:
        // these aren't error states
        return;

    case ConnectError:
    case ControlSocketError: {
        QAbstractSocket::SocketError controlSocketError = data->controlSocket->error();
        if (socks5State != Connected) {
            switch (controlSocketError) {
            case QAbstractSocket::ConnectionRefusedError:
                q->setError(QAbstractSocket::ProxyConnectionRefusedError,
                             QSocks5SocketEngine::tr("Connection to proxy refused"));
                break;
            case QAbstractSocket::RemoteHostClosedError:
                q->setError(QAbstractSocket::ProxyConnectionClosedError,
                             QSocks5SocketEngine::tr("Connection to proxy closed prematurely"));
                break;
            case QAbstractSocket::HostNotFoundError:
                q->setError(QAbstractSocket::ProxyNotFoundError,
                             QSocks5SocketEngine::tr("Proxy host not found"));
                break;
            case QAbstractSocket::SocketTimeoutError:
                if (state == ConnectError) {
                    q->setError(QAbstractSocket::ProxyConnectionTimeoutError,
                                 QSocks5SocketEngine::tr("Connection to proxy timed out"));
                    break;
                }
                /* fall through */
            default:
                q->setError(controlSocketError, data->controlSocket->errorString());
                break;
            }
        } else {
            q->setError(controlSocketError, data->controlSocket->errorString());
        }
        break;
    }

    case AuthenticatingError:
        q->setError(QAbstractSocket::ProxyAuthenticationRequiredError,
                    extraMessage.isEmpty() ?
                     QSocks5SocketEngine::tr("Proxy authentication failed") :
                     QSocks5SocketEngine::tr("Proxy authentication failed: %1").arg(extraMessage));
        break;

    case RequestError:
        // error code set by caller (overload)
        break;

    case SocksError:
        q->setError(QAbstractSocket::ProxyProtocolError,
                     QSocks5SocketEngine::tr("SOCKS version 5 protocol error"));
        break;

    case HostNameLookupError:
        q->setError(QAbstractSocket::HostNotFoundError,
                    QAbstractSocket::tr("Host not found"));
        break;
    }

    q->setState(QAbstractSocket::UnconnectedState);
    socks5State = state;
}

void QSocks5SocketEnginePrivate::setErrorState(Socks5State state, Socks5Error socks5error)
{
    Q_Q(QSocks5SocketEngine);
    switch (socks5error) {
    case SocksFailure:
        q->setError(QAbstractSocket::NetworkError,
                     QSocks5SocketEngine::tr("General SOCKSv5 server failure"));
        break;
    case ConnectionNotAllowed:
        q->setError(QAbstractSocket::SocketAccessError,
                     QSocks5SocketEngine::tr("Connection not allowed by SOCKSv5 server"));
        break;
    case NetworkUnreachable:
        q->setError(QAbstractSocket::NetworkError,
                    QAbstractSocket::tr("Network unreachable"));
        break;
    case HostUnreachable:
        q->setError(QAbstractSocket::HostNotFoundError,
                    QAbstractSocket::tr("Host not found"));
        break;
    case ConnectionRefused:
        q->setError(QAbstractSocket::ConnectionRefusedError,
                    QAbstractSocket::tr("Connection refused"));
        break;
    case TTLExpired:
        q->setError(QAbstractSocket::NetworkError,
                     QSocks5SocketEngine::tr("TTL expired"));
        break;
    case CommandNotSupported:
        q->setError(QAbstractSocket::UnsupportedSocketOperationError,
                     QSocks5SocketEngine::tr("SOCKSv5 command not supported"));
        break;
    case AddressTypeNotSupported:
        q->setError(QAbstractSocket::UnsupportedSocketOperationError,
                     QSocks5SocketEngine::tr("Address type not supported"));
        break;

    default:
        q->setError(QAbstractSocket::UnknownSocketError,
                     QSocks5SocketEngine::tr("Unknown SOCKSv5 proxy error code 0x%1").arg(int(socks5error), 16));
        break;
    }

    setErrorState(state, QString());
}

void QSocks5SocketEnginePrivate::reauthenticate()
{
    Q_Q(QSocks5SocketEngine);

    // we require authentication
    QAuthenticator auth;
    emit q->proxyAuthenticationRequired(proxyInfo, &auth);

    if (!auth.user().isEmpty() || !auth.password().isEmpty()) {
        // we have new credentials, let's try again
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("authentication failure: retrying connection");
#endif
        socks5State = QSocks5SocketEnginePrivate::Uninitialized;

        delete data->authenticator;
        proxyInfo.setUser(auth.user());
        proxyInfo.setPassword(auth.password());
        data->authenticator = new QSocks5PasswordAuthenticator(proxyInfo.user(), proxyInfo.password());

        {
            const QSignalBlocker blocker(data->controlSocket);
            data->controlSocket->abort();
        }
        data->controlSocket->connectToHost(proxyInfo.hostName(), proxyInfo.port());
    } else {
        // authentication failure

        setErrorState(AuthenticatingError);
        data->controlSocket->close();
        emitConnectionNotification();
    }
}

void QSocks5SocketEnginePrivate::parseAuthenticationMethodReply()
{
    // not enough data to begin
    if (data->controlSocket->bytesAvailable() < 2)
        return;

    QByteArray buf = data->controlSocket->read(2);
    if (buf.at(0) != S5_VERSION_5) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("Socks5 version incorrect");
#endif
        setErrorState(SocksError);
        data->controlSocket->close();
        emitConnectionNotification();
        return;
    }

    bool authComplete = false;
    if (uchar(buf.at(1)) == S5_AUTHMETHOD_NONE) {
        authComplete = true;
    } else if (uchar(buf.at(1)) == S5_AUTHMETHOD_NOTACCEPTABLE) {
        reauthenticate();
        return;
    } else if (buf.at(1) != data->authenticator->methodId()
               || !data->authenticator->beginAuthenticate(data->controlSocket, &authComplete)) {
        setErrorState(AuthenticatingError, QLatin1String("Socks5 host did not support authentication method."));
        socketError = QAbstractSocket::SocketAccessError; // change the socket error
        emitConnectionNotification();
        return;
    }

    if (authComplete)
        sendRequestMethod();
    else
        socks5State = Authenticating;
}

void QSocks5SocketEnginePrivate::parseAuthenticatingReply()
{
    bool authComplete = false;
    if (!data->authenticator->continueAuthenticate(data->controlSocket, &authComplete)) {
        reauthenticate();
        return;
    }
    if (authComplete)
        sendRequestMethod();
}

void QSocks5SocketEnginePrivate::sendRequestMethod()
{
    QHostAddress address;
    quint16 port = 0;
    char command = 0;
    if (mode == ConnectMode) {
        command = S5_CONNECT;
        address = peerAddress;
        port = peerPort;
    } else if (mode == BindMode) {
        command = S5_BIND;
        address = localAddress;
        port = localPort;
    } else {
#ifndef QT_NO_UDPSOCKET
        command = S5_UDP_ASSOCIATE;
        address = localAddress; //data->controlSocket->localAddress();
        port = localPort;
#endif
    }

    QByteArray buf;
    buf.reserve(270); // big enough for domain name;
    buf[0] = S5_VERSION_5;
    buf[1] = command;
    buf[2] = 0x00;
    if (peerName.isEmpty() && !qt_socks5_set_host_address_and_port(address, port, &buf)) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("error setting address %s : %i", address.toString().toUtf8().constData(), port);
#endif
        //### set error code ....
        return;
    } else if (!peerName.isEmpty() && !qt_socks5_set_host_name_and_port(peerName, port, &buf)) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("error setting peer name %s : %i", peerName.toUtf8().constData(), port);
#endif
        //### set error code ....
        return;
    }
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("sending %s", dump(buf).toUtf8().constData());
#endif
    QByteArray sealedBuf;
    if (!data->authenticator->seal(buf, &sealedBuf)) {
        // ### Handle this error.
    }
    data->controlSocket->write(sealedBuf);
    data->controlSocket->flush();
    socks5State = RequestMethodSent;
}

void QSocks5SocketEnginePrivate::parseRequestMethodReply()
{
    Q_Q(QSocks5SocketEngine);
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG3("parseRequestMethodReply()");
#endif

    QByteArray inBuf;
    if (!data->authenticator->unSeal(data->controlSocket, &inBuf)) {
        // ### check error and not just not enough data
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("unSeal failed, needs more data");
#endif
        return;
    }

    inBuf.prepend(receivedHeaderFragment);
    receivedHeaderFragment.clear();
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("%s", dump(inBuf).toUtf8().constData());
#endif
    if (inBuf.size() < 3) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("need more data for request reply header .. put this data somewhere");
#endif
        receivedHeaderFragment = inBuf;
        return;
    }

    QHostAddress address;
    quint16 port = 0;

    if (inBuf.at(0) != S5_VERSION_5 || inBuf.at(2) != 0x00) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("socks protocol error");
#endif
        setErrorState(SocksError);
    } else if (inBuf.at(1) != S5_SUCCESS) {
        Socks5Error socks5Error = Socks5Error(inBuf.at(1));
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("Request error : %d", socks5Error);
#endif
        if ((socks5Error == SocksFailure || socks5Error == ConnectionNotAllowed)
            && !peerName.isEmpty()) {
            // Dante seems to use this error code to indicate hostname resolution failure
            setErrorState(HostNameLookupError);
        } else {
            setErrorState(RequestError, socks5Error);
        }
    } else {
        // connection success, retrieve the remote addresses
        int pos = 3;
        int err = qt_socks5_get_host_address_and_port(inBuf, &address, &port, &pos);
        if (err == -1) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("error getting address");
#endif
            setErrorState(SocksError);
        } else if (err == 0) {
            //need more data
            receivedHeaderFragment = inBuf;
            return;
        } else {
            inBuf.remove(0, pos);
            for (int i = inBuf.size() - 1; i >= 0 ; --i)
                data->controlSocket->ungetChar(inBuf.at(i));
        }
    }

    if (socks5State == RequestMethodSent) {
        // no error
        localAddress = address;
        localPort = port;

        if (mode == ConnectMode) {
            socks5State = Connected;
            // notify the upper layer that we're done
            q->setState(QAbstractSocket::ConnectedState);
            emitConnectionNotification();
        } else if (mode == BindMode) {
            socks5State = BindSuccess;
            q->setState(QAbstractSocket::ListeningState);
        } else {
            socks5State = UdpAssociateSuccess;
        }
    } else if (socks5State == BindSuccess) {
        // no error and we got a connection
        bindData->peerAddress = address;
        bindData->peerPort = port;

        emitReadNotification();
    } else {
        // got an error
        data->controlSocket->close();
        emitConnectionNotification();
    }
}

void QSocks5SocketEnginePrivate::_q_emitPendingReadNotification()
{
    Q_Q(QSocks5SocketEngine);
    readNotificationPending = false;
    if (readNotificationEnabled) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("emitting readNotification");
#endif
        QPointer<QSocks5SocketEngine> qq = q;
        emit q->readNotification();
        if (!qq)
            return;
        // check if there needs to be a new zero read notification
        if (data && data->controlSocket->state() == QAbstractSocket::UnconnectedState
                && data->controlSocket->error() == QAbstractSocket::RemoteHostClosedError) {
            connectData->readBuffer.clear();
            emitReadNotification();
        }
    }
}

void QSocks5SocketEnginePrivate::emitReadNotification()
{
    Q_Q(QSocks5SocketEngine);
    readNotificationActivated = true;
    if (readNotificationEnabled && !readNotificationPending) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("queueing readNotification");
#endif
        readNotificationPending = true;
        QMetaObject::invokeMethod(q, "_q_emitPendingReadNotification", Qt::QueuedConnection);
    }
}

void QSocks5SocketEnginePrivate::_q_emitPendingWriteNotification()
{
    writeNotificationPending = false;
    Q_Q(QSocks5SocketEngine);
    if (writeNotificationEnabled) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("emitting writeNotification");
#endif
        emit q->writeNotification();
    }
}

void QSocks5SocketEnginePrivate::emitWriteNotification()
{
    Q_Q(QSocks5SocketEngine);
    writeNotificationActivated = true;
    if (writeNotificationEnabled && !writeNotificationPending) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("queueing writeNotification");
#endif
        writeNotificationPending = true;
        QMetaObject::invokeMethod(q, "_q_emitPendingWriteNotification", Qt::QueuedConnection);
    }
}

void QSocks5SocketEnginePrivate::_q_emitPendingConnectionNotification()
{
    connectionNotificationPending = false;
    Q_Q(QSocks5SocketEngine);
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("emitting connectionNotification");
#endif
    emit q->connectionNotification();
}

void QSocks5SocketEnginePrivate::emitConnectionNotification()
{
    Q_Q(QSocks5SocketEngine);
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("queueing connectionNotification");
#endif
    connectionNotificationPending = true;
    QMetaObject::invokeMethod(q, "_q_emitPendingConnectionNotification", Qt::QueuedConnection);
}

QSocks5SocketEngine::QSocks5SocketEngine(QObject *parent)
:QAbstractSocketEngine(*new QSocks5SocketEnginePrivate(), parent)
{
}

QSocks5SocketEngine::~QSocks5SocketEngine()
{
    Q_D(QSocks5SocketEngine);

    if (d->data) {
        delete d->data->authenticator;
        delete d->data->controlSocket;
    }
    if (d->connectData)
        delete d->connectData;
#ifndef QT_NO_UDPSOCKET
    if (d->udpData) {
        delete d->udpData->udpSocket;
        delete d->udpData;
    }
#endif
    if (d->bindData)
        delete d->bindData;
}

static QBasicAtomicInt descriptorCounter = Q_BASIC_ATOMIC_INITIALIZER(1);

bool QSocks5SocketEngine::initialize(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol protocol)
{
    Q_D(QSocks5SocketEngine);

    d->socketDescriptor = descriptorCounter.fetchAndAddRelaxed(1);

    d->socketType = type;
    d->socketProtocol = protocol;

    return true;
}

bool QSocks5SocketEngine::initialize(qintptr socketDescriptor, QAbstractSocket::SocketState socketState)
{
    Q_D(QSocks5SocketEngine);

#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("initialize %i", socketDescriptor);
#endif

    // this is only valid for the other side of a bind, nothing else is supported

    if (socketState != QAbstractSocket::ConnectedState) {
        //### must be connected state ???
        return false;
    }

    QSocks5BindData *bindData = socks5BindStore()->retrieve(socketDescriptor);
    if (bindData) {

        d->socketState = QAbstractSocket::ConnectedState;
        d->socketType = QAbstractSocket::TcpSocket;
        d->connectData = new QSocks5ConnectData;
        d->data = d->connectData;
        d->mode = QSocks5SocketEnginePrivate::ConnectMode;
        d->data->controlSocket = bindData->controlSocket;
        bindData->controlSocket = 0;
        d->data->controlSocket->setParent(this);
        d->socketProtocol = d->data->controlSocket->localAddress().protocol();
        d->data->authenticator = bindData->authenticator;
        bindData->authenticator = 0;
        d->localPort = bindData->localPort;
        d->localAddress = bindData->localAddress;
        d->peerPort = bindData->peerPort;
        d->peerAddress = bindData->peerAddress;
        delete bindData;

        QObject::connect(d->data->controlSocket, SIGNAL(connected()), this, SLOT(_q_controlSocketConnected()),
                         Qt::DirectConnection);
        QObject::connect(d->data->controlSocket, SIGNAL(readyRead()), this, SLOT(_q_controlSocketReadNotification()),
                         Qt::DirectConnection);
        QObject::connect(d->data->controlSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(_q_controlSocketBytesWritten()),
                         Qt::DirectConnection);
        QObject::connect(d->data->controlSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(_q_controlSocketError(QAbstractSocket::SocketError)),
                         Qt::DirectConnection);
        QObject::connect(d->data->controlSocket, SIGNAL(disconnected()), this, SLOT(_q_controlSocketDisconnected()),
                         Qt::DirectConnection);
        QObject::connect(d->data->controlSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                         this, SLOT(_q_controlSocketStateChanged(QAbstractSocket::SocketState)),
                         Qt::DirectConnection);

        d->socks5State = QSocks5SocketEnginePrivate::Connected;

        if (d->data->controlSocket->bytesAvailable() != 0)
            d->_q_controlSocketReadNotification();
        return true;
    }
    return false;
}

void QSocks5SocketEngine::setProxy(const QNetworkProxy &networkProxy)
{
    Q_D(QSocks5SocketEngine);
    d->proxyInfo = networkProxy;
}

qintptr QSocks5SocketEngine::socketDescriptor() const
{
    Q_D(const QSocks5SocketEngine);
    return d->socketDescriptor;
}

bool QSocks5SocketEngine::isValid() const
{
    Q_D(const QSocks5SocketEngine);
    return d->socketType != QAbstractSocket::UnknownSocketType
           && d->socks5State != QSocks5SocketEnginePrivate::SocksError
           && (d->socketError == QAbstractSocket::UnknownSocketError
               || d->socketError == QAbstractSocket::SocketTimeoutError
               || d->socketError == QAbstractSocket::UnfinishedSocketOperationError);
}

bool QSocks5SocketEngine::connectInternal()
{
    Q_D(QSocks5SocketEngine);

    if (!d->data) {
        if (socketType() == QAbstractSocket::TcpSocket) {
            d->initialize(QSocks5SocketEnginePrivate::ConnectMode);
#ifndef QT_NO_UDPSOCKET
        } else if (socketType() == QAbstractSocket::UdpSocket) {
            d->initialize(QSocks5SocketEnginePrivate::UdpAssociateMode);
            // all udp needs to be bound
            if (!bind(QHostAddress(QLatin1String("0.0.0.0")), 0))
                return false;

            setState(QAbstractSocket::ConnectedState);
            return true;
#endif
        } else {
            qFatal("QSocks5SocketEngine::connectToHost: in QTcpServer mode");
            return false;
        }
    }

    if (d->socks5State == QSocks5SocketEnginePrivate::Uninitialized
        && d->socketState != QAbstractSocket::ConnectingState) {
        setState(QAbstractSocket::ConnectingState);
        //limit buffer in internal socket, data is buffered in the external socket under application control
        d->data->controlSocket->setReadBufferSize(65536);
        d->data->controlSocket->connectToHost(d->proxyInfo.hostName(), d->proxyInfo.port());
        return false;
    }
    return false;
}

bool QSocks5SocketEngine::connectToHost(const QHostAddress &address, quint16 port)
{
    Q_D(QSocks5SocketEngine);
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("connectToHost %s : %i", address.toString().toUtf8().constData(), port);
#endif

    setPeerAddress(address);
    setPeerPort(port);
    d->peerName.clear();

    return connectInternal();
}

bool QSocks5SocketEngine::connectToHostByName(const QString &hostname, quint16 port)
{
    Q_D(QSocks5SocketEngine);

    setPeerAddress(QHostAddress());
    setPeerPort(port);
    d->peerName = hostname;

    return connectInternal();
}

void QSocks5SocketEnginePrivate::_q_controlSocketConnected()
{
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("_q_controlSocketConnected");
#endif
    QByteArray buf(3, 0);
    buf[0] = S5_VERSION_5;
    buf[1] = 0x01;
    buf[2] = data->authenticator->methodId();
    data->controlSocket->write(buf);
    socks5State = AuthenticationMethodsSent;
}

void QSocks5SocketEnginePrivate::_q_controlSocketReadNotification()
{
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("_q_controlSocketReadNotification socks5state %s bytes available %llu", s5StateToString(socks5State).toUtf8().constData(), data->controlSocket->bytesAvailable());
#endif

    if (data->controlSocket->bytesAvailable() == 0) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("########## bogus read why do we get these ... on windows only");
#endif
        return;
    }

    switch (socks5State) {
        case AuthenticationMethodsSent:
            parseAuthenticationMethodReply();
            break;
        case Authenticating:
            parseAuthenticatingReply();
            break;
        case RequestMethodSent:
            parseRequestMethodReply();
            break;
        case Connected: {
            QByteArray buf;
            if (!data->authenticator->unSeal(data->controlSocket, &buf)) {
                // qDebug() << "unseal error maybe need to wait for more data";
            }
            if (buf.size()) {
#ifdef FJ_INSP_NETWORKLOG
                DEBUG_SOCKET_QDEBUG2("%s", dump(buf).toUtf8().constData());
#endif
                connectData->readBuffer += buf;
                emitReadNotification();
            }
            break;
        }
        case BindSuccess:
            // only get here if command is bind
            if (mode == BindMode) {
                parseRequestMethodReply();
                break;
            }

            // fall through
        default:
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("QSocks5SocketEnginePrivate::_q_controlSocketReadNotification: "
                     "Unexpectedly received data while in state=%d and mode=%d",
                     socks5State, mode);
#endif
            break;
    };
}

void QSocks5SocketEnginePrivate::_q_controlSocketBytesWritten()
{
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("_q_controlSocketBytesWritten");
#endif

    if (socks5State != Connected
        || (mode == ConnectMode
        && data->controlSocket->bytesToWrite()))
        return;
    if (data->controlSocket->bytesToWrite() < MaxWriteBufferSize) {
        emitWriteNotification();
        writeNotificationActivated = false;
    }
}

void QSocks5SocketEnginePrivate::_q_controlSocketError(QAbstractSocket::SocketError error)
{
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("controlSocketError %d %s", error, (data->controlSocket->errorString()).toUtf8().constData());
#endif

    if (error == QAbstractSocket::SocketTimeoutError)
        return;                 // ignore this error -- comes from the waitFor* functions

    if (error == QAbstractSocket::RemoteHostClosedError
        && socks5State == Connected) {
        // clear the read buffer in connect mode so that bytes available returns 0
        // if there already is a read notification pending then this will be processed first
        if (!readNotificationPending)
            connectData->readBuffer.clear();
        emitReadNotification();
        data->controlSocket->close();
        // cause a disconnect in the outer socket
        emitWriteNotification();
    } else if (socks5State == Uninitialized
               || socks5State == AuthenticationMethodsSent
               || socks5State == Authenticating
               || socks5State == RequestMethodSent) {
        setErrorState(socks5State == Uninitialized ? ConnectError : ControlSocketError);
        data->controlSocket->close();
        emitConnectionNotification();
    } else {
        q_func()->setError(data->controlSocket->error(), data->controlSocket->errorString());
        emitReadNotification();
        emitWriteNotification();
    }
}

void QSocks5SocketEnginePrivate::_q_controlSocketDisconnected()
{
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("_q_controlSocketDisconnected");
#endif
}

void QSocks5SocketEnginePrivate::_q_controlSocketStateChanged(QAbstractSocket::SocketState state)
{
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("_q_controlSocketStateChanged %d", state);
#endif
}

#ifndef QT_NO_UDPSOCKET
void QSocks5SocketEnginePrivate::checkForDatagrams() const
{
    // udp should be unbuffered so we need to do some polling at certain points
    if (udpData->udpSocket->hasPendingDatagrams())
        const_cast<QSocks5SocketEnginePrivate *>(this)->_q_udpSocketReadNotification();
}

void QSocks5SocketEnginePrivate::_q_udpSocketReadNotification()
{
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG3("_q_udpSocketReadNotification()");
#endif

    // check some state stuff
    if (!udpData->udpSocket->hasPendingDatagrams()) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("false read ??");
#endif
        return;
    }

    while (udpData->udpSocket->hasPendingDatagrams()) {
        QByteArray sealedBuf(udpData->udpSocket->pendingDatagramSize(), 0);
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG3("new datagram");
#endif
        udpData->udpSocket->readDatagram(sealedBuf.data(), sealedBuf.size());
        QByteArray inBuf;
        if (!data->authenticator->unSeal(sealedBuf, &inBuf)) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("failed unsealing datagram discarding");
#endif
            return;
        }
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("%s", dump(inBuf).toUtf8().constData());
#endif
        int pos = 0;
        const char *buf = inBuf.constData();
        if (inBuf.size() < 4) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("bugus udp data, discarding");
#endif
            return;
        }
        QSocks5RevivedDatagram datagram;
        if (buf[pos++] != 0 || buf[pos++] != 0) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("invalid datagram discarding");
#endif
            return;
        }
        if (buf[pos++] != 0) { //### add fragmentation reading support
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("don't support fragmentation yet disgarding");
#endif
            return;
        }
        if (qt_socks5_get_host_address_and_port(inBuf, &datagram.address, &datagram.port, &pos) != 1) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("failed to get address from datagram disgarding");
#endif
            return;
        }
        datagram.data = QByteArray(&buf[pos], inBuf.size() - pos);
        udpData->pendingDatagrams.enqueue(datagram);
    }
    emitReadNotification();
}
#endif // QT_NO_UDPSOCKET

bool QSocks5SocketEngine::bind(const QHostAddress &addr, quint16 port)
{
    Q_D(QSocks5SocketEngine);

    // when bind wee will block until the bind is finished as the info from the proxy server is needed

    QHostAddress address;
    if (addr.protocol() == QAbstractSocket::AnyIPProtocol)
        address = QHostAddress::AnyIPv4; //SOCKS5 doesn't support dual stack, and there isn't any implementation of udp on ipv6 yet
    else
        address = addr;

    if (!d->data) {
        if (socketType() == QAbstractSocket::TcpSocket) {
            d->initialize(QSocks5SocketEnginePrivate::BindMode);
#ifndef QT_NO_UDPSOCKET
        } else if (socketType() == QAbstractSocket::UdpSocket) {
            d->initialize(QSocks5SocketEnginePrivate::UdpAssociateMode);
#endif
        } else {
            //### something invalid
            return false;
        }
    }

#ifndef QT_NO_UDPSOCKET
    if (d->mode == QSocks5SocketEnginePrivate::UdpAssociateMode) {
        if (!d->udpData->udpSocket->bind(address, port)) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("local udp bind failed");
#endif
            setError(d->udpData->udpSocket->error(), d->udpData->udpSocket->errorString());
            return false;
        }
        d->localAddress = d->udpData->udpSocket->localAddress();
        d->localPort = d->udpData->udpSocket->localPort();
    } else
#endif
    if (d->mode == QSocks5SocketEnginePrivate::BindMode) {
        d->localAddress = address;
        d->localPort = port;
    } else {
        //### something invalid
        return false;
    }

    int msecs = SOCKS5_BLOCKING_BIND_TIMEOUT;
    QElapsedTimer stopWatch;
    stopWatch.start();
    d->data->controlSocket->connectToHost(d->proxyInfo.hostName(), d->proxyInfo.port());
    if (!d->waitForConnected(msecs, 0) ||
        d->data->controlSocket->state() == QAbstractSocket::UnconnectedState) {
        // waitForConnected sets the error state and closes the socket
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("waitForConnected to proxy server %s", (d->data->controlSocket->errorString()).toUtf8().constData());
#endif
        return false;
    }
    if (d->socks5State == QSocks5SocketEnginePrivate::BindSuccess) {
        setState(QAbstractSocket::BoundState);
        return true;
#ifndef QT_NO_UDPSOCKET
    } else if (d->socks5State == QSocks5SocketEnginePrivate::UdpAssociateSuccess) {
        setState(QAbstractSocket::BoundState);
        d->udpData->associateAddress = d->localAddress;
        d->localAddress = QHostAddress();
        d->udpData->associatePort = d->localPort;
        d->localPort = 0;
        QUdpSocket dummy;
#ifndef QT_NO_BEARERMANAGEMENT
        dummy.setProperty("_q_networksession", property("_q_networksession"));
#endif
        dummy.setProxy(QNetworkProxy::NoProxy);
        if (!dummy.bind()
            || writeDatagram(0,0, d->data->controlSocket->localAddress(), dummy.localPort()) != 0
            || !dummy.waitForReadyRead(qt_timeout_value(msecs, stopWatch.elapsed()))
            || dummy.readDatagram(0,0, &d->localAddress, &d->localPort) != 0) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("udp actual address and port lookup failed");
#endif
            setState(QAbstractSocket::UnconnectedState);
            setError(dummy.error(), dummy.errorString());
            d->data->controlSocket->close();
            //### reset and error
            return false;
        }
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("udp actual address and port %s : %i", d->localAddress.toString().toUtf8().constData(), d->localPort);
#endif
        return true;
#endif // QT_NO_UDPSOCKET
    }

    // binding timed out
    setError(QAbstractSocket::SocketTimeoutError,
             QLatin1String(QT_TRANSLATE_NOOP("QSocks5SocketEngine", "Network operation timed out")));

///###    delete d->udpSocket;
///###    d->udpSocket = 0;
    return false;
}


bool QSocks5SocketEngine::listen()
{
    Q_D(QSocks5SocketEngine);

#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("listen()");
#endif

    // check that we are in bound and then go to listening.
    if (d->socketState == QAbstractSocket::BoundState) {
        d->socketState = QAbstractSocket::ListeningState;

        // check if we already have a connection
        if (d->socks5State == QSocks5SocketEnginePrivate::BindSuccess)
            d->emitReadNotification();

        return true;
    }
    return false;
}

int QSocks5SocketEngine::accept()
{
    Q_D(QSocks5SocketEngine);
    // check we are listing ---

#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("accept()");
#endif

    qintptr sd = -1;
    switch (d->socks5State) {
    case QSocks5SocketEnginePrivate::BindSuccess:
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("BindSuccess adding %i to the bind store", d->socketDescriptor);
#endif
        d->data->controlSocket->disconnect();
        d->data->controlSocket->setParent(0);
        d->bindData->localAddress = d->localAddress;
        d->bindData->localPort = d->localPort;
        sd = d->socketDescriptor;
        socks5BindStore()->add(sd, d->bindData);
        d->data = 0;
        d->bindData = 0;
        d->socketDescriptor = 0;
        //### do something about this socket layer ... set it closed and an error about why ...
        // reset state and local port/address
        d->socks5State = QSocks5SocketEnginePrivate::Uninitialized; // ..??
        d->socketState = QAbstractSocket::UnconnectedState;
        break;
    case QSocks5SocketEnginePrivate::ControlSocketError:
        setError(QAbstractSocket::ProxyProtocolError, QLatin1String("Control socket error"));
        break;
    default:
        setError(QAbstractSocket::ProxyProtocolError, QLatin1String("SOCKS5 proxy error"));
        break;
    }
    return sd;
}

void QSocks5SocketEngine::close()
{
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("close()");
#endif
    Q_D(QSocks5SocketEngine);
    if (d->data && d->data->controlSocket) {
        if (d->data->controlSocket->state() == QAbstractSocket::ConnectedState) {
            int msecs = 100;
            QElapsedTimer stopWatch;
            stopWatch.start();
            while (!d->data->controlSocket->bytesToWrite()) {
               if (!d->data->controlSocket->waitForBytesWritten(qt_timeout_value(msecs, stopWatch.elapsed())))
                   break;
            }
        }
        d->data->controlSocket->close();
    }
#ifndef QT_NO_UDPSOCKET
    if (d->udpData && d->udpData->udpSocket)
        d->udpData->udpSocket->close();
#endif
}

qint64 QSocks5SocketEngine::bytesAvailable() const
{
    Q_D(const QSocks5SocketEngine);
    if (d->mode == QSocks5SocketEnginePrivate::ConnectMode)
        return d->connectData->readBuffer.size();
#ifndef QT_NO_UDPSOCKET
    else if (d->mode == QSocks5SocketEnginePrivate::UdpAssociateMode
             && !d->udpData->pendingDatagrams.isEmpty())
        return d->udpData->pendingDatagrams.first().data.size();
#endif
    return 0;
}

qint64 QSocks5SocketEngine::read(char *data, qint64 maxlen)
{
    Q_D(QSocks5SocketEngine);
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("read( , maxlen = %d)", maxlen);
#endif
    if (d->mode == QSocks5SocketEnginePrivate::ConnectMode) {
        if (d->connectData->readBuffer.size() == 0) {
            if (d->data->controlSocket->state() == QAbstractSocket::UnconnectedState) {
                //imitate remote closed
                close();
                setError(QAbstractSocket::RemoteHostClosedError,
                         QLatin1String("Remote host closed connection###"));
                setState(QAbstractSocket::UnconnectedState);
                return -1;
            } else {
                return 0;       // nothing to be read
            }
        }
        qint64 copy = qMin<qint64>(d->connectData->readBuffer.size(), maxlen);
        memcpy(data, d->connectData->readBuffer.constData(), copy);
        d->connectData->readBuffer.remove(0, copy);
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("read %s", dump(QByteArray(data, copy)).toUtf8().constData());
#endif
        return copy;
#ifndef QT_NO_UDPSOCKET
    } else if (d->mode == QSocks5SocketEnginePrivate::UdpAssociateMode) {
        return readDatagram(data, maxlen);
#endif
    }
    return 0;
}

qint64 QSocks5SocketEngine::write(const char *data, qint64 len)
{
    Q_D(QSocks5SocketEngine);
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("write, %s", dump(QByteArray(data, len)).toUtf8().constData());
#endif

    if (d->mode == QSocks5SocketEnginePrivate::ConnectMode) {
        // clamp down the amount of bytes to transfer at once
        len = qMin<qint64>(len, MaxWriteBufferSize) - d->data->controlSocket->bytesToWrite();
        if (len <= 0)
            return 0;

        QByteArray buf = QByteArray::fromRawData(data, len);
        QByteArray sealedBuf;
        if (!d->data->authenticator->seal(buf, &sealedBuf)) {
            // ### Handle this error.
        }

        qint64 written = d->data->controlSocket->write(sealedBuf);
        if (written <= 0) {
#ifdef FJ_INSP_NETWORKLOG
            DEBUG_SOCKET_QDEBUG2("native write returned %lld", written);
#endif
            return written;
        }
        d->data->controlSocket->waitForBytesWritten(0);
        //NB: returning len rather than written for the OK case, because the "sealing" may increase the length
        return len;
#ifndef QT_NO_UDPSOCKET
    } else if (d->mode == QSocks5SocketEnginePrivate::UdpAssociateMode) {
        // send to connected address
        return writeDatagram(data, len, d->peerAddress, d->peerPort);
#endif
    }
    //### set an error ???
    return -1;
}

#ifndef QT_NO_UDPSOCKET
#ifndef QT_NO_NETWORKINTERFACE
bool QSocks5SocketEngine::joinMulticastGroup(const QHostAddress &,
                                             const QNetworkInterface &)
{
    setError(QAbstractSocket::UnsupportedSocketOperationError,
             QLatin1String("Operation on socket is not supported"));
    return false;
}

bool QSocks5SocketEngine::leaveMulticastGroup(const QHostAddress &,
                                              const QNetworkInterface &)
{
    setError(QAbstractSocket::UnsupportedSocketOperationError,
             QLatin1String("Operation on socket is not supported"));
    return false;
}


QNetworkInterface QSocks5SocketEngine::multicastInterface() const
{
    return QNetworkInterface();
}

bool QSocks5SocketEngine::setMulticastInterface(const QNetworkInterface &)
{
    setError(QAbstractSocket::UnsupportedSocketOperationError,
             QLatin1String("Operation on socket is not supported"));
    return false;
}
#endif // QT_NO_NETWORKINTERFACE

qint64 QSocks5SocketEngine::readDatagram(char *data, qint64 maxlen, QHostAddress *addr,
                                        quint16 *port)
{
    Q_D(QSocks5SocketEngine);

    d->checkForDatagrams();

    if (d->udpData->pendingDatagrams.isEmpty())
        return 0;

    QSocks5RevivedDatagram datagram = d->udpData->pendingDatagrams.dequeue();
    int copyLen = qMin<int>(maxlen, datagram.data.size());
    memcpy(data, datagram.data.constData(), copyLen);
    if (addr)
        *addr = datagram.address;
    if (port)
        *port = datagram.port;
    return copyLen;
}

qint64 QSocks5SocketEngine::writeDatagram(const char *data, qint64 len, const QHostAddress &address,
                                         quint16 port)
{
    Q_D(QSocks5SocketEngine);

    // it is possible to send with out first binding with udp, but socks5 requires a bind.
    if (!d->data) {
        d->initialize(QSocks5SocketEnginePrivate::UdpAssociateMode);
        // all udp needs to be bound
        if (!bind(QHostAddress(QLatin1String("0.0.0.0")), 0)) {
            //### set error
            return -1;
        }
    }

    QByteArray outBuf;
    outBuf.reserve(270 + len);
    outBuf[0] = 0x00;
    outBuf[1] = 0x00;
    outBuf[2] = 0x00;
    if (!qt_socks5_set_host_address_and_port(address, port, &outBuf)) {
    }
    outBuf += QByteArray(data, len);
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("sending %s", dump(outBuf).toUtf8().constData());
#endif
    QByteArray sealedBuf;
    if (!d->data->authenticator->seal(outBuf, &sealedBuf)) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("sealing data failed");
#endif
        setError(QAbstractSocket::SocketAccessError, d->data->authenticator->errorString());
        return -1;
    }
    if (d->udpData->udpSocket->writeDatagram(sealedBuf, d->udpData->associateAddress, d->udpData->associatePort) != sealedBuf.size()) {
        //### try frgamenting
        if (d->udpData->udpSocket->error() == QAbstractSocket::DatagramTooLargeError)
            setError(d->udpData->udpSocket->error(), d->udpData->udpSocket->errorString());
        //### else maybe more serious error
        return -1;
    }

    return len;
}

bool QSocks5SocketEngine::hasPendingDatagrams() const
{
    Q_D(const QSocks5SocketEngine);
    Q_INIT_CHECK(false);

    d->checkForDatagrams();

    return !d->udpData->pendingDatagrams.isEmpty();
}

qint64 QSocks5SocketEngine::pendingDatagramSize() const
{
    Q_D(const QSocks5SocketEngine);

    d->checkForDatagrams();

    if (!d->udpData->pendingDatagrams.isEmpty())
        return d->udpData->pendingDatagrams.head().data.size();
    return 0;
}
#endif // QT_NO_UDPSOCKET

qint64 QSocks5SocketEngine::bytesToWrite() const
{
    Q_D(const QSocks5SocketEngine);
    if (d->data && d->data->controlSocket) {
        return d->data->controlSocket->bytesToWrite();
    } else {
        return 0;
    }
}

int QSocks5SocketEngine::option(SocketOption option) const
{
    Q_D(const QSocks5SocketEngine);
    if (d->data && d->data->controlSocket) {
        // convert the enum and call the real socket
        if (option == QAbstractSocketEngine::LowDelayOption)
            return d->data->controlSocket->socketOption(QAbstractSocket::LowDelayOption).toInt();
        if (option == QAbstractSocketEngine::KeepAliveOption)
            return d->data->controlSocket->socketOption(QAbstractSocket::KeepAliveOption).toInt();
    }
    return -1;
}

bool QSocks5SocketEngine::setOption(SocketOption option, int value)
{
    Q_D(QSocks5SocketEngine);
    if (d->data && d->data->controlSocket) {
        // convert the enum and call the real socket
        if (option == QAbstractSocketEngine::LowDelayOption)
            d->data->controlSocket->setSocketOption(QAbstractSocket::LowDelayOption, value);
        if (option == QAbstractSocketEngine::KeepAliveOption)
            d->data->controlSocket->setSocketOption(QAbstractSocket::KeepAliveOption, value);
        return true;
    }
    return false;
}

bool QSocks5SocketEnginePrivate::waitForConnected(int msecs, bool *timedOut)
{
    if (data->controlSocket->state() == QAbstractSocket::UnconnectedState)
        return false;

    const Socks5State wantedState =
        mode == ConnectMode ? Connected :
        mode == BindMode ? BindSuccess :
        UdpAssociateSuccess;

    QElapsedTimer stopWatch;
    stopWatch.start();

    while (socks5State != wantedState) {
        if (!data->controlSocket->waitForReadyRead(qt_timeout_value(msecs, stopWatch.elapsed()))) {
            if (data->controlSocket->state() == QAbstractSocket::UnconnectedState)
                return true;

            setErrorState(QSocks5SocketEnginePrivate::ControlSocketError);
            if (timedOut && data->controlSocket->error() == QAbstractSocket::SocketTimeoutError)
                *timedOut = true;
            return false;
        }
    }

    return true;
}

bool QSocks5SocketEngine::waitForRead(int msecs, bool *timedOut)
{
    Q_D(QSocks5SocketEngine);
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("waitForRead %d", msecs);
#endif

    d->readNotificationActivated = false;

    QElapsedTimer stopWatch;
    stopWatch.start();

    // are we connected yet?
    if (!d->waitForConnected(msecs, timedOut))
        return false;
    if (d->data->controlSocket->state() == QAbstractSocket::UnconnectedState)
        return true;

    // we're connected
    if (d->mode == QSocks5SocketEnginePrivate::ConnectMode ||
        d->mode == QSocks5SocketEnginePrivate::BindMode) {
        while (!d->readNotificationActivated) {
            if (!d->data->controlSocket->waitForReadyRead(qt_timeout_value(msecs, stopWatch.elapsed()))) {
                if (d->data->controlSocket->state() == QAbstractSocket::UnconnectedState)
                    return true;

                setError(d->data->controlSocket->error(), d->data->controlSocket->errorString());
                if (timedOut && d->data->controlSocket->error() == QAbstractSocket::SocketTimeoutError)
                    *timedOut = true;
                return false;
            }
        }
#ifndef QT_NO_UDPSOCKET
    } else {
        while (!d->readNotificationActivated) {
            if (!d->udpData->udpSocket->waitForReadyRead(qt_timeout_value(msecs, stopWatch.elapsed()))) {
                setError(d->udpData->udpSocket->error(), d->udpData->udpSocket->errorString());
                if (timedOut && d->udpData->udpSocket->error() == QAbstractSocket::SocketTimeoutError)
                    *timedOut = true;
                return false;
            }
        }
#endif // QT_NO_UDPSOCKET
    }


    bool ret = d->readNotificationActivated;
    d->readNotificationActivated = false;

#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("waitForRead returned %d", ret);
#endif
    return ret;
}


bool QSocks5SocketEngine::waitForWrite(int msecs, bool *timedOut)
{
    Q_D(QSocks5SocketEngine);
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("waitForWrite %d", msecs);
#endif

    QElapsedTimer stopWatch;
    stopWatch.start();

    // are we connected yet?
    if (!d->waitForConnected(msecs, timedOut))
        return false;
    if (d->data->controlSocket->state() == QAbstractSocket::UnconnectedState)
        return true;

    // we're connected

    // flush any bytes we may still have buffered in the time that we have left
    if (d->data->controlSocket->bytesToWrite())
        d->data->controlSocket->waitForBytesWritten(qt_timeout_value(msecs, stopWatch.elapsed()));
    while ((msecs == -1 || stopWatch.elapsed() < msecs)
           && d->data->controlSocket->state() == QAbstractSocket::ConnectedState
           && d->data->controlSocket->bytesToWrite() >= MaxWriteBufferSize)
        d->data->controlSocket->waitForBytesWritten(qt_timeout_value(msecs, stopWatch.elapsed()));
    return d->data->controlSocket->bytesToWrite() < MaxWriteBufferSize;
}

bool QSocks5SocketEngine::waitForReadOrWrite(bool *readyToRead, bool *readyToWrite,
                                            bool checkRead, bool checkWrite,
                                            int msecs, bool *timedOut)
{
    Q_UNUSED(checkRead);
    if (!checkWrite) {
        bool canRead = waitForRead(msecs, timedOut);
        if (readyToRead)
            *readyToRead = canRead;
        return canRead;
    }

    bool canWrite = waitForWrite(msecs, timedOut);
    if (readyToWrite)
        *readyToWrite = canWrite;
    return canWrite;
}

bool QSocks5SocketEngine::isReadNotificationEnabled() const
{
    Q_D(const QSocks5SocketEngine);
    return d->readNotificationEnabled;
}

void QSocks5SocketEngine::setReadNotificationEnabled(bool enable)
{
    Q_D(QSocks5SocketEngine);

#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("setReadNotificationEnabled( %d )", enable);
#endif

    bool emitSignal = false;
    if (!d->readNotificationEnabled
        && enable) {
        if (d->mode == QSocks5SocketEnginePrivate::ConnectMode)
            emitSignal = !d->connectData->readBuffer.isEmpty();
#ifndef QT_NO_UDPSOCKET
        else if (d->mode == QSocks5SocketEnginePrivate::UdpAssociateMode)
            emitSignal = !d->udpData->pendingDatagrams.isEmpty();
#endif
        else if (d->mode == QSocks5SocketEnginePrivate::BindMode
            && d->socketState == QAbstractSocket::ListeningState
            && d->socks5State == QSocks5SocketEnginePrivate::BindSuccess)
            emitSignal = true;
    }

    d->readNotificationEnabled = enable;

    if (emitSignal)
        d->emitReadNotification();
}

bool QSocks5SocketEngine::isWriteNotificationEnabled() const
{
    Q_D(const QSocks5SocketEngine);
    return d->writeNotificationEnabled;
}

void QSocks5SocketEngine::setWriteNotificationEnabled(bool enable)
{
    Q_D(QSocks5SocketEngine);
    d->writeNotificationEnabled = enable;
    if (enable && d->socketState == QAbstractSocket::ConnectedState) {
        if (d->mode == QSocks5SocketEnginePrivate::ConnectMode && d->data->controlSocket->bytesToWrite())
            return; // will be emitted as a result of bytes written
       d->emitWriteNotification();
       d->writeNotificationActivated = false;
    }
}

bool QSocks5SocketEngine::isExceptionNotificationEnabled() const
{
    Q_D(const QSocks5SocketEngine);
    return d->exceptNotificationEnabled;
}

void QSocks5SocketEngine::setExceptionNotificationEnabled(bool enable)
{
    Q_D(QSocks5SocketEngine);
    d->exceptNotificationEnabled = enable;
}

QAbstractSocketEngine *
QSocks5SocketEngineHandler::createSocketEngine(QAbstractSocket::SocketType socketType,
                                               const QNetworkProxy &proxy, QObject *parent)
{
    Q_UNUSED(socketType);

    // proxy type must have been resolved by now
    if (proxy.type() != QNetworkProxy::Socks5Proxy) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("not proxying");
#endif
        return 0;
    }
    QScopedPointer<QSocks5SocketEngine> engine(new QSocks5SocketEngine(parent));
    engine->setProxy(proxy);
    return engine.take();
}

QAbstractSocketEngine *QSocks5SocketEngineHandler::createSocketEngine(qintptr socketDescriptor, QObject *parent)
{
#ifdef FJ_INSP_NETWORKLOG
    DEBUG_SOCKET_QDEBUG2("createSocketEngine %i", socketDescriptor);
#endif
    if (socks5BindStore()->contains(socketDescriptor)) {
#ifdef FJ_INSP_NETWORKLOG
        DEBUG_SOCKET_QDEBUG2("bind store contains %i", socketDescriptor);
#endif
        return new QSocks5SocketEngine(parent);
    }
    return 0;
}

#endif // QT_NO_SOCKS5

QT_END_NAMESPACE
