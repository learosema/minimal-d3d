/*********************************************************************
 * Minimal D3D11 Win32 App
 *  - Renders a single full‑screen triangle using a tiny HLSL shader.
 *  - No vertex buffer, no constant buffer, no input layout.
 *  - Uses ID3D11Device / IDXGISwapChain / ID3D11RenderTargetView.
 *
 * Build:
 *   cl /EHsc /W4 /DUNICODE /D_UNICODE main.cpp d3dcompiler.lib d3d11.lib
 *********************************************************************/

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ---------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------
HWND               g_hWnd = nullptr;
ID3D11Device*      g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain*    g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;
D3D11_VIEWPORT     g_viewport = {0};

// ---------------------------------------------------------------------
// HLSL Shaders (embedded as string literals)
// ---------------------------------------------------------------------
static const char g_vsSrc[] =
R"(
float4 VSMain(uint id : SV_VertexID) : SV_Position
{
    // 3 vertices that cover the whole screen (clip space)
    float2 pos[3] = {
        float2(-1.0, -1.0),   // bottom-left
        float2( 3.0, -1.0),   // bottom-right (extends beyond screen)
        float2(-1.0,  3.0)    // top-left (extends beyond screen)
    };
    return float4(pos[id], 0.0, 1.0);
}
)";

static const char g_psSrc[] =
R"(
float4 PSMain() : SV_Target
{
    // Simple red output
    return float4(1.0, 0.0, 0.0, 1.0);
}
)";

// ---------------------------------------------------------------------
// Helper: Compile a shader from source
// ---------------------------------------------------------------------
HRESULT CompileShader(const char* src, const char* entry, const char* target,
                      ID3DBlob** ppBlob)
{
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( _DEBUG )
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ID3DBlob* pError = nullptr;
    HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
                            entry, target, flags, 0, ppBlob, &pError);
    if (FAILED(hr))
    {
        if (pError)
        {
            OutputDebugStringA((char*)pError->GetBufferPointer());
            pError->Release();
        }
    }
    return hr;
}

// ---------------------------------------------------------------------
// Create the D3D11 device, swap chain, render target, and shaders
// ---------------------------------------------------------------------
HRESULT InitD3D()
{
    HRESULT hr;

    // Swap chain description
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 0;          // let DXGI pick the window size
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // Create device and swap chain
    hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
                                       nullptr, 0, nullptr, 0,
                                       D3D11_SDK_VERSION,
                                       &sd, &g_pSwapChain,
                                       &g_pd3dDevice, nullptr,
                                       &g_pImmediateContext);
    if (FAILED(hr)) return hr;

    // Create render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(hr)) return hr;
    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr)) return hr;

    // Bind the view
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

    // Setup viewport
    RECT rc;
    GetClientRect(g_hWnd, &rc);
    g_viewport.TopLeftX = 0.0f;
    g_viewport.TopLeftY = 0.0f;
    g_viewport.Width    = static_cast<FLOAT>(rc.right - rc.left);
    g_viewport.Height   = static_cast<FLOAT>(rc.bottom - rc.top);
    g_viewport.MinDepth = 0.0f;
    g_viewport.MaxDepth = 1.0f;
    g_pImmediateContext->RSSetViewports(1, &g_viewport);

    // Compile shaders
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    hr = CompileShader(g_vsSrc, "VSMain", "vs_5_0", &vsBlob);
    if (FAILED(hr)) return hr;
    hr = CompileShader(g_psSrc, "PSMain", "ps_5_0", &psBlob);
    if (FAILED(hr))
    {
        vsBlob->Release();
        return hr;
    }

    // Create shader objects
    hr = g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(),
                                          vsBlob->GetBufferSize(),
                                          nullptr, &g_pVertexShader);
    if (FAILED(hr))
    {
        vsBlob->Release(); psBlob->Release();
        return hr;
    }
    hr = g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(),
                                         psBlob->GetBufferSize(),
                                         nullptr, &g_pPixelShader);
    if (FAILED(hr))
    {
        vsBlob->Release(); psBlob->Release();
        return hr;
    }

    // No input layout needed because we use SV_VertexID

    vsBlob->Release();
    psBlob->Release();

    return S_OK;
}

// ---------------------------------------------------------------------
// Render a single frame
// ---------------------------------------------------------------------
void Render()
{
    // Clear to cornflower blue (just to see the clear)
    float clearColor[4] = { 0.4f, 0.6f, 0.9f, 1.0f };
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

    // Set shaders
    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

    // Draw 3 vertices (full‑screen triangle)
    g_pImmediateContext->Draw(3, 0);

    // Present
    g_pSwapChain->Present(1, 0);
}

// ---------------------------------------------------------------------
// Cleanup
// ---------------------------------------------------------------------
void Cleanup()
{
    if (g_pImmediateContext) g_pImmediateContext->ClearState();
    if (g_pPixelShader)       g_pPixelShader->Release();
    if (g_pVertexShader)      g_pVertexShader->Release();
    if (g_pRenderTargetView)  g_pRenderTargetView->Release();
    if (g_pSwapChain)         g_pSwapChain->Release();
    if (g_pImmediateContext)  g_pImmediateContext->Release();
    if (g_pd3dDevice)         g_pd3dDevice->Release();
}

// ---------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------
// WinMain
// ---------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"MinimalD3D11Window";
    RegisterClassEx(&wc);

    // Create window
    g_hWnd = CreateWindowEx(0, wc.lpszClassName, L"Minimal D3D11",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            800, 600,
                            nullptr, nullptr, hInstance, nullptr);
    if (!g_hWnd) return -1;
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    // Init D3D
    if (FAILED(InitD3D())) return -1;

    // Main loop
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // PeekMessage instead of GetMessage to keep rendering
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        Render();
    }

    Cleanup();
    return 0;
}