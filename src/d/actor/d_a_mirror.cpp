/**
 * d_a_mirror.cpp
 * Mirror of Twilight
 */

#include "d/dolzel_rel.h" // IWYU pragma: keep

#include "d/actor/d_a_mirror.h"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include "JSystem/J3DGraphBase/J3DMaterial.h"
#include "d/actor/d_a_player.h"
#include "d/d_com_inf_game.h"
#include <gf/GFGeometry.h>
#include <gf/GFLight.h>
#include "m_Do/m_Do_lib.h"
#if TARGET_PC
#include "dusk/vr/vr_main.hpp"
#include <algorithm>
#endif

static BOOL daMirror_c_createHeap(fopAc_ac_c* i_this) {
    return ((daMirror_c*)i_this)->createHeap();
}

static DUSK_CONSTEXPR char DUSK_CONST* l_arcName = "Mirror";

static DUSK_CONSTEXPR char DUSK_CONST* l_arcName2 = "MR-Table";

dMirror_packet_c::dMirror_packet_c() {
#if TARGET_PC
    GXInitTexObj(&mTexObj, nullptr, 0, 0, static_cast<GXTexFmt>(-1), GX_MAX_TEXWRAPMODE,
                 GX_MAX_TEXWRAPMODE, GX_FALSE);
#endif
    reset();
}

void dMirror_packet_c::reset() {
#if TARGET_PC
    mbReset = true;
    mbHadEntry = false;
#else
    mModelCount = 0;
#endif
}

void dMirror_packet_c::calcMinMax() {
    mMinVal.set(FLT_MAX, FLT_MAX, FLT_MAX);
    mMaxVal.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    cXyz* quad = mQuad;
    for (int i = 0; i < 4; i++, quad++) {
        f32 val = quad->x;
        if (val < mMinVal.x) {
            mMinVal.x = val;
        }

        val = quad->x;
        if (val > mMaxVal.x) {
            mMaxVal.x = val;
        }

        val = quad->y;
        if (val < mMinVal.y) {
            mMinVal.y = val;
        }

        val = quad->y;
        if (val > mMaxVal.y) {
            mMaxVal.y = val;
        }

        val = quad->z;
        if (val < mMinVal.z) {
            mMinVal.z = val;
        }

        val = quad->z;
        if (val > mMaxVal.z) {
            mMaxVal.z = val;
        }
    }
}

int dMirror_packet_c::entryModel(J3DModel* i_model) {
#if TARGET_PC
    if (mbReset) {
        mModelCount = 0;
        mbReset = false;
    }
#endif

    if (mModelCount >= 0x40) {
        return 0;
    }

    mModels[mModelCount++] = i_model;
#if TARGET_PC
    mbHadEntry = true;
#endif
    return 1;
}

