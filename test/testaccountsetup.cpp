#include <QTest>
#include <QSignalSpy>

#include <QtHttpServer>

#include <QJsonDocument>
#include <QJsonObject>

#include "account.h"
#include "accountmanager.h"
#include "accountstate.h"
#include "libsync/creds/httpcredentials.h"
#include "libsync/networkjobs.h"

namespace {
class HttpCredentialsText : public OCC::HttpCredentials
{
public:
    HttpCredentialsText(const QString &user, const QString &password)
        : HttpCredentials(user, password)
        , // FIXME: not working with client certs yet (qknight)
        _sslTrusted(false)
    {
    }

    void askFromUser() override
    {
        _password = QStringLiteral("admin");
        _ready = true;
        persist();
        emit asked();
    }

    void setWasFetched(bool value) { _wasFetched = value; }

    void setSSLTrusted(bool isTrusted) { _sslTrusted = isTrusted; }

    bool sslIsTrusted() override { return _sslTrusted; }

private:
    bool _sslTrusted;
};

QByteArray fakeReplyStatusPhp = R"({
    "installed": true,
    "maintenance": false,
    "needsDbUpgrade": false,
    "version": "22.2.0.2",
    "versionstring": "22.2.0",
    "edition": "",
    "productname": "Nextcloud",
    "extendedSupport": false
})";

