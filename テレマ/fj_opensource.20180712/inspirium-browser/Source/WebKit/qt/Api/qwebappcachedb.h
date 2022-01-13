/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#ifndef QWEBAPPCACHEDB_H
#define QWEBAPPCACHEDB_H

#include "qwebkitglobal.h"

class QWEBKIT_EXPORT QWebAppCacheDB {
public:
    QWebAppCacheDB();
    ~QWebAppCacheDB();
    void acquire();
    void release();
};

#endif /* QWEBAPPCACHEDB_H */

