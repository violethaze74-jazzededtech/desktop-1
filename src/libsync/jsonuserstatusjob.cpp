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

#include "jsonuserstatusjob.h"
#include "account.h"
#include "userstatusjob.h"

#include <networkjobs.h>

#include <QDateTime>
#include <QtGlobal>
#include <QJsonDocument>
#include <QJsonValue>
#include <QLoggingCategory>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace {
OCC::UserStatus::OnlineStatus stringToUserOnlineStatus(const QString &status)
{
    // it needs to match the Status enum
    const QHash<QString, OCC::UserStatus::OnlineStatus> preDefinedStatus {
        { "online", OCC::UserStatus::OnlineStatus::Online },
        { "dnd", OCC::UserStatus::OnlineStatus::DoNotDisturb },
        { "away", OCC::UserStatus::OnlineStatus::Away },
        { "offline", OCC::UserStatus::OnlineStatus::Offline },
        { "invisible", OCC::UserStatus::OnlineStatus::Invisible }
    };

    // api should return invisible, dnd,... toLower() it is to make sure
    // it matches _preDefinedStatus, otherwise the default is online (0)
    return preDefinedStatus.value(status.toLower(), OCC::UserStatus::OnlineStatus::Online);
}

QString onlineStatusToString(OCC::UserStatus::OnlineStatus status)
{
    switch (status) {
    case OCC::UserStatus::OnlineStatus::Online:
        return QStringLiteral("online");
    case OCC::UserStatus::OnlineStatus::DoNotDisturb:
        return QStringLiteral("dnd");
    case OCC::UserStatus::OnlineStatus::Away:
        return QStringLiteral("offline");
    case OCC::UserStatus::OnlineStatus::Offline:
        return QStringLiteral("offline");
    case OCC::UserStatus::OnlineStatus::Invisible:
        return QStringLiteral("invisible");
    }
    Q_UNREACHABLE();
}

const QString baseUrl("/ocs/v2.php/apps/user_status/api/v1");
const QString userStatusBaseUrl = baseUrl + QStringLiteral("/user_status");
}

