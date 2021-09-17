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

#ifndef BULKPROPAGATORJOB_H
#define BULKPROPAGATORJOB_H

#include "owncloudpropagator.h"

#include "abstractnetworkjob.h"

#include <QLoggingCategory>
#include <QVector>
#include <QMap>
#include <QByteArray>

namespace OCC {

Q_DECLARE_LOGGING_CATEGORY(lcBulkPropagatorJob)

class BulkPropagatorJob : public PropagatorCompositeJob
{
    Q_OBJECT

    /* This is a minified version of the SyncFileItem,
     * that holds only the specifics about the file that's
     * being uploaded.
     *
     * This is needed if we wanna apply changes on the file
     * that's being uploaded while keeping the original on disk.
     */
    struct UploadFileInfo {
      QString _file; /// I'm still unsure if I should use a SyncFilePtr here.
      QString _path; /// the full path on disk.
      qint64 _size;
    };

public:

    explicit BulkPropagatorJob(OwncloudPropagator *propagator,
                               const QVector<SyncFileItemPtr> &items);

    bool scheduleSelfOrChild() override;

    JobParallelism parallelism() override;

private slots:

    void slotComputeContentChecksum(SyncFileItemPtr item,
                                    UploadFileInfo fileToUpload);

    // Content checksum computed, compute the transmission checksum
    void slotComputeTransmissionChecksum(SyncFileItemPtr item,
                                         UploadFileInfo fileToUpload,
                                         const QByteArray &contentChecksumType,
                                         const QByteArray &contentChecksum);

    // transmission checksum computed, prepare the upload
    void slotStartUpload(SyncFileItemPtr item,
                         UploadFileInfo fileToUpload,
                         const QByteArray &transmissionChecksumType,
                         const QByteArray &transmissionChecksum);

    // invoked on internal error to unlock a folder and faile
    void slotOnErrorStartFolderUnlock(SyncFileItem::Status status, const QString &errorString);

    void slotPutFinished();

    void slotUploadProgress(qint64, qint64);

    void slotJobDestroyed(QObject *job);

private:

    void startUploadFile(SyncFileItemPtr item, UploadFileInfo fileToUpload);

    void doStartUpload(UploadFileInfo fileToUpload);

    void adjustLastJobTimeout(AbstractNetworkJob *job, qint64 fileSize);

    /** Bases headers that need to be sent on the PUT, or in the MOVE for chunking-ng */
    QMap<QByteArray, QByteArray> headers();

    QVector<SyncFileItemPtr> _items;

    QVector<AbstractNetworkJob *> _jobs; /// network jobs that are currently in transit
};

}

#endif // BULKPROPAGATORJOB_H
