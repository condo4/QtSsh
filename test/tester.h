#ifndef TESTER_H
#define TESTER_H

#include <QObject>
#include <sshclient.h>
#include <QTcpServer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(testssh)

class Tester : public QObject
{
    Q_OBJECT

    enum direction {
        TEST_NONE = 0,
        TEST_SRV2CLI = 1,
        TEST_CLI2SRV = 2,
        TEST_BIDIR = 3
    };

    QString m_hostname;
    QString m_login;
    QString m_password;
    QByteArray m_bigdata;
    QByteArray m_readsrv;
    QByteArray m_readcli;
    QTcpServer m_srv;
    QTcpSocket m_cli;
    QTcpSocket *m_srvSocket;
    QTimer m_timeout;
    SshClient m_ssh;
    direction m_mode {TEST_NONE};
    QString m_testName;
    QByteArray &m_currentDataToUse;
    QList<long> m_sizeForDataSet = {1024*1024, 4*1024*1024, 8*1024*1024, 16*1024*1024, 32*1024*1024, 64*1024*1024, 128*1024*1024, 256*1024*1024};
    QList<long> m_sizeForBenchmarkDataSet = {1024*1024*4};
    QMap<long, QByteArray> m_dataSetBySize;
    QEventLoop m_waitTestEnd;
    QEventLoop m_waitGeneralPurpose;

public:
    explicit Tester(QString hostname, QString login, QString password, QObject *parent = nullptr);

public slots:
    void checkTest();
    void newServerConnection();
    void serverDataReady();
    void clientDataReady();
    void clientConnected();

signals:
    void endtest(bool res);
    void endSubTest(int result);
    void serverBufferChange();
    void clientBufferChange();
    void newClientConnected(QTcpSocket *client);

private:
    bool testRemoteProcess(const QString &name);
    void findFirstDifference(const QByteArray &buffer);
    void compareResults();
    void populateTestData();
    void dumpData(const QString &testFilename, const QByteArray &data);
private slots:

    void initTestCase();
    void test1_RemoteProcess();
    void test2_directTunnelComClientToServer();
    void test3_directTunnelComServerToClient();
    void test4_directTunnelComBothWays();
    void test5_doubleDirectTunnelComBothWays();
    void test6_reverseTunnelComClientToServer();
    void test7_reverseTunnelComServerToClient();
    void test8_reverseTunnelBothWays();
    void test9_doubleReverseTunnelBothWays();
    void test2_directTunnelComClientToServer_data();
    void test3_directTunnelComServerToClient_data();
    void test4_directTunnelComBothWays_data();
    void test5_doubleDirectTunnelComBothWays_data();
    void test6_reverseTunnelComClientToServer_data();
    void test7_reverseTunnelComServerToClient_data();
    void test8_reverseTunnelBothWays_data();
    void test9_doubleReverseTunnelBothWays_data();
    void init();
    void cleanup();
    void test10_DirectAndReverseTunnelBothWays_data();
    void test10_DirectAndReverseTunnelBothWays();
    void benchmark1_directTunnelComClientToServer();
    void benchmark2_directTunnelComServerToClient();
    void benchmark3_directTunnelBothWays();
    void benchmark4_reverseTunnelComClientToServer();
    void benchmark5_reverseTunnelComServerToClient();
    void benchmark6_reverseTunnelBothWays();
};

#endif // TESTER_H
