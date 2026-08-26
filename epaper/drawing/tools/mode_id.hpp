#pragma once

namespace epaper {
namespace tools {

enum class ModeId {
    Pen,
    Selection, // sel_rect / sel_freeform arms under this id for HandTouch
    Eraser,    // later
};

} // namespace tools
} // namespace epaper
