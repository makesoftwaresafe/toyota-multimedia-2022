/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#ifndef QWEBLOCALSTORAGEDB_H
#define QWEBLOCALSTORAGEDB_H

#include "qwebkitglobal.h"

class QWEBKIT_EXPORT QWebLocalStorageDB {
public:
    QWebLocalStorageDB();
    ~QWebLocalStorageDB();
    void acquire();
    void release();
};

#endif /* QWEBLOCALSTORAGEDB_H */