void dMirror_packet_c::mirrorZdraw(f32* param_0, f32* param_1, f32 param_2, f32 param_3,
                                   f32 param_4, f32 param_5, f32 param_6, f32 param_7) {
    ZoneScoped;
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXColor color = {0, 0, 0, 0xFF};
    GXSetTevColor(GX_TEVREG0, color);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetZCompLoc(GX_TRUE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_OR);
    GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_OR, GX_GREATER, 0);
    GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, g_clearColor);
    GXSetFogRangeAdj(0, 0, NULL);
    GXSetCullMode(GX_CULL_BACK);
    GXSetNumIndStages(0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetZMode(GX_ENABLE, GX_GEQUAL, GX_ENABLE);
    GXLoadPosMtxImm(j3dSys.getViewMtx(), 0);
    GXSetCurrentMtx(0);

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    for (int i = 0; i < 4; i++) {
        GXPosition3f32(mQuad[i].x, mQuad[i].y, mQuad[i].z);
    }
    GXEnd();

    if (mViewScale.y > 0.0f) {
        GXSetZMode(GX_ENABLE, GX_ALWAYS, GX_ENABLE);
        GXSetColorUpdate(GX_DISABLE);
        GXSetAlphaUpdate(GX_DISABLE);
        Mtx44 mtx;
        C_MTXOrtho(mtx, param_1[1], param_1[1] + param_1[3], param_1[0], param_1[0] + param_1[2], 0,
                   100.0f);
        GXSetProjection(mtx, GX_ORTHOGRAPHIC);
        GXLoadPosMtxImm(mDoMtx_getIdentity(), 0);

        param_3 -= 1.0f;
        param_4 -= 1.0f;
        param_5 += 1.0f;
        param_6 += 1.0f;

        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        GXPosition3f32(param_3, param_4, -100.0f);
        GXPosition3f32(param_5, param_4, -100.0f);
        GXPosition3f32(param_5, param_6, -100.0f);
        GXPosition3f32(param_3, param_6, -100.0f);
        GXEnd();

        GXSetProjectionv(param_0);
        GXSetZMode(1, GX_ALWAYS, 1);
        GXLoadPosMtxImm(j3dSys.getViewMtx(), 0);

        if (GXGetTexObjWidth(&mTexObj)) {
            GXLoadTexObj(&mTexObj, GX_TEXMAP0);
            GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 60);
            GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
            GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
            GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE,
                            GX_TEVPREV);
            GXSetNumChans(0);
            GXSetNumTexGens(1);
            GXSetZCompLoc(GX_FALSE);
            GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_POS_XYZ, GX_S8, 0);
            GXBegin(GX_QUADS, GX_VTXFMT0, 4);

            static s8 const l_texCoord[4][2] = {
                {0x00, 0x00},
                {0x01, 0x00},
                {0x01, 0x01},
                {0x00, 0x01},
            };

            cXyz* quad = mQuad;
            const s8* texPtr = &l_texCoord[0][0];
            for (int i = 0; i < 4; quad++, texPtr += 2, i++) {
                GXPosition3f32(quad->x, quad->y, quad->z);
                GXTexCoord2s8(texPtr[0], texPtr[1]);
            }
        } else {
            GXBegin(GX_QUADS, GX_VTXFMT0, 4);
            cXyz* quad = mQuad;
            for (int i = 0; i < 4; quad++, i++) {
                GXPosition3f32(quad->x, quad->y, quad->z);
            }
        }

        GXEnd();
    } else {
        GXSetProjectionv(param_0);
    }

    GXSetColorUpdate(GX_ENABLE);
    GXSetAlphaUpdate(GX_DISABLE);
}

void dMirror_packet_c::modelDraw(J3DModel* i_model, Mtx param_1) {
    Mtx& model_mtx = i_model->getBaseTRMtx();
    cXyz sp28(model_mtx[0][3], model_mtx[1][3], model_mtx[2][3]);

    cXyz sp1C;
    cMtx_multVec(param_1, &sp28, &sp1C);

    cXyz sp10;
    cMtx_multVec(j3dSys.getViewMtx(), &sp28, &sp10);

    if (mViewScale.y > 0.0f && sp1C.z > sp10.z) {
        return;
    }

    dScnKy_env_light_c* kankyo = dKy_getEnvlight();
    JUT_ASSERT(0, kankyo != NULL);

    GXColor color = {0};
    color.r = kankyo->bg_amb_col[0].r;
    color.g = kankyo->bg_amb_col[0].g;
    color.b = kankyo->bg_amb_col[0].b;
    color.a = kankyo->bg_amb_col[0].a;

    J3DModelData* modelData = i_model->getModelData();
#if TARGET_PC
    j3dSys.setTexture(modelData->getTexture());
#endif
    u16 materialNum = modelData->getMaterialNum();
    for (u16 i = 0; i < materialNum; i++) {
        J3DMatPacket* matPacket = i_model->getMatPacket(i);
        J3DShapePacket* shapePacket = matPacket->getShapePacket();
        J3DShape* shape = shapePacket->getShape();

        if (!shape->checkFlag(1)) {
            J3DMaterial* material = modelData->getMaterialNodePointer(i);
            u32 texGenNum = material->getTexGenBlock()->getTexGenNum();
            u8 colorChanNum = material->getColorBlock()->getColorChanNum();
            u8 tevStageNum = material->getTevBlock()->getTevStageNum();
            u8 indTexStageNum = material->getIndBlock()->getIndTexStageNum();

            material->load();
            matPacket->callDL();
            shape->loadPreDrawSetting();
            if (shapePacket->getDisplayListObj() != NULL) {
                shapePacket->getDisplayListObj()->callDL();
            }

            GFSetGenMode2(texGenNum, (GXChannelID)colorChanNum, (GXTevStageID)tevStageNum,
                          indTexStageNum, GX_CULL_FRONT);
            GFSetChanAmbColor(GX_COLOR0, color);
            shapePacket->setBaseMtxPtr((Mtx*)param_1);
            shapePacket->drawFast();
            shapePacket->setBaseMtxPtr((Mtx*)j3dSys.getViewMtx());
        }
        shape->resetVcdVatCache();
    }
}

