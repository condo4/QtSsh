#include "tester.h"
#include <sshprocess.h>
#include <sshtunnelin.h>
#include <sshtunnelout.h>
#include <QFile>
#include <QDateTime>
#include <QTest>
#include <QDataStream>

Q_LOGGING_CATEGORY(testssh, "test.ssh", QtInfoMsg)

#define TestTimeOut (30*1000) // 15s
//#define TEST_ENABLE 0xFFFF
#define TEST_ENABLE 0x000F
#define DUMP_IF_ERROR 0
#define BENCHMARK_REPEAT 100

static inline QString prettySize(long size)
{
    if (static_cast<double>(size)/1024/1024 >= 1.0 )
        return QString("%1 MB").arg(static_cast<double>(size)/1024/1024);
    else if (static_cast<double>(size)/1024 >= 1.0)
        return QString("%1 KB").arg(static_cast<double>(size)/1024);
    else
        return QString("%1 Bytes").arg(size);
}

Tester::Tester(QString hostname, QString login, QString password, QObject *parent)
    : QObject(parent)
    , m_hostname(hostname)
    , m_login(login)
    , m_password(password)
    , m_ssh("SshConnection")
    , m_currentDataToUse(m_bigdata)
{
    connect(&m_timeout, &QTimer::timeout, this, &Tester::checkTest);
    connect(this, &Tester::serverBufferChange, this, &Tester::checkTest);
    connect(this, &Tester::clientBufferChange, this, &Tester::checkTest);
    connect(&m_srv, &QTcpServer::newConnection, this, &Tester::newServerConnection);
    connect(&m_cli, &QTcpSocket::readyRead, this, &Tester::clientDataReady);
    connect(&m_cli, &QTcpSocket::connected, this, &Tester::clientConnected);
    connect(this, &Tester::endSubTest, &m_waitTestEnd, &QEventLoop::exit, Qt::QueuedConnection);
    m_timeout.setSingleShot(true);
}

void Tester::init()
{
    qCDebug(testssh) << "Initializing test" << m_testName;
    m_readcli.clear();
    m_readsrv.clear();
    m_readcli.reserve(m_currentDataToUse.size());
    m_readsrv.reserve(m_currentDataToUse.size());
    m_timeout.start(TestTimeOut);
}

void Tester::cleanup()
{
    QTest::qWait(100); // Let some time for asynchronous libssh cleanup
#if DUMP_IF_ERROR
    QString testName = QTest::currentTestFunction();
    QString testFilename = testName.left(testName.indexOf("_"));
    if ( QTest::currentTestFailed() )
    {
        if( m_mode & TEST_CLI2SRV)
        {
            dumpData(testFilename + "_readsrv.bin", m_readsrv);
        }
        if ( m_mode & TEST_SRV2CLI )
        {
            dumpData(testFilename + "_readcli.bin", m_readcli);
        }
        dumpData(testFilename + "_expected.bin", m_currentDataToUse);
    }
#endif
}

void Tester::initTestCase()
{
    qCInfo(testssh) << "Generating test dataset";
#define RANDOM_DATA 1
#if RANDOM_DATA
    qsrand(static_cast<unsigned int>(QDateTime::currentDateTime().toMSecsSinceEpoch()));
    QDataStream streamer(&m_bigdata, QIODevice::WriteOnly);
    foreach (long size, m_sizeForDataSet )
    {
        qCInfo(testssh) << "Generating for" << prettySize(size);
        QByteArray tempArray;
        tempArray.reserve(static_cast<int>(size));
        QDataStream streamer(&tempArray, QIODevice::WriteOnly);
        for (int ii = 0; ii < size; ii += sizeof(int) )
        {
            streamer << rand();
        }
        m_dataSetBySize[size] = tempArray;
    }
#else
    foreach (long size, m_sizeForDataSet )
    {
        qCInfo(testssh) << "Generating for" << prettySize(size);
        QByteArray tempArray;
        tempArray.reserve(static_cast<int>(size));
        QDataStream streamer(&tempArray, QIODevice::WriteOnly);
        for(int ii=0; ii < size; ii+=4)
        {
            streamer << ii;
        }
        m_dataSetBySize[size] = tempArray;
    }

#endif
    if (!m_srv.listen())
    {
        qCCritical(testssh) << "Can't listen server port";
        emit endtest(false);
        return;
    }

    m_ssh.setPassphrase(m_password);
    QEventLoop waitssh;
    QObject::connect(&m_ssh, &SshClient::sshReady, &waitssh, &QEventLoop::quit);
    QObject::connect(&m_ssh, &SshClient::sshError, &waitssh, &QEventLoop::quit);
    m_ssh.connectToHost(m_login, m_hostname, 22);
    waitssh.exec();
    if(m_ssh.sshState() != SshClient::SshState::Ready)
    {
        qCCritical(testssh) << "Can't connect to connexion server";
        emit endtest(false);
        return;
    }
    qCInfo(testssh) << "SSH connected";
}

void Tester::test1_RemoteProcess()
{
#if (TEST_ENABLE & 0x1)
    QEventLoop wait;
    SshProcess *proc = m_ssh.getChannel<SshProcess>("command");
    QObject::connect(proc, &SshProcess::finished, &wait, &QEventLoop::quit);
    QObject::connect(proc, &SshProcess::failed, &wait, &QEventLoop::quit);
    proc->runCommand("cat /proc/cpuinfo");
    wait.exec();
    QString devs = proc->result();
    QVERIFY2(devs.size() > 50, "Remote process has not enougth result to be right");
#else
    QSKIP("Remote process disable by TEST_ENABLE variable");
#endif
}

void Tester::test2_directTunnelComClientToServer_data()
{
    populateTestData();
    m_mode = TEST_CLI2SRV;
    m_testName = "SshTunnelOut    CLI >--O--> SRV";
}

static int channelid=1;
void Tester::test2_directTunnelComClientToServer()
{
#if ((TEST_ENABLE & 0x2) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    QFETCH(QByteArray, dataForTest);
    m_currentDataToUse=dataForTest;
    SshTunnelOut *out1 = m_ssh.getChannel<SshTunnelOut>(QString("T2_OUT_%1").arg(channelid++));
    out1->listen(m_srv.serverPort());
    m_cli.connectToHost("127.0.0.1", out1->localPort());
    int result = m_waitTestEnd.exec();
    // Cleanup test
    m_cli.disconnectFromHost();
    if ( m_cli.state() != QAbstractSocket::UnconnectedState )
        m_cli.waitForDisconnected(1000);

    // Test results
    QVERIFY2(result == 0, "Test failed in timeout");
    out1->close();
    compareResults();
#endif
}

void Tester::test3_directTunnelComServerToClient_data()
{
   populateTestData();
   m_mode = TEST_SRV2CLI;
   m_testName = "SshTunnelOut    CLI <--O--< SRV";
}

void Tester::test3_directTunnelComServerToClient()
{
#if ((TEST_ENABLE & 0x4) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    QFETCH(QByteArray, dataForTest);
    m_currentDataToUse=dataForTest;
    SshTunnelOut *out1 = m_ssh.getChannel<SshTunnelOut>(QString("T3_OUT_%1").arg(channelid++));
    out1->listen(m_srv.serverPort());
    m_cli.connectToHost("127.0.0.1", out1->localPort());
    int result = m_waitTestEnd.exec();
    // Cleanup test
    m_cli.disconnectFromHost();
    if ( m_cli.state() != QAbstractSocket::UnconnectedState )
        m_cli.waitForDisconnected(1000);

    // Test results
    QVERIFY2(result == 0, "Test failed in timeout");
    out1->close();
    compareResults();
#endif
}

void Tester::test4_directTunnelComBothWays_data()
{
    populateTestData();
    m_mode = TEST_BIDIR;
    m_testName = "SshTunnelOut    CLI <--O--> SRV";
}
void Tester::test4_directTunnelComBothWays()
{
#if ((TEST_ENABLE & 0x8) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    QFETCH(QByteArray, dataForTest);
    m_currentDataToUse=dataForTest;
    SshTunnelOut *out1 = m_ssh.getChannel<SshTunnelOut>("T4_OUT");
    out1->listen(m_srv.serverPort());
    m_cli.connectToHost("127.0.0.1", out1->localPort());
    int result = m_waitTestEnd.exec();
    m_cli.disconnectFromHost();
    if ( m_cli.state() != QAbstractSocket::UnconnectedState )
        m_cli.waitForDisconnected(1000);
    QVERIFY2(result == 0, "Test failed in timeout");
    compareResults();
#endif
}

