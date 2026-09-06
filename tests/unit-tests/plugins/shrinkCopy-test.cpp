#include <catch2/catch_test_macros.hpp>
#include "shrinkCopy/ShrinkCopy.h"
#include <vector>
#include <filesystem>
#include "SysInitImpl.h"
#include "SysInitIntf.h"

namespace {
class ImageLayer : public tTJSDispatch {
public:
    long width, height;
    bool reversed;
    std::vector<std::uint32_t> pixels;
    ImageLayer(long w, long h, bool reverse = false)
        : width(w), height(h), reversed(reverse), pixels(w*h, 0) {}
    tjs_error IsInstanceOf(tjs_uint32, const tjs_char*, tjs_uint32*,
                          const tjs_char* name, iTJSDispatch2*) override {
        return ttstr(name) == TJS_W("Layer") ? TJS_S_TRUE : TJS_S_FALSE;
    }
    tjs_error PropGet(tjs_uint32, const tjs_char* name, tjs_uint32*,
                      tTJSVariant* result, iTJSDispatch2*) override {
        ttstr key(name);
        if(key == TJS_W("hasImage")) *result = tjs_int(1);
        else if(key == TJS_W("imageWidth")) *result = tjs_int(width);
        else if(key == TJS_W("imageHeight")) *result = tjs_int(height);
        else if(key == TJS_W("mainImageBufferPitch"))
            *result = tjs_int(width * 4 * (reversed ? -1 : 1));
        else if(key == TJS_W("mainImageBuffer") || key == TJS_W("mainImageBufferForWrite"))
            *result = tjs_int64(reinterpret_cast<tjs_intptr_t>(pixels.data() + (reversed ? (height-1)*width : 0)));
        else return TJS_E_MEMBERNOTFOUND;
        return TJS_S_OK;
    }
    tjs_error FuncCall(tjs_uint32, const tjs_char* name, tjs_uint32*,
                      tTJSVariant*, tjs_int count, tTJSVariant** args,
                      iTJSDispatch2*) override {
        if(ttstr(name) != TJS_W("setImageSize") || count != 2) return TJS_E_MEMBERNOTFOUND;
        width=args[0]->AsInteger(); height=args[1]->AsInteger();
        pixels.resize(width*height); return TJS_S_OK;
    }
};
}

TEST_CASE("shrinkCopy averages four-byte RGBA pixels to one pixel") {
    ImageLayer src(2,2), dst(1,1);
    src.pixels={0x00000000,0x40404040,0x80808080,0xc0c0c0c0};
    ShrinkCopy copy(&dst,0,0,1,1,&src,0,0,2,2);
    REQUIRE(copy.check()); REQUIRE(copy.clip()); copy.copy();
    REQUIRE(dst.pixels[0] == 0x60606060);
}
TEST_CASE("shrinkCopy supports negative pitch and clipped destinations") {
    ImageLayer src(4,4,true), dst(2,2,true);
    std::fill(src.pixels.begin(),src.pixels.end(),0xff2468ac);
    ShrinkCopy copy(&dst,-1,0,2,2,&src,0,0,4,4);
    REQUIRE(copy.check()); REQUIRE(copy.clip()); copy.copy();
    REQUIRE(dst.pixels[0] == 0xff2468ac);
    REQUIRE(dst.pixels[2] == 0xff2468ac);
    REQUIRE(dst.pixels[1] == 0);
    REQUIRE(dst.pixels[3] == 0);
}
TEST_CASE("shrinkCopy rejects enlargement") {
    ImageLayer src(2,2), dst(4,4);
    ShrinkCopy copy(&dst,0,0,4,4,&src,0,0,2,2);
    REQUIRE_FALSE(copy.check());
}
TEST_CASE("shrinkCopyFast preserves remainder edges and makes pixels opaque") {
    ImageLayer src(3,3), dst(1,1);
    std::fill(src.pixels.begin(),src.pixels.end(),0x102468ac);
    LimitedShrink copy(&dst,&src,2,0);
    REQUIRE(copy.check()); REQUIRE(copy.resize()); copy.copy();
    REQUIRE(dst.width==2); REQUIRE(dst.height==2);
    for(auto pixel:dst.pixels) REQUIRE(pixel==0xff2468ac);
}
TEST_CASE("shrinkCopy plugin registers callable Layer methods") {
    TVPNativeProjectDir = ttstr((std::filesystem::current_path().string()+"/").c_str());
    TVPProjectDir = TVPNormalizeStorageName(TVPNativeProjectDir);
    TVPSetCurrentDirectory(TVPProjectDir);
    TVPInitScriptEngine();
    ncbAutoRegister::AllRegist();
    REQUIRE(ncbAutoRegister::LoadModule(TJS_W("shrinkCopy.dll")));
    tTJSVariant layer;
    auto* global=TVPGetScriptDispatch();
    REQUIRE(TJS_SUCCEEDED(global->PropGet(0,TJS_W("Layer"),nullptr,&layer,global)));
    global->Release();
    auto* klass=layer.AsObjectNoAddRef();
    tTJSVariant method;
    REQUIRE(TJS_SUCCEEDED(klass->PropGet(0,TJS_W("shrinkCopy"),nullptr,&method,klass)));
    ImageLayer src(2,2),dst(1,1);
    src.pixels={0x00000000,0x40404040,0x80808080,0xc0c0c0c0};
    tTJSVariant a[]={tjs_int(0),tjs_int(0),tjs_int(1),tjs_int(1),tTJSVariant(&src),tjs_int(0),tjs_int(0),tjs_int(2),tjs_int(2)};
    tTJSVariant* args[]={&a[0],&a[1],&a[2],&a[3],&a[4],&a[5],&a[6],&a[7],&a[8]};
    REQUIRE(method.AsObjectNoAddRef()->FuncCall(0,nullptr,nullptr,nullptr,8,args,&dst)==TJS_E_BADPARAMCOUNT);
    REQUIRE(TJS_SUCCEEDED(method.AsObjectNoAddRef()->FuncCall(0,nullptr,nullptr,nullptr,9,args,&dst)));
    REQUIRE(dst.pixels[0]==0x60606060);
    REQUIRE(TJS_SUCCEEDED(klass->PropGet(0,TJS_W("shrinkCopyFast"),nullptr,&method,klass)));
}
