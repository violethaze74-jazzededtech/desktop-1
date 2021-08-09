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

#include "setuserstatusdialog.h"
#include "accountfwd.h"
#include "ocsuserstatusconnector.h"
#include "setuserstatusdialogmodel.h"
#include "emojimodel.h"
#include "account.h"

#include <QQuickView>
#include <QLoggingCategory>
#include <QQmlError>
#include <QQmlEngine>
#include <QQuickView>
#include <QQuickItem>

Q_LOGGING_CATEGORY(lcUserStatusDialog, "nextcloud.gui.systray")

namespace OCC {
namespace SetUserStatusDialog {

    static bool logErrors(const QList<QQmlError> &errors)
    {
        bool isError = false;

        for (const auto &error : errors) {
            isError = true;
            qCWarning(lcUserStatusDialog) << error.toString();
        }

        return isError;
    }

    class SetUserStatusDialogView : public QQuickView
    {
    public:
        SetUserStatusDialogView(std::shared_ptr<UserStatusConnector> userStatusConnector)
        {
            _model = new SetUserStatusDialogModel(userStatusConnector);

            setFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
            QObject::connect(_model, &SetUserStatusDialogModel::finished, this, [this]() {
                close();
            });
            engine()->setObjectOwnership(this, QQmlEngine::CppOwnership);
            engine()->setObjectOwnership(_model, QQmlEngine::CppOwnership);
            setInitialProperties({ { "setUserStatusDialogModel", QVariant::fromValue(_model) } });
            logErrors(errors());
            setSource(QUrl("qrc:/qml/src/gui/SetUserStatusView.qml"));
            logErrors(errors());
        }

        ~SetUserStatusDialogView()
        {
            _model->deleteLater();
        }

        bool event(QEvent *event) override
        {
            if (event->type() == QEvent::Close) {
                deleteLater();
            }

            return QQuickView::event(event);
        }

    private:
        SetUserStatusDialogModel *_model;
    };

    void show(AccountPtr account)
    {
        auto view = new SetUserStatusDialogView(account->userStatusJob());
        view->show();
    }
}
}
