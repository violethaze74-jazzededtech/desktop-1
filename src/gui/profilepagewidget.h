#pragma once

#include "ocsprofileconnector.h"

#include <QBoxLayout>
#include <QLabel>

#include <cstddef>

namespace OCC {

class ProfilePageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProfilePageWidget(QWidget *parent = nullptr);

    void setProfileConnector(std::unique_ptr<OcsProfileConnector> profileConnector, const QString &userId);

private:
    void onHovercardFetched();
    void onIconLoaded(const std::size_t &hovercardActionIndex);

    void recreateLayout();
    void resetLayout();
    void createLayout();

    std::unique_ptr<OcsProfileConnector> _profileConnector;

    QVBoxLayout *_mainLayout{};
    std::vector<QLabel *> _profilePageButtonIcons;
};
}
