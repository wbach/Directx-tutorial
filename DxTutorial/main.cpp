#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <string>
#include <xnamath.h>

const int WIDTH = 640;
const int HEIGHT = 480;

IDXGISwapChain *swapchain;             // the pointer to the swap chain interface
ID3D11Device *dev;                     // the pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;           // the pointer to our Direct3D device context
ID3D11RenderTargetView *backbuffer;    // global declaration
ID3D11Texture2D*        depthStencil = NULL;
ID3D11DepthStencilView* depthStencilView = NULL;


ID3D11VertexShader *pVS;    // the vertex shader
ID3D11PixelShader *pPS;     // the pixel shader
ID3D10Blob *VS, *PS;
ID3D11Buffer *pVBuffer;
ID3D11Buffer *pCubeIndicesBuffer;
ID3D11Buffer *pCubeBuffer;
ID3D11Buffer* pConstantBuffer;
ID3D11InputLayout *pLayout;
int DRAW_SIZE = 0;

XMMATRIX  WorldMatrix;
XMMATRIX  ViewMatrix;
XMMATRIX  ProjectionMatrix;

FLOAT objectPosX = 0;

struct ConstantBuffer
{
    XMMATRIX mWorld;
    XMMATRIX mView;
    XMMATRIX mProjection;
};

struct VERTEX
{
    FLOAT X, Y, Z;      // position
    D3DXCOLOR Color;    // color
};

VERTEX TriangleVertices[] =
{
    {0.0f, 0.5f, 0.0f, D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f)},
    {0.45f, -0.5, 0.0f, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f)},
    {-0.45f, -0.5f, 0.0f, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f)}
};

VERTEX CubeVertices[] =
{
    { -1.0f, 1.0f, -1.0f, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f) },
    { 1.0f, 1.0f, -1.0f, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f) },
    { 1.0f, 1.0f, 1.0f, D3DXCOLOR(0.0f, 1.0f, 1.0f, 1.0f) },
    { -1.0f, 1.0f, 1.0f, D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f) },
    { -1.0f, -1.0f, -1.0f, D3DXCOLOR(1.0f, 0.0f, 1.0f, 1.0f) },
    { 1.0f, -1.0f, -1.0f, D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f) },
    { 1.0f, -1.0f, 1.0f, D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f) },
    { -1.0f, -1.0f, 1.0f, D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f) },
};

WORD CubeIndices[] =
{
    3,1,0,
    2,1,3,

    0,5,4,
    1,5,0,

    3,4,7,
    0,4,3,

    1,6,5,
    2,6,1,

    2,7,6,
    3,7,2,

    6,4,5,
    7,4,6,
};

// function prototypes
void InitD3D(HWND hWnd);     // sets up and initializes Direct3D
void CleanD3D(void);         // closes Direct3D and releases memory
void RenderFrame(void);

LPSTR className = (LPSTR)"Klasa Okienka";

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = className;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Wysoka Komisja odmawia rejestracji tego okna!", "Niestety...",
            MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    auto hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, "DirectX tutorial", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL)
    {
        MessageBox(NULL, "Can not create a window.", "Error", MB_ICONEXCLAMATION);
        return 1;
    }

    InitD3D(hwnd);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG message;
    // Enter the infinite message loop
    while (true)
    {
        // Check to see if any messages are waiting in the queue
        if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
        {
            // translate keystroke messages into the right format
            TranslateMessage(&message);

            // send the message to the WindowProc function
            DispatchMessage(&message);

            // check to see if it's time to quit
            if (message.message == WM_QUIT)
                break;
            continue;
        }

        RenderFrame();
    }
}

void InitDirectx(HWND hwnd)
{
    // create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC scd;

    // clear out the struct for use
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    // fill the swap chain description struct
    scd.BufferCount = 1;                                    // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
    scd.OutputWindow = hwnd;                                // the window to be used
    scd.SampleDesc.Count = 4;                               // how many multisamples
    scd.Windowed = TRUE;                                    // windowed/full-screen mode

    // create a device, device context and swap chain using the information in the scd struct
    D3D11CreateDeviceAndSwapChain(NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        NULL,
        NULL,
        NULL,
        D3D11_SDK_VERSION,
        &scd,
        &swapchain,
        &dev,
        NULL,
        &devcon);
}

