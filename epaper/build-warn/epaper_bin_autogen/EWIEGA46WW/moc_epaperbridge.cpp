/****************************************************************************
** Meta object code from reading C++ file 'epaperbridge.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../epaperbridge.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'epaperbridge.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN12EpaperBridgeE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN12EpaperBridgeE = QtMocHelpers::stringData(
    "EpaperBridge",
    "availableChanged",
    "",
    "penModeAttachedChanged",
    "monoModeAttachedChanged",
    "overlayStrokePenChanged",
    "statusChanged",
    "setOverlayStrokePen",
    "pen",
    "attachPenModeRegion",
    "QQuickItem*",
    "host",
    "attachMonoModeRegion",
    "swapPen",
    "rect",
    "beginStrokeBlock",
    "endStrokeBlock",
    "restoreXochitl",
    "available",
    "penModeAttached",
    "monoModeAttached",
    "overlayStrokePen",
    "status"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN12EpaperBridgeE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       5,  106, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   86,    2, 0x06,    6 /* Public */,
       3,    0,   87,    2, 0x06,    7 /* Public */,
       4,    0,   88,    2, 0x06,    8 /* Public */,
       5,    0,   89,    2, 0x06,    9 /* Public */,
       6,    0,   90,    2, 0x06,   10 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       7,    1,   91,    2, 0x02,   11 /* Public */,
       9,    1,   94,    2, 0x02,   13 /* Public */,
      12,    1,   97,    2, 0x02,   15 /* Public */,
      13,    1,  100,    2, 0x02,   17 /* Public */,
      15,    0,  103,    2, 0x02,   19 /* Public */,
      16,    0,  104,    2, 0x02,   20 /* Public */,
      17,    0,  105,    2, 0x02,   21 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Bool, QMetaType::Bool,    8,
    QMetaType::Bool, 0x80000000 | 10,   11,
    QMetaType::Bool, 0x80000000 | 10,   11,
    QMetaType::Void, QMetaType::QRect,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      18, QMetaType::Bool, 0x00015001, uint(0), 0,
      19, QMetaType::Bool, 0x00015001, uint(1), 0,
      20, QMetaType::Bool, 0x00015001, uint(2), 0,
      21, QMetaType::Bool, 0x00015001, uint(3), 0,
      22, QMetaType::QString, 0x00015001, uint(4), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject EpaperBridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN12EpaperBridgeE.offsetsAndSizes,
    qt_meta_data_ZN12EpaperBridgeE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN12EpaperBridgeE_t,
        // property 'available'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'penModeAttached'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'monoModeAttached'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'overlayStrokePen'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'status'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<EpaperBridge, std::true_type>,
        // method 'availableChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'penModeAttachedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'monoModeAttachedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'overlayStrokePenChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'statusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setOverlayStrokePen'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'attachPenModeRegion'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<QQuickItem *, std::false_type>,
        // method 'attachMonoModeRegion'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<QQuickItem *, std::false_type>,
        // method 'swapPen'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QRect &, std::false_type>,
        // method 'beginStrokeBlock'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'endStrokeBlock'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'restoreXochitl'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void EpaperBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<EpaperBridge *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->availableChanged(); break;
        case 1: _t->penModeAttachedChanged(); break;
        case 2: _t->monoModeAttachedChanged(); break;
        case 3: _t->overlayStrokePenChanged(); break;
        case 4: _t->statusChanged(); break;
        case 5: { bool _r = _t->setOverlayStrokePen((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 6: { bool _r = _t->attachPenModeRegion((*reinterpret_cast< std::add_pointer_t<QQuickItem*>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 7: { bool _r = _t->attachMonoModeRegion((*reinterpret_cast< std::add_pointer_t<QQuickItem*>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 8: _t->swapPen((*reinterpret_cast< std::add_pointer_t<QRect>>(_a[1]))); break;
        case 9: _t->beginStrokeBlock(); break;
        case 10: _t->endStrokeBlock(); break;
        case 11: _t->restoreXochitl(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QQuickItem* >(); break;
            }
            break;
        case 7:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QQuickItem* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (EpaperBridge::*)();
            if (_q_method_type _q_method = &EpaperBridge::availableChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (EpaperBridge::*)();
            if (_q_method_type _q_method = &EpaperBridge::penModeAttachedChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (EpaperBridge::*)();
            if (_q_method_type _q_method = &EpaperBridge::monoModeAttachedChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (EpaperBridge::*)();
            if (_q_method_type _q_method = &EpaperBridge::overlayStrokePenChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (EpaperBridge::*)();
            if (_q_method_type _q_method = &EpaperBridge::statusChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->available(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->penModeAttached(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->monoModeAttached(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->overlayStrokePen(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->status(); break;
        default: break;
        }
    }
}

const QMetaObject *EpaperBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EpaperBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN12EpaperBridgeE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int EpaperBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void EpaperBridge::availableChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void EpaperBridge::penModeAttachedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void EpaperBridge::monoModeAttachedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void EpaperBridge::overlayStrokePenChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void EpaperBridge::statusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