QByteArray fakeReplyCapabilities = R"({{"ocs":{"data":{"capabilities":{"activity":{"apiv2":["filters","filters-api","previews","rich-strings"]},"bruteforce":{"delay":0},"circles":{"circle":{"config":{"coreFlags":[1,2,4],"systemFlags":[512,1024,2048]},"constants":{"flags":{"1":"Single","1024":"Hidden","128":"Friends","16":"Open","16384":"Circle Invite","2":"Personal","2048":"Backend","256":"Password Protected","32":"Invite","32768":"Federated","4":"System","4096":"Local","512":"No Owner","64":"Join Request","65536":"Mount point","8":"Visible","8192":"Root"},"source":{"core":{"1":"Nextcloud User","10000":"Nextcloud App","16":"Circle","2":"Nextcloud Group","4":"Email Address","8":"Contact"},"extra":{"10001":"Circles App","10002":"Admin Command Line"}}}},"member":{"constants":{"level":{"1":"Member","4":"Moderator","8":"Admin","9":"Owner"}},"type":{"0":"single","1":"user","10000":"app","16":"circle","2":"group","4":"mail","8":"contact"}},"settings":{"allowedCircles":131071,"allowedUserTypes":31,"frontendEnabled":true,"membersLimit":-1},"status":{"globalScale":false},"version":"22.1.1"},"core":{"pollinterval":60,"webdav-root":"remote.php/webdav"},"dav":{"chunking":"1.0"},"deck":{"apiVersions":["1.0","1.1"],"canCreateBoards":true,"version":"1.5.3"},"external":{"v1":["sites","device","groups","redirect"]},"files":{"bigfilechunking":true,"blacklisted_files":[".htaccess"],"comments":true,"directEditing":{"etag":"c748e8fc588b54fc5af38c4481a19d20","url":"https://cloud.nextcloud.com/ocs/v2.php/apps/files/api/v1/directEditing"},"undelete":true,"versioning":true},"files_sharing":{"api_enabled":true,"default_permissions":31,"federation":{"expire_date":{"enabled":true},"expire_date_supported":{"enabled":true},"incoming":true,"outgoing":true},"group":{"enabled":true,"expire_date":{"enabled":true}},"group_sharing":true,"public":{"enabled":true,"expire_date":{"days":31,"enabled":true,"enforced":false},"expire_date_internal":{"enabled":false},"expire_date_remote":{"enabled":false},"multiple_links":true,"password":{"askForOptionalPassword":false,"enforced":false},"send_mail":false,"upload":true,"upload_files_drop":true},"resharing":true,"sharebymail":{"enabled":true,"expire_date":{"enabled":true,"enforced":false},"password":{"enabled":true,"enforced":false},"upload_files_drop":{"enabled":true}},"sharee":{"always_show_unique":true,"query_lookup_default":false},"user":{"expire_date":{"enabled":true},"send_mail":false}},"notes":{"api_version":["0.2","1.2"],"version":"4.1.1"},"notifications":{"admin-notifications":["ocs","cli"],"ocs-endpoints":["list","get","delete","delete-all","icons","rich-strings","action-web","user-status"],"push":["devices","object-data","delete"]},"notify_push":{"endpoints":{"pre_auth":"https://cloud.nextcloud.com/apps/notify_push/pre_auth","websocket":"wss://push.cloud.nextcloud.com/ws"},"type":["files","activities","notifications"]},"ocm":{"apiVersion":"1.0-proposal1","enabled":true,"endPoint":"https://cloud.nextcloud.com/ocm","resourceTypes":[{"name":"file","protocols":{"webdav":"/public.php/webdav/"},"shareTypes":["user","group"]}]},"password_policy":{"api":{"generate":"https://cloud.nextcloud.com/ocs/v2.php/apps/password_policy/api/v1/generate","validate":"https://cloud.nextcloud.com/ocs/v2.php/apps/password_policy/api/v1/validate"},"enforceNonCommonPassword":true,"enforceNumericCharacters":false,"enforceSpecialCharacters":false,"enforceUpperLowerCase":false,"minLength":8},"provisioning_api":{"AccountPropertyScopesFederationEnabled":true,"AccountPropertyScopesVersion":2,"version":"1.12.0"},"richdocuments":{"collabora":{"convert-to":{"available":true},"hasMobileSupport":true,"hasProxyPrefix":false,"hasTemplateSaveAs":false,"hasTemplateSource":true,"productName":"Collabora Online","productVersion":"6.4.9","productVersionHash":"78239aa"},"config":{"disable_certificate_verification":"","doc_format":"odf","edit_groups":"","public_wopi_url":null,"timeout":15,"use_groups":"","wopi_url":"https://collabora.cloud.nextcloud.com"},"direct_editing":true,"mimetypes":["application/vnd.oasis.opendocument.text","application/vnd.oasis.opendocument.spreadsheet","application/vnd.oasis.opendocument.graphics","application/vnd.oasis.opendocument.presentation","application/vnd.lotus-wordpro","application/vnd.visio","application/vnd.ms-visio.drawing","application/vnd.wordperfect","application/msonenote","application/msword","application/rtf","text/rtf","application/vnd.openxmlformats-officedocument.wordprocessingml.document","application/vnd.openxmlformats-officedocument.wordprocessingml.template","application/vnd.ms-word.document.macroEnabled.12","application/vnd.ms-word.template.macroEnabled.12","application/vnd.ms-excel","application/vnd.openxmlformats-officedocument.spreadsheetml.sheet","application/vnd.openxmlformats-officedocument.spreadsheetml.template","application/vnd.ms-excel.sheet.macroEnabled.12","application/vnd.ms-excel.template.macroEnabled.12","application/vnd.ms-excel.addin.macroEnabled.12","application/vnd.ms-excel.sheet.binary.macroEnabled.12","application/vnd.ms-powerpoint","application/vnd.openxmlformats-officedocument.presentationml.presentation","application/vnd.openxmlformats-officedocument.presentationml.template","application/vnd.openxmlformats-officedocument.presentationml.slideshow","application/vnd.ms-powerpoint.addin.macroEnabled.12","application/vnd.ms-powerpoint.presentation.macroEnabled.12","application/vnd.ms-powerpoint.template.macroEnabled.12","application/vnd.ms-powerpoint.slideshow.macroEnabled.12","text/csv"],"mimetypesNoDefaultOpen":["image/svg+xml","application/pdf","text/plain","text/spreadsheet"],"productName":"Collabora Online","templates":true,"version":"4.2.3"},"spreed":{"config":{"attachments":{"allowed":true,"folder":"/Talk"},"chat":{"max-length":32000,"read-privacy":0},"conversations":{"can-create":true},"previews":{"max-gif-size":3145728}},"features":["audio","video","chat-v2","conversation-v4","guest-signaling","empty-group-room","guest-display-names","multi-room-users","favorites","last-room-activity","no-ping","system-messages","delete-messages","mention-flag","in-call-flags","conversation-call-flags","notification-levels","invite-groups-and-mails","locked-one-to-one-rooms","read-only-rooms","listable-rooms","chat-read-marker","webinary-lobby","start-call-flag","chat-replies","circles-support","force-mute","sip-support","chat-read-status","phonebook-search","raise-hand","room-description","rich-object-sharing","temp-user-avatar-api","geo-location-sharing","voice-message-sharing","signaling-v3","publishing-permissions","clear-history","chat-reference-id"]},"theming":{"background":"https://cloud.nextcloud.com/core/img/background.png?v=69","background-default":true,"background-plain":false,"color":"#0082c9","color-element":"#0082c9","color-element-bright":"#0082c9","color-element-dark":"#0082c9","color-text":"#ffffff","favicon":"https://cloud.nextcloud.com/core/img/logo/logo.svg?v=69","logo":"https://cloud.nextcloud.com/core/img/logo/logo.svg?v=69","logoheader":"https://cloud.nextcloud.com/core/img/logo/logo.svg?v=69","name":"Nextcloud","slogan":"a safe home for all your data","url":"https://nextcloud.com"},"user_status":{"enabled":true,"supports_emoji":true},"weather_status":{"enabled":true}},"version":{"edition":"","extendedSupport":false,"major":22,"micro":0,"minor":2,"string":"22.2.0"}},"meta":{"itemsperpage":"","message":"OK","status":"ok","statuscode":100,"totalitems":""}}}})";

}

