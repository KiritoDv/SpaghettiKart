#include "GraphicsContext.h"

#include <libultraship.h>
static GfxList stack[1024];
static GfxList* out = stack;

void N64GraphicsContext::drawLine(float x1, float y1, float x2, float y2) {
#ifdef HMUI_DRAW_LINE_WITH_3D
    gDPPipeSync(out++);
    gDPSetCycleType(out++, G_CYC_2CYCLE);
    gDPSetRenderMode(out++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);
    gDPSetCombineLERP(out++, PRIMITIVE, 0, SHADE, 0, 0, 0, 0, 0, COMBINED, 0, COMBINED_ALPHA, 0, 0, 0, 0, 0);
    gDPSetPrimColor(out++, 0, 255, 255, 255, 255);
    gSPLineWidth(out++, 1);
    gSPLine3D(out++, (s32)(x1 * 4.0f), (s32)(y1 * 4.0f), (s32)(x2 * 4.0f), (s32)(y2 * 4.0f));
    gDPPipeSync(out++);
#else
    // Unimplemented: Fast3D does not support 2D line drawing directly for now.
#endif
}

void N64GraphicsContext::drawRect(const Rect& rect, const Color2D& color) {
    fillRect(Rect(rect.x, rect.y, rect.width, 1.0f), color); // Top
    fillRect(Rect(rect.x, rect.y + rect.height - 1.0f, rect.width, 1.0f), color); // Bottom
    fillRect(Rect(rect.x, rect.y, 1.0f, rect.height), color); // Left
    fillRect(Rect(rect.x + rect.width - 1.0f, rect.y, 1.0f, rect.height), color); // Right
}

void N64GraphicsContext::fillRect(const Rect& rect, const Color2D& color) {
    gDPPipeSync(out++);
    gDPSetCycleType(out++, G_CYC_2CYCLE);
    gDPSetRenderMode(out++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);
    gDPSetCombineLERP(out++, PRIMITIVE, 0, SHADE, 0, 0, 0, 0, 0, COMBINED, 0, COMBINED_ALPHA, 0, 0, 0, 0, 0);
    gDPSetPrimColor(out++, 0, 255, (u8)(color.r * 255), (u8)(color.g * 255), (u8)(color.b * 255), (u8)(color.a * 255));
    gDPFillWideRectangle(out++, rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
    gDPPipeSync(out++);
}

void N64GraphicsContext::drawText(float x, float y, const char* text, const Color2D& color) {
    // TODO: Implement text system later
}

void N64GraphicsContext::drawImage(const Rect& rect, const char* texture, const Color2D& color, float scale) {
    gDPSetPrimColor(out++, 0, 255, (u8)(color.r * 255), (u8)(color.g * 255), (u8)(color.b * 255), (u8)(color.a * 255));
    gDPLoadTextureBlock(out++, texture, G_IM_FMT_RGBA, G_IM_SIZ_32b, rect.width, rect.height, 0, G_TX_NOMIRROR | G_TX_WRAP,
                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gSPWideTextureRectangle(out++, (s32) (rect.x * 4.0f), (s32) (rect.y * 4.0f),
                            (s32) ((rect.x + rect.width * scale) * 4.0f), (s32) ((rect.y + rect.height * scale) * 4.0f),
                            G_TX_RENDERTILE, 0, 0, (s32) (1.0f / scale * 1024.0f), (s32) (1.0f / scale * 1024.0f));
}

void N64GraphicsContext::drawImageEx(const Rect& rect, const Rect& srcRect, const char* texture, const Color2D& color) {
    gDPSetPrimColor(out++, 0, 255, (u8)(color.r * 255), (u8)(color.g * 255), (u8)(color.b * 255), (u8)(color.a * 255));
    gDPSetTile(out++, G_IM_FMT_RGBA, G_IM_SIZ_32b, 0, 0, G_TX_LOADTILE, 0, G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD);
    gDPTileSync(out++);
    gDPSetTile(out++, G_IM_FMT_RGBA, G_IM_SIZ_32b, 2, 0, G_TX_RENDERTILE, 0, G_TX_NOMIRROR, 3, G_TX_NOLOD, G_TX_NOMIRROR, 3, G_TX_NOLOD);
    gDPSetTileSize(out++, G_TX_RENDERTILE, 0, 0, srcRect.width << G_TEXTURE_IMAGE_FRAC, srcRect.height << G_TEXTURE_IMAGE_FRAC);
    gDPPipeSync(out++);
    gDPSetTextureImage(out++, G_IM_FMT_RGBA, G_IM_SIZ_32b, 1, texture);
    gDPLoadSync(out++);
    gDPLoadBlock(out++, G_TX_LOADTILE, 0, 0, rect.width * rect.height - 1, CALC_DXT(rect.width, G_IM_SIZ_32b_BYTES));
    gSPWideTextureRectangle(out++, rect.x << 2, y << 2, (rect.x + rect.width) << 2, (rect.y + rect.height) << 2, G_TX_RENDERTILE, srcRect.x, srcRect.y, 4 << 10, 1 << 10);
}

void N64GraphicsContext::setScissor(const Rect& rect) {
    gDPSetScissor(out++, G_SC_NON_INTERLACE, (s32)(rect.x * 4.0f), (s32)(rect.y * 4.0f),
                  (s32)((rect.x + rect.width) * 4.0f), (s32)((rect.y + rect.height) * 4.0f));
}

void N64GraphicsContext::clearScissor() {
    gDPSetScissor(out++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH * 4, SCREEN_HEIGHT * 4);
}

void N64GraphicsContext::build(GfxList** gen) {
    gSPEndDisplayList(out++);
    gDPPipeSync(out++);
    gDPSetCycleType(out++, G_CYC_2CYCLE);
    gDPSetRenderMode(out++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);
    gDPSetCombineLERP(out++, PRIMITIVE, 0, SHADE, 0, 0, 0, 0, 0, COMBINED, 0, COMBINED_ALPHA, 0, 0, 0, 0, 0);
    gSPDisplayList(((*gen)++), stack);
    out = &stack[0];
}