/*** COPYRIGHT FUJITSU LIMITED 2017 ***/

#ifndef QWEBICONDB_H
#define QWEBICONDB_H

#include "qwebkitglobal.h"

class QWEBKIT_EXPORT QWebIconDB {
public:
    QWebIconDB();
    ~QWebIconDB();
    void acquire();
    void release();
};

#endif /* QWEBICONDB_H */