void Tester::test5_doubleDirectTunnelComBothWays_data()
{
   populateTestData();
   m_mode = TEST_BIDIR;
   m_testName = "SshTunnelOut    CLI <-O-O-> SRV";
}
void Tester::test5_doubleDirectTunnelComBothWays()
{
#if ((TEST_ENABLE & 0x10) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    QFETCH(QByteArray, dataForTest);
    m_currentDataToUse=dataForTest;
    SshTunnelOut *out1 = m_ssh.getChannel<SshTunnelOut>("T5_OUT1");
    out1->listen(m_srv.serverPort());
    SshTunnelOut *out2 = m_ssh.getChannel<SshTunnelOut>("T5_OUT2");
    out2->listen(out1->localPort());
    qCDebug(testssh) << "SRV: " << m_srv.serverPort();
    qCDebug(testssh) << "OUT1: " << out1->localPort();
    qCDebug(testssh) << "OUT2: " << out2->localPort();

    m_cli.connectToHost("127.0.0.1", out2->localPort());
    int result = m_waitTestEnd.exec();
    m_cli.disconnectFromHost();
    if ( m_cli.state() != QAbstractSocket::UnconnectedState )
        m_cli.waitForDisconnected(1000);
    QVERIFY2(result == 0, "Test failed in timeout");
    compareResults();
#endif
}

void Tester::test6_reverseTunnelComClientToServer_data()
{
   populateTestData();
   m_mode = TEST_CLI2SRV;
   m_testName = "SshTunnelIn     CLI >--I--> SRV";
}
void Tester::test6_reverseTunnelComClientToServer()
{
#if ((TEST_ENABLE & 0x20) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    // Remote tunnel tcp server is local and client is remote
    QEventLoop wait;
    QFETCH(QByteArray, dataForTest);
    m_currentDataToUse=dataForTest;
    SshTunnelIn *in1 = m_ssh.getChannel<SshTunnelIn>("T6_IN");
    QObject::connect(in1, &SshChannel::stateChanged, [&wait, in1](){ if(in1->channelState() == SshChannel::Read) wait.quit();});
    in1->listen("127.0.0.1", m_srv.serverPort(), 0);
    if(in1->channelState() != SshChannel::Read) wait.exec();
    m_cli.connectToHost("127.0.0.1", in1->remotePort());
    int result = m_waitTestEnd.exec();
    m_cli.disconnectFromHost();
    if ( m_cli.state() != QAbstractSocket::UnconnectedState )
        m_cli.waitForDisconnected(1000);
    QVERIFY2(result == 0, "Test failed in timeout");
    compareResults();
#endif
}

void Tester::test7_reverseTunnelComServerToClient_data()
{
   populateTestData();
   m_mode = TEST_SRV2CLI;
   m_testName = "SshTunnelIn     CLI <--I--< SRV";
}
void Tester::test7_reverseTunnelComServerToClient()
{
#if ((TEST_ENABLE & 0x40) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    QFETCH(QByteArray, dataForTest);
    m_currentDataToUse=dataForTest;
    SshTunnelIn *in1 = m_ssh.getChannel<SshTunnelIn>("T7_IN");
    in1->listen("127.0.0.1", m_srv.serverPort(), 0);
    while(in1->channelState() != SshChannel::Read)
    {
        qApp->processEvents();
    }
    m_cli.connectToHost("127.0.0.1", in1->remotePort());
    int result = m_waitTestEnd.exec();
    m_cli.disconnectFromHost();
    if ( m_cli.state() != QAbstractSocket::UnconnectedState )
        m_cli.waitForDisconnected(1000);
    QVERIFY2(result == 0, "Test failed in timeout");
    compareResults();
#endif
}

