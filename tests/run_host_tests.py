"""Run firmware regressions with --compiler pointing to a host g++ executable."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(__doc__)
parser.add_argument('--compiler', required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
env = dict(os.environ)
env['PATH'] = str(Path(args.compiler).resolve().parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory() as temporary:
    def run(name, sources, includes=()):
        executable = Path(temporary) / (name + '.exe')
        subprocess.run([args.compiler, '-std=c++14', *includes, *map(str, sources), '-o', str(executable)], check=True, env=env, cwd=root)
        subprocess.run([str(executable)], check=True, env=env)
        print(name + ' passed', flush=True)
    run('viseme_scores', ['tests/viseme_scores.cpp', 'src/Renderer/VisemeCore.cpp'])
    run('fan_pwm', ['tests/fan_pwm.cpp', 'src/Devices/Fan/Fan.cpp'], ['-Itests/host', '-Isrc'])
    run('boop', ['tests/boop.cpp', 'src/Renderer/Boop.cpp'], ['-Itests/host', '-Isrc'])
    display = (root / 'src/Devices/LEDMatrixDisplay/Hub75DMA.cpp').read_text()
    constructor = display[display.index('    ready = matrix->begin()'):display.index('int Hub75DMA::getRefreshRate()')]
    queue = (root / 'src/KMMXController/ControllerControl.cpp').read_text()
    queue = queue[queue.index('bool KMMXController::queueControl('):]
    test = Path(temporary) / 'startup_queue.cpp'
    test.write_text(r"""
#include <cassert>
#include <atomic>
#include <deque>
struct SerialStub { void println(const char*) {} } Serial;
bool succeeds; int refresh, touches;
struct Matrix {
    int calculated_refresh_rate = refresh;
    bool begin() { return succeeds; }
    void setBrightness8(int) { ++touches; }
    void clearScreen() { ++touches; }
};
struct Hub75DMA {
    bool ready = false; Matrix storage; Matrix* matrix = &storage; int panelBrightness = 50;
    Hub75DMA() {
""" + constructor + r"""
};
int task = 2;
int xTaskGetCurrentTaskHandle() { return task; }
const int portMAX_DELAY = -1;
struct SensorData { int proximity = 0; };
struct KMMXController {
    using ControlHandler = void (KMMXController::*)(int);
    struct RenderCommand { ControlHandler handler; int value; SensorData sample; };
    int renderTaskHandle = 1;
    std::atomic<bool> renderRunning{false};
    std::deque<RenderCommand> commands;
    std::deque<RenderCommand>* commandQueue = &commands;
    bool queueControl(ControlHandler, int);
    int result = 0;
    void apply(int value) { result = result * 10 + value; }
};
int xQueueSend(std::deque<KMMXController::RenderCommand>* queue, const KMMXController::RenderCommand* command, int) {
    assert(queue->size() < 16); queue->push_back(*command); return 1;
}
""" + queue + r"""
int main() {
    for (int scenario = 0; scenario < 3; ++scenario) {
        succeeds = scenario != 0; refresh = scenario == 1 ? 0 : 120; touches = 0;
        Hub75DMA display;
        assert(display.ready == (scenario == 2));
        assert(touches == (scenario == 2 ? 2 : 0));
    }
    KMMXController controller;
    assert(controller.queueControl(&KMMXController::apply, 9));
    assert(controller.commands.empty());
    controller.renderRunning.store(true);
    for (int i = 0; i < 13; ++i) assert(controller.queueControl(&KMMXController::apply, 0));
    for (int value : {1, 2, 3}) assert(controller.queueControl(&KMMXController::apply, value));
    task = 1;
    assert(!controller.queueControl(&KMMXController::apply, 9));
    while (!controller.commands.empty()) {
        const auto command = controller.commands.front(); controller.commands.pop_front();
        (controller.*command.handler)(command.value);
    }
    assert(controller.result == 123);
}
""")
    run('startup_queue', [test])

    callbacks = (root / 'src/Network/BLE.cpp').read_text()
    callbacks = callbacks[callbacks.index('static void setCheekBgColor'):callbacks.index('class VisemeSettingsCallbacks')]
    test = Path(temporary) / 'ble_colors.cpp'
    test.write_text(r"""
#include <cassert>
#include <cstdint>
#include <vector>
#include <array>
struct __FlashStringHelper;
#define F(value) reinterpret_cast<const __FlashStringHelper*>(value)
struct SerialStub { template<class T> void print(T) {} template<class T> void println(T) {} } Serial;
struct NimBLEConnInfo {};
struct Value {
    std::vector<uint8_t> bytes;
    size_t length() const { return bytes.size(); }
    const uint8_t* data() const { return bytes.data(); }
};
struct NimBLECharacteristic {
    Value value; int reads = 0;
    Value getValue() { ++reads; return value; }
};
struct NimBLECharacteristicCallbacks {
    virtual void onWrite(NimBLECharacteristic*, NimBLEConnInfo&) = 0;
};
struct Controller;
using KMMXController = Controller;
struct Controller {
    std::array<uint8_t, 3> bg{}, fade{}, top{}, bottom{}, spiral{}, circle{};
    void setCheekBackgroundColor(uint8_t r, uint8_t g, uint8_t b) { bg = {r,g,b}; }
    void setCheekFadeColor(uint8_t r, uint8_t g, uint8_t b) { fade = {r,g,b}; }
    Controller& displayControl() { return *this; }
    void getGradientBottomColor(uint8_t& r, uint8_t& g, uint8_t& b) { r=bottom[0];g=bottom[1];b=bottom[2]; }
    void getGradientTopColor(uint8_t& r, uint8_t& g, uint8_t& b) { r=top[0];g=top[1];b=top[2]; }
    void setGradientColors(uint8_t r,uint8_t g,uint8_t b,uint8_t x,uint8_t y,uint8_t z) { top={r,g,b};bottom={x,y,z}; }
    void setDualSpiralColor(uint8_t r,uint8_t g,uint8_t b) { spiral={r,g,b}; }
    void setDualCircleColor(uint8_t r,uint8_t g,uint8_t b) { circle={r,g,b}; }
};
struct BLEManager { static BLEManager* instance; bool debugEnabled=false; Controller controller; } manager;
BLEManager* BLEManager::instance = &manager;
""" + callbacks + r"""
int main() {
    RgbWriteCallbacks bg(F("bg"), setCheekBgColor), fade(F("fade"), setCheekFadeColor);
    RgbWriteCallbacks first(F("c1"), setEffectColor1), second(F("c2"), setEffectColor2);
    NimBLECharacteristicCallbacks* handlers[] = {&bg, &fade, &first, &second};
    auto& c = manager.controller;
    std::array<uint8_t,3>* results[] = {&c.bg, &c.fade, &c.top, &c.bottom};
    NimBLEConnInfo info;
    for (int round=0; round<10; ++round) for (int i=0; i<4; ++i) {
        NimBLECharacteristic characteristic;
        characteristic.value.bytes = {uint8_t(round), uint8_t(i), 99, 77};
        handlers[i]->onWrite(&characteristic, info);
        assert(characteristic.reads == 1);
        assert((*results[i] == std::array<uint8_t,3>{uint8_t(round), uint8_t(i), 99}));
        const auto previous = *results[i];
        for (int length=0; length<3; ++length) {
            characteristic.value.bytes.resize(length); characteristic.reads=0;
            handlers[i]->onWrite(&characteristic, info);
            assert(characteristic.reads == 1 && *results[i] == previous);
        }
    }
}
""")
    run('ble_colors', [test])
