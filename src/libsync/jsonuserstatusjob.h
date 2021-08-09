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

#pragma once

#include "accountfwd.h"
#include "userstatusjob.h"

#include <QPointer>

namespace OCC {

class JsonApiJob;
class SimpleNetworkJob;

class OWNCLOUDSYNC_EXPORT JsonUserStatusJob : public UserStatusJob
{
public:
    explicit JsonUserStatusJob(AccountPtr account, QObject *parent = nullptr);

    void fetchUserStatus() override;

    void fetchPredefinedStatuses() override;

    void setUserStatus(const UserStatus &userStatus) override;

    void clearMessage() override;

    UserStatus userStatus() const override;

private:
    void onUserStatusFetched(const QJsonDocument &json, int statusCode);
    void onPredefinedStatusesFetched(const QJsonDocument &json, int statusCode);
    void onUserStatusStateSet(const QJsonDocument &json, int statusCode);
    void onUserStatusMessageSet(const QJsonDocument &json, int statusCode);
    void onMessageCleared(const QJsonDocument &json, int statusCode);

    void logResponse(const QString &message, const QJsonDocument &json, int statusCode);

    void deleteClearMesssageJob();
    void deleteSetMessageJob();
    void deleteSetOnlineStatusJob();
    void deleteGetPredefinedStatusesJob();
    void deleteGetUserStatusJob();

    AccountPtr _account;

    bool _userStatusSupported = false;
    bool _userStatusEmojisSupported = false;

    QPointer<JsonApiJob> _clearMessageJob {};
    QPointer<JsonApiJob> _setMessageJob {};
    QPointer<JsonApiJob> _setOnlineStatusJob {};
    QPointer<JsonApiJob> _getPredefinedStausesJob {};
    QPointer<JsonApiJob> _getUserStatusJob {};

    UserStatus _userStatus;
};
}
