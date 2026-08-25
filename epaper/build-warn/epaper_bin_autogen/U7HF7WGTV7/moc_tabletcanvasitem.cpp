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
    "handTouchArmedChanged",
    "selectionChromeChanged",
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
    "onPointerStart",
    "x",
    "y",
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
    "handTouchArmed",
    "manipulationUnavailable",
    "manipulationUnavailableRect",
    "encloseCtaRect",
    "encloseVisible",
    "encloseRefuseReason",
    "selectionBoundsRect",
    "handleCount",
    "handleSize",
    "modeChipVisible",
    "modeChipLabel",
    "modeChipRect",
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
      35,   14, // methods
      33,  315, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      13,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    5,  224,    2, 0x06,   34 /* Public */,
       8,    0,  235,    2, 0x06,   40 /* Public */,
       9,    0,  236,    2, 0x06,   41 /* Public */,
      10,    0,  237,    2, 0x06,   42 /* Public */,
      11,    0,  238,    2, 0x06,   43 /* Public */,
      12,    0,  239,    2, 0x06,   44 /* Public */,
      13,    0,  240,    2, 0x06,   45 /* Public */,
      14,    0,  241,    2, 0x06,   46 /* Public */,
      15,    0,  242,    2, 0x06,   47 /* Public */,
      16,    0,  243,    2, 0x06,   48 /* Public */,
      17,    0,  244,    2, 0x06,   49 /* Public */,
      18,    0,  245,    2, 0x06,   50 /* Public */,
      19,    0,  246,    2, 0x06,   51 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      20,    0,  247,    2, 0x02,   52 /* Public */,
      21,    0,  248,    2, 0x02,   53 /* Public */,
      22,    3,  249,    2, 0x02,   54 /* Public */,
      27,    1,  256,    2, 0x02,   58 /* Public */,
      29,    0,  259,    2, 0x02,   60 /* Public */,
      30,    0,  260,    2, 0x02,   61 /* Public */,
      31,    4,  261,    2, 0x02,   62 /* Public */,
      35,    4,  270,    2, 0x02,   67 /* Public */,
      36,    3,  279,    2, 0x02,   72 /* Public */,
      37,    0,  286,    2, 0x02,   76 /* Public */,
      38,    2,  287,    2, 0x02,   77 /* Public */,
      39,    0,  292,    2, 0x02,   80 /* Public */,
      40,    0,  293,    2, 0x02,   81 /* Public */,
      41,    3,  294,    2, 0x02,   82 /* Public */,
      43,    3,  301,    2, 0x02,   86 /* Public */,
      44,    0,  308,    2, 0x02,   90 /* Public */,
      45,    0,  309,    2, 0x02,   91 /* Public */,
      46,    0,  310,    2, 0x02,   92 /* Public */,
      47,    0,  311,    2, 0x02,   93 /* Public */,
      48,    0,  312,    2, 0x02,   94 /* Public */,
      49,    0,  313,    2, 0x02,   95 /* Public */,
      50,    0,  314,    2, 0x02,   96 /* Public */,

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
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 23, QMetaType::QPointF, QMetaType::QReal,   24,   25,   26,
    QMetaType::Void, QMetaType::QString,   28,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal, QMetaType::Bool,   32,   33,   26,   34,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal, QMetaType::Bool,   32,   33,   26,   34,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::Bool,   32,   33,   34,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal,   32,   33,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal,   32,   33,   42,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal,   32,   33,   42,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      51, QMetaType::Bool, 0x00015401, uint(-1), 0,
      52, QMetaType::QString, 0x00015001, uint(11), 0,
      53, QMetaType::Bool, 0x00015001, uint(12), 0,
      54, QMetaType::Bool, 0x00015001, uint(1), 0,
      55, QMetaType::Bool, 0x00015001, uint(1), 0,
      56, QMetaType::Int, 0x00015001, uint(2), 0,
      57, QMetaType::QPointF, 0x00015001, uint(11), 0,
      58, QMetaType::QString, 0x00015103, uint(3), 0,
      59, QMetaType::Bool, 0x00015001, uint(4), 0,
      60, QMetaType::Bool, 0x00015001, uint(4), 0,
      61, QMetaType::Bool, 0x00015001, uint(3), 0,
      62, QMetaType::QString, 0x00015001, uint(5), 0,
      63, QMetaType::Bool, 0x00015001, uint(6), 0,
      64, QMetaType::QString, 0x00015001, uint(7), 0,
      65, QMetaType::QRectF, 0x00015001, uint(7), 0,
      66, QMetaType::QRectF, 0x00015001, uint(7), 0,
      67, QMetaType::Bool, 0x00015001, uint(7), 0,
      68, QMetaType::QString, 0x00015001, uint(7), 0,
      69, QMetaType::QRectF, 0x00015001, uint(7), 0,
      70, QMetaType::Int, 0x00015001, uint(7), 0,
      71, QMetaType::QReal, 0x00015001, uint(7), 0,
      72, QMetaType::Bool, 0x00015001, uint(7), 0,
      73, QMetaType::QString, 0x00015001, uint(7), 0,
      74, QMetaType::QRectF, 0x00015001, uint(7), 0,
      75, QMetaType::QString, 0x00015001, uint(8), 0,
      76, QMetaType::Bool, 0x00015001, uint(8), 0,
      77, QMetaType::Bool, 0x00015001, uint(8), 0,
      78, QMetaType::QRectF, 0x00015001, uint(9), 0,
      79, QMetaType::QRectF, 0x00015001, uint(10), 0,
      80, QMetaType::QRectF, 0x00015001, uint(10), 0,
      81, QMetaType::QRectF, 0x00015001, uint(10), 0,
      82, QMetaType::QRectF, 0x00015001, uint(10), 0,
      83, QMetaType::QRectF, 0x00015001, uint(10), 0,

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
        // property 'handTouchArmed'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'manipulationUnavailable'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'manipulationUnavailableRect'
        QtPrivate::TypeAndForceComplete<QRectF, std::true_type>,
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
        // method 'handTouchArmedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectionChromeChanged'
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
        case 6: _t->handTouchArmedChanged(); break;
        case 7: _t->selectionChromeChanged(); break;
        case 8: _t->followChanged(); break;
        case 9: _t->toolChipRectChanged(); break;
        case 10: _t->trailingChromeChanged(); break;
        case 11: _t->debugChanged(); break;
        case 12: _t->debugLogVisibleChanged(); break;
        case 13: _t->requestUndo(); break;
        case 14: _t->requestRedo(); break;
        case 15: _t->ingestPoint((*reinterpret_cast< std::add_pointer_t<QEvent::Type>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QPointF>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3]))); break;
        case 16: _t->armTool((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 17: _t->toggleRecogInkBox(); break;
        case 18: _t->toggleRecogConnector(); break;
        case 19: _t->onPointerStart((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 20: _t->onPointerMove((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 21: _t->onPointerEnd((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        case 22: _t->onPointerCancel(); break;
        case 23: _t->onFingerTap((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2]))); break;
        case 24: _t->onSecondContact(); break;
        case 25: _t->onContactsCleared(); break;
        case 26: _t->onPinchStart((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3]))); break;
        case 27: _t->onPinchUpdate((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[3]))); break;
        case 28: _t->onPinchEnd(); break;
        case 29: _t->toggleHandTouch(); break;
        case 30: _t->cancelHandTouch(); break;
        case 31: _t->encloseSelection(); break;
        case 32: _t->tapModeChip(); break;
        case 33: _t->tapFollowToggle(); break;
        case 34: _t->toggleDebugLog(); break;
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
            if (_q_method_type _q_method = &TabletCanvasItem::handTouchArmedChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::selectionChromeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::followChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::toolChipRectChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::trailingChromeChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::debugChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _q_method_type = void (TabletCanvasItem::*)();
            if (_q_method_type _q_method = &TabletCanvasItem::debugLogVisibleChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 12;
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
        case 12: *reinterpret_cast< bool*>(_v) = _t->handTouchArmed(); break;
        case 13: *reinterpret_cast< QString*>(_v) = _t->manipulationUnavailable(); break;
        case 14: *reinterpret_cast< QRectF*>(_v) = _t->manipulationUnavailableRect(); break;
        case 15: *reinterpret_cast< QRectF*>(_v) = _t->encloseCtaRect(); break;
        case 16: *reinterpret_cast< bool*>(_v) = _t->encloseVisible(); break;
        case 17: *reinterpret_cast< QString*>(_v) = _t->encloseRefuseReason(); break;
        case 18: *reinterpret_cast< QRectF*>(_v) = _t->selectionBoundsRect(); break;
        case 19: *reinterpret_cast< int*>(_v) = _t->handleCount(); break;
        case 20: *reinterpret_cast< qreal*>(_v) = _t->handleSize(); break;
        case 21: *reinterpret_cast< bool*>(_v) = _t->modeChipVisible(); break;
        case 22: *reinterpret_cast< QString*>(_v) = _t->modeChipLabel(); break;
        case 23: *reinterpret_cast< QRectF*>(_v) = _t->modeChipRectProp(); break;
        case 24: *reinterpret_cast< QString*>(_v) = _t->followDirection(); break;
        case 25: *reinterpret_cast< bool*>(_v) = _t->followPressed(); break;
        case 26: *reinterpret_cast< bool*>(_v) = _t->followUnavailable(); break;
        case 27: *reinterpret_cast< QRectF*>(_v) = _t->toolChipRect(); break;
        case 28: *reinterpret_cast< QRectF*>(_v) = _t->followToggleRect(); break;
        case 29: *reinterpret_cast< QRectF*>(_v) = _t->usbLinkRect(); break;
        case 30: *reinterpret_cast< QRectF*>(_v) = _t->debugToggleRect(); break;
        case 31: *reinterpret_cast< QRectF*>(_v) = _t->handTouchToggleRect(); break;
        case 32: *reinterpret_cast< QRectF*>(_v) = _t->debugLogRect(); break;
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
        if (_id < 35)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 35;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 35)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 35;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 33;
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
void TabletCanvasItem::handTouchArmedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void TabletCanvasItem::selectionChromeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void TabletCanvasItem::followChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void TabletCanvasItem::toolChipRectChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void TabletCanvasItem::trailingChromeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void TabletCanvasItem::debugChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void TabletCanvasItem::debugLogVisibleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}
QT_WARNING_POP
