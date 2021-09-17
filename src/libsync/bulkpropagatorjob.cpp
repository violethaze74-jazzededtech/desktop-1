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

#include "bulkpropagatorjob.h"

#include "syncfileitem.h"
#include "syncengine.h"
#include "propagateupload.h"
#include "propagatorjobs.h"
#include "filesystem.h"
#include "account.h"
#include "common/utility.h"
#include "common/checksums.h"

#include <QFileInfo>
#include <QDir>

namespace {

/**
 * We do not want to upload files that are currently being modified.
 * To avoid that, we don't upload files that have a modification time
 * that is too close to the current time.
 *
 * This interacts with the msBetweenRequestAndSync delay in the folder
 * manager. If that delay between file-change notification and sync
 * has passed, we should accept the file for upload here.
 */
bool fileIsStillChanging(const OCC::SyncFileItem &item)
{
    const QDateTime modtime = OCC::Utility::qDateTimeFromTime_t(item._modtime);
    const qint64 msSinceMod = modtime.msecsTo(QDateTime::currentDateTimeUtc());

    return std::chrono::milliseconds(msSinceMod) < OCC::SyncEngine::minimumFileAgeForUpload
        // if the mtime is too much in the future we *do* upload the file
        && msSinceMod > -10000;
}

}

namespace OCC {

Q_LOGGING_CATEGORY(lcBulkPropagatorJob, "nextcloud.sync.propagator.bulkupload", QtInfoMsg)

BulkPropagatorJob::BulkPropagatorJob(OwncloudPropagator *propagator,
                                     const QVector<SyncFileItemPtr> &items)
    : PropagatorCompositeJob(propagator)
    , _items(std::move(items))
{
}

bool BulkPropagatorJob::scheduleSelfOrChild()
{
    if (_items.empty()) {
        return PropagatorCompositeJob::scheduleSelfOrChild();
    }

    for(auto &oneItemJob : _items) {
        appendTask(oneItemJob);
    }
    _items.clear();
    return PropagatorCompositeJob::scheduleSelfOrChild();
}

PropagatorJob::JobParallelism BulkPropagatorJob::parallelism()
{
    return PropagatorJob::JobParallelism::WaitForFinished;
}

void BulkPropagatorJob::startUploadFile(SyncFileItemPtr item, UploadFileInfo fileToUpload)
{
    if (propagator()->_abortRequested) {
        return;
    }

    // Check if the specific file can be accessed
    if (propagator()->hasCaseClashAccessibilityProblem(fileToUpload._file)) {
        //done(SyncFileItem::NormalError, tr("File %1 cannot be uploaded because another file with the same name, differing only in case, exists").arg(QDir::toNativeSeparators(item->_file)));
        return;
    }

    // Check if we believe that the upload will fail due to remote quota limits
    const qint64 quotaGuess = propagator()->_folderQuota.value(
                QFileInfo(fileToUpload._file).path(), std::numeric_limits<qint64>::max());
    if (fileToUpload._size > quotaGuess) {
        // Necessary for blacklisting logic
        item->_httpErrorCode = 507;
        emit propagator()->insufficientRemoteStorage();
        //done(SyncFileItem::DetailError, tr("Upload of %1 exceeds the quota for the folder").arg(Utility::octetsToString(fileToUpload._size)));
        return;
    }

    //propagator()->_activeJobList.append(this);

    qCDebug(lcBulkPropagatorJob) << "Running the compute checksum";
    return slotComputeContentChecksum(item, fileToUpload);
}

void BulkPropagatorJob::doStartUpload(UploadFileInfo fileToUpload)
{
    if (propagator()->_abortRequested)
        return;

    QByteArray transmissionChecksumHeader;
    qint64 fileSize = fileToUpload._size;
    auto currentHeaders = headers();
    currentHeaders[QByteArrayLiteral("OC-Total-Length")] = QByteArray::number(fileSize);
    currentHeaders[QByteArrayLiteral("OC-Chunk-Size")] = QByteArray::number(fileSize);

    QString path = fileToUpload._file;

    qCInfo(lcBulkPropagatorJob) << propagator()->fullRemotePath(path) << transmissionChecksumHeader;
    currentHeaders[checkSumHeaderC] = transmissionChecksumHeader;

    const QString fileName = fileToUpload._path;
    auto device = std::make_unique<UploadDevice>(
            fileName, 0, fileSize, &propagator()->_bandwidthManager);
    if (!device->open(QIODevice::ReadOnly)) {
        qCWarning(lcBulkPropagatorJob) << "Could not prepare upload device: " << device->errorString();

        // If the file is currently locked, we want to retry the sync
        // when it becomes available again.
        if (FileSystem::isFileLocked(fileName)) {
            emit propagator()->seenLockedFile(fileName);
        }
        // Soft error because this is likely caused by the user modifying his files while syncing
        //abortWithError(SyncFileItem::SoftError, device->errorString());
        return;
    }

    // job takes ownership of device via a QScopedPointer. Job deletes itself when finishing
    auto devicePtr = device.get(); // for connections later
    auto *job = new PUTFileJob(propagator()->account(), propagator()->fullRemotePath(path), std::move(device), currentHeaders, 0, this);
    _jobs.append(job);
    connect(job, &PUTFileJob::finishedSignal, this, &BulkPropagatorJob::slotPutFinished);
    connect(job, &PUTFileJob::uploadProgress, this, &BulkPropagatorJob::slotUploadProgress);
    connect(job, &PUTFileJob::uploadProgress, devicePtr, &UploadDevice::slotJobUploadProgress);
    connect(job, &QObject::destroyed, this, &BulkPropagatorJob::slotJobDestroyed);
    adjustLastJobTimeout(job, fileSize);
    job->start();
    //propagator()->_activeJobList.append(this);

    propagator()->scheduleNextJob();
}

void BulkPropagatorJob::slotComputeContentChecksum(SyncFileItemPtr item,
                                                   UploadFileInfo fileToUpload)
{
    qDebug() << "Trying to compute the checksum of the file";
    qDebug() << "Still trying to understand if this is the local file or the uploaded one";
    if (propagator()->_abortRequested) {
        return;
    }

    const QString filePath = propagator()->fullLocalPath(item->_file);

    // remember the modtime before checksumming to be able to detect a file
    // change during the checksum calculation - This goes inside of the item->_file
    // and not the fileToUpload because we are checking the original file, not there
    // probably temporary one.
    item->_modtime = FileSystem::getModTime(filePath);

    const QByteArray checksumType = propagator()->account()->capabilities().preferredUploadChecksumType();

    // Maybe the discovery already computed the checksum?
    // Should I compute the checksum of the original (item->_file)
    // or the maybe-modified? (fileToUpload._file) ?

    QByteArray existingChecksumType, existingChecksum;
    parseChecksumHeader(item->_checksumHeader, &existingChecksumType, &existingChecksum);
    if (existingChecksumType == checksumType) {
        slotComputeTransmissionChecksum(item, fileToUpload, checksumType, existingChecksum);
        return;
    }

    // Compute the content checksum.
    auto computeChecksum = new ComputeChecksum(this);
    computeChecksum->setChecksumType(checksumType);

//    connect(computeChecksum, &ComputeChecksum::done,
//        this, &BulkPropagatorJob::slotComputeTransmissionChecksum);
    connect(computeChecksum, &ComputeChecksum::done,
        computeChecksum, &QObject::deleteLater);
    computeChecksum->start(fileToUpload._path);
}

void BulkPropagatorJob::slotComputeTransmissionChecksum(SyncFileItemPtr item,
                                                        UploadFileInfo fileToUpload,
                                                        const QByteArray &contentChecksumType,
                                                        const QByteArray &contentChecksum)
{
    item->_checksumHeader = makeChecksumHeader(contentChecksumType, contentChecksum);

    // Reuse the content checksum as the transmission checksum if possible
    const auto supportedTransmissionChecksums =
        propagator()->account()->capabilities().supportedChecksumTypes();
    if (supportedTransmissionChecksums.contains(contentChecksumType)) {
        slotStartUpload(item, fileToUpload, contentChecksumType, contentChecksum);
        return;
    }

    // Compute the transmission checksum.
    auto computeChecksum = new ComputeChecksum(this);
    if (uploadChecksumEnabled()) {
        computeChecksum->setChecksumType(propagator()->account()->capabilities().uploadChecksumType());
    } else {
        computeChecksum->setChecksumType(QByteArray());
    }

//    connect(computeChecksum, &ComputeChecksum::done,
//        this, &BulkPropagatorJob::slotStartUpload);
    connect(computeChecksum, &ComputeChecksum::done,
        computeChecksum, &QObject::deleteLater);
    computeChecksum->start(fileToUpload._path);
}

void BulkPropagatorJob::slotStartUpload(SyncFileItemPtr item,
                                        UploadFileInfo fileToUpload,
                                        const QByteArray &transmissionChecksumType,
                                        const QByteArray &transmissionChecksum)
{
    QByteArray _transmissionChecksumHeader;

    // Remove ourselfs from the list of active job, before any posible call to done()
    // When we start chunks, we will add it again, once for every chunks.
    //propagator()->_activeJobList.removeOne(this);

    _transmissionChecksumHeader = makeChecksumHeader(transmissionChecksumType, transmissionChecksum);

    // If no checksum header was not set, reuse the transmission checksum as the content checksum.
    if (item->_checksumHeader.isEmpty()) {
        item->_checksumHeader = _transmissionChecksumHeader;
    }

    const QString fullFilePath = fileToUpload._path;
    const QString originalFilePath = propagator()->fullLocalPath(item->_file);

    if (!FileSystem::fileExists(fullFilePath)) {
        return slotOnErrorStartFolderUnlock(SyncFileItem::SoftError, tr("File Removed (start upload) %1").arg(fullFilePath));
    }
    time_t prevModtime = item->_modtime; // the _item value was set in PropagateUploadFile::start()
    // but a potential checksum calculation could have taken some time during which the file could
    // have been changed again, so better check again here.

    item->_modtime = FileSystem::getModTime(originalFilePath);
    if (prevModtime != item->_modtime) {
        propagator()->_anotherSyncNeeded = true;
        qDebug() << "prevModtime" << prevModtime << "Curr" << item->_modtime;
        return slotOnErrorStartFolderUnlock(SyncFileItem::SoftError, tr("Local file changed during syncing. It will be resumed."));
    }

    fileToUpload._size = FileSystem::getSize(fullFilePath);
    item->_size = FileSystem::getSize(originalFilePath);

    // But skip the file if the mtime is too close to 'now'!
    // That usually indicates a file that is still being changed
    // or not yet fully copied to the destination.
    if (fileIsStillChanging(*item)) {
        propagator()->_anotherSyncNeeded = true;
        return slotOnErrorStartFolderUnlock(SyncFileItem::SoftError, tr("Local file changed during sync."));
    }

    doStartUpload(fileToUpload);
}

void BulkPropagatorJob::slotOnErrorStartFolderUnlock(SyncFileItem::Status status, const QString &errorString)
{
    //done(status, errorString);
}

void BulkPropagatorJob::slotPutFinished()
{
}

void BulkPropagatorJob::slotUploadProgress(qint64, qint64)
{
}

void BulkPropagatorJob::slotJobDestroyed(QObject *job)
{
}

void BulkPropagatorJob::adjustLastJobTimeout(AbstractNetworkJob *job, qint64 fileSize)
{
    constexpr double threeMinutes = 3.0 * 60 * 1000;

    job->setTimeout(qBound(
        job->timeoutMsec(),
        // Calculate 3 minutes for each gigabyte of data
        qRound64(threeMinutes * fileSize / 1e9),
        // Maximum of 30 minutes
                        static_cast<qint64>(30 * 60 * 1000)));
}

QMap<QByteArray, QByteArray> BulkPropagatorJob::headers()
{
    return {};
}

}