void dMirror_packet_c::mainDraw() {
    ZoneScoped;
    j3dSys.reinitGX();

    cXyz sp19C[5];

    Mtx sp16C;

    f32 sp150[7];
    GXGetProjectionv(sp150);

    f32 sp138[6];
    GXGetViewportv(sp138);

    u32 scissor[4];
    GXGetScissor(&scissor[0], &scissor[1], &scissor[2], &scissor[3]);

    f32 var_f31 = FLT_MAX;
    f32 var_f30 = FLT_MAX;
    f32 temp_f0 = -FLT_MAX;
    f32 var_f29 = temp_f0;
    f32 var_f28 = temp_f0;
    f32 var_f27 = temp_f0;

    f32 temp_f26 = scissor[0];
    f32 temp_f25 = temp_f26 + scissor[2];
    f32 temp_f24 = scissor[1];
    f32 temp_f23 = temp_f24 + scissor[3];

    int prjPosNum = 4;

    view_class* view = dComIfGd_getView();
    f32 temp_f22 = -view->near_;
    cXyz* var_r21 = mQuad;
    cXyz* var_r22 = sp19C;
    int var_r23 = 0;
    int var_r28 = 0;
    for (int i = 0; i < 4; i++) {
        cMtx_multVec(view->viewMtx, var_r21, var_r22);
        if (var_r22->z >= temp_f22) {
            var_r23++;
        } else {
            var_r28 = i;
        }

        var_r21++;
        var_r22++;
    }

    if (var_r23 < 4) {
        if (var_r23 != 0) {
            int var_r27 = -1;

            for (int i = 0; i < 4; i++) {
                int temp_r5 = (var_r28 + 1) % 4;
                if (var_r27 < 0) {
                    if (sp19C[temp_r5].z >= temp_f22) {
                        var_r27 = temp_r5;
                    }
                } else if (sp19C[temp_r5].z < temp_f22) {
                    int temp_r29 = (var_r27 + 3) % 4;
                    cXyz sp11C = sp19C[var_r27] - sp19C[temp_r29];
                    f32 temp_f1 = sp11C.z;
                    temp_f1 = (temp_f22 - sp19C[temp_r29].z) / temp_f1;
                    sp11C *= temp_f1;

                    JUT_ASSERT(0, 0 <= prjPosNum && prjPosNum < (4 + 1));

                    sp19C[4] = sp19C[temp_r29] + sp11C;

                    prjPosNum++;

                    sp11C = sp19C[var_r28] - sp19C[temp_r5];
                    sp11C *= (temp_f22 - sp19C[temp_r5].z) / sp11C.z;
                    sp19C[var_r28] = sp19C[temp_r5] + sp11C;

                    for (int j = var_r27; j != var_r28; j = (j + 1) % 4) {
                        sp19C[j] = sp19C[var_r28];
                        temp_r29++;
                    }
                    break;
                }

                var_r28 = temp_r5;
            }
        }

        f32 aspect = view->aspect;
        f32 temp_f0_2 = tanf(MTXDegToRad(view->fovy * 0.5f));
        view_port_class* view_port = dComIfGd_getViewport();

        f32 var_f3;
        f32 var_f4;
        f32 var_f5;
        f32 var_f6;
        if (view_port->x_orig != 0.0f) {
            var_f3 = (((view_port->x_orig * 2.0f) + view_port->width) * 0.5f) - (FB_WIDTH / 2);
            var_f5 = FB_WIDTH;
        } else {
            var_f3 = view_port->x_orig;
            var_f5 = view_port->width;
        }

        if (view_port->y_orig != 0.0f) {
            var_f4 = (((view_port->y_orig * 2.0f) + view_port->height) * 0.5f) - (FB_HEIGHT / 2);
            var_f6 = FB_HEIGHT;
        } else {
            var_f4 = view_port->y_orig;
            var_f6 = view_port->height;
        }

#if TARGET_PC
        // VR (2026-10-18): view->fovy/aspect are the flatscreen camera's, not
        // the headset's, so the screen rect the reflection is limited to was
        // wrong. Project the near-clipped corners with each eye's real
        // projection instead and take the bounding rect over both eyes (the
        // same logical rect is mapped into both halves).
        float vrProj[2][6];
        Mtx vrEyeT[2];
        const int vrEyes = dusk::vr::getMirrorStereoProjection(vrProj, vrEyeT);
        if (vrEyes > 0) {
            for (int i = 0; i < prjPosNum; i++) {
                if (sp19C[i].z > var_f27) {
                    var_f27 = sp19C[i].z;
                }
                for (int eye = 0; eye < vrEyes; eye++) {
                    cXyz p;
                    cMtx_multVec(vrEyeT[eye], &sp19C[i], &p);
                    const f32 w = -p.z;
                    if (w < 1e-3f) {
                        continue;
                    }
                    const f32 ndcX = (vrProj[eye][0] * p.x + vrProj[eye][1] * p.z) / w;
                    const f32 ndcY = (vrProj[eye][2] * p.y + vrProj[eye][3] * p.z) / w;
                    f32 sx = var_f3 + (1.0f + ndcX) * (var_f5 * 0.5f);
                    f32 sy = var_f4 + (1.0f - ndcY) * (var_f6 * 0.5f);
                    sx = cLib_minMaxLimit<f32>(sx, temp_f26, temp_f25);
                    sy = cLib_minMaxLimit<f32>(sy, temp_f24, temp_f23);
                    var_f31 = std::min(var_f31, sx);
                    var_f29 = std::max(var_f29, sx);
                    var_f30 = std::min(var_f30, sy);
                    var_f28 = std::max(var_f28, sy);
                }
            }
        } else
#endif
        {
        Vec* var_r3 = sp19C;
        for (int i = 0; i < prjPosNum; i++) {
            var_r3->y = (var_r3->y / (var_r3->z * temp_f0_2));
            var_r3->x = (var_r3->x / (aspect * (-var_r3->z * temp_f0_2)));
            var_r3->x = (var_f3 + ((1.0f + var_r3->x) * (var_f5 * 0.5f)));
            var_r3->y = (var_f4 + ((1.0f + var_r3->y) * (var_f6 * 0.5f)));

            var_r3->x = cLib_minMaxLimit<f32>(var_r3->x, temp_f26, temp_f25);
            var_r3->y = cLib_minMaxLimit<f32>(var_r3->y, temp_f24, temp_f23);

            if (var_r3->x < var_f31) {
                var_f31 = var_r3->x;
            }

            if (var_r3->x > var_f29) {
                var_f29 = var_r3->x;
            }

            if (var_r3->y < var_f30) {
                var_f30 = var_r3->y;
            }

            if (var_r3->y > var_f28) {
                var_f28 = var_r3->y;
            }

            if (var_r3->z > var_f27) {
                var_f27 = var_r3->z;
            }

            var_r3++;
        }
        }

        if (fabsf(var_f29 - var_f31) < 8.0f || fabsf(var_f28 - var_f30) < 8.0f) {
            return;
        }

        GXSetScissor((u32)var_f31, (u32)var_f30, (u32)(var_f29 - var_f31),
                     (u32)(var_f28 - var_f30));

        cXyz sp110 = mQuad[1] - mQuad[0];
        cXyz sp104 = mQuad[2] - mQuad[1];
        cXyz spF8 = sp110.outprod(sp104);
        spF8.normalizeZP();

        f32 temp_f7 = (spF8.x * mQuad[0].x) + (spF8.y * mQuad[0].y) + (spF8.z * mQuad[0].z);
        f32 temp_f22_4 =
            ((spF8.z * view->lookat.center.z) +
             ((spF8.x * view->lookat.center.x) + (spF8.y * view->lookat.center.y))) -
            temp_f7;

        cXyz spEC =
            view->lookat.eye -
            (spF8 * (2.0f * (((spF8.z * view->lookat.eye.z) +
                              ((spF8.x * view->lookat.eye.x) + (spF8.y * view->lookat.eye.y))) -
                             temp_f7)));
        cXyz spE0 = view->lookat.center - (spF8 * (2.0f * temp_f22_4));

        cXyz spD4(0.0f, 1.0f, 0.0f);
        if (mViewScale.y > 0.0f) {
            sp110 = spEC - view->lookat.eye;
            sp104 = sp110.outprod(view->lookat.up);
            spD4 = sp110.outprod(sp104);
            spD4.normalizeZP();
            spD4 *= cXyz(-1.0f, -1.0f, -1.0f);
        }

#if TARGET_PC
        if (vrEyes > 0) {
            // VR: the reflected camera above is built from the flatscreen
            // camera (view->lookat), which doesn't follow the headset, so the
            // reflection stayed fixed relative to the head. Reflect the
            // actual view through the mirror plane instead: view * R, with
            // R = reflection through n.x = d (n = spF8, d = temp_f7). Mirror
            // handedness is the same as the original (det -1), so the
            // GX_CULL_FRONT in modelDraw() still applies, and the stereo
            // per-eye correction is applied on top in the shader.
            Mtx refl;
            const f32 n[3] = {spF8.x, spF8.y, spF8.z};
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    refl[r][c] = (r == c ? 1.0f : 0.0f) - 2.0f * n[r] * n[c];
                }
                refl[r][3] = 2.0f * temp_f7 * n[r];
            }
            MTXConcat(j3dSys.getViewMtx(), refl, sp16C);
            mDoMtx_stack_c::copy(sp16C);
        } else
