#include <QTest>

#include <QtHttpServer>

class TestAccountSetup : public QObject
{
    Q_OBJECT

private slots:
    void testInitialSetup()
    {
        QHttpServer httpServer;
        httpServer.route("/login", []() {
            return "Logged in sucessfuly";
        });
        httpServer.route("/files", []() {
            return "<files><file/><file/></files>";
        });
        httpServer.listen(QHostAddress::LocalHost, 80);

        QTest::qWait(10000);
    }
};

QTEST_GUILESS_MAIN(TestAccountSetup)
#include "testaccountsetup.moc"
