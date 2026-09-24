// Copyright (c) 2026 Pixelforge Ports contributors
#include "nova2.h"
#include "native_bindings.h"
#include "input_bridge.h"
#include "app_exit.h"
#include "port_env.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <strings.h>

namespace {
// GLGame.onKeyDown passes KeyEvent.getScanCode(), not getKeyCode().
enum XperiaScanCode {
    KEY_BACK = 158,
    KEY_DPAD_UP = 103, KEY_DPAD_DOWN = 108,
    KEY_DPAD_LEFT = 105, KEY_DPAD_RIGHT = 106,
    KEY_MENU = 139,
    KEY_BUTTON_A = 305, KEY_BUTTON_B = 304, // circle / cross
    KEY_BUTTON_X = 308, KEY_BUTTON_Y = 307, // triangle / square
    KEY_BUTTON_L1 = 310, KEY_BUTTON_R1 = 311,
    KEY_BUTTON_L2 = 310, KEY_BUTTON_R2 = 311,
    KEY_BUTTON_L3 = 0, KEY_BUTTON_R3 = 0,
    KEY_BUTTON_START = 28, KEY_BUTTON_SELECT = 314,
};

using TouchPadFn = donor::GLGame_nativeTouchPadMoved_14;
static donor::GLGame_nativeSetOnKeyDown_10 key_down;
static donor::GLGame_nativeSetOnKeyUp_11 key_up;
static donor::GLGame_nativeTouchPressed_17 touch_down;
static donor::GLGame_nativeTouchReleased_18 touch_up;
static donor::GLGame_nativeTouchMoved_13 touch_move;
static TouchPadFn touchpad_move;
static TouchPadFn touchpad_down;
static TouchPadFn touchpad_up;
static SDL_GameController *pad = nullptr;
static std::map<int, bool> held;
static float cx = 320, cy = 240;
static float lx = 0, ly = 0, rx = 0, ry = 0;
static bool touching = false;
static bool cursor_mode = true;
static bool dpad_up = false, dpad_down = false;
static bool dpad_left = false, dpad_right = false;
static bool shoulders[2] = {}, triggers[2] = {};
static int *control_scheme = nullptr;

struct VirtualPad {
    int id;
    float center_x;
    float center_y;
    float radius_x;
    float radius_y;
    bool held;
};

// Physical Xperia touchpad coordinates; independent of display resolution.
static VirtualPad move_pad{0, 180, 180, 170, 170, false};
static VirtualPad aim_pad{1, 786, 180, 170, 170, false};
static float aim_x = 786, aim_y = 180;

static void key(int code, bool down) {
    if (!code || held[code] == down) return;
    held[code] = down;
    (down ? key_down : key_up)(nova_env, (jclass)&game_class, code);
}

static int face_layout_button(int button) {
    static int xbox = -1;
    if (xbox < 0) {
        const char *layout = port_getenv("FACE_LAYOUT");
        xbox = layout && !strcasecmp(layout, "xbox");
    }
    if (!xbox) return button;
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return SDL_CONTROLLER_BUTTON_B;
        case SDL_CONTROLLER_BUTTON_B: return SDL_CONTROLLER_BUTTON_A;
        case SDL_CONTROLLER_BUTTON_X: return SDL_CONTROLLER_BUTTON_Y;
        case SDL_CONTROLLER_BUTTON_Y: return SDL_CONTROLLER_BUTTON_X;
        default: return button;
    }
}

static void enter_gameplay(void) {
    cursor_mode = false;
    // notifyTouchPad* accepts input only for the Xperia scheme (8).
    if (control_scheme) *control_scheme = 8;
}

static void enter_menu(void) {
    cursor_mode = true;
    if (cx < 0 || cy < 0) {
        cx = nova_width * .5f;
        cy = nova_height * .5f;
    }
}

static float axis_value(Sint16 value) {
    float v = value / 32768.f;
    float magnitude = std::fabs(v);
    if (magnitude < .20f) return 0;
    return std::copysign((magnitude - .20f) / .80f, v);
}

static void update_virtual_pad(VirtualPad &stick, float x, float y) {
    const bool active = std::fabs(x) > .08f || std::fabs(y) > .08f;
    const int center_x = int(stick.center_x);
    const int center_y = int(stick.center_y);
    const int px = center_x + int(x * stick.radius_x);
    const int py = center_y + int(y * stick.radius_y);

    if (active && !stick.held) {
        touchpad_down(nova_env, (jclass)&game_class, center_x, center_y, stick.id);
        stick.held = true;
    }
    if (active) {
        touchpad_move(nova_env, (jclass)&game_class, px, py, stick.id);
    } else if (stick.held) {
        touchpad_up(nova_env, (jclass)&game_class, px, py, stick.id);
        stick.held = false;
    }
}

