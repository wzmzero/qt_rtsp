/****************************************************************************
** Meta object code from reading C++ file 'database_service.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/client/database/database_service.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'database_service.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_demo__client__SQLiteDatabaseService_t {
    uint offsetsAndSizes[40];
    char stringdata0[36];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[24];
    char stringdata4[4];
    char stringdata5[21];
    char stringdata6[30];
    char stringdata7[4];
    char stringdata8[25];
    char stringdata9[15];
    char stringdata10[10];
    char stringdata11[14];
    char stringdata12[18];
    char stringdata13[5];
    char stringdata14[6];
    char stringdata15[5];
    char stringdata16[8];
    char stringdata17[25];
    char stringdata18[34];
    char stringdata19[4];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_demo__client__SQLiteDatabaseService_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_demo__client__SQLiteDatabaseService_t qt_meta_stringdata_demo__client__SQLiteDatabaseService = {
    {
        QT_MOC_LITERAL(0, 35),  // "demo::client::SQLiteDatabaseS..."
        QT_MOC_LITERAL(36, 15),  // "saveConfigAsync"
        QT_MOC_LITERAL(52, 0),  // ""
        QT_MOC_LITERAL(53, 23),  // "demo::client::AppConfig"
        QT_MOC_LITERAL(77, 3),  // "cfg"
        QT_MOC_LITERAL(81, 20),  // "insertTelemetryAsync"
        QT_MOC_LITERAL(102, 29),  // "demo::client::TelemetryPacket"
        QT_MOC_LITERAL(132, 3),  // "pkt"
        QT_MOC_LITERAL(136, 24),  // "insertSnapshotEventAsync"
        QT_MOC_LITERAL(161, 14),  // "screenshotPath"
        QT_MOC_LITERAL(176, 9),  // "reasonTag"
        QT_MOC_LITERAL(186, 13),  // "isTargetEvent"
        QT_MOC_LITERAL(200, 17),  // "insertAppLogAsync"
        QT_MOC_LITERAL(218, 4),  // "tsMs"
        QT_MOC_LITERAL(223, 5),  // "level"
        QT_MOC_LITERAL(229, 4),  // "type"
        QT_MOC_LITERAL(234, 7),  // "message"
        QT_MOC_LITERAL(242, 24),  // "insertPlaybackIndexAsync"
        QT_MOC_LITERAL(267, 33),  // "demo::client::PlaybackIndexRe..."
        QT_MOC_LITERAL(301, 3)   // "rec"
    },
    "demo::client::SQLiteDatabaseService",
    "saveConfigAsync",
    "",
    "demo::client::AppConfig",
    "cfg",
    "insertTelemetryAsync",
    "demo::client::TelemetryPacket",
    "pkt",
    "insertSnapshotEventAsync",
    "screenshotPath",
    "reasonTag",
    "isTargetEvent",
    "insertAppLogAsync",
    "tsMs",
    "level",
    "type",
    "message",
    "insertPlaybackIndexAsync",
    "demo::client::PlaybackIndexRecord",
    "rec"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_demo__client__SQLiteDatabaseService[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   44,    2, 0x0a,    1 /* Public */,
       5,    1,   47,    2, 0x0a,    3 /* Public */,
       8,    4,   50,    2, 0x0a,    5 /* Public */,
      12,    4,   59,    2, 0x0a,   10 /* Public */,
      17,    1,   68,    2, 0x0a,   15 /* Public */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, 0x80000000 | 6, QMetaType::QString, QMetaType::QString, QMetaType::Bool,    7,    9,   10,   11,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString, QMetaType::QString, QMetaType::QString,   13,   14,   15,   16,
    QMetaType::Void, 0x80000000 | 18,   19,

       0        // eod
};

Q_CONSTINIT const QMetaObject demo::client::SQLiteDatabaseService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_demo__client__SQLiteDatabaseService.offsetsAndSizes,
    qt_meta_data_demo__client__SQLiteDatabaseService,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_demo__client__SQLiteDatabaseService_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SQLiteDatabaseService, std::true_type>,
        // method 'saveConfigAsync'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const demo::client::AppConfig &, std::false_type>,
        // method 'insertTelemetryAsync'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const demo::client::TelemetryPacket &, std::false_type>,
        // method 'insertSnapshotEventAsync'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const demo::client::TelemetryPacket &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'insertAppLogAsync'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'insertPlaybackIndexAsync'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const demo::client::PlaybackIndexRecord &, std::false_type>
    >,
    nullptr
} };

void demo::client::SQLiteDatabaseService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SQLiteDatabaseService *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->saveConfigAsync((*reinterpret_cast< std::add_pointer_t<demo::client::AppConfig>>(_a[1]))); break;
        case 1: _t->insertTelemetryAsync((*reinterpret_cast< std::add_pointer_t<demo::client::TelemetryPacket>>(_a[1]))); break;
        case 2: _t->insertSnapshotEventAsync((*reinterpret_cast< std::add_pointer_t<demo::client::TelemetryPacket>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 3: _t->insertAppLogAsync((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 4: _t->insertPlaybackIndexAsync((*reinterpret_cast< std::add_pointer_t<demo::client::PlaybackIndexRecord>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< demo::client::AppConfig >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< demo::client::TelemetryPacket >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< demo::client::TelemetryPacket >(); break;
            }
            break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< demo::client::PlaybackIndexRecord >(); break;
            }
            break;
        }
    }
}

const QMetaObject *demo::client::SQLiteDatabaseService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *demo::client::SQLiteDatabaseService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_demo__client__SQLiteDatabaseService.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "IDatabaseService"))
        return static_cast< IDatabaseService*>(this);
    return QObject::qt_metacast(_clname);
}

int demo::client::SQLiteDatabaseService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
