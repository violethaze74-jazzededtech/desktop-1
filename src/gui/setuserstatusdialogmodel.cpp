/*
 * Copyright (C) by Felix Weilbach <felix.weilbach@nextcloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "setuserstatusdialogmodel.h"

#include <ocsuserstatusconnector.h>
#include <userstatusconnector.h>
#include <theme.h>

#include <QDateTime>
#include <QLoggingCategory>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace OCC {

Q_LOGGING_CATEGORY(lcUserStatusDialogModel, "nextcloud.gui.userstatusdialogmodel", QtInfoMsg)

SetUserStatusDialogModel::SetUserStatusDialogModel(QObject *parent)
    : QObject(parent)
    , _dateTimeProvider(new DateTimeProvider)
{
}

SetUserStatusDialogModel::SetUserStatusDialogModel(std::shared_ptr<UserStatusConnector> userStatusJob,
    QObject *parent)
    : QObject(parent)
    , _userStatusJob(std::move(userStatusJob))
    , _userStatus("no-id", "", "😀",
          UserStatus::OnlineStatus::Online, false, {})
    , _dateTimeProvider(new DateTimeProvider)
{
    init();
}

SetUserStatusDialogModel::SetUserStatusDialogModel(std::shared_ptr<UserStatusConnector> userStatusJob,
    std::unique_ptr<DateTimeProvider> dateTimeProvider,
    QObject *parent)
    : QObject(parent)
    , _userStatusJob(std::move(userStatusJob))
    , _dateTimeProvider(std::move(dateTimeProvider))
{
    init();
}

SetUserStatusDialogModel::SetUserStatusDialogModel(const UserStatus &userStatus,
    std::unique_ptr<DateTimeProvider> dateTimeProvider, QObject *parent)
    : QObject(parent)
    , _userStatus(userStatus)
    , _dateTimeProvider(std::move(dateTimeProvider))
{
}

SetUserStatusDialogModel::SetUserStatusDialogModel(const UserStatus &userStatus,
    QObject *parent)
    : QObject(parent)
    , _userStatus(userStatus)
{
}

void SetUserStatusDialogModel::init()
{
    if (!_userStatusJob) {
        return;
    }

    connect(_userStatusJob.get(), &UserStatusConnector::userStatusFetched, this,
        &SetUserStatusDialogModel::onUserStatusFetched);
    connect(_userStatusJob.get(), &UserStatusConnector::predefinedStatusesFetched, this,
        &SetUserStatusDialogModel::onPredefinedStatusesFetched);
    connect(_userStatusJob.get(), &UserStatusConnector::userStatusSet, this,
        &SetUserStatusDialogModel::onUserStatusSet);
    connect(_userStatusJob.get(), &UserStatusConnector::messageCleared, this,
        &SetUserStatusDialogModel::onMessageCleared);
    connect(_userStatusJob.get(), &UserStatusConnector::error, this,
        &SetUserStatusDialogModel::onError);

    _userStatusJob->fetchUserStatus();
    _userStatusJob->fetchPredefinedStatuses();
}

SetUserStatusDialogModel::~SetUserStatusDialogModel()
{
    qCDebug(lcUserStatusDialogModel) << "Destroyed";
}

void SetUserStatusDialogModel::onUserStatusSet()
{
    qCDebug(lcUserStatusDialogModel) << "Emit finished";
    emit finished();
}

void SetUserStatusDialogModel::onMessageCleared()
{
    emit finished();
}

void SetUserStatusDialogModel::onError(UserStatusConnector::Error error)
{
    qCWarning(lcUserStatusDialogModel) << "Error:" << error;

    switch (error) {
    case UserStatusConnector::Error::CouldNotFetchPredefinedUserStatuses:
        setError(tr("Could not fetch predefined statuses. Make sure you are connected to the internet."));
        return;

    case UserStatusConnector::Error::CouldNotFetchUserStatus:
        setError(tr("Could not fetch user status. Make sure you are connected to the internet."));
        return;

    case UserStatusConnector::Error::UserStatusNotSupported:
        setError(tr("User status feature is not supported on the server."));
        return;

    case UserStatusConnector::Error::EmojisNotSupported:
        setError(tr("Emojis feature is not supported on the server."));
        return;

    case UserStatusConnector::Error::CouldNotSetUserStatus:
        setError(tr("Could not set user status. Make sure you are connected to the internet."));
        return;

    case UserStatusConnector::Error::CouldNotClearMessage:
        setError(tr("Could not clear user status message. Make sure you are connected to the internet."));
        return;
    }

    Q_UNREACHABLE();
}

void SetUserStatusDialogModel::setError(const QString &reason)
{
    _errorMessage = reason;
    emit errorMessageChanged();
    emit showError();
}

void SetUserStatusDialogModel::setOnlineStatus(UserStatus::OnlineStatus status)
{
    if (status == _userStatus.state()) {
        return;
    }

    _userStatus.setState(status);
    emit onlineStatusChanged();
}

QUrl SetUserStatusDialogModel::onlineIcon() const
{
    return Theme::instance()->statusOnlineImageSource();
}

QUrl SetUserStatusDialogModel::awayIcon() const
{
    return Theme::instance()->statusAwayImageSource();
}
QUrl SetUserStatusDialogModel::dndIcon() const
{
    return Theme::instance()->statusDoNotDisturbImageSource();
}
QUrl SetUserStatusDialogModel::invisibleIcon() const
{
    return Theme::instance()->statusInvisibleImageSource();
}

UserStatus::OnlineStatus SetUserStatusDialogModel::onlineStatus() const
{
    return _userStatus.state();
}

QString SetUserStatusDialogModel::userStatusMessage() const
{
    return _userStatus.message();
}

void SetUserStatusDialogModel::setUserStatusMessage(const QString &message)
{
    _userStatus.setMessage(message);
    _userStatus.setMessagePredefined(false);
}

void SetUserStatusDialogModel::setUserStatusEmoji(const QString &emoji)
{
    _userStatus.setIcon(emoji);
    _userStatus.setMessagePredefined(false);
    emit userStatusChanged();
}

QString SetUserStatusDialogModel::userStatusEmoji() const
{
    return _userStatus.icon();
}

void SetUserStatusDialogModel::onUserStatusFetched(const UserStatus &userStatus)
{
    if (userStatus.state() != UserStatus::OnlineStatus::Offline) {
        _userStatus.setState(userStatus.state());
    }
    _userStatus.setMessage(userStatus.message());
    _userStatus.setMessagePredefined(userStatus.messagePredefined());
    _userStatus.setId(userStatus.id());
    _userStatus.setClearAt(userStatus.clearAt());

    if (!userStatus.icon().isEmpty()) {
        _userStatus.setIcon(userStatus.icon());
    }

    emit userStatusChanged();
    emit onlineStatusChanged();
    emit clearAtChanged();
}

Optional<ClearAt> SetUserStatusDialogModel::clearStageTypeToDateTime(ClearStageType type) const
{
    switch (type) {
    case ClearStageType::DontClear:
        return {};

    case ClearStageType::HalfHour: {
        ClearAt clearAt;
        clearAt._type = ClearAtType::Period;
        clearAt._period = 60 * 30;
        return clearAt;
    }

    case ClearStageType::OneHour: {
        ClearAt clearAt;
        clearAt._type = ClearAtType::Period;
        clearAt._period = 60 * 60;
        return clearAt;
    }

    case ClearStageType::FourHour: {
        ClearAt clearAt;
        clearAt._type = ClearAtType::Period;
        clearAt._period = 60 * 60 * 4;
        return clearAt;
    }

    case ClearStageType::Today: {
        ClearAt clearAt;
        clearAt._type = ClearAtType::EndOf;
        clearAt._endof = "day";
        return clearAt;
    }

    case ClearStageType::Week: {
        ClearAt clearAt;
        clearAt._type = ClearAtType::EndOf;
        clearAt._endof = "week";
        return clearAt;
    }

    default:
        Q_UNREACHABLE();
    }
}

void SetUserStatusDialogModel::setUserStatus()
{
    _userStatusJob->setUserStatus(_userStatus);
}

void SetUserStatusDialogModel::clearUserStatus()
{
    _userStatusJob->clearMessage();
}

void SetUserStatusDialogModel::onPredefinedStatusesFetched(const std::vector<UserStatus> &statuses)
{
    _predefinedStatuses = statuses;
    emit predefinedStatusesChanged();
}

UserStatus SetUserStatusDialogModel::predefinedStatus(int index) const
{
    Q_ASSERT(0 <= index && index < static_cast<int>(_predefinedStatuses.size()));
    return _predefinedStatuses[index];
}

int SetUserStatusDialogModel::predefinedStatusesCount() const
{
    return static_cast<int>(_predefinedStatuses.size());
}

void SetUserStatusDialogModel::setPredefinedStatus(int index)
{
    Q_ASSERT(0 <= index && index < static_cast<int>(_predefinedStatuses.size()));

    _userStatus.setMessagePredefined(true);
    _userStatus.setId(_predefinedStatuses[index].id());
    _userStatus.setMessage(_predefinedStatuses[index].message());
    _userStatus.setIcon(_predefinedStatuses[index].icon());
    _userStatus.setClearAt(_predefinedStatuses[index].clearAt());

    emit userStatusChanged();
    emit clearAtChanged();
}

QString SetUserStatusDialogModel::clearAtStageToString(ClearStageType stage) const
{
    switch (stage) {
    case ClearStageType::DontClear:
        return tr("Don't clear");

    case ClearStageType::HalfHour:
        return tr("30 minutes");

    case ClearStageType::OneHour:
        return tr("1 hour");

    case ClearStageType::FourHour:
        return tr("4 hours");

    case ClearStageType::Today:
        return tr("Today");

    case ClearStageType::Week:
        return tr("This week");

    default:
        Q_UNREACHABLE();
    }
}

QStringList SetUserStatusDialogModel::clearAtStages() const
{
    QStringList clearAtStages;
    std::transform(_clearStages.begin(), _clearStages.end(),
        std::back_inserter(clearAtStages),
        [this](const ClearStageType &stage) { return clearAtStageToString(stage); });

    return clearAtStages;
}

void SetUserStatusDialogModel::setClearAt(int index)
{
    Q_ASSERT(0 <= index && index < static_cast<int>(_clearStages.size()));
    _userStatus.setClearAt(clearStageTypeToDateTime(_clearStages[index]));
    emit clearAtChanged();
}

QString SetUserStatusDialogModel::errorMessage() const
{
    return _errorMessage;
}

QString SetUserStatusDialogModel::timeDifferenceToString(int differenceSecs) const
{
    if (differenceSecs < 60) {
        return tr("Less than a minute");
    } else if (differenceSecs < 60 * 60) {
        const auto minutesLeft = std::ceil(differenceSecs / 60.0);
        if (minutesLeft == 1) {
            return tr("1 minute");
        } else {
            return tr("%1 minutes").arg(minutesLeft);
        }
    } else if (differenceSecs < 60 * 60 * 24) {
        const auto hoursLeft = std::ceil(differenceSecs / 60.0 / 60.0);
        if (hoursLeft == 1) {
            return tr("1 hour");
        } else {
            return tr("%1 hours").arg(hoursLeft);
        }
    } else {
        const auto daysLeft = std::ceil(differenceSecs / 60.0 / 60.0 / 24.0);
        if (daysLeft == 1) {
            return tr("1 day");
        } else {
            return tr("%1 days").arg(daysLeft);
        }
    }
}

QString SetUserStatusDialogModel::clearAtReadable(const Optional<ClearAt> &clearAt) const
{
    if (clearAt) {
        switch (clearAt->_type) {
        case ClearAtType::Period: {
            return timeDifferenceToString(clearAt->_period);
        }

        case ClearAtType::Timestamp: {
            const int difference = static_cast<int>(clearAt->_timestamp - _dateTimeProvider->currentDateTime().toTime_t());
            return timeDifferenceToString(difference);
        }

        case ClearAtType::EndOf: {
            if (clearAt->_endof == "day") {
                return tr("Today");
            } else if (clearAt->_endof == "week") {
                return tr("This week");
            }
            Q_UNREACHABLE();
        }

        default:
            Q_UNREACHABLE();
        }
    }
    return tr("Don't clear");
}

QString SetUserStatusDialogModel::predefinedStatusClearAt(int index) const
{
    return clearAtReadable(predefinedStatus(index).clearAt());
}

QString SetUserStatusDialogModel::clearAt() const
{
    return clearAtReadable(_userStatus.clearAt());
}
}
