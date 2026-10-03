#pragma once
/**
 * Wire / apply kind strings for DocEdit. Pointer-stable; compare with strcmp.
 * @implements [SRS-EP-07] structural edit kinds
 * @implements [SRS-EP-08] closed transmit op.type list
 */

namespace epaper {
namespace document {
namespace edit_kind {

constexpr const char kAppendInk[] = "append_ink";
constexpr const char kCreateFrame[] = "create_frame";
constexpr const char kCreateGroup[] = "create_group";
constexpr const char kCreateText[] = "create_text";
constexpr const char kCreatePrimitive[] = "create_primitive";
constexpr const char kCreateConnector[] = "create_connector";
constexpr const char kCreateSmartGroup[] = "create_smart_group";
constexpr const char kJoinSmartGroup[] = "join_smart_group";
constexpr const char kSetSmartTransform[] = "set_smart_transform";
constexpr const char kSetInkScaleMode[] = "set_ink_scale_mode";
constexpr const char kSetInkSamples[] = "set_ink_samples";
constexpr const char kSetEndpointInk[] = "set_endpoint_ink";
constexpr const char kReparent[] = "reparent";
constexpr const char kRemoveNode[] = "remove_node";
constexpr const char kCompound[] = "compound";
constexpr const char kRestoreSnapshot[] = "restore_snapshot";

} // namespace edit_kind

inline bool kindEq(const char *a, const char *b)
{
    if (a == b)
        return true;
    if (!a || !b)
        return false;
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}

/** @implements [SRS-EP-07] structural op set for the undo ring */
inline bool isStructuralKind(const char *kind)
{
    using namespace edit_kind;
    return kindEq(kind, kAppendInk) || kindEq(kind, kCreateSmartGroup)
        || kindEq(kind, kJoinSmartGroup) || kindEq(kind, kSetSmartTransform)
        || kindEq(kind, kSetInkScaleMode) || kindEq(kind, kReparent) || kindEq(kind, kRemoveNode)
        || kindEq(kind, kCreateFrame) || kindEq(kind, kCreateGroup) || kindEq(kind, kCreateText)
        || kindEq(kind, kCreatePrimitive) || kindEq(kind, kCreateConnector)
        || kindEq(kind, kSetInkSamples) || kindEq(kind, kSetEndpointInk)
        || kindEq(kind, kCompound);
}

inline bool isCreateKind(const char *kind)
{
    using namespace edit_kind;
    return kindEq(kind, kAppendInk) || kindEq(kind, kCreateFrame) || kindEq(kind, kCreateGroup)
        || kindEq(kind, kCreateText) || kindEq(kind, kCreatePrimitive)
        || kindEq(kind, kCreateSmartGroup) || kindEq(kind, kCreateConnector);
}

/** @implements [SRS-EP-08] SRS-IN-09 closed op.type list */
inline bool isClosedTransmitKind(const char *kind)
{
    using namespace edit_kind;
    return kindEq(kind, kAppendInk) || kindEq(kind, kCreateSmartGroup)
        || kindEq(kind, kJoinSmartGroup) || kindEq(kind, kSetSmartTransform)
        || kindEq(kind, kSetInkScaleMode) || kindEq(kind, kReparent) || kindEq(kind, kRemoveNode)
        || kindEq(kind, kCompound) || kindEq(kind, kSetInkSamples)
        || kindEq(kind, kRestoreSnapshot);
}

} // namespace document
} // namespace epaper