#endif
        {
            mDoMtx_lookAt(sp16C, &spEC, &spE0, &spD4, view->bank);
            mDoMtx_stack_c::scaleS(mViewScale);
            mDoMtx_stack_c::concat(sp16C);
        }
        J3DShape::resetVcdVatCache();

#if TARGET_PC
        // VR: the reflection shows Link's whole body (head, hat, ears), like
        // his shadow, not the first-person hidden version.
        const bool vrShownHidden = dusk::vr::beginMirrorShowsHiddenModels();
#endif
        for (int i = 0; i < mModelCount; i++) {
            modelDraw(mModels[i], mDoMtx_stack_c::get());
        }
#if TARGET_PC
        if (vrShownHidden) {
            dusk::vr::endMirrorShowsHiddenModels();
        }
#endif

        j3dSys.reinitGX();
        GXSetScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
        mirrorZdraw(sp150, sp138, view->far_, var_f31, var_f30, var_f29, var_f28, var_f27);
    }
}

void dMirror_packet_c::draw() {
    mDoLib_clipper::changeFar(dComIfGd_getView()->far_);
    if (!mDoLib_clipper::clip(j3dSys.getViewMtx(), &mMaxVal, &mMinVal)) {
        mainDraw();
    }

    mDoLib_clipper::resetFar();
    reset();
}