void Tester::test8_reverseTunnelBothWays_data()
{
   populateTestData();
   m_mode = TEST_BIDIR;
   m_testName = "SshTunnelIn     CLI <--I--> SRV";
}
void Tester::test8_reverseTunnelBothWays()
{
#if ((TEST_ENABLE & 0x80) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    QFETCH(QByteArray, dataForTest);
    m_currentDataToUse=dataForTest;
    QSharedPointer<SshTunnelIn> in1 = m_ssh.getTunnelIn("T7_IN", m_srv.serverPort(), 0);
    if (in1->valid())
    {

    m_cli.connectToHost("127.0.0.1", in1->remotePort());
    int result = m_waitTestEnd.exec();
    m_cli.disconnectFromHost();
    if ( m_cli.state() != QAbstractSocket::UnconnectedState )
        m_cli.waitForDisconnected(1000);
    QVERIFY2(result == 0, "Test failed in timeout");
    compareResults();
    }
    else
    {
        QFAIL("Cannot create tunnelin");
    }
#endif
}

void Tester::test9_doubleReverseTunnelBothWays_data()
{
    populateTestData();
    m_mode = TEST_BIDIR;
    m_testName = "SshTunnelIn     CLI <-I-I-> SRV";
}

void Tester::test9_doubleReverseTunnelBothWays()
{
#if ((TEST_ENABLE & 0x100) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    QFETCH(QByteArray, dataForTest);
    m_currentDataToUse=dataForTest;
    QSharedPointer<SshTunnelIn> in1 = m_ssh.getTunnelIn("T9_IN1", m_srv.serverPort(), 0);
    if (in1->valid())
    {
        QTime timer;
        timer.start();
        while (timer.elapsed() < 250 )
        {
            QCoreApplication::processEvents();
        }
        QSharedPointer<SshTunnelIn> in2 = m_ssh.getTunnelIn("T9_IN2", in1->remotePort(), 0);
        timer.start();
        while (timer.elapsed() < 250 )
        {
            QCoreApplication::processEvents();
        }
        m_cli.connectToHost("127.0.0.1", in2->remotePort());
        int result = m_waitTestEnd.exec();
        QVERIFY2(result == 0, "Test failed in timeout");
        compareResults();
    }
    else
        QFAIL("Cannot create tunnelin");
#endif
}

void Tester::test10_DirectAndReverseTunnelBothWays_data()
{
    populateTestData();
    m_mode = TEST_BIDIR;
    m_testName = "SshTunnel In and Out BIDIR     CLI <-O-I-> SRV";
}
void Tester::test10_DirectAndReverseTunnelBothWays()
{
#if ((TEST_ENABLE & 0x200) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    QFETCH(QByteArray, dataForTest);
    m_currentDataToUse = dataForTest;
    QSharedPointer<SshTunnelIn> in1 = m_ssh.getTunnelIn("T10_IN1", m_srv.serverPort(), 0);
    if ( in1->valid() )
    {
        QSharedPointer<SshTunnelOut> out1 = m_ssh.getTunnelOut("T10_OUT1", in1->remotePort());
        m_cli.connectToHost("127.0.0.1", out1->localPort());
        int result = m_waitTestEnd.exec();
        m_cli.disconnectFromHost();
        if ( m_cli.state() != QAbstractSocket::UnconnectedState )
            m_cli.waitForDisconnected(1000);
        QVERIFY2(result == 0, "Test failed in timeout");
        compareResults();
    }
    else
         QFAIL("Cannot create tunnelin");
#endif
}

void Tester::benchmark1_directTunnelComClientToServer()
{
#if ((TEST_ENABLE & 0x400) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    m_currentDataToUse=m_dataSetBySize[1024*1024*4];
    m_mode = TEST_CLI2SRV;
    m_testName = "SshTunnel Out Benchmark CLI ->O-> SRV";
    QSharedPointer<SshTunnelOut> out1 = m_ssh.getTunnelOut("B2_OUT", m_srv.serverPort());
    m_cli.connectToHost("127.0.0.1", out1->localPort());
    QVERIFY(m_cli.waitForConnected(1000));
    int count = BENCHMARK_REPEAT;
    QTime timer;
    timer.start();
    while (count)
    {
        if ( count != BENCHMARK_REPEAT )
            m_cli.write(m_currentDataToUse);
        if ( m_waitTestEnd.exec() )
        {
            qCCritical(testssh) << "Test failed during benchmark";
        }
        m_readcli.clear();
        m_readsrv.clear();
        m_readcli.reserve(m_currentDataToUse.size());
        m_readsrv.reserve(m_currentDataToUse.size());
        m_timeout.stop();
        m_timeout.start(TestTimeOut);
        count--;
    }
    long elapsed = timer.elapsed();
    QTest::setBenchmarkResult(1000.0 * BENCHMARK_REPEAT * static_cast<qreal>(m_currentDataToUse.count()) / (elapsed ), QTest::BytesPerSecond);
    // Cleanup test
    m_cli.disconnectFromHost();
    if ( m_cli.state() != QAbstractSocket::UnconnectedState )
        m_cli.waitForDisconnected(1000);
#endif
}