namespace OCC {

Q_LOGGING_CATEGORY(lcJsonUserStatusJob, "nextcloud.gui.jsonuserstatusjob", QtInfoMsg)

JsonUserStatusJob::JsonUserStatusJob(AccountPtr account, QObject *parent)
    : UserStatusJob(parent)
    , _account(account)
{
    Q_ASSERT(_account);
    _userStatusSupported = _account->capabilities().userStatus();
    _userStatusEmojisSupported = _account->capabilities().userStatusSupportsEmoji();
}

void JsonUserStatusJob::fetchUserStatus()
{
    qCDebug(lcJsonUserStatusJob) << "Try to fetch user status";

    if (!_userStatusSupported) {
        qCDebug(lcJsonUserStatusJob) << "User status not supported";
        emit error(Error::UserStatusNotSupported);
        return;
    }

    deleteGetUserStatusJob();
    _getUserStatusJob = new JsonApiJob(_account, userStatusBaseUrl, this);
    connect(_getUserStatusJob, &JsonApiJob::jsonReceived, this, &JsonUserStatusJob::onUserStatusFetched);
    _getUserStatusJob->start();
}

static UserStatus jsonToUserStatus(const QJsonDocument &json)
{
    const QJsonObject defaultValues {
        { "icon", "" },
        { "message", "" },
        { "status", "online" },
        { "messageIsPredefined", "false" },
        { "statusIsUserDefined", "false" }
    };

    const auto retrievedData = json.object().value("ocs").toObject().value("data").toObject(defaultValues);

    Optional<ClearAt> clearAt {};
    if (retrievedData.contains("clearAt") && !retrievedData.value("clearAt").isNull()) {
        ClearAt clearAtValue;
        clearAtValue._type = ClearAtType::Timestamp;
        clearAtValue._timestamp = retrievedData.value("clearAt").toInt();
        clearAt = clearAtValue;
    }

    const UserStatus userStatus(retrievedData.value("messageId").toString(),
        retrievedData.value("message").toString().trimmed(),
        retrievedData.value("icon").toString().trimmed(), stringToUserOnlineStatus(retrievedData.value("status").toString()),
        retrievedData.value("messageIsPredefined").toBool(false), clearAt);

    return userStatus;
}

void JsonUserStatusJob::onUserStatusFetched(const QJsonDocument &json, int statusCode)
{
    deleteGetUserStatusJob();
    logResponse("user status fetched", json, statusCode);

    if (statusCode != 200) {
        qCInfo(lcJsonUserStatusJob) << "Slot fetch UserStatus finished with status code" << statusCode;
        emit error(Error::CouldNotFetchUserStatus);
        return;
    }

    _userStatus = jsonToUserStatus(json);
    emit userStatusFetched(_userStatus);
}

void JsonUserStatusJob::fetchPredefinedStatuses()
{
    if (!_userStatusSupported) {
        emit error(Error::UserStatusNotSupported);
        return;
    }

    deleteGetPredefinedStatusesJob();

    _getPredefinedStausesJob = new JsonApiJob(_account,
        baseUrl + QStringLiteral("/predefined_statuses"), this);
    connect(_getPredefinedStausesJob, &JsonApiJob::jsonReceived, this, &JsonUserStatusJob::onPredefinedStatusesFetched);
    _getPredefinedStausesJob->start();
}

void JsonUserStatusJob::onPredefinedStatusesFetched(const QJsonDocument &json, int statusCode)
{
    logResponse("predefined statuses", json, statusCode);

    if (statusCode != 200) {
        qCInfo(lcJsonUserStatusJob) << "Slot predefined user statuses finished with status code" << statusCode;
        emit error(Error::CouldNotFetchPredefinedUserStatuses);
        return;
    }

    const auto jsonData = json.object().value("ocs").toObject().value("data");
    Q_ASSERT(jsonData.isArray());
    if (!jsonData.isArray()) {
        return;
    }

    std::vector<UserStatus> statuses;
    const auto jsonDataArray = jsonData.toArray();
    for (const auto &jsonEntry : jsonDataArray) {
        Q_ASSERT(jsonEntry.isObject());
        if (!jsonEntry.isObject()) {
            continue;
        }

        const auto jsonEntryObject = jsonEntry.toObject();

        Optional<ClearAt> clearAt;
        {
            if (jsonEntryObject.value("clearAt").isObject() && !jsonEntryObject.value("clearAt").isNull()) {
                ClearAt clearAtValue;
                const auto clearAtObject = jsonEntryObject.value("clearAt").toObject();
                const auto typeValue = clearAtObject.value("type").toString("period");
                if (typeValue == "period") {
                    const auto timeValue = clearAtObject.value("time").toInt(0);
                    clearAtValue._type = ClearAtType::Period;
                    clearAtValue._period = timeValue;
                } else if (typeValue == "end-of") {
                    const auto timeValue = clearAtObject.value("time").toString("day");
                    clearAtValue._type = ClearAtType::EndOf;
                    clearAtValue._endof = timeValue;
                } else {
                    qCWarning(lcJsonUserStatusJob) << "Can not handle clear type value" << typeValue;
                }
                clearAt = clearAtValue;
            }
        }

        statuses.emplace_back(
            jsonEntryObject.value("id").toString("no-id"),
            jsonEntryObject.value("message").toString("No message"),
            jsonEntryObject.value("icon").toString("no-icon"),
            OCC::UserStatus::OnlineStatus::Online,
            true,
            clearAt);
    }

    deleteGetPredefinedStatusesJob();
    emit predefinedStatusesFetched(statuses);
}

void JsonUserStatusJob::logResponse(const QString &message, const QJsonDocument &json, int statusCode)
{
    qCDebug(lcJsonUserStatusJob) << "Response from:" << message << "Status:" << statusCode << "Json:" << json;
}

static quint64 clearAtToTimestamp(const ClearAt &clearAt)
{
    switch (clearAt._type) {
    case ClearAtType::Period: {
        return QDateTime::currentDateTime().addSecs(clearAt._period).toTime_t();
    }

    case ClearAtType::EndOf: {
        if (clearAt._endof == "day") {
            return QDate::currentDate().addDays(1).startOfDay().toTime_t();
        } else if (clearAt._endof == "week") {
            const auto days = Qt::Sunday - QDate::currentDate().dayOfWeek();
            return QDate::currentDate().addDays(days + 1).startOfDay().toTime_t();
        } else {
            qCWarning(lcJsonUserStatusJob) << "Can not handle clear at endof day type" << clearAt._endof;
        }
    }

    case ClearAtType::Timestamp: {
        return clearAt._timestamp;
    }
    }

    return 0;
}

static quint64 clearAtToTimestamp(const OCC::Optional<ClearAt> &clearAt)
{
    if (clearAt) {
        return clearAtToTimestamp(*clearAt);
    }
    return 0;
}

void JsonUserStatusJob::setUserStatus(const UserStatus &userStatus)
{
    if (!_userStatusSupported) {
        emit error(Error::UserStatusNotSupported);
        return;
    }

    // Set the state
    {
        deleteSetOnlineStatusJob();
        _setOnlineStatusJob = new JsonApiJob(_account,
            userStatusBaseUrl + QStringLiteral("/status"), this);
        _setOnlineStatusJob->usePUT();
        // Set body
        QJsonObject dataObject;
        dataObject.insert("statusType", onlineStatusToString(userStatus.state()));
        QJsonDocument body;
        body.setObject(dataObject);
        _setOnlineStatusJob->setBody(body);
        connect(_setOnlineStatusJob, &JsonApiJob::jsonReceived, this, &JsonUserStatusJob::onUserStatusStateSet);
        _setOnlineStatusJob->start();
    }

    deleteSetMessageJob();

    if (userStatus.messagePredefined()) {
        _setMessageJob = new JsonApiJob(_account, userStatusBaseUrl + QStringLiteral("/message/predefined"), this);
        _setMessageJob->usePUT();
        // Set body
        QJsonObject dataObject;
        dataObject.insert("messageId", userStatus.id());
        if (userStatus.clearAt()) {
            dataObject.insert("clearAt", static_cast<int>(clearAtToTimestamp(userStatus.clearAt())));
        } else {
            dataObject.insert("clearAt", QJsonValue());
        }
        QJsonDocument body;
        body.setObject(dataObject);
        _setMessageJob->setBody(body);
        connect(_setMessageJob, &JsonApiJob::jsonReceived, this, &JsonUserStatusJob::onUserStatusMessageSet);
        _setMessageJob->start();
    } else {
        if (!_userStatusEmojisSupported) {
            emit error(Error::EmojisNotSupported);
            return;
        }
        _setMessageJob = new JsonApiJob(_account, userStatusBaseUrl + QStringLiteral("/message/custom"), this);
        _setMessageJob->usePUT();
        // Set body
        QJsonObject dataObject;
        dataObject.insert("statusIcon", userStatus.icon());
        dataObject.insert("message", userStatus.message());
        const auto clearAt = userStatus.clearAt();
        if (clearAt) {
            dataObject.insert("clearAt", static_cast<int>(clearAtToTimestamp(*clearAt)));
        } else {
            dataObject.insert("clearAt", QJsonValue());
        }
        QJsonDocument body;
        body.setObject(dataObject);
        _setMessageJob->setBody(body);
        connect(_setMessageJob, &JsonApiJob::jsonReceived, this, &JsonUserStatusJob::onUserStatusMessageSet);
        _setMessageJob->start();
    }
}

void JsonUserStatusJob::onUserStatusStateSet(const QJsonDocument &json, int statusCode)
{
    deleteSetOnlineStatusJob();
    logResponse("Online status set", json, statusCode);

    if (statusCode != 200) {
        emit error(Error::CouldNotSetUserStatus);
        return;
    }
}

void JsonUserStatusJob::onUserStatusMessageSet(const QJsonDocument &json, int statusCode)
{
    deleteSetMessageJob();
    logResponse("Message set", json, statusCode);

    if (statusCode != 200) {
        emit error(Error::CouldNotSetUserStatus);
        return;
    }

    // We fetch the user status again because json does not contain
    // the new message when user status was set from a predefined
    // message
    fetchUserStatus();

    emit userStatusSet();
}

void JsonUserStatusJob::clearMessage()
{
    deleteClearMesssageJob();

    _clearMessageJob = new JsonApiJob(_account, userStatusBaseUrl + QStringLiteral("/message"));
    _clearMessageJob->useDELETE();
    connect(_clearMessageJob, &JsonApiJob::jsonReceived, this, &JsonUserStatusJob::onMessageCleared);
    _clearMessageJob->start();
}

UserStatus JsonUserStatusJob::userStatus() const
{
    return _userStatus;
}

void JsonUserStatusJob::onMessageCleared(const QJsonDocument &json, int statusCode)
{
    deleteClearMesssageJob();
    logResponse("Message cleared", json, statusCode);

    if (statusCode != 200) {
        emit error(Error::CouldNotClearMessage);
        return;
    }

    _userStatus = {};
    emit messageCleared();
}

void JsonUserStatusJob::deleteClearMesssageJob()
{
    if (_clearMessageJob) {
        _clearMessageJob->deleteLater();
    }
}

void JsonUserStatusJob::deleteSetMessageJob()
{
    if (_setMessageJob) {
        _setMessageJob->deleteLater();
    }
}

void JsonUserStatusJob::deleteSetOnlineStatusJob()
{
    if (_setOnlineStatusJob) {
        _setOnlineStatusJob->deleteLater();
    }
}

void JsonUserStatusJob::deleteGetPredefinedStatusesJob()
{
    if (_getPredefinedStausesJob) {
        _getPredefinedStausesJob->deleteLater();
    }
}

void JsonUserStatusJob::deleteGetUserStatusJob()
{
    if (_getUserStatusJob) {
        _getUserStatusJob->deleteLater();
    }
}
}
