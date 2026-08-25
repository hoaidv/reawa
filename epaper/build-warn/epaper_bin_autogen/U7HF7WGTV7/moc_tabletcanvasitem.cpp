/****************************************************************************
** Meta object code from reading C++ file 'tabletcanvasitem.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../drawing/tabletcanvasitem.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tabletcanvasitem.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16TabletCanvasItemE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN16TabletCanvasItemE = QtMocHelpers::stringData(
    "TabletCanvasItem",
    "segmentDrawn",
    "",
    "x1",
    "y1",
    "x2",
    "y2",
    "lineWidth",
    "historyChanged",
    "strokeCountChanged",
    "toolModeChanged",
    "recogChanged",
    "lastStrokeLatchChanged",
    "followChanged",
    "toolChipRectChanged",
    "trailingChromeChanged",
    "debugChanged",
    "debugLogVisibleChanged",
    "requestUndo",
    "requestRedo",
    "ingestPoint",
    "QEvent::Type",
    "type",
    "pos",
    "pressure",
    "armTool",
    "mode",
    "toggleRecogInkBox",
    "toggleRecogConnector",
    "tapFollowToggle",
    "toggleDebugLog",
    "paintsInk",
    "debugInfo",
    "debugLogVisible",
    "canUndo",
    "canRedo",
    "strokeCount",
    "lastPoint",
    "toolMode",
    "recogInkBoxArmed",
    "recogConnectorArmed",
    "recogTogglesDimmed",
    "lastStrokeLatch",
    "followDirection",
    "followPressed",
    "followUnavailable",
    "toolChipRect",
    "followToggleRect",
    "usbLinkRect",
    "debugToggleRect",
    "handTouchToggleRect",
    "debugLogRect"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN16TabletCanvasItemE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
      21,  165, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    5,  128,    2, 0x06,   22 /* Public */,
       8,    0,  139,    2, 0x06,   28 /* Public */,
       9,    0,  140,    2, 0x06,   29 /* Public */,
      10,    0,  141,    2, 0x06,   30 /* Public */,
      11,    0,  142,    2, 0x06,   31 /* Public */,
      12,    0,  143,    2, 0x06,   32 /* Public */,
      13,    0,  144,    2, 0x06,   33 /* Public */,
      14,    0,  145,    2, 0x06,   34 /* Public */,
      15,    0,  146,    2, 0x06,   35 /* Public */,
      16,    0,  147,    2, 0x06,   36 /* Public */,
      17,    0,  148,    2, 0x06,   37 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      18,    0,  149,    2, 0x02,   38 /* Public */,
      19,    0,  150,    2, 0x02,   39 /* Public */,
      20,    3,  151,    2, 0x02,   40 /* Public */,
      25,    1,  158,    2, 0x02,   44 /* Public */,
      27,    0,  161,    2, 0x02,   46 /* Public */,
      28,    0,  162,    2, 0x02,   47 /* Public */,
      29,    0,  163,    2, 0x02,   48 /* Public */,
      30,    0,  164,    2, 0x02,   49 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal,    3,    4,    5,    6,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 21, QMetaType::QPointF, QMetaType::QReal,   22,   23,   24,
    QMetaType::Void, QMetaType::QString,   26,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      31, QMetaType::Bool, 0x00015401, uint(-1), 0,
      32, QMetaType::QString, 0x00015001, uint(9), 0,
      33, QMetaType::Bool, 0x00015001, uint(10), 0,
      34, QMetaType::Bool, 0x00015001, uint(1), 0,
      35, QMetaType::Bool, 0x00015001, uint(1), 0,
      36, QMetaType::Int, 0x00015001, uint(2), 0,
      37, QMetaType::QPointF, 0x00015001, uint(9), 0,
      38, QMetaType::QString, 0x00015103, uint(3), 0,
      39, QMetaType::Bool, 0x00015001, uint(4), 0,
      40, QMetaType::Bool, 0x00015001, uint(4), 0,
      41, QMetaType::Bool, 0x00015001, uint(3), 0,
      42, QMetaType::QString, 0x00015001, uint(5), 0,
      43, QMetaType::QString, 0x00015001, uint(6), 0,
      44, QMetaType::Bool, 0x00015001, uint(6), 0,
      45, QMetaType::Bool, 0x00015001, uint(6), 0,
      46, QMetaType::QRectF, 0x00015001, uint(7), 0,
      47, QMetaType::QRectF, 0x00015001, uint(8), 0,
      48, QMetaType::QRectF, 0x00015001, uint(8), 0,
      49, QMetaType::QRectF, 0x00015001, uint(8), 0,
      50, QMetaType::QRectF, 0x00015001, uint(8), 0,
      51, QMetaType::QRectF, 0x00015001, uint(8), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject TabletCanvasItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QQuickPaintedItem::staticMetaObject>(),
    qt_meta_stringdata_ZN16TabletCanvasItemE.offsetsAndSizes,
    qt_meta_data_ZN16TabletCanvasItemE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN16TabletCanvasItemE_t,
        // property 'paintsInk'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'debugInfo'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'debugLogVisible'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'canUndo'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'canRedo'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'strokeCount'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'lastPoint'
        QtPrivate::TypeAndForceComplete<QPointF, std::true_type>,
        // property 'toolMode'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'recogInkBoxArmed'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'recogConnectorArmed'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'recogTogglesDimmed'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'lastStrokeLatch'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'followDirection'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'followPressed'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'followUnavailable'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'toolChipRect'
        QtPrivate::TypeAndForceComplete<QRectF, std::true_type>,
        // property 'followToggleRect'
        QtPrivate::TypeAndForceComplete<QRectF, std::true_type>,
        // property 'usbLinkRect'
        QtPrivate::TypeAndForceComplete<QRectF, std::true_type>,
        // property 'debugToggleRect'
        QtPrivate::TypeAndForceComplete<QRectF, std::true_type>,
        // property 'handTouchToggleRect'
        QtPrivate::TypeAndForceComplete<QRectF, std::true_type>,
        // property 'debugLogRect'
        QtPrivate::TypeAndForceComplete<QRectF, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TabletCanvasItem, std::true_type>,
        // method 'segmentDrawn'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        // method 'historyChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'strokeCountChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toolModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'recogChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lastStrokeLatchChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'followChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toolChipRectChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'trailingChromeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'debugChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'debugLogVisibleChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestUndo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestRedo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'ingestPoint'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QEvent::Type, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        // method 'armTool'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'toggleRecogInkBox'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleRecogConnector'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'tapFollowToggle'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleDebugLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void TabletCanvasItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TabletCanvasItem *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->segmentDrawn((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[5]))); break;
        case 1: _t->historyChanged(); break;
        case 2: _t->strokeCountChanged(); break;
        case 3: _t->toolModeChanged(); break;
        case 4: _t->recogChanged(); break;
        case 5: _t->lastStrokeLatchChanged(); break;
        case 6: _t->followChanged(); break;
        case 7: _t->toolChipRectChanged(); break;
        case 8: _t->trailingChromeChanged(); break;
        case 9: _t->debugChanged(); break;
        case 10: _t->debugLogVisibleChanged(); break;
        case 11: _t->requestUndo(); break;
        case 12: _t->requestRedo(); break;
        case 13: _t->ingestPoint((*reinterpret_cast< std::add_pointer_t<QEvent::Type>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3]))); break;
        case 14: _t->armTool((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: _t->toggleRecogInkBox(); break;
        case 16: _t->toggleRecogConnector(); break;
        case 17: _t->tapFollowToggle(); break;
        case 18: _t->toggleDebugLog(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (TabletCanvasItem::*)(qreal , qreal , qreal , qreal , qreal );
            if (_q_method_type _q_method = &TabletCanvasItem::segmentDrawn; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::historyChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::strokeCountChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::toolModeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::recogChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::lastStrokeLatchChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::followChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::toolChipRectChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::trailingChromeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::debugChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::debugLogVisibleChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->paintsInk(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->debugInfo(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->debugLogVisible(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->canUndo(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->canRedo(); break;
        case 5: *reinterpret_cast< int*>(_v) = _t->strokeCount(); break;
        case 6: *reinterpret_cast< QPointF*>(_v) = _t->lastPoint(); break;
        case 7: *reinterpret_cast< QString*>(_v) = _t->toolMode(); break;
        case 8: *reinterpret_cast< bool*>(_v) = _t->recogInkBoxArmed(); break;
        case 9: *reinterpret_cast< bool*>(_v) = _t->recogConnectorArmed(); break;
        case 10: *reinterpret_cast< bool*>(_v) = _t->recogTogglesDimmed(); break;
        case 11: *reinterpret_cast< QString*>(_v) = _t->lastStrokeLatch(); break;
        case 12: *reinterpret_cast< QString*>(_v) = _t->followDirection(); break;
        case 13: *reinterpret_cast< bool*>(_v) = _t->followPressed(); break;
        case 14: *reinterpret_cast< bool*>(_v) = _t->followUnavailable(); break;
        case 15: *reinterpret_cast< QRectF*>(_v) = _t->toolChipRect(); break;
        case 16: *reinterpret_cast< QRectF*>(_v) = _t->followToggleRect(); break;
        case 17: *reinterpret_cast< QRectF*>(_v) = _t->usbLinkRect(); break;
        case 18: *reinterpret_cast< QRectF*>(_v) = _t->debugToggleRect(); break;
        case 19: *reinterpret_cast< QRectF*>(_v) = _t->handTouchToggleRect(); break;
        case 20: *reinterpret_cast< QRectF*>(_v) = _t->debugLogRect(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 7: _t->setToolMode(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *TabletCanvasItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TabletCanvasItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN16TabletCanvasItemE.stringdata0))
        return static_cast<void*>(this);
    return QQuickPaintedItem::qt_metacast(_clname);
}

int TabletCanvasItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QQuickPaintedItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 19;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    return _id;
}

// SIGNAL 0
void TabletCanvasItem::segmentDrawn(qreal _t1, qreal _t2, qreal _t3, qreal _t4, qreal _t5)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TabletCanvasItem::historyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void TabletCanvasItem::strokeCountChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void TabletCanvasItem::toolModeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void TabletCanvasItem::recogChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void TabletCanvasItem::lastStrokeLatchChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void TabletCanvasItem::followChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void TabletCanvasItem::toolChipRectChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void TabletCanvasItem::trailingChromeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void TabletCanvasItem::debugChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void TabletCanvasItem::debugLogVisibleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}
QT_WARNING_POP