void InitRenderTarget()
{
    // get the address of the back buffer
    ID3D11Texture2D *pBackBuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    // use the back buffer address to create the render target
    dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
    pBackBuffer->Release();

    // set the render target as the back buffer
    devcon->OMSetRenderTargets(1, &backbuffer, depthStencilView);
}

void InitDepthSetncilView()
{
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = WIDTH;
    descDepth.Height = HEIGHT;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    auto hr = dev->CreateTexture2D(&descDepth, NULL, &depthStencil);
    if (FAILED(hr))
        return;

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = dev->CreateDepthStencilView(depthStencil, &descDSV, &depthStencilView);
    if (FAILED(hr))
        return;

    devcon->OMSetRenderTargets(1, &backbuffer, depthStencilView);
}

void InitViewPort()
{
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = WIDTH;
    viewport.Height = HEIGHT;

    devcon->RSSetViewports(1, &viewport);
}

void CompileShader(const std::string& filename, const std::string& mainFunctionName, const std::string& version, ID3D10Blob*&  blop)
{
    ID3DBlob* pErrorBlob;
    auto result = D3DX11CompileFromFile(filename.c_str(), 0, 0, mainFunctionName.c_str(), version.c_str(), 0, 0, 0, &blop, &pErrorBlob, 0);

    if (FAILED(result))
    {
        std::string errorMessage = "Error when compile file : " + filename + ", main function : " + mainFunctionName + ", version  : " + version;

        if (pErrorBlob != NULL)
        {
            errorMessage += ", reason : ";
            errorMessage += (char*)pErrorBlob->GetBufferPointer();
        }

        MessageBox(NULL, errorMessage.c_str(), "Error", MB_OK);
        return;
    }
}

void InitShaders()
{
    CompileShader("shaders.shader", "VS", "vs_4_0", VS);
    CompileShader("shaders.shader", "PS", "ps_4_0", PS);

    auto hr = dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);

    if (FAILED(hr))
    {
        VS->Release();
        return;
    }

    hr = dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

    if (FAILED(hr))
    {
        PS->Release();
        return;
    }

}

void CreateTriangleVBO()
{
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
    bd.ByteWidth = sizeof(VERTEX) * 3;             // size is the VERTEX struct * 3
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;       // use as a vertex buffer
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    // allow CPU to write in buffer
    //bd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = TriangleVertices;

    dev->CreateBuffer(&bd, &InitData, &pVBuffer);       // create the buffer
}

void CopyTriangleDataToVBO()
{
    D3D11_MAPPED_SUBRESOURCE ms;
    devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);   // map the buffer
    memcpy(ms.pData, TriangleVertices, sizeof(TriangleVertices));       // copy the data
    devcon->Unmap(pVBuffer, NULL);                                     // unmap the buffer
}

void CreateCubeVBO()
{
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;                // write access access by CPU and GPU
    bd.ByteWidth = sizeof(VERTEX) * 8;             // size is the VERTEX struct * 3
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;       // use as a vertex buffer
    bd.CPUAccessFlags = 0;    // allow CPU to write in buffer

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = CubeVertices;

    auto hr = dev->CreateBuffer(&bd, &InitData, &pCubeBuffer);       // create the buffer

    if (FAILED(hr))
    {
        MessageBox(NULL,
            __FUNCTION__, "Error", MB_OK);
        return;
    }

    UINT stride = sizeof(VERTEX);
    UINT offset = 0;
    devcon->IASetVertexBuffers(0, 1, &pCubeBuffer, &stride, &offset);
}

void CreateCubeIndicesVBO()
{
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * 36;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = CubeIndices;

    auto hr = dev->CreateBuffer(&bd, &InitData, &pCubeIndicesBuffer);       // create the buffer

    if (FAILED(hr))
    {
        MessageBox(NULL,
            __FUNCTION__, "Error", MB_OK);
        return;
    }

    devcon->IASetIndexBuffer(pCubeIndicesBuffer, DXGI_FORMAT_R16_UINT, 0);

    //D3D11_MAPPED_SUBRESOURCE ms;
    //devcon->Map(pCubeIndicesBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);   // map the buffer
    //memcpy(ms.pData, CubeIndices, sizeof(CubeIndices));       // copy the data
    //devcon->Unmap(pCubeIndicesBuffer, NULL);                                     // unmap the buffer
}

