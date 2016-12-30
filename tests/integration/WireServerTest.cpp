#include <cucumber-cpp/internal/connectors/wire/WireServer.hpp>

#include <gmock/gmock.h>

#include <boost/filesystem/operations.hpp>
#include <boost/thread.hpp>
#include <boost/timer.hpp>

#include <stdlib.h>
#include <sstream>

using namespace cucumber::internal;
using namespace boost::posix_time;
using namespace boost::asio::ip;
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
using namespace boost::asio::local;
#endif
using namespace std;
using namespace testing;
using boost::bind;
using boost::thread;
namespace fs = boost::filesystem;

static const time_duration THREAD_TEST_TIMEOUT = milliseconds(4000);

MATCHER(IsConnected, string(negation ? "is not" : "is") +
        " connected") { return arg.good(); }

MATCHER(HasTerminated, "") {
    return !arg.joinable();
}

MATCHER(EventuallyTerminates, "") {
    thread *t = const_cast<thread *>(arg);
    return t->timed_join(THREAD_TEST_TIMEOUT);
}

MATCHER_P(EventuallyReceives, value, "") {
    tcp::iostream *stream = const_cast<tcp::iostream *>(&arg);
    std::string output;
// FIXME It should not block
    (*stream) >> output;
//    boost::timer timer;
//    double timeout = THREAD_TEST_TIMEOUT.total_milliseconds() / 1000.0;
//    while (timer.elapsed() < timeout) {
//        if (stream->rdbuf()->available() > 0) { // it is zero even if it doesn't block!
//            (*stream) >> output;
//            break;
//        }
//        boost::this_thread::yield();
//    }
    return (output == value);
}

class MockProtocolHandler : public ProtocolHandler {
public:
    MOCK_CONST_METHOD1(handle, string(const string &request));
};

class SocketServerTest : public Test {

protected:
    StrictMock<MockProtocolHandler> protocolHandler;
    thread *serverThread;

    virtual void SetUp() {
        SocketServer* server = createListeningServer();
        ASSERT_TRUE(server);
        serverThread = new thread(&SocketServer::acceptOnce, server);
    }

    virtual void TearDown() {
        if (serverThread) {
            serverThread->timed_join(THREAD_TEST_TIMEOUT);
            delete serverThread;
            serverThread = NULL;
        }
        destroyListeningServer();
    }

    virtual SocketServer* createListeningServer() = 0;
    virtual void destroyListeningServer() = 0;
};

class TCPSocketServerTest : public SocketServerTest {
protected:
    TCPSocketServer *server;
    TCPSocketServerTest() :
        server(NULL) {
    }

    virtual TCPSocketServer* createListeningServer() {
        server = new TCPSocketServer(&protocolHandler);
        server->listen(0);
        return server;
    }

    virtual void destroyListeningServer() {
        delete server;
        server = NULL;
    }
};

TEST_F(TCPSocketServerTest, exitsOnFirstConnectionClosed) {
    // given
    tcp::iostream client(server->listenEndpoint());
    ASSERT_THAT(client, IsConnected());

    // when
    client.close();

    // then
    EXPECT_THAT(serverThread, EventuallyTerminates());
}

TEST_F(TCPSocketServerTest, moreThanOneClientCanConnect) {
    // given
    tcp::iostream client1(server->listenEndpoint());
    ASSERT_THAT(client1, IsConnected());

    // when
    tcp::iostream client2(server->listenEndpoint());

    //then
    ASSERT_THAT(client2, IsConnected());
}

TEST_F(TCPSocketServerTest, receiveAndSendsSingleLineMassages) {
    {
        InSequence s;
        EXPECT_CALL(protocolHandler, handle("12")).WillRepeatedly(Return("A"));
        EXPECT_CALL(protocolHandler, handle("3")).WillRepeatedly(Return("B"));
        EXPECT_CALL(protocolHandler, handle("4")).WillRepeatedly(Return("C"));
    }

    // given
    tcp::iostream client(server->listenEndpoint());
    ASSERT_THAT(client, IsConnected());

    // when
    client << "1" << flush << "2" << endl << flush;
    client << "3" << endl << "4" << endl << flush;

    // then
    EXPECT_THAT(client, EventuallyReceives("A"));
    EXPECT_THAT(client, EventuallyReceives("B"));
    EXPECT_THAT(client, EventuallyReceives("C"));
}

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
class UnixSocketServerTest : public SocketServerTest {
protected:
    UnixSocketServer *server;
    UnixSocketServerTest() :
        server(NULL) {
    }

    virtual UnixSocketServer* createListeningServer() {
        fs::path socket = fs::temp_directory_path() / fs::unique_path();
        server = new UnixSocketServer(&protocolHandler);
        server->listen(socket.string());
        return server;
    }

    virtual void destroyListeningServer() {
        delete server;
        server = NULL;
    }
};

TEST_F(UnixSocketServerTest, clientCanConnect) {
    // given
    stream_protocol::iostream client;

    // when
    client.connect(server->listenEndpoint());

    // then
    EXPECT_THAT(client, IsConnected());
}
#endif
