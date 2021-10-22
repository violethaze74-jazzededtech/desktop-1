#pragma once

#include "accountfwd.h"

#include <QObject>
#include <QPixmap>
#include <QUrl>
#include <QString>

namespace OCC {

struct HovercardAction
{
public:
    HovercardAction();
    HovercardAction(QString title, QUrl iconUrl, QUrl link);

    QString _title;
    QUrl _iconUrl;
    QPixmap _icon;
    QUrl _link;
};

struct Hovercard
{
    std::vector<HovercardAction> _actions;
};

class OcsProfileConnector : public QObject
{
    Q_OBJECT
public:
    explicit OcsProfileConnector(AccountPtr account, QObject *parent = nullptr);

    void fetchHovercard(const QString &userId);
    Hovercard hovercard() const;

signals:
    void hovercardFetched();
    void iconLoaded(const std::size_t hovercardActionIndex);

private:
    void onHovercardFetched(const QJsonDocument &json, int statusCode);

    void fetchIcons();
    void startFetchIconJob(const std::size_t hovercardActionIndex);
    void setHovercardActionIcon(const std::size_t index, const QPixmap &pixmap);
    void loadHovercardActionIcon(const std::size_t hovercardActionIndex, const QByteArray &iconData);

    AccountPtr _account;
    Hovercard _currentHovercard;
};
}

Q_DECLARE_METATYPE(OCC::HovercardAction)
Q_DECLARE_METATYPE(OCC::Hovercard)