void CopyCubeDataToVBOs()
{
    D3D11_MAPPED_SUBRESOURCE ms;
    devcon->Map(pCubeBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);   // map the buffer
    memcpy(ms.pData, CubeVertices, sizeof(CubeVertices));       // copy the data
    devcon->Unmap(pCubeBuffer, NULL);                                     // unmap the buffer
}

void CreateInputLayout()
{
    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    dev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
    devcon->IASetInputLayout(pLayout);
}

void InitTriangleVBO()
{
    CreateTriangleVBO();
    // CopyTriangleDataToVBO();

    UINT stride = sizeof(VERTEX);
    UINT offset = 0;
    devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
    DRAW_SIZE = 3;
}

void InitCubeVBO()
{
    CreateCubeVBO();
    // CopyCubeDataToVBOs();
    CreateCubeIndicesVBO();
    DRAW_SIZE = 36;
}

void InitVBO()
{
    InitCubeVBO();
    InitTriangleVBO();

    // select which primtive type we are using
    devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void InitPipeline()
{
    InitShaders();
    InitVBO();
    CreateInputLayout();

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    auto result = dev->CreateBuffer(&bd, NULL, &pConstantBuffer);
    if (FAILED(result))
    {
        MessageBox(NULL,
            "pConstantBuffer error.", "Error", MB_OK);
        return;
    }
}

void UseShader()
{
    // set the shader objects
    devcon->VSSetShader(pVS, 0, 0);
    devcon->PSSetShader(pPS, 0, 0);
}

void InitD3D(HWND hwnd)
{
    InitDirectx(hwnd);
    InitRenderTarget();
    InitDepthSetncilView();
    InitViewPort();
    InitPipeline();
    UseShader();

    WorldMatrix = XMMatrixIdentity();

    XMVECTOR Eye = XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
    XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    ViewMatrix = XMMatrixLookAtLH(Eye, At, Up);

    ProjectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV2, WIDTH / (FLOAT)HEIGHT, 0.01f, 100.0f);
    devcon->VSSetConstantBuffers(0, 1, &pConstantBuffer);
}

void DrawTriangle(ConstantBuffer& cb)
{
    UINT stride = sizeof(VERTEX);
    UINT offset = 0;
    devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
    cb.mWorld = XMMatrixTranspose(XMMatrixTranslation(objectPosX, 2, 0));
    devcon->UpdateSubresource(pConstantBuffer, 0, NULL, &cb, 0, 0);
    devcon->Draw(3, 0);
}

void DrawCube(ConstantBuffer& cb)
{
    UINT stride = sizeof(VERTEX);
    UINT offset = 0;

    devcon->IASetVertexBuffers(0, 1, &pCubeBuffer, &stride, &offset);
    cb.mWorld = XMMatrixTranspose(XMMatrixTranslation(objectPosX, 0, 0));
    devcon->UpdateSubresource(pConstantBuffer, 0, NULL, &cb, 0, 0);
    devcon->DrawIndexed(36, 0, 0);
}

void Draw()
{
    // select which vertex buffer to display
    // draw the vertex buffer to the back buffer
    objectPosX += 0.001f;

    if (objectPosX > 8)
        objectPosX = -8;

    UINT stride = sizeof(VERTEX);
    UINT offset = 0;

    ConstantBuffer cb;
    cb.mView = XMMatrixTranspose(ViewMatrix);
    cb.mProjection = XMMatrixTranspose(ProjectionMatrix);

    DrawTriangle(cb);
    DrawCube(cb);
}


void RenderFrame(void)
{
    // clear the back buffer to a deep blue
    FLOAT color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    devcon->ClearRenderTargetView(backbuffer, color);
    devcon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // do 3D rendering on the back buffer here
    Draw();

    // switch the back buffer and the front buffer
    swapchain->Present(0, 0);
}

void CleanD3D()
{
    pVS->Release();
    pPS->Release();
    swapchain->Release();
    depthStencil->Release();
    depthStencilView->Release();
    backbuffer->Release();
    pConstantBuffer->Release();
    dev->Release();
    devcon->Release();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (msg)
    {
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        CleanD3D();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

