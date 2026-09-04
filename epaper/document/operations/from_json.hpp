#pragma once
/**
 * JsonValue → DocEdit factory (wire / fixtures).
 * Included after DeviceDocument is complete so *edit.hpp method bodies can
 * use the tree.
 * @implements [SRS-EP-07] typed edit dispatch
 */

#include "append_ink_edit.hpp"
#include "compound_edit.hpp"
#include "create_connector_edit.hpp"
#include "create_frame_edit.hpp"
#include "create_group_edit.hpp"
#include "create_primitive_edit.hpp"
#include "create_smart_group_edit.hpp"
#include "create_text_edit.hpp"
#include "join_smart_group_edit.hpp"
#include "remove_node_edit.hpp"
#include "reparent_edit.hpp"
#include "set_endpoint_ink_edit.hpp"
#include "set_ink_samples_edit.hpp"
#include "set_ink_scale_mode_edit.hpp"
#include "set_smart_transform_edit.hpp"

namespace epaper {
namespace document {

inline std::unique_ptr<DocEdit> DocEdit::fromJson(const JsonValue &j)
{
    const std::string type = j.getString("type");
    const JsonValue *payload = j.get("payload");
    const JsonValue empty = JsonValue::object({});
    const JsonValue &p = payload ? *payload : empty;
    if (kindEq(type.c_str(), edit_kind::kAppendInk))
        return AppendInkEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kCreateFrame))
        return CreateFrameEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kCreateGroup))
        return CreateGroupEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kCreateText))
        return CreateTextEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kCreatePrimitive))
        return CreatePrimitiveEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kCreateConnector))
        return CreateConnectorEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kCreateSmartGroup))
        return CreateSmartGroupEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kJoinSmartGroup))
        return JoinSmartGroupEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kSetSmartTransform))
        return SetSmartTransformEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kSetInkScaleMode))
        return SetInkScaleModeEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kSetInkSamples))
        return SetInkSamplesEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kSetEndpointInk))
        return SetEndpointInkEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kReparent))
        return ReparentEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kRemoveNode))
        return RemoveNodeEdit::fromPayload(j, p);
    if (kindEq(type.c_str(), edit_kind::kCompound))
        return CompoundEdit::fromPayload(j, p);
    return nullptr;
}

} // namespace document
} // namespace epaper
