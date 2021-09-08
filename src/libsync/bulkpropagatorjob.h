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

#include <QLoggingCategory>
#include <QVector>

namespace OCC {

Q_DECLARE_LOGGING_CATEGORY(lcBulkPropagatorJob)

class BulkPropagatorJob : public PropagatorCompositeJob
{
    Q_OBJECT
public:

    explicit BulkPropagatorJob(OwncloudPropagator *propagator,
                               const QVector<SyncFileItemPtr> &items);

    bool scheduleSelfOrChild() override;

    JobParallelism parallelism() override;

private:

    QVector<SyncFileItemPtr> _items;
};

}

#endif // BULKPROPAGATORJOB_H
