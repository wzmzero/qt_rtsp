/****************************************************************************
** Meta object code from reading C++ file 'main_window.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/client/app/main_window.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'main_window.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_MainWindow_t {
    uint offsetsAndSizes[36];
    char stringdata0[11];
    char stringdata1[11];
    char stringdata2[1];
    char stringdata3[10];
    char stringdata4[13];
    char stringdata5[12];
    char stringdata6[30];
    char stringdata7[4];
    char stringdata8[8];
    char stringdata9[12];
    char stringdata10[6];
    char stringdata11[5];
    char stringdata12[25];
    char stringdata13[6];
    char stringdata14[20];
    char stringdata15[16];
    char stringdata16[19];
    char stringdata17[25];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_MainWindow_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
        QT_MOC_LITERAL(0, 10),  // "MainWindow"
        QT_MOC_LITERAL(11, 10),  // "onStartAll"
        QT_MOC_LITERAL(22, 0),  // ""
        QT_MOC_LITERAL(23, 9),  // "onStopAll"
        QT_MOC_LITERAL(33, 12),  // "onSaveConfig"
        QT_MOC_LITERAL(46, 11),  // "onTelemetry"
        QT_MOC_LITERAL(58, 29),  // "demo::client::TelemetryPacket"
        QT_MOC_LITERAL(88, 3),  // "pkt"
        QT_MOC_LITERAL(92, 7),  // "onFrame"
        QT_MOC_LITERAL(100, 11),  // "QVideoFrame"
        QT_MOC_LITERAL(112, 5),  // "frame"
        QT_MOC_LITERAL(118, 4),  // "tsMs"
        QT_MOC_LITERAL(123, 24),  // "onConnectionStateChanged"
        QT_MOC_LITERAL(148, 5),  // "state"
        QT_MOC_LITERAL(154, 19),  // "onCaptureScreenshot"
        QT_MOC_LITERAL(174, 15),  // "onRefreshEvents"
        QT_MOC_LITERAL(190, 18),  // "onLogFilterChanged"
        QT_MOC_LITERAL(209, 24)   // "blinkConnectionIndicator"
    },
    "MainWindow",
    "onStartAll",
    "",
    "onStopAll",
    "onSaveConfig",
    "onTelemetry",
    "demo::client::TelemetryPacket",
    "pkt",
    "onFrame",
    "QVideoFrame",
    "frame",
    "tsMs",
    "onConnectionStateChanged",
    "state",
    "onCaptureScreenshot",
    "onRefreshEvents",
    "onLogFilterChanged",
    "blinkConnectionIndicator"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_MainWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   74,    2, 0x08,    1 /* Private */,
       3,    0,   75,    2, 0x08,    2 /* Private */,
       4,    0,   76,    2, 0x08,    3 /* Private */,
       5,    1,   77,    2, 0x08,    4 /* Private */,
       8,    2,   80,    2, 0x08,    6 /* Private */,
      12,    1,   85,    2, 0x08,    9 /* Private */,
      14,    0,   88,    2, 0x08,   11 /* Private */,
      15,    0,   89,    2, 0x08,   12 /* Private */,
      16,    0,   90,    2, 0x08,   13 /* Private */,
      17,    0,   91,    2, 0x08,   14 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, 0x80000000 | 9, QMetaType::LongLong,   10,   11,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.offsetsAndSizes,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_MainWindow_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'onStartAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onStopAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSaveConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTelemetry'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const demo::client::TelemetryPacket &, std::false_type>,
        // method 'onFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVideoFrame &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'onConnectionStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onCaptureScreenshot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRefreshEvents'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLogFilterChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'blinkConnectionIndicator'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onStartAll(); break;
        case 1: _t->onStopAll(); break;
        case 2: _t->onSaveConfig(); break;
        case 3: _t->onTelemetry((*reinterpret_cast< std::add_pointer_t<demo::client::TelemetryPacket>>(_a[1]))); break;
        case 4: _t->onFrame((*reinterpret_cast< std::add_pointer_t<QVideoFrame>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 5: _t->onConnectionStateChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->onCaptureScreenshot(); break;
        case 7: _t->onRefreshEvents(); break;
        case 8: _t->onLogFilterChanged(); break;
        case 9: _t->blinkConnectionIndicator(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
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
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QVideoFrame >(); break;
            }
            break;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