void Tester::benchmark2_directTunnelComServerToClient()
{
#if ((TEST_ENABLE & 0x800) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    m_mode = TEST_SRV2CLI;
    m_testName = "SshTunnel Out Benchmark CLI <-O<- SRV";
    m_currentDataToUse=m_dataSetBySize[1024*1024*4];
    QSharedPointer<SshTunnelOut> out1 = m_ssh.getTunnelOut("T2_OUT", m_srv.serverPort());
    int count = BENCHMARK_REPEAT;
    QTime timer;
    m_cli.connectToHost("127.0.0.1", out1->localPort());
    timer.start();
    while (count)
    {
        if ( count != BENCHMARK_REPEAT )
            m_srvSocket->write(m_currentDataToUse);
        int result = m_waitTestEnd.exec();
        if ( result )
        {
            qCCritical(testssh) << "Test failed during benchmark";
        }
        m_readcli.clear();
        m_readsrv.clear();
        m_readcli.reserve(m_currentDataToUse.size());
        m_readsrv.reserve(m_currentDataToUse.size());
        m_timeout.start(TestTimeOut);
        count--;
    }
    long elapsed = timer.elapsed();
    QTest::setBenchmarkResult(1000.0 * BENCHMARK_REPEAT * static_cast<qreal>(m_currentDataToUse.count()) / (elapsed ), QTest::BytesPerSecond);
    // Cleanup test
    m_cli.disconnectFromHost();
    if ( m_cli.state() != QAbstractSocket::UnconnectedState )
        m_cli.waitForDisconnected(1000);
#endif
}

void Tester::benchmark3_directTunnelBothWays()
{
#if ((TEST_ENABLE & 0x1000) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    m_mode = TEST_BIDIR;
    m_testName = "SshTunnel Out Benchmark CLI <-O<- SRV";
    m_currentDataToUse=m_dataSetBySize[1024*1024*4];
    QSharedPointer<SshTunnelOut> out1 = m_ssh.getTunnelOut("T2_OUT", m_srv.serverPort());
    int count = BENCHMARK_REPEAT;
    QTime timer;
    m_cli.connectToHost("127.0.0.1", out1->localPort());
    timer.start();
    while (count)
    {
        if ( count != BENCHMARK_REPEAT )
        {
            m_srvSocket->write(m_currentDataToUse);
            m_cli.write(m_currentDataToUse);
        }
        int result = m_waitTestEnd.exec();
        if ( result )
        {
            qCCritical(testssh) << "Test failed during benchmark";
        }
        m_readcli.clear();
        m_readsrv.clear();
        m_readcli.reserve(m_currentDataToUse.size());
        m_readsrv.reserve(m_currentDataToUse.size());
        m_timeout.start(TestTimeOut);
        count--;
    }
    long elapsed = timer.elapsed();
    QTest::setBenchmarkResult(1000.0 * BENCHMARK_REPEAT * static_cast<qreal>(m_currentDataToUse.count()) / (elapsed ), QTest::BytesPerSecond);
    // Cleanup test
    m_cli.disconnectFromHost();
    if ( m_cli.state() != QAbstractSocket::UnconnectedState )
        m_cli.waitForDisconnected(1000);
#endif
}