daMirror_c::daMirror_c() {
    m_entryModel = &daMirror_c::entryModel;
}

dMirror_packet_c::~dMirror_packet_c() {}

BOOL daMirror_c::createHeap() {
    J3DModelData* modelData = (J3DModelData*)dComIfG_getObjectRes(l_arcName, 4);
    mpModel = mDoExt_J3DModel__create(modelData, 0x80000, 0x11000084);
    if (mpModel == NULL) {
        return false;
    }

    return true;
}

void daMirror_c::setModelMtx() {
    mpModel->setBaseScale(scale);

    mDoMtx_stack_c::transS(current.pos.x, current.pos.y, current.pos.z);
    mDoMtx_stack_c::YrotM(shape_angle.y);
    mDoMtx_stack_c::XrotM(shape_angle.x);
    mDoMtx_stack_c::ZrotM(0x2000);
    mpModel->setBaseTRMtx(mDoMtx_stack_c::get());
}

static int daMirror_create(daMirror_c* i_this) {
    fopAcM_ct(i_this, daMirror_c);
    return i_this->daMirror_c::create();
}

int daMirror_c::create() {
    if (getSw() != 0xFF && !fopAcM_isSwitch(this, getSw())) {
        return cPhs_COMPLEATE_e;
    }

    if (m_myObj != NULL) {
        return cPhs_INIT_e;
    }

    int type = getType();
    if (type == 2) {
        int phase_state = dComIfG_resLoad(this, l_arcName);
        switch (phase_state) {
        default:
            return phase_state;
        case cPhs_COMPLEATE_e:
            if (!fopAcM_entrySolidHeap(this, daMirror_c_createHeap, 0x1540)) {
                return cPhs_ERROR_e;
            }
            break;
        }

        setModelMtx();
        mDoLib_setResTimgObj((ResTIMG*)dComIfG_getObjectRes(l_arcName, 7), &mPacket.getTexObj(), 0,
                             NULL);
        Vec src[4] = {
            {-72.5f, 145.0f, 0.0f},
            {72.5f, 145.0f, 0.0f},
            {72.5f, 0.0f, 0.0f},
            {-72.5f, 0.0f, 0.0f},
        };
        mDoMtx_stack_c::scaleS(scale);
        mDoMtx_stack_c::revConcat(mpModel->getBaseTRMtx());
        cMtx_multVecArray(mDoMtx_stack_c::get(), src, mPacket.getQuad(), 4);
        mPacket.getViewScale().set(-1.0f, 1.0f, 1.0f);
    } else {
        if (type == 1) {
            scale *= 10.0f;
            mPacket.getViewScale().set(1.0, -1.0, 1.0);
        } else {
            if (type == 3) {
                int phase_state = dComIfG_resLoad(this, l_arcName2);
                switch (phase_state) {
                default:
                    return phase_state;
                case cPhs_COMPLEATE_e:
                    mDoLib_setResTimgObj((ResTIMG*)dComIfG_getObjectRes(l_arcName2, 0x25),
                                         &mPacket.getTexObj(), 0, NULL);
                    break;
                }
            }

            mPacket.getViewScale().set(-1.0f, 1.0f, 1.0f);
        }

        mDoMtx_stack_c::transS(current.pos.x, current.pos.y, current.pos.z);
        mDoMtx_stack_c::YrotM(shape_angle.y);
        mDoMtx_stack_c::XrotM(shape_angle.x);
        mDoMtx_stack_c::scaleM(scale.x, scale.y, scale.z);
        static Vec const l_mirrorQuad[4] = {
            {-50.0f, 100.0f, 0.0f},
            {50.0f, 100.0f, 0.0f},
            {50.0f, 0.0f, 0.0f},
            {-50.0f, 0.0f, 0.0f},
        };
        mDoMtx_stack_c::multVecArray(l_mirrorQuad, mPacket.getQuad(), 4);
    }

    m_myObj = this;
    mPacket.calcMinMax();
    static const Vec l_mirrorLook = {0.0f, 50.0f, 0.0f};
    mDoMtx_stack_c::multVec(&l_mirrorLook, &eyePos);
    return cPhs_COMPLEATE_e;
}

