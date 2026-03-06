/****************************************************************************
** Meta object code from reading C++ file 'tcp_client_worker.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/client/network/tcp_client_worker.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tcp_client_worker.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_demo__client__TcpClientWorker_t {
    uint offsetsAndSizes[28];
    char stringdata0[30];
    char stringdata1[18];
    char stringdata2[1];
    char stringdata3[30];
    char stringdata4[7];
    char stringdata5[11];
    char stringdata6[4];
    char stringdata7[6];
    char stringdata8[5];
    char stringdata9[5];
    char stringdata10[5];
    char stringdata11[12];
    char stringdata12[12];
    char stringdata13[15];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_demo__client__TcpClientWorker_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_demo__client__TcpClientWorker_t qt_meta_stringdata_demo__client__TcpClientWorker = {
    {
        QT_MOC_LITERAL(0, 29),  // "demo::client::TcpClientWorker"
        QT_MOC_LITERAL(30, 17),  // "telemetryReceived"
        QT_MOC_LITERAL(48, 0),  // ""
        QT_MOC_LITERAL(49, 29),  // "demo::client::TelemetryPacket"
        QT_MOC_LITERAL(79, 6),  // "packet"
        QT_MOC_LITERAL(86, 10),  // "logMessage"
        QT_MOC_LITERAL(97, 3),  // "msg"
        QT_MOC_LITERAL(101, 5),  // "start"
        QT_MOC_LITERAL(107, 4),  // "host"
        QT_MOC_LITERAL(112, 4),  // "port"
        QT_MOC_LITERAL(117, 4),  // "stop"
        QT_MOC_LITERAL(122, 11),  // "onReadyRead"
        QT_MOC_LITERAL(134, 11),  // "onConnected"
        QT_MOC_LITERAL(146, 14)   // "onDisconnected"
    },
    "demo::client::TcpClientWorker",
    "telemetryReceived",
    "",
    "demo::client::TelemetryPacket",
    "packet",
    "logMessage",
    "msg",
    "start",
    "host",
    "port",
    "stop",
    "onReadyRead",
    "onConnected",
    "onDisconnected"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_demo__client__TcpClientWorker[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   56,    2, 0x06,    1 /* Public */,
       5,    1,   59,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    2,   62,    2, 0x0a,    5 /* Public */,
      10,    0,   67,    2, 0x0a,    8 /* Public */,
      11,    0,   68,    2, 0x08,    9 /* Private */,
      12,    0,   69,    2, 0x08,   10 /* Private */,
      13,    0,   70,    2, 0x08,   11 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::UShort,    8,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject demo::client::TcpClientWorker::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_demo__client__TcpClientWorker.offsetsAndSizes,
    qt_meta_data_demo__client__TcpClientWorker,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_demo__client__TcpClientWorker_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TcpClientWorker, std::true_type>,
        // method 'telemetryReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const demo::client::TelemetryPacket &, std::false_type>,
        // method 'logMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'start'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint16, std::false_type>,
        // method 'stop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void demo::client::TcpClientWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TcpClientWorker *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->telemetryReceived((*reinterpret_cast< std::add_pointer_t<demo::client::TelemetryPacket>>(_a[1]))); break;
        case 1: _t->logMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->start((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint16>>(_a[2]))); break;
        case 3: _t->stop(); break;
        case 4: _t->onReadyRead(); break;
        case 5: _t->onConnected(); break;
        case 6: _t->onDisconnected(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< demo::client::TelemetryPacket >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TcpClientWorker::*)(const demo::client::TelemetryPacket & );
            if (_t _q_method = &TcpClientWorker::telemetryReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TcpClientWorker::*)(const QString & );
            if (_t _q_method = &TcpClientWorker::logMessage; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *demo::client::TcpClientWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *demo::client::TcpClientWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_demo__client__TcpClientWorker.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int demo::client::TcpClientWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void demo::client::TcpClientWorker::telemetryReceived(const demo::client::TelemetryPacket & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void demo::client::TcpClientWorker::logMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