void Tester::benchmark4_reverseTunnelComClientToServer()
{
#if ((TEST_ENABLE & 0x2000) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    m_currentDataToUse=m_dataSetBySize[1024*1024*4];
    m_mode = TEST_CLI2SRV;
    m_testName = "SshTunnel IN Benchmark CLI ->I-> SRV";

    QSharedPointer<SshTunnelIn> in1 = m_ssh.getTunnelIn("T7_IN", m_srv.serverPort(), 0);
    if ( in1->valid())
    {
        m_cli.connectToHost("127.0.0.1", in1->remotePort());
        int count = BENCHMARK_REPEAT;
        QTime timer;
        timer.start();
        while (count)
        {
            if ( count != BENCHMARK_REPEAT )
                m_cli.write(m_currentDataToUse);
            if ( m_waitTestEnd.exec() )
            {
                qCCritical(testssh) << "Test failed during benchmark";
            }
            m_readcli.clear();
            m_readsrv.clear();
            m_readcli.reserve(m_currentDataToUse.size());
            m_readsrv.reserve(m_currentDataToUse.size());
            m_timeout.stop();
            m_timeout.start(TestTimeOut);
            count--;
        }
        long elapsed = timer.elapsed();
        QTest::setBenchmarkResult(1000.0 * BENCHMARK_REPEAT * static_cast<qreal>(m_currentDataToUse.count()) / (elapsed ), QTest::BytesPerSecond);
    }
    else
    {
        QFAIL("Cannot create tunnelin");
    }
    // Cleanup test
    m_cli.disconnectFromHost();
    if ( m_cli.state() != QAbstractSocket::UnconnectedState )
        m_cli.waitForDisconnected(1000);
#endif
}

void Tester::benchmark5_reverseTunnelComServerToClient()
{
#if ((TEST_ENABLE & 0x4000) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    m_mode = TEST_SRV2CLI;
    m_testName = "SshTunnel Out Benchmark CLI <-O<- SRV";
    m_currentDataToUse=m_dataSetBySize[1024*1024*4];
    QSharedPointer<SshTunnelIn> in1 = m_ssh.getTunnelIn("T7_IN", m_srv.serverPort(), 0);
    if ( in1->valid())
    {
        int count = BENCHMARK_REPEAT;
        QTime timer;
        m_cli.connectToHost("127.0.0.1", in1->remotePort());
        timer.start();
        while (count)
        {
            if ( count != BENCHMARK_REPEAT )
                m_srvSocket->write(m_currentDataToUse);
            int result = m_waitTestEnd.exec();
            if ( result )
            {
                qCCritical(testssh) << "Test failed during benchmark";
            }
            m_readcli.clear();
            m_readsrv.clear();
            m_readcli.reserve(m_currentDataToUse.size());
            m_readsrv.reserve(m_currentDataToUse.size());
            m_timeout.start(TestTimeOut);
            count--;
        }
        long elapsed = timer.elapsed();
        QTest::setBenchmarkResult(1000.0 * BENCHMARK_REPEAT * static_cast<qreal>(m_currentDataToUse.count()) / (elapsed ), QTest::BytesPerSecond);
        // Cleanup test
        m_cli.disconnectFromHost();
        if ( m_cli.state() != QAbstractSocket::UnconnectedState )
            m_cli.waitForDisconnected(1000);
    }
#endif
}

void Tester::benchmark6_reverseTunnelBothWays()
{
#if ((TEST_ENABLE & 0x8000) == 0)
    QSKIP("Test disabled by TEST_ENABLE variable");
#else
    m_mode = TEST_BIDIR;
    m_testName = "SshTunnel Out Benchmark CLI <-O<- SRV";
    m_currentDataToUse=m_dataSetBySize[1024*1024*4];
    SshTunnelIn *in1 = m_ssh.getChannel<SshTunnelIn>("T7_IN");
    in1->configure(m_srv.serverPort(), 0);
    if ( in1->valid())
    {
        int count = BENCHMARK_REPEAT;
        QTime timer;
        m_cli.connectToHost("127.0.0.1", in1->remotePort());
        timer.start();
        while (count)
        {
            if ( count != BENCHMARK_REPEAT )
            {
                m_srvSocket->write(m_currentDataToUse);
                m_cli.write(m_currentDataToUse);
            }
            int result = m_waitTestEnd.exec();
            if ( result )
            {
                qCCritical(testssh) << "Test failed during benchmark";
            }
            m_readcli.clear();
            m_readsrv.clear();
            m_readcli.reserve(m_currentDataToUse.size());
            m_readsrv.reserve(m_currentDataToUse.size());
            m_timeout.start(TestTimeOut);
            count--;
        }
        long elapsed = timer.elapsed();
        QTest::setBenchmarkResult(1000.0 * BENCHMARK_REPEAT * static_cast<qreal>(m_currentDataToUse.count()) / (elapsed ), QTest::BytesPerSecond);
        // Cleanup test
        m_cli.disconnectFromHost();
        if ( m_cli.state() != QAbstractSocket::UnconnectedState )
            m_cli.waitForDisconnected(1000);
    }
#endif
}

