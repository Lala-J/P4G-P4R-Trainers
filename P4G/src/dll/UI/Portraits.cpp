#include "UI/Portraits.h"

#include <d3d11.h>
#include <windows.h>
#include <cstdio>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "Util/Cpk.h"
#include "Util/Log.h"

namespace Portraits {

namespace {

struct Item {
    enum State { Queued, Decoded, Ready, Failed } state = Queued;
    std::vector<uint8_t> rgba;
    int w = 0, h = 0;
    int top = 0, bottom = 0;  // rows that contain the figure
    ID3D11ShaderResourceView* srv = nullptr;
};

ID3D11Device* g_dev = nullptr;
std::mutex g_mu;
std::map<int, Item> g_items;
std::vector<int> g_queue;
std::thread g_worker;
bool g_stop = false;

int Key(int c, int e, int o) { return c << 16 | e << 8 | o; }

void Decode(int key) {
    static Cpk::Archive* arc = nullptr;
    static bool opened = false;
    if (!opened) {
        opened = true;
        arc = new Cpk::Archive();
        if (!arc->Open(Cpk::GameDir() + L"\\data.cpk")) {
            LOGW("portraits: data.cpk could not be read");
            delete arc;
            arc = nullptr;
        }
    }
    Item tmp;
    char name[64];
    snprintf(name, sizeof name, "bustup/b%d_%d_%d.bin", key >> 16, (key >> 8) & 0xFF, key & 0xFF);
    std::vector<uint8_t> pak, tmx;
    bool ok = arc && arc->Read(name, pak) && Cpk::PakEntry(pak, "b", tmx) &&
              Cpk::TmxToRgba(tmx, tmp.rgba, tmp.w, tmp.h);
    if (ok) {
        // Find the rows that hold the figure, to crop the empty space below it.
        tmp.top = tmp.h;
        tmp.bottom = 0;
        for (int y = 0; y < tmp.h; ++y)
            for (int x = 0; x < tmp.w; x += 4)
                if (tmp.rgba[(size_t(y) * tmp.w + x) * 4 + 3] > 16) {
                    if (y < tmp.top) tmp.top = y;
                    tmp.bottom = y;
                    break;
                }
        if (tmp.bottom <= tmp.top) ok = false;
    }
    std::lock_guard<std::mutex> lk(g_mu);
    Item& it = g_items[key];
    if (ok) {
        it.rgba.swap(tmp.rgba);
        it.w = tmp.w;
        it.h = tmp.h;
        it.top = tmp.top;
        it.bottom = tmp.bottom;
        it.state = Item::Decoded;
    } else {
        it.state = Item::Failed;
        LOGW("portraits: %s unavailable", name);
    }
}

void Worker() {
    for (;;) {
        int key = -1;
        {
            std::lock_guard<std::mutex> lk(g_mu);
            if (g_stop) return;
            if (!g_queue.empty()) {
                key = g_queue.back();
                g_queue.pop_back();
            }
        }
        if (key < 0) {
            Sleep(30);
            continue;
        }
        Decode(key);
    }
}

}  // namespace

void Init(void* dev) {
    g_dev = static_cast<ID3D11Device*>(dev);
    if (g_dev) g_dev->AddRef();
    g_stop = false;
    g_worker = std::thread(Worker);
}

void Shutdown() {
    {
        std::lock_guard<std::mutex> lk(g_mu);
        g_stop = true;
    }
    if (g_worker.joinable()) g_worker.join();
    for (auto& [k, it] : g_items)
        if (it.srv) it.srv->Release();
    g_items.clear();
    if (g_dev) g_dev->Release();
    g_dev = nullptr;
}

ImTextureID Get(int c, int e, int o, ImVec2* uv0, ImVec2* uv1) {
    if (!g_dev) return ImTextureID{};
    int key = Key(c, e, o);
    std::lock_guard<std::mutex> lk(g_mu);
    auto f = g_items.find(key);
    if (f == g_items.end()) {
        g_items[key] = Item{};
        g_queue.push_back(key);
        return ImTextureID{};
    }
    Item& it = f->second;
    if (it.state == Item::Decoded) {
        D3D11_TEXTURE2D_DESC d{};
        d.Width = it.w;
        d.Height = it.h;
        d.MipLevels = 1;
        d.ArraySize = 1;
        d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        d.SampleDesc.Count = 1;
        d.Usage = D3D11_USAGE_IMMUTABLE;
        d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA s{it.rgba.data(), UINT(it.w * 4), 0};
        ID3D11Texture2D* tex = nullptr;
        if (SUCCEEDED(g_dev->CreateTexture2D(&d, &s, &tex))) {
            g_dev->CreateShaderResourceView(tex, nullptr, &it.srv);
            tex->Release();
        }
        it.rgba.clear();
        it.rgba.shrink_to_fit();
        it.state = it.srv ? Item::Ready : Item::Failed;
    }
    if (it.state != Item::Ready) return ImTextureID{};
    if (uv0) *uv0 = ImVec2(0, float(it.top) / it.h);
    if (uv1) *uv1 = ImVec2(1, float(it.bottom + 1) / it.h);
    return reinterpret_cast<ImTextureID>(it.srv);
}

}  // namespace Portraits
