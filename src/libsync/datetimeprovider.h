#pragma once

#include "owncloudlib.h"

#include <qdatetime.h>

namespace OCC {

class OWNCLOUDSYNC_EXPORT DateTimeProvider
{
public:
    virtual ~DateTimeProvider();

    virtual QDateTime currentDateTime() const;

    virtual QDate currentDate() const;
};
}