int daMirror_c::Delete() {
    int type = getType();
    if (type == 2) {
        dComIfG_resDelete(this, l_arcName);
    } else if (type == 3) {
        dComIfG_resDelete(this, l_arcName2);
    }

    m_myObj = NULL;
    return 1;
}

static int daMirror_Delete(daMirror_c* i_this) {
    return i_this->Delete();
}

static int daMirror_execute(daMirror_c* i_this) {
    return i_this->execute();
}

int daMirror_c::execute() {
    if (this != m_myObj) {
        if (m_myObj == NULL) {
            if (create() == cPhs_ERROR_e) {
                fopAcM_delete(this);
            }
        }

        return 1;
    }

    daPy_py_c* player = daPy_getLinkPlayerActorClass();
    JUT_ASSERT(0, player != NULL);

    if (mPacket.getViewScale().y > 0.0f && player->getKandelaarFlamePos() &&
        fopAcM_searchActorDistance2(this, player) < 40000.0f)
    {
        if (fopAcM_seenActorAngleY(this, player) < 0x4000) {
            daPy_py_c::setLookPos(&eyePos);
        }
    }

    return 1;
}

int daMirror_c::draw() {
    if (this != daMirror_c::m_myObj) {
        return 1;
    }

    if (mpModel != NULL) {
        g_env_light.settingTevStruct(0x10, &current.pos, &tevStr);
        g_env_light.setLightTevColorType(mpModel, &tevStr);
        mDoExt_modelUpdateDL(mpModel);
    }

#if TARGET_PC
    if (mPacket.mbReset && !mPacket.mbHadEntry) {
        mPacket.mModelCount = 0;
    }
    mPacket.mbHadEntry = true;
#endif
    dComIfGd_getOpaListBG()->entryImm(&mPacket, 0);
    return 1;
}

