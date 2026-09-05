#pragma once
/**
 * 2D affine for nested SmartGroup paint / hit.
 * @implements [SRS-EP-76] RenderingContext affine
 * @implements [ADR-0039] compose at paint
 */

#include "doc_model.hpp"

#include <cmath>

namespace epaper {
namespace document {

/** Column-vector affine: (x,y) → (a x + c y + e, b x + d y + f). */
struct Affine {
    double a = 1;
    double b = 0;
    double c = 0;
    double d = 1;
    double e = 0;
    double f = 0;

    void apply(double x, double y, double *ox, double *oy) const
    {
        if (!ox || !oy)
            return;
        *ox = a * x + c * y + e;
        *oy = b * x + d * y + f;
    }
};

struct RenderingContext {
    Affine transform;
};

inline Affine affineIdentity() { return {}; }

/** P ∘ Q — apply Q first, then P. */
inline Affine affineCompose(const Affine &P, const Affine &Q)
{
    Affine o;
    o.a = P.a * Q.a + P.c * Q.b;
    o.b = P.b * Q.a + P.d * Q.b;
    o.c = P.a * Q.c + P.c * Q.d;
    o.d = P.b * Q.c + P.d * Q.d;
    o.e = P.a * Q.e + P.c * Q.f + P.e;
    o.f = P.b * Q.e + P.d * Q.f + P.f;
    return o;
}

inline Affine affineScale(double sx, double sy)
{
    Affine m;
    m.a = sx != 0 ? sx : 1;
    m.d = sy != 0 ? sy : 1;
    return m;
}

inline Affine affineRotate(double rad)
{
    Affine m;
    const double csn = std::cos(rad);
    const double sn = std::sin(rad);
    m.a = csn;
    m.b = sn;
    m.c = -sn;
    m.d = csn;
    return m;
}

inline Affine affineTranslate(double tx, double ty)
{
    Affine m;
    m.e = tx;
    m.f = ty;
    return m;
}

/** Scale, then rotate, then translate (shipped SmartGroup own-transform). */
inline Affine affineOwn(const SmartTransform &t)
{
    return affineCompose(affineTranslate(t.x, t.y),
                         affineCompose(affineRotate(t.rotation), affineScale(t.scaleX, t.scaleY)));
}

/** Translate + rotate, no scale — fixedInk content-outcome of this group. */
inline Affine affineTR(const SmartTransform &t)
{
    return affineCompose(affineTranslate(t.x, t.y), affineRotate(t.rotation));
}

inline Affine affineInverse(const Affine &m)
{
    const double det = m.a * m.d - m.c * m.b;
    Affine o;
    if (std::abs(det) < 1e-18)
        return o;
    const double inv = 1.0 / det;
    o.a = m.d * inv;
    o.b = -m.b * inv;
    o.c = -m.c * inv;
    o.d = m.a * inv;
    o.e = -(o.a * m.e + o.c * m.f);
    o.f = -(o.b * m.e + o.d * m.f);
    return o;
}

inline Affine contentOutcome(const Affine &ctx, const DocNode &sg)
{
    if (sg.kind != NodeKind::SmartGroup)
        return ctx;
    if (sg.inkScaleMode == "withBounds")
        return affineCompose(ctx, affineOwn(sg.transform));
    return affineCompose(ctx, affineTR(sg.transform));
}

inline Affine outcomeAffine(const Affine &ctx, const DocNode &sg)
{
    if (sg.kind != NodeKind::SmartGroup)
        return ctx;
    return affineCompose(ctx, affineOwn(sg.transform));
}

} // namespace document
} // namespace epaper
