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

namespace OCC {

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

}