static void update_aim(float x, float y, float dt) {
    if (x == 0 && y == 0) {
        update_virtual_pad(aim_pad, 0, 0);
        aim_x = 786; aim_y = 180;
        return;
    }
    // The right pad consumes relative deltas. Rebase at its edge without
    // sending a reverse movement, then continue dragging while the stick holds.
    const float dx = x * 300.f * std::clamp(dt, 0.f, .05f);
    const float dy = y * 300.f * std::clamp(dt, 0.f, .05f);
    if (!aim_pad.held || aim_x + dx < 616 || aim_x + dx > 956 ||
        aim_y + dy < 10 || aim_y + dy > 350) {
        update_virtual_pad(aim_pad, 0, 0);
        aim_x = 786; aim_y = 180;
        touchpad_down(nova_env, (jclass)&game_class, 786, 180, aim_pad.id);
        aim_pad.held = true;
    }
    aim_x += dx; aim_y += dy;
    touchpad_move(nova_env, (jclass)&game_class, int(aim_x), int(aim_y), aim_pad.id);
}

static void shoulder(int side, bool trigger, bool down) {
    (trigger ? triggers : shoulders)[side] = down;
    if (down) enter_gameplay();
    key(side ? KEY_BUTTON_R1 : KEY_BUTTON_L1, triggers[side] || shoulders[side]);
}

static void release_virtual_pads(void) {
    update_virtual_pad(move_pad, 0, 0);
    update_virtual_pad(aim_pad, 0, 0);
}

static void set_dpad(int button, bool down) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP: dpad_up = down; break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: dpad_down = down; break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: dpad_left = down; break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: dpad_right = down; break;
    }
}

static void handle_button(int raw_button, bool down) {
    const int button = face_layout_button(raw_button);
    if (button == SDL_CONTROLLER_BUTTON_DPAD_UP ||
        button == SDL_CONTROLLER_BUTTON_DPAD_DOWN ||
        button == SDL_CONTROLLER_BUTTON_DPAD_LEFT ||
        button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
        if (cursor_mode) set_dpad(button, down);
        else {
            const int code = button == SDL_CONTROLLER_BUTTON_DPAD_UP ? KEY_DPAD_UP :
                button == SDL_CONTROLLER_BUTTON_DPAD_DOWN ? KEY_DPAD_DOWN :
                button == SDL_CONTROLLER_BUTTON_DPAD_LEFT ? KEY_DPAD_LEFT : KEY_DPAD_RIGHT;
            key(code, down);
        }
        return;
    }

    switch (button) {
        case SDL_CONTROLLER_BUTTON_A:
            if (cursor_mode) android_input_cursor_press(down);
            else { if (down) enter_gameplay(); key(KEY_BUTTON_A, down); }
            return;
        case SDL_CONTROLLER_BUTTON_B:
            if (cursor_mode) key(KEY_BACK, down);
            else { if (down) enter_gameplay(); key(KEY_BUTTON_B, down); }
            return;
        case SDL_CONTROLLER_BUTTON_X:
            if (down) enter_gameplay();
            key(KEY_BUTTON_X, down);
            return;
        case SDL_CONTROLLER_BUTTON_Y:
            if (down) enter_gameplay();
            key(KEY_BUTTON_Y, down);
            return;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            shoulder(0, false, down);
            return;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            shoulder(1, false, down);
            return;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
            if (down) enter_gameplay();
            key(KEY_BUTTON_L3, down);
            return;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
            if (down) enter_gameplay();
            key(KEY_BUTTON_R3, down);
            return;
        case SDL_CONTROLLER_BUTTON_START:
            if (down) enter_menu();
            key(KEY_MENU, down);
            return;
        case SDL_CONTROLLER_BUTTON_BACK:
            key(KEY_BUTTON_SELECT, down); return;
        default: return;
    }
}

static void update_trigger(int axis, float value) {
    shoulder(axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ? 0 : 1, true, value > .40f);
}
} // namespace

