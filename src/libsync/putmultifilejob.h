/*
 * Copyright 2021 (c) Matthieu Gallien <matthieu.gallien@nextcloud.com>
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

#ifndef PUTMULTIFILEJOB_H
#define PUTMULTIFILEJOB_H

#include "abstractnetworkjob.h"

#include "account.h"

#include <QLoggingCategory>
#include <QMap>
#include <QByteArray>
#include <QUrl>
#include <QString>
#include <QElapsedTimer>
#include <QHttpMultiPart>
#include <memory>

class QIODevice;

namespace OCC {

Q_DECLARE_LOGGING_CATEGORY(lcPutMultiFileJob)

/**
 * @brief The PutMultiFileJob class
 * @ingroup libsync
 */
class OWNCLOUDSYNC_EXPORT PutMultiFileJob : public AbstractNetworkJob
{
    Q_OBJECT

public:
    // Takes ownership of the device
    explicit PutMultiFileJob(AccountPtr account, const QString &path, std::unique_ptr<QIODevice> device,
        const QMap<QByteArray, QByteArray> &headers, int chunk, QObject *parent = nullptr)
        : AbstractNetworkJob(account, path, parent)
        , _body(QHttpMultiPart::MixedType)
        , _device(device.release())
        , _headers(headers)
        , _chunk(chunk)
    {
        _device->setParent(this);
    }
    explicit PutMultiFileJob(AccountPtr account, const QUrl &url, std::unique_ptr<QIODevice> device,
        const QMap<QByteArray, QByteArray> &headers, int chunk, QObject *parent = nullptr)
        : AbstractNetworkJob(account, QString(), parent)
        , _body(QHttpMultiPart::MixedType)
        , _device(device.release())
        , _headers(headers)
        , _url(url)
        , _chunk(chunk)
    {
        _device->setParent(this);
    }
    ~PutMultiFileJob() override;

    void start() override;

    bool finished() override;

    QIODevice *device()
    {
        return _device;
    }

    QString errorString() const override
    {
        return _errorString.isEmpty() ? AbstractNetworkJob::errorString() : _errorString;
    }

    std::chrono::milliseconds msSinceStart() const
    {
        return std::chrono::milliseconds(_requestTimer.elapsed());
    }

signals:
    void finishedSignal();
    void uploadProgress(qint64, qint64);

private:
    QHttpMultiPart _body;
    QIODevice *_device;
    QMap<QByteArray, QByteArray> _headers;
    QString _errorString;
    QUrl _url;
    QElapsedTimer _requestTimer;
    int _chunk;

};

}

#endif // PUTMULTIFILEJOB_H
