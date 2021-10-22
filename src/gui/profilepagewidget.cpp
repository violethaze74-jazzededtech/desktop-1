#include "profilepagewidget.h"
#include "guiutility.h"
#include "theme.h"

#include <QPushButton>

namespace OCC {

ProfilePageWidget::ProfilePageWidget(QWidget *parent)
    : QWidget(parent)
{
}

void ProfilePageWidget::setProfileConnector(
    std::unique_ptr<OcsProfileConnector> profileConnector, const QString &userId)
{
    _profileConnector = std::move(profileConnector);
    _profileConnector->fetchHovercard(userId);
    connect(
        _profileConnector.get(), &OcsProfileConnector::hovercardFetched, this, &ProfilePageWidget::onHovercardFetched);
    connect(_profileConnector.get(), &OcsProfileConnector::iconLoaded, this, &ProfilePageWidget::onIconLoaded);
}

void ProfilePageWidget::resetLayout()
{
    _mainLayout = new QVBoxLayout;
    _mainLayout->setSpacing(0);
    setLayout(_mainLayout);
    _profilePageButtonIcons.clear();
}

void ProfilePageWidget::createLayout()
{
    const auto hovercardActions = _profileConnector->hovercard()._actions;
    _profilePageButtonIcons.reserve(hovercardActions.size());
    for (const auto &hovercardAction : hovercardActions) {
        const auto button = new QPushButton;
        auto buttonSizePolicy = button->sizePolicy();
        buttonSizePolicy.setHorizontalStretch(1);
        button->setSizePolicy(buttonSizePolicy);
        button->setText(hovercardAction._title);
        const auto link = hovercardAction._link;
        connect(button, &QPushButton::clicked, button, [link] { Utility::openBrowser(link); });

        auto icon = new QLabel;
        QSizePolicy sizePolicy;
        sizePolicy.setHorizontalPolicy(QSizePolicy::Policy::Minimum);
        sizePolicy.setVerticalPolicy(QSizePolicy::Policy::Minimum);
        icon->setSizePolicy(sizePolicy);

        _profilePageButtonIcons.push_back(icon);

        const auto row = new QWidget;
        const auto rowLayout = new QHBoxLayout;
        row->setLayout(rowLayout);
        rowLayout->addWidget(icon);
        rowLayout->addWidget(button);
        _mainLayout->addWidget(row);
    }
}

void ProfilePageWidget::recreateLayout()
{
    resetLayout();
    createLayout();
}

void ProfilePageWidget::onHovercardFetched()
{
    recreateLayout();
}

void ProfilePageWidget::onIconLoaded(const std::size_t &hovercardActionIndex)
{
    if (hovercardActionIndex >= _profilePageButtonIcons.size()) {
        return;
    }
    auto icon = _profilePageButtonIcons[hovercardActionIndex];
    const auto hovercardAction = _profileConnector->hovercard()._actions[hovercardActionIndex];
    icon->setPixmap(hovercardAction._icon);
}
}
