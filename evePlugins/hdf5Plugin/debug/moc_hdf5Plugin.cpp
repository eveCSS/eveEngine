/****************************************************************************
** Meta object code from reading C++ file 'hdf5Plugin.h'
**
** Created: Wed Apr 28 15:12:15 2010
**      by: The Qt Meta Object Compiler version 59 (Qt 4.4.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../hdf5Plugin.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'hdf5Plugin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.4.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_hdf5Plugin[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets

       0        // eod
};

static const char qt_meta_stringdata_hdf5Plugin[] = {
    "hdf5Plugin\0"
};

const QMetaObject hdf5Plugin::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_hdf5Plugin,
      qt_meta_data_hdf5Plugin, 0 }
};

const QMetaObject *hdf5Plugin::metaObject() const
{
    return &staticMetaObject;
}

void *hdf5Plugin::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_hdf5Plugin))
        return static_cast<void*>(const_cast< hdf5Plugin*>(this));
    if (!strcmp(_clname, "eveFileWriter"))
        return static_cast< eveFileWriter*>(const_cast< hdf5Plugin*>(this));
    if (!strcmp(_clname, "de.ptb.epics.eve.FileWriterInterface/1.0"))
        return static_cast< eveFileWriter*>(const_cast< hdf5Plugin*>(this));
    return QObject::qt_metacast(_clname);
}

int hdf5Plugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