class TestAccountSetup : public QObject
{
    Q_OBJECT

private slots:
    void testInitialSetup()
    {
        QHttpServer httpServer;
        httpServer.route(QLatin1String("/status.php"), []() {
            return QJsonDocument::fromJson(fakeReplyStatusPhp).toJson(QJsonDocument::Compact);
        });
        httpServer.route(QLatin1String("/ocs/v1.php/cloud/capabilities"), []() {
            return QJsonDocument::fromJson(fakeReplyCapabilities).toJson(QJsonDocument::Compact);
        });
        
        httpServer.listen(QHostAddress::LocalHost);

        const auto servers = httpServer.servers();

        if (!servers.isEmpty()) {
            const auto host = servers.at(0)->serverAddress().toString();
            const auto port = servers.at(0)->serverPort();

            if (!host.isEmpty() && port != 0) {
                auto account = OCC::Account::create();
                account->setUrl(QUrl::fromUserInput(QStringLiteral("http://") + host + QLatin1Char(':') + QString::number(port)));
                if (account->url().isValid()) {
                    auto *credentials = new HttpCredentialsText("admin", "admin");
                    credentials->setWasFetched(true);
                    if (credentials) {
                        account->setCredentials(credentials);

                        OCC::AccountManager::instance()->addAccount(account);
                        
                        const auto list = OCC::AccountManager::instance()->accounts();

                        QSignalSpy finishedSpy(list.first().data(), &OCC::AccountState::stateChanged);

                        list.first()->checkConnectivity();

                        finishedSpy.wait();

                        QEventLoop loop;
                        auto *job = new OCC::JsonApiJob(account, QLatin1String("ocs/v1.php/cloud/capabilities"));
                        QObject::connect(job, &OCC::JsonApiJob::jsonReceived, [&](const QJsonDocument &json) {
                            auto caps = json.object()
                                            .value("ocs")
                                            .toObject()
                                            .value("data")
                                            .toObject()
                                            .value("capabilities")
                                            .toObject();
                            qDebug() << "Server capabilities" << caps;
                            account->setCapabilities(caps.toVariantMap());
                            account->setServerVersion(
                                caps["core"].toObject()["status"].toObject()["version"].toString());
                            loop.quit();
                        });
                        job->start();
                        loop.exec();

                        if (job->reply()->error() != QNetworkReply::NoError) {
                            return;
                        }

                        job = new OCC::JsonApiJob(account, QLatin1String("ocs/v1.php/cloud/user"));
                        QObject::connect(job, &OCC::JsonApiJob::jsonReceived, [&](const QJsonDocument &json) {
                            const QJsonObject data = json.object().value("ocs").toObject().value("data").toObject();
                            account->setDavUser(data.value("id").toString());
                            account->setDavDisplayName(data.value("display-name").toString());
                            loop.quit();
                        });
                        job->start();
                        loop.exec();
                    }
                }
            }
            QTest::qWait(10000);
        }
    }
};

QTEST_GUILESS_MAIN(TestAccountSetup)
#include "testaccountsetup.moc"
