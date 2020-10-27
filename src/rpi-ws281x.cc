#include <stdio.h>

#include "napi.h"

extern "C" {
  #include "rpi_ws281x/ws2811.h"
}

#define TARGET_FREQ     800000
#define GPIO_PIN_LEFT   18
#define GPIO_PIN_RIGHT  21
#define DMANUM          5

ws2811_t lstring, rstring;
ws2811_channel_t l0data, l1data, r0data, r1data;

Napi::Value init(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() != 1) {
        std::string err = "Wrong number of arguments " + info.Length();
        Napi::TypeError::New(env, err.c_str()).ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Wrong type number of leds").ThrowAsJavaScriptException();
        return env.Null();
    }

    lstring.freq = TARGET_FREQ;
    lstring.dmanum = DMANUM;

    l0data.gpionum = GPIO_PIN_LEFT;
    l0data.invert = 0;
    l0data.count = 0;
    l0data.brightness = 255;

    l1data.gpionum = 0;
    l1data.invert = 0;
    l1data.count = 0;
    l1data.brightness = 255;

    lstring.channel[0] = l0data;
    lstring.channel[1] = l1data;

    lstring.channel[0].count = info[0].As<Napi::Number>().Int32Value();

    ws2811_return_t err = ws2811_init(&lstring);

    if (err) {
        printf("error initializing %i %s\n", err, ws2811_get_return_t_str(err));
        Napi::TypeError::New(env, "Init Error: Left Led").ThrowAsJavaScriptException();
    }

    rstring.freq = TARGET_FREQ;
    rstring.dmanum = DMANUM;

    r0data.gpionum = GPIO_PIN_RIGHT;
    r0data.invert = 0;
    r0data.count = 0;
    r0data.brightness = 255;

    r1data.gpionum = 0;
    r1data.invert = 0;
    r1data.count = 0;
    r1data.brightness = 255;

    rstring.channel[0] = r0data;
    rstring.channel[1] = r1data;

    rstring.channel[0].count = info[0].As<Napi::Number>().Int32Value();

    err = ws2811_init(&rstring);

    if (err) {
        printf("error initializing %i %s\n", err, ws2811_get_return_t_str(err));
        Napi::TypeError::New(env, "Init Error: Right Led").ThrowAsJavaScriptException();
    }

    return env.Null();
}

Napi::Value setBrightness(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() != 1) {
        std::string err = "Wrong number of arguments " + info.Length();
        Napi::TypeError::New(env, err.c_str()).ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsNumber()) {
        Napi::TypeError::New(env, "Wrong type argument 0").ThrowAsJavaScriptException();
        return env.Null();
    }
    int32_t brightness = info[0].As<Napi::Number>().Int32Value();

    lstring.channel[0].brightness = brightness;
    rstring.channel[0].brightness = brightness;

    return env.Null();
}

Napi::Value reset(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    memset(lstring.channel[0].leds, 0, sizeof(*lstring.channel[0].leds) * lstring.channel[0].count);
    memset(rstring.channel[0].leds, 0, sizeof(*rstring.channel[0].leds) * rstring.channel[0].count);

    ws2811_render(&lstring);
    ws2811_wait(&lstring);
    ws2811_fini(&lstring);

    ws2811_render(&rstring);
    ws2811_wait(&rstring);
    ws2811_fini(&rstring);

    return env.Null();
}

Napi::Value render(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    if (info.Length() != 2) {
        std::string err = "Wrong number of arguments " + info.Length();
        Napi::TypeError::New(env, err.c_str()).ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsTypedArray()) {
        Napi::TypeError::New(env, "Wrong type argument 0").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Uint32Array lvalues = info[0].As<Napi::Uint32Array>();
    Napi::Uint32Array rvalues = info[1].As<Napi::Uint32Array>();

    if (lvalues.ByteLength() < sizeof(*lstring.channel[0].leds) * lstring.channel[0].count) {
        Napi::TypeError::New(env, "Wrong length for left led").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (rvalues.ByteLength() < sizeof(*rstring.channel[0].leds) * rstring.channel[0].count) {
        Napi::TypeError::New(env, "Wrong length for right led").ThrowAsJavaScriptException();
        return env.Null();
    }

    memcpy(lstring.channel[0].leds, lvalues.Data(), sizeof(*lstring.channel[0].leds) * lstring.channel[0].count);
    memcpy(rstring.channel[0].leds, rvalues.Data(), sizeof(*rstring.channel[0].leds) * rstring.channel[0].count);

    ws2811_wait(&lstring);
    ws2811_render(&lstring);

    ws2811_wait(&rstring);
    ws2811_render(&rstring);

    return env.Null();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "init"), Napi::Function::New(env, init));
    exports.Set(Napi::String::New(env, "setBrightness"), Napi::Function::New(env, setBrightness));
    exports.Set(Napi::String::New(env, "reset"), Napi::Function::New(env, reset));
    exports.Set(Napi::String::New(env, "render"), Napi::Function::New(env, render));

    return exports;
}

NODE_API_MODULE(wrapper, Init)