void Tester::dumpData(const QString &testFilename, const QByteArray &data)
{
    QFile fres("/tmp/" + testFilename);
    if(!fres.open(QIODevice::WriteOnly))
    {
        qCCritical(testssh) << fres.errorString();
    }
    else
    {
        qCInfo(testssh) << "Dumping buffer into" << fres.fileName();
        fres.write(data);
        fres.close();
    }
}

void Tester::findFirstDifference(const QByteArray &buffer)
{
    for (int ii = 0; ii < buffer.length(); ii++)
    {
        if (buffer.at(ii) != m_currentDataToUse.at(ii))
        {
            qCInfo(testssh) << "Difference starts at" << QString("%1").arg(ii, 0, 16);
            break;
        }
    }
}

inline void Tester::compareResults()
{
    qCDebug(testssh) << "Compare results :" << "Srv data=" << m_readsrv.count()
                     << ",Cli data=" << m_readcli.count()
                     << ", expected="<< m_currentDataToUse.count();
    if( m_mode & TEST_CLI2SRV)
    {
        QVERIFY2(m_readsrv == m_currentDataToUse, "Data received by server have been corrupted");
    }
    if ( m_mode & TEST_SRV2CLI )
    {
        QVERIFY2(m_readcli == m_currentDataToUse, "Data received by client have been corrupted");
    }
}

inline void Tester::populateTestData()
{
    QTest::addColumn<QByteArray>("dataForTest");

    foreach(long size, m_sizeForDataSet)
    {
        QTest::newRow(QString("Buffer of size %1").arg(prettySize(size)).toLocal8Bit().constData()) << m_dataSetBySize[size];
    }
}

void Tester::checkTest()
{
    bool res = true;
    if( (m_mode & TEST_CLI2SRV) && (m_readsrv.size() < m_currentDataToUse.size()) )
        res = false;
    if ( (m_mode & TEST_SRV2CLI) && (m_readcli.size() < m_currentDataToUse.size()) )
        res = false;

    if( res )
    {
        m_timeout.stop();
        emit endSubTest(0);
    }
    else if ( m_timeout.remainingTime() <= 0  )     // All the datas are not received and timeout occurs
    {
        /* Test failed Dump test */
        qCInfo(testssh) << "checkTest() Timeout  SRV(" << m_readsrv.size()
                            << ") CLI(" << m_readcli.size()
                            << ") NEED(" << m_currentDataToUse.size() << ")";
        emit endSubTest(-1);
    }
}

void Tester::newServerConnection()
{
    QTcpSocket *clientConnection = m_srv.nextPendingConnection();
    qCDebug(testssh) << "Server: New connection";
    clientConnection->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    clientConnection->setReadBufferSize(2097152);
    clientConnection->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 2097152);
    connect(clientConnection, &QTcpSocket::readyRead, this, &Tester::serverDataReady);
    if(m_mode & TEST_SRV2CLI)
    {
        qCDebug(testssh) << "Write data in server socket";
        clientConnection->write(m_currentDataToUse);
        m_srvSocket = clientConnection;
    }
}

void Tester::serverDataReady()
{
    static long receivedData = 0;
    QTcpSocket *clientConnection = qobject_cast<QTcpSocket*>(sender());
    if(clientConnection)
    {
        while (clientConnection->bytesAvailable() > 0 )
        {
            QByteArray res = clientConnection->readAll();
            receivedData += res.size();
            m_readsrv.append(res);
        }
    }
    if(m_readsrv.size() >= m_currentDataToUse.size())
    {
        emit serverBufferChange();
    }
}

void Tester::clientDataReady()
{
    QByteArray res = m_cli.readAll();
    m_readcli.append(res);
    if(m_readcli.size() >= m_currentDataToUse.size())
        emit clientBufferChange();
}

void Tester::clientConnected()
{
    qCDebug(testssh) << "Client socket connected";
    if(m_mode & TEST_CLI2SRV)
    {
        qCDebug(testssh) << "Write data in client socket" << m_currentDataToUse.size();
        m_cli.write(m_currentDataToUse);
    }
}
