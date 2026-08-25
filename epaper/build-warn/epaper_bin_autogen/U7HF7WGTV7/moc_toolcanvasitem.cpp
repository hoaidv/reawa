/****************************************************************************
** Meta object code from reading C++ file 'toolcanvasitem.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../drawing/toolcanvasitem.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'toolcanvasitem.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14ToolCanvasItemE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN14ToolCanvasItemE = QtMocHelpers::stringData(
    "ToolCanvasItem",
    "surfaceChanged",
    "",
    "handTouchArmedChanged",
    "selectionChromeChanged",
    "onPointerStart",
    "x",
    "y",
    "pressure",
    "pen",
    "onPointerMove",
    "onPointerEnd",
    "onPointerCancel",
    "onFingerTap",
    "onSecondContact",
    "onContactsCleared",
    "onPinchStart",
    "scale",
    "onPinchUpdate",
    "onPinchEnd",
    "toggleHandTouch",
    "cancelHandTouch",
    "encloseSelection",
    "tapModeChip",
    "surface",
    "TabletCanvasItem*",
    "handTouchArmed",
    "encloseCtaRect",
    "encloseVisible",
    "encloseRefuseReason",
    "selectionBoundsRect",
    "handleCount",
    "handleSize",
    "modeChipVisible",
    "modeChipLabel",
    "modeChipRect",
    "manipulationUnavailable",
    "manipulationUnavailableRect"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN14ToolCanvasItemE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
      13,  171, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  116,    2, 0x06,   14 /* Public */,
       3,    0,  117,    2, 0x06,   15 /* Public */,
       4,    0,  118,    2, 0x06,   16 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       5,    4,  119,    2, 0x02,   17 /* Public */,
      10,    4,  128,    2, 0x02,   22 /* Public */,
      11,    3,  137,    2, 0x02,   27 /* Public */,
      12,    0,  144,    2, 0x02,   31 /* Public */,
      13,    2,  145,    2, 0x02,   32 /* Public */,
      14,    0,  150,    2, 0x02,   35 /* Public */,
      15,    0,  151,    2, 0x02,   36 /* Public */,
      16,    3,  152,    2, 0x02,   37 /* Public */,
      18,    3,  159,    2, 0x02,   41 /* Public */,
      19,    0,  166,    2, 0x02,   45 /* Public */,
      20,    0,  167,    2, 0x02,   46 /* Public */,
      21,    0,  168,    2, 0x02,   47 /* Public */,
      22,    0,  169,    2, 0x02,   48 /* Public */,
      23,    0,  170,    2, 0x02,   49 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal, QMetaType::Bool,    6,    7,    8,    9,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal, QMetaType::Bool,    6,    7,    8,    9,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::Bool,    6,    7,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal,    6,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal,    6,    7,   17,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal,    6,    7,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      24, 0x80000000 | 25, 0x0001510b, uint(0), 0,
      26, QMetaType::Bool, 0x00015001, uint(1), 0,
      27, QMetaType::QRectF, 0x00015001, uint(2), 0,
      28, QMetaType::Bool, 0x00015001, uint(2), 0,
      29, QMetaType::QString, 0x00015001, uint(2), 0,
      30, QMetaType::QRectF, 0x00015001, uint(2), 0,
      31, QMetaType::Int, 0x00015001, uint(2), 0,
      32, QMetaType::QReal, 0x00015001, uint(2), 0,
      33, QMetaType::Bool, 0x00015001, uint(2), 0,
      34, QMetaType::QString, 0x00015001, uint(2), 0,
      35, QMetaType::QRectF, 0x00015001, uint(2), 0,
      36, QMetaType::QString, 0x00015001, uint(2), 0,
      37, QMetaType::QRectF, 0x00015001, uint(2), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject ToolCanvasItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QQuickPaintedItem::staticMetaObject>(),
    qt_meta_stringdata_ZN14ToolCanvasItemE.offsetsAndSizes,
    qt_meta_data_ZN14ToolCanvasItemE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN14ToolCanvasItemE_t,
        // property 'surface'
        QtPrivate::TypeAndForceComplete<TabletCanvasItem*, std::true_type>,
        // property 'handTouchArmed'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'encloseCtaRect'
        QtPrivate::TypeAndForceComplete<QRectF, std::true_type>,
        // property 'encloseVisible'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'encloseRefuseReason'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'selectionBoundsRect'
        QtPrivate::TypeAndForceComplete<QRectF, std::true_type>,
        // property 'handleCount'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'handleSize'
        QtPrivate::TypeAndForceComplete<qreal, std::true_type>,
        // property 'modeChipVisible'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'modeChipLabel'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'modeChipRect'
        QtPrivate::TypeAndForceComplete<QRectF, std::true_type>,
        // property 'manipulationUnavailable'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'manipulationUnavailableRect'
        QtPrivate::TypeAndForceComplete<QRectF, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ToolCanvasItem, std::true_type>,
        // method 'surfaceChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handTouchArmedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectionChromeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPointerStart'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onPointerMove'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onPointerEnd'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onPointerCancel'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onFingerTap'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        // method 'onSecondContact'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onContactsCleared'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPinchStart'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        // method 'onPinchUpdate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        // method 'onPinchEnd'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleHandTouch'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cancelHandTouch'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'encloseSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'tapModeChip'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void ToolCanvasItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ToolCanvasItem *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->surfaceChanged(); break;
        case 1: _t->handTouchArmedChanged(); break;
        case 2: _t->selectionChromeChanged(); break;
        case 3: _t->onPointerStart((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 4: _t->onPointerMove((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 5: _t->onPointerEnd((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 6: _t->onPointerCancel(); break;
        case 7: _t->onFingerTap((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2]))); break;
        case 8: _t->onSecondContact(); break;
        case 9: _t->onContactsCleared(); break;
        case 10: _t->onPinchStart((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3]))); break;
        case 11: _t->onPinchUpdate((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3]))); break;
        case 12: _t->onPinchEnd(); break;
        case 13: _t->toggleHandTouch(); break;
        case 14: _t->cancelHandTouch(); break;
        case 15: _t->encloseSelection(); break;
        case 16: _t->tapModeChip(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (ToolCanvasItem::*)();
            if (_q_method_type _q_method = &ToolCanvasItem::surfaceChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (ToolCanvasItem::*)();
            if (_q_method_type _q_method = &ToolCanvasItem::handTouchArmedChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (ToolCanvasItem::*)();
            if (_q_method_type _q_method = &ToolCanvasItem::selectionChromeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< TabletCanvasItem**>(_v) = _t->surface(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->handTouchArmed(); break;
        case 2: *reinterpret_cast< QRectF*>(_v) = _t->encloseCtaRect(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->encloseVisible(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->encloseRefuseReason(); break;
        case 5: *reinterpret_cast< QRectF*>(_v) = _t->selectionBoundsRect(); break;
        case 6: *reinterpret_cast< int*>(_v) = _t->handleCount(); break;
        case 7: *reinterpret_cast< qreal*>(_v) = _t->handleSize(); break;
        case 8: *reinterpret_cast< bool*>(_v) = _t->modeChipVisible(); break;
        case 9: *reinterpret_cast< QString*>(_v) = _t->modeChipLabel(); break;
        case 10: *reinterpret_cast< QRectF*>(_v) = _t->modeChipRectProp(); break;
        case 11: *reinterpret_cast< QString*>(_v) = _t->manipulationUnavailable(); break;
        case 12: *reinterpret_cast< QRectF*>(_v) = _t->manipulationUnavailableRect(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setSurface(*reinterpret_cast< TabletCanvasItem**>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *ToolCanvasItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ToolCanvasItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN14ToolCanvasItemE.stringdata0))
        return static_cast<void*>(this);
    return QQuickPaintedItem::qt_metacast(_clname);
}

int ToolCanvasItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QQuickPaintedItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 17;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void ToolCanvasItem::surfaceChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ToolCanvasItem::handTouchArmedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ToolCanvasItem::selectionChromeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