void input_init() {
    control_scheme = reinterpret_cast<int *>(so_symbol(nova_module, "nCurrentControlScheme"));
    key_down = native<donor::GLGame_nativeSetOnKeyDown_10>("GLGame_nativeSetOnKeyDown");
    key_up = native<donor::GLGame_nativeSetOnKeyUp_11>("GLGame_nativeSetOnKeyUp");
    touch_down = native<donor::GLGame_nativeTouchPressed_17>("GLGame_nativeTouchPressed");
    touch_up = native<donor::GLGame_nativeTouchReleased_18>("GLGame_nativeTouchReleased");
    touch_move = native<donor::GLGame_nativeTouchMoved_13>("GLGame_nativeTouchMoved");
    touchpad_move = native<donor::GLGame_nativeTouchPadMoved_14>("GLGame_nativeTouchPadMoved");
    touchpad_down = native<donor::GLGame_nativeTouchPadPressed_15>("GLGame_nativeTouchPadPressed");
    touchpad_up = native<donor::GLGame_nativeTouchPadReleased_16>("GLGame_nativeTouchPadReleased");
    cx = nova_width * .5f;
    cy = nova_height * .5f;
    SDL_GameControllerEventState(SDL_ENABLE);
    for (int n = 0; n < SDL_NumJoysticks(); ++n) {
        if (SDL_IsGameController(n)) {
            pad = SDL_GameControllerOpen(n);
            if (pad) break;
        }
    }
    SDL_Log("NOVA2 input: Xperia scancodes and relative touchpad; controller %s",
            pad ? SDL_GameControllerName(pad) : "not detected");
}

extern "C" void android_input_cursor_position(float *x, float *y, int *visible) {
    if (x) *x = cx;
    if (y) *y = cy;
    if (visible) *visible = cursor_mode ? 1 : 0;
}

void android_input_cursor_set(float x, float y) {
    enter_menu();
    cx = std::clamp(x, 0.f, float(nova_width - 1));
    cy = std::clamp(y, 0.f, float(nova_height - 1));
    if (touching) touch_move(nova_env, (jclass)&game_class, cx, cy, 0);
}

void android_input_cursor_press(bool down) {
    if (touching == down) return;
    touching = down;
    (down ? touch_down : touch_up)(nova_env, (jclass)&game_class, cx, cy, 0);
}

bool android_input_inject_control(const char *name, bool down) {
    if (!name) return false;
    if (!strcmp(name, "l1") || !strcmp(name, "l2") ||
        !strcmp(name, "r1") || !strcmp(name, "r2")) {
        shoulder(name[0] == 'r', name[1] == '2', down);
        return true;
    }
    if (!strcmp(name, "a")) {
        if (cursor_mode) android_input_cursor_press(down);
        else key(KEY_BUTTON_A, down);
        return true;
    }
    if (!strcmp(name, "b")) {
        key(cursor_mode ? KEY_BACK : KEY_BUTTON_B, down);
        return true;
    }
    struct Binding { const char *name; int code; };
    static const Binding bindings[] = {
        {"x", KEY_BUTTON_X}, {"y", KEY_BUTTON_Y},
        {"l1", KEY_BUTTON_L1}, {"r1", KEY_BUTTON_R1},
        {"l2", KEY_BUTTON_L2}, {"r2", KEY_BUTTON_R2},
        {"start", KEY_MENU}, {"back", KEY_BUTTON_SELECT},
        {"select", KEY_BUTTON_SELECT},
    };
    for (const auto &binding : bindings) {
        if (!strcmp(name, binding.name)) {
            if (down && binding.code != KEY_BUTTON_SELECT) enter_gameplay();
            key(binding.code, down);
            return true;
        }
    }
    if (!strcmp(name, "up") || !strcmp(name, "down") ||
        !strcmp(name, "left") || !strcmp(name, "right")) {
        const int code = !strcmp(name, "up") ? KEY_DPAD_UP :
            !strcmp(name, "down") ? KEY_DPAD_DOWN :
            !strcmp(name, "left") ? KEY_DPAD_LEFT : KEY_DPAD_RIGHT;
        key(code, down);
        return true;
    }
    if (!strcmp(name, "mouse_left")) {
        android_input_cursor_press(down);
        return true;
    }
    return false;
}

bool android_input_inject_stick(const char *name, float x, float y) {
    if (!name) return false;
    if (!strcmp(name, "left")) {
        lx = x; ly = y;
        if (std::fabs(x) > .20f || std::fabs(y) > .20f) enter_gameplay();
        return true;
    }
    if (!strcmp(name, "right")) {
        rx = x; ry = y;
        return true;
    }
    return false;
}