static int daMirror_draw(daMirror_c* i_this) {
    return i_this->draw();
}

int daMirror_c::entryModel(J3DModel* i_model) {
    return mPacket.entryModel(i_model);
}

static DUSK_CONST actor_method_class daMirror_METHODS = {
    (process_method_func)daMirror_create,  (process_method_func)daMirror_Delete,
    (process_method_func)daMirror_execute, (process_method_func)NULL,
    (process_method_func)daMirror_draw,
};

DUSK_PROFILE actor_process_profile_definition DUSK_CONST g_profile_MIRROR = {
    /* Layer ID     */ fpcLy_CURRENT_e,
    /* List ID      */ 7,
    /* List Prio    */ fpcPi_CURRENT_e,
    /* Proc Name    */ fpcNm_MIRROR_e,
    /* Proc SubMtd  */ &g_fpcLf_Method.base,
    /* Size         */ sizeof(daMirror_c),
    /* Size Other   */ 0,
    /* Parameters   */ 0,
    /* Leaf SubMtd  */ &g_fopAc_Method.base,
    /* Draw Prio    */ fpcDwPi_MIRROR_e,
    /* Actor SubMtd */ &daMirror_METHODS,
    /* Status       */ fopAcStts_UNK_0x40000_e | fopAcStts_NOPAUSE_e,
    /* Group        */ fopAc_UNK_GROUP_5_e,
    /* Cull Type    */ fopAc_CULLBOX_0_e,
};
