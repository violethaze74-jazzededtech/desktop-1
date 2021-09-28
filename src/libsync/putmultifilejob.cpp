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

#include "putmultifilejob.h"

#include <QHttpPart>

namespace OCC {

Q_LOGGING_CATEGORY(lcPutMultiFileJob, "nextcloud.sync.networkjob.put.multi", QtInfoMsg)

PutMultiFileJob::~PutMultiFileJob() = default;

void PutMultiFileJob::start()
{
    qCInfo(lcPutMultiFileJob) << "PUT of" << path() << _headers;
    QNetworkRequest req;
    auto onePart = QHttpPart{};
    onePart.setBodyDevice(_device);

    for (QMap<QByteArray, QByteArray>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        onePart.setRawHeader(it.key(), it.value());
        if (it.key() == "OC-Total-Length" ||
                it.key() == "X-OC-Mtime" ||
                it.key() == "OC-Checksum" ||
                it.key() == "If-Match" ||
                it.key() == "OC-Chunk-Size" ||
                it.key() == "OC-Conflict" ||
                it.key() == "OC-ConflictBaseFileId" ||
                it.key() == "OC-ConflictInitialBasePath") {
            req.setRawHeader(it.key(), it.value());
        } else {
            qDebug() << it.key();
        }
    }
    onePart.setRawHeader("OC-Path", path().toUtf8().mid(1));

    req.setPriority(QNetworkRequest::LowPriority); // Long uploads must not block non-propagation jobs.

    _body.append(onePart);

    sendRequest("PUT", makeDavUrl({}), req, &_body);

    if (reply()->error() != QNetworkReply::NoError) {
        qCWarning(lcPutMultiFileJob) << " Network error: " << reply()->errorString();
    }

    connect(reply(), &QNetworkReply::uploadProgress, this, &PutMultiFileJob::uploadProgress);
    connect(this, &AbstractNetworkJob::networkActivity, account().data(), &Account::propagatorNetworkActivity);
    _requestTimer.start();
    AbstractNetworkJob::start();
}

bool PutMultiFileJob::finished()
{
    _device->close();

    qCInfo(lcPutMultiFileJob) << "PUT of" << reply()->request().url().toString() << path() << "FINISHED WITH STATUS"
                     << replyStatusString()
                     << reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                     << reply()->attribute(QNetworkRequest::HttpReasonPhraseAttribute);

    emit finishedSignal();
    return true;
}

}