void input_event(const SDL_Event &event) {
    if (event.type == SDL_CONTROLLERDEVICEADDED && !pad &&
        SDL_IsGameController(event.cdevice.which)) {
        pad = SDL_GameControllerOpen(event.cdevice.which);
        if (pad) SDL_Log("NOVA2 input: controller connected: %s", SDL_GameControllerName(pad));
    }
    const bool disconnected = event.type == SDL_CONTROLLERDEVICEREMOVED && pad &&
        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad)) == event.cdevice.which;
    if (disconnected || (event.type == SDL_WINDOWEVENT &&
                        event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)) {
        for (auto &entry : held) if (entry.second) key(entry.first, false);
        release_virtual_pads();
        android_input_cursor_press(false);
        shoulders[0] = shoulders[1] = triggers[0] = triggers[1] = false;
        dpad_up = dpad_down = dpad_left = dpad_right = false;
        lx = ly = rx = ry = 0;
        if (disconnected) {
            SDL_GameControllerClose(pad);
            pad = nullptr;
            SDL_Log("NOVA2 input: controller disconnected");
        }
    }
    if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
        const bool down = event.type == SDL_CONTROLLERBUTTONDOWN;
        handle_button(event.cbutton.button, down);
    }
    if (event.type == SDL_CONTROLLERAXISMOTION) {
        const float value = axis_value(event.caxis.value);
        switch (event.caxis.axis) {
            case SDL_CONTROLLER_AXIS_LEFTX: lx = value; break;
            case SDL_CONTROLLER_AXIS_LEFTY: ly = value; break;
            case SDL_CONTROLLER_AXIS_RIGHTX: rx = value; break;
            case SDL_CONTROLLER_AXIS_RIGHTY: ry = value; break;
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: update_trigger(event.caxis.axis, value); break;
            default: break;
        }
        if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ||
            event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
            if (std::fabs(lx) > .20f || std::fabs(ly) > .20f) enter_gameplay();
        }
    }
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        if (event.key.repeat) return;
        const bool down = event.type == SDL_KEYDOWN;
        switch (event.key.keysym.sym) {
            case SDLK_UP: dpad_up = down; break;
            case SDLK_DOWN: dpad_down = down; break;
            case SDLK_LEFT: dpad_left = down; break;
            case SDLK_RIGHT: dpad_right = down; break;
            case SDLK_SPACE:
                if (cursor_mode) android_input_cursor_press(down);
                else key(KEY_BUTTON_A, down);
                break;
            case SDLK_LCTRL: key(cursor_mode ? KEY_BACK : KEY_BUTTON_B, down); break;
            case SDLK_r: key(KEY_BUTTON_X, down); break;
            case SDLK_e: key(KEY_BUTTON_Y, down); break;
            case SDLK_ESCAPE: key(KEY_MENU, down); break;
            case SDLK_RETURN: enter_menu(); key(KEY_MENU, down); break;
            case SDLK_F12: if (down) android_app_request_exit("exit key"); break;
        }
    }
    if (event.type == SDL_MOUSEMOTION)
        android_input_cursor_set(event.motion.x, event.motion.y);
    if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) &&
        event.button.button == SDL_BUTTON_LEFT) {
        android_input_cursor_set(event.button.x, event.button.y);
        android_input_cursor_press(event.type == SDL_MOUSEBUTTONDOWN);
    }
}

void input_tick(float dt) {
    if (pad) {
        SDL_GameControllerUpdate();
        lx = axis_value(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX));
        ly = axis_value(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY));
        rx = axis_value(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTX));
        ry = axis_value(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTY));
        update_trigger(SDL_CONTROLLER_AXIS_TRIGGERLEFT,
                       axis_value(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT)));
        update_trigger(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
                       axis_value(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT)));
    }

    if (std::fabs(lx) > .20f || std::fabs(ly) > .20f) enter_gameplay();
    if (cursor_mode) {
        const float mx = rx + float(dpad_right) - float(dpad_left);
        const float my = ry + float(dpad_down) - float(dpad_up);
        const float magnitude = std::hypot(mx, my);
        if (magnitude > 0) {
            const float scale = magnitude > 1 ? 1 / magnitude : 1;
            cx = std::clamp(cx + mx * scale * 420.f * dt, 0.f, float(nova_width - 1));
            cy = std::clamp(cy + my * scale * 420.f * dt, 0.f, float(nova_height - 1));
            if (touching) touch_move(nova_env, (jclass)&game_class, cx, cy, 0);
        }
        release_virtual_pads();
    } else {
        dpad_up = dpad_down = dpad_left = dpad_right = false;
        update_virtual_pad(move_pad, lx, ly);
        update_aim(rx, ry, dt);
    }
}
