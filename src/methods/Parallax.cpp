#include "Parallax.hpp"

#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>

namespace methods
{

static void DoubleToED(double d, float &high, float &low)
{
  high = static_cast<float>(d);
  low  = static_cast<float>(d - high);
}

struct EmulatedDoubleVec3
{
  glm::vec3 high;
  glm::vec3 low;
};

static EmulatedDoubleVec3 DoubleToED(const glm::dvec3 &d)
{
  EmulatedDoubleVec3 v{};
  DoubleToED(d.x, v.high.x, v.low.x);
  DoubleToED(d.y, v.high.y, v.low.y);
  DoubleToED(d.z, v.high.z, v.low.z);
  return v;
}

struct EmulatedDoubleMat4
{
  glm::mat4 high;
  glm::mat4 low;
};

static EmulatedDoubleMat4 DoubleToED(const glm::dmat4 &d)
{
  EmulatedDoubleMat4 m{};
  for (u32 x = 0; x < 4; x++)
  {
    for (u32 y = 0; y < 4; y++)
    {
      DoubleToED(d[x][y], m.high[x][y], m.low[x][y]);
    }
  }
  return m;
}

struct EDSceneData
{
  EmulatedDoubleMat4 viewProjection;
  EmulatedDoubleMat4 model;
};

struct EDPerMeshData
{
  EmulatedDoubleMat4 transform{};
};

struct EmulatedDoubleVertex
{
  EmulatedDoubleVec3 pos;
  glm::vec3          normal{};
  glm::vec2          textureCoord{};
};

using VertexFormat = EmulatedDoubleVertex;

#if 1
struct SceneData
{
  glm::mat4 viewProjection;
};

struct PerMeshData
{
  glm::mat4 transform{};
};

// using VertexFormat = ModelVertex;
#endif

struct BBDebugVertex
{
  glm::vec3 position;
  glm::vec3 color;
};

static std::vector<BBDebugVertex> sBBVertices = {
  {{-1.0f, 1.0f, -1.0f}, {0, 0, 1}},
  {{1.0f, 1.0f, -1.0f}, {0, 1, 0}},
  {{-1.0f, -1.0f, -1.0f}, {1, 0, 0}},
  {{1.0f, -1.0f, -1.0f}, {0, 1, 1}},
  {{-1.0f, 1.0f, 1.0f}, {0, 0, 1}},
  {{1.0f, 1.0f, 1.0f}, {1, 0, 0}},
  {{-1.0f, -1.0f, 1.0f}, {0, 1, 0}},
  {{1.0f, -1.0f, 1.0f}, {0, 1, 1}},
};

static std::vector<VertexFormat> sEDBBVertices = {
  VertexFormat{DoubleToED(glm::dvec3{-1.0f, 1.0f, -1.0f}), {0, 0, 1}},
  VertexFormat{DoubleToED(glm::dvec3{1.0f, 1.0f, -1.0f}), {0, 1, 0}},
  VertexFormat{DoubleToED(glm::dvec3{-1.0f, -1.0f, -1.0f}), {1, 0, 0}},
  VertexFormat{DoubleToED(glm::dvec3{1.0f, -1.0f, -1.0f}), {0, 1, 1}},
  VertexFormat{DoubleToED(glm::dvec3{-1.0f, 1.0f, 1.0f}), {0, 0, 1}},
  VertexFormat{DoubleToED(glm::dvec3{1.0f, 1.0f, 1.0f}), {1, 0, 0}},
  VertexFormat{DoubleToED(glm::dvec3{-1.0f, -1.0f, 1.0f}), {0, 1, 0}},
  VertexFormat{DoubleToED(glm::dvec3{1.0f, -1.0f, 1.0f}), {0, 1, 1}},
};

// clang-format off
  static std::vector<u32> sBBIndices = {
    0, 1, 2,          // side 1
    2, 1, 3, 
    4, 0, 6, // side 2
    6, 0, 2, 
    7, 5, 6, // side 3
    6, 5, 4, 
    3, 1, 7, // side 4
    7, 1, 5, 
    4, 5, 0, // side 5
    0, 5, 1, 
    3, 7, 2, // side 6
    2, 7, 6,
  };
// clang-format on

Parallax::Parallax(ID3D11Device3 *device, ShaderWatcher &shaderWatcher) :
    mShadersHandle{shaderWatcher.RegisterShader(
      VERT_PATH,
      PIXEL_PATH,
      {
        {
          .SemanticName         = "POSITION",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(ModelVertex, position),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "NORMAL",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(ModelVertex, normal),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "TEXCOORD",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(ModelVertex, textureCoord),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
      })},
    mVertBuf{},
    mIndexBuf{},
    mConstantBuf{dx::CreateConstantBuffer<SceneData>(device, nullptr)},
    mDevice{device},
    mBBDebugShadersHandle{shaderWatcher.RegisterShader(
      BB_DEBUG_VERT_PATH,
      BB_DEBUG_PIXEL_PATH,
      {
        {
          .SemanticName         = "POSITION",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(BBDebugVertex, position),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "COLOR",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(BBDebugVertex, color),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
      })},
    mBBTexShaderHandle{shaderWatcher.RegisterShader(
      BB_TEXTURED_VERT_PATH,
      BB_TEXTURED_PIXEL_PATH,
      {
        {
          .SemanticName         = "POSITION_HIGH",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(VertexFormat, pos),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "POSITION_LOW",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(VertexFormat, pos) + sizeof(glm::vec3),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "NORMAL",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(VertexFormat, normal),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "TEXCOORD",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(VertexFormat, textureCoord),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
      })},
    mBBDebugConstantBuf{dx::CreateConstantBuffer<EDSceneData>(device, nullptr)}
{
  mBBDebugIndexCount = sBBIndices.size();

  mBBDebugVertBuf = dx::CreateVertexBuffer<VertexFormat>(
    mDevice,
    sEDBBVertices.size(),
    {sEDBBVertices.begin(), sEDBBVertices.end()});
  mBBDebugIndexBuf =
    dx::CreateIndexBuffer<u32>(mDevice, sBBIndices.size(), {sBBIndices.begin(), sBBIndices.end()});

  CD3D11_RASTERIZER_DESC rsDesc{CD3D11_DEFAULT{}};
  rsDesc.FrontCounterClockwise = true;
  rsDesc.FillMode              = D3D11_FILL_WIREFRAME;

  dx::ThrowIfFailed(mDevice->CreateRasterizerState(&rsDesc, mBBDebugRSState.GetAddressOf()));

  D3D11_TEXTURE2D_DESC quadTextureDesc = {
    .Width     = 1920,
    .Height    = 1080,
    .MipLevels = 1,
    .ArraySize = 1,
    .Format    = DXGI_FORMAT_R8G8B8A8_UNORM,
    .SampleDesc =
      {
        .Count   = 1,
        .Quality = 0,
      },
    .Usage          = D3D11_USAGE_DEFAULT,
    .BindFlags      = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    .CPUAccessFlags = 0,
    .MiscFlags      = 0,
  };

  dx::ThrowIfFailed(
    device->CreateTexture2D(&quadTextureDesc, nullptr, mImposterCubeTexture.GetAddressOf()));

  D3D11_SHADER_RESOURCE_VIEW_DESC quadViewDesc = {
    .Format        = quadTextureDesc.Format,
    .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
    .Texture2D =
      {
        .MostDetailedMip = 0,
        .MipLevels       = 1,
      },
  };

  dx::ThrowIfFailed(device->CreateShaderResourceView(
    mImposterCubeTexture.Get(),
    &quadViewDesc,
    mImposterTextureView.GetAddressOf()));

  dx::ThrowIfFailed(device->CreateRenderTargetView(
    mImposterCubeTexture.Get(),
    nullptr,
    mImposterTarget.GetAddressOf()));

  D3D11_TEXTURE2D_DESC quadDepthDesc = {
    .Width     = 1920,
    .Height    = 1080,
    .MipLevels = 1,
    .ArraySize = 1,
    .Format    = DXGI_FORMAT_R24G8_TYPELESS,
    .SampleDesc =
      {
        .Count   = 1,
        .Quality = 0,
      },
    .Usage          = D3D11_USAGE_DEFAULT,
    .BindFlags      = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
    .CPUAccessFlags = 0,
    .MiscFlags      = 0,
  };

  dx::ThrowIfFailed(
    device->CreateTexture2D(&quadDepthDesc, 0, mImposterDepthBuffer.GetAddressOf()));

  D3D11_DEPTH_STENCIL_VIEW_DESC quadDepthStencilViewDesc = {
    .Format        = DXGI_FORMAT_D24_UNORM_S8_UINT,
    .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
    .Flags         = 0,
    .Texture2D =
      {
        .MipSlice = 0,
      },
  };

  dx::ThrowIfFailed(device->CreateDepthStencilView(
    mImposterDepthBuffer.Get(),
    &quadDepthStencilViewDesc,
    mImposterDepthView.GetAddressOf()));

  D3D11_SHADER_RESOURCE_VIEW_DESC quadDepthViewDesc = {
    .Format        = DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
    .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
    .Texture2D =
      {
        .MostDetailedMip = 0,
        .MipLevels       = 1,
      },
  };

  dx::ThrowIfFailed(device->CreateShaderResourceView(
    mImposterDepthBuffer.Get(),
    &quadDepthViewDesc,
    mImposterDepthTexView.GetAddressOf()));

  CD3D11_SAMPLER_DESC samplerDesc{CD3D11_DEFAULT{}};
  dx::ThrowIfFailed(device->CreateSamplerState(&samplerDesc, mImposterSamplerState.GetAddressOf()));

  mImposterTargetCB = dx::CreateConstantBuffer<SceneData>(mDevice, nullptr);

  for (size_t i = 0; i < mGpuDisjointQueries.size(); i++)
  {

    mGpuDisjointQueries[i]    = dx::CreateDisjointQuery(mDevice);
    mImposterToTexStart[i]    = dx::CreateTimeQuery(mDevice);
    mImposterToTexEnd[i]      = dx::CreateTimeQuery(mDevice);
    mDrawImposterCubeStart[i] = dx::CreateTimeQuery(mDevice);
    mDrawImposterCubeEnd[i]   = dx::CreateTimeQuery(mDevice);
  }
}

void Parallax::SetScene(const Scene &scene)
{
  mDraws          = {};
  mModelConstants = {};
  mBBDebugModelConstants.Reset();
  // mBBTransforms = {};
  mBoundingBox = {};
  mPos         = {};
  mScene       = scene;
  std::vector<ModelVertex> vertices;
  std::vector<u32>         indices;

  u32         vertexStart = 0;
  u32         startIndex  = 0;
  BoundingBox bb{};

  for (auto &mesh : scene.model.parts)
  {
    for (auto &vertex : mesh.vertices)
    {
      bb.min = glm::min(bb.min, glm::dvec3{vertex.position});
      bb.max = glm::max(bb.max, glm::dvec3{vertex.position});
      vertices.push_back(vertex);
    }
    for (auto &index : mesh.indices)
    {
      indices.push_back(index);
    }
    mDraws.push_back({
      .startIndex{startIndex},
      .baseVertex{vertexStart},
      .indexCount{static_cast<u32>(mesh.indices.size())},
    });
    vertexStart += mesh.vertices.size();
    startIndex += mesh.indices.size();
  }
  mDrawIDBuf = dx::CreateDrawIDBuffer(mDevice, mDraws.size());
  mVertBuf   = dx::CreateVertexBuffer<ModelVertex>(
    mDevice,
    vertices.size(),
    std::span{
      vertices.begin(),
      vertices.end(),
    });

  mIndexBuf = dx::CreateIndexBuffer<uint32_t>(
    mDevice,
    indices.size(),
    std::span<uint32_t>{indices.begin(), indices.end()});

  std::vector<PerMeshData> perMeshData(mDraws.size());
  for (u32 i = 0; i < perMeshData.size(); i++)
  {
    PerMeshData data{
      .transform = scene.model.transforms[i],
    };
    const BoundingBox &bb = scene.model.parts[i].boundingBox;

    mBoundingBox.min = glm::min(mBoundingBox.min, bb.min);
    mBoundingBox.max = glm::max(mBoundingBox.max, bb.max);

    // mBBTransforms.emplace_back(bbDebugData.transform);
    mModelConstants.emplace_back(dx::CreateConstantBuffer<PerMeshData>(mDevice, &data));
  }
  PerMeshData bbDebugData{
    .transform = glm::scale(
      glm::translate(glm::mat4(1.0), glm::vec3{mBoundingBox.max + mBoundingBox.min} / 2.0f),
      mBoundingBox.GetScale()),
  };
  mBBDebugModelConstants = dx::CreateConstantBuffer<PerMeshData>(mDevice, &bbDebugData);
}

void Parallax::Draw(
  u32                     width,
  u32                     height,
  ID3D11DeviceContext3   *ctx,
  const glm::dmat4       &cameraProjection,
  const glm::dmat4       &camera,
  const glm::dmat4       &projection,
  const glm::dvec3       &modelPos,
  const glm::dvec3       &cameraPos,
  const glm::dvec3       &sceneOrigin,
  const ArcballCamera    &arcball,
  dx::RenderContext      &renderContext,
  ShaderWatcher          &shaderWatcher,
  ID3D11RenderTargetView *targetView,
  bool                    recordDrawTime,
  u32                     testFrameCount)
{
  auto *annotation = renderContext.annotation.Get();

  D3D11_MAPPED_SUBRESOURCE mapped{};
  dx::ThrowIfFailed(ctx->Map(mConstantBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
  SceneData data{
    .viewProjection = cameraProjection * glm::translate(glm::identity<glm::dmat4>(), modelPos),
  };
  memcpy(mapped.pData, (void *)&data, sizeof(SceneData));
  ctx->Unmap(mConstantBuf.Get(), 0);

  D3D11_MAPPED_SUBRESOURCE mapped2{};
  dx::ThrowIfFailed(
    ctx->Map(mBBDebugConstantBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped2));
  EDSceneData *data2 = reinterpret_cast<EDSceneData *>(mapped2.pData);

  glm::mat4 t = glm::scale(
    glm::translate(
      glm::mat4(1.0),
      glm::vec3{modelPos} + glm::vec3{mBoundingBox.max + mBoundingBox.min} / 2.0f),
    mBoundingBox.GetScale());
  data2->viewProjection = DoubleToED(glm::mat4{cameraProjection} * t);

  ctx->Unmap(mBBDebugConstantBuf.Get(), 0);

  const auto DrawModel = [&]() {
    annotation->BeginEvent(L"DrawModel");
    RenderProgram        rp  = shaderWatcher.GetRenderProgram(mShadersHandle);
    ID3D11DeviceContext *ctx = renderContext.DeviceContext();

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetIndexBuffer(mIndexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
    u32 stride = sizeof(VertexFormat);
    u32 offset = 0;
    ctx->IASetVertexBuffers(0, 1, mVertBuf.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(rp.inputLayout);
    ctx->VSSetShader(rp.vertexShader, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, mConstantBuf.GetAddressOf());

    ctx->RSSetState(renderContext.rasterizerState.Get());

    ctx->PSSetShader(rp.pixelShader, nullptr, 0);
    ctx->OMSetRenderTargets(1, &targetView, renderContext.depthStencilView.Get());
    for (u32 i = 0; i < mDraws.size(); i++)
    {
      const DrawOffsets draw = mDraws[i];
      ctx->VSSetConstantBuffers(1, 1, mModelConstants[i].GetAddressOf());
      ctx->DrawIndexed(draw.indexCount, draw.startIndex, draw.baseVertex);
    }
    annotation->EndEvent();
  };

  // TODO: keep for visualization
  const auto DrawBoundingBoxOutline = [&]() {
    annotation->BeginEvent(L"DrawBoundingBoxOutline");

    RenderProgram        rp  = shaderWatcher.GetRenderProgram(mBBDebugShadersHandle);
    ID3D11DeviceContext *ctx = renderContext.DeviceContext();

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetIndexBuffer(mBBDebugIndexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
    u32 stride = sizeof(BBDebugVertex);
    u32 offset = 0;
    ctx->IASetVertexBuffers(0, 1, mBBDebugVertBuf.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(rp.inputLayout);
    ctx->VSSetShader(rp.vertexShader, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, mBBDebugConstantBuf.GetAddressOf());

    ctx->RSSetState(mBBDebugRSState.Get());

    ctx->PSSetShader(rp.pixelShader, nullptr, 0);
    ctx->OMSetRenderTargets(1, &targetView, renderContext.depthStencilView.Get());

    ctx->VSSetConstantBuffers(1, 1, mBBDebugModelConstants.GetAddressOf());
    ctx->DrawIndexed(mBBDebugIndexCount, 0, 0);
    annotation->EndEvent();
  };

  const auto DrawImposterCube = [&]() {
    annotation->BeginEvent(L"DrawImposterCube");

    RenderProgram        rp  = shaderWatcher.GetRenderProgram(mBBTexShaderHandle);
    ID3D11DeviceContext *ctx = renderContext.DeviceContext();

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetIndexBuffer(mBBDebugIndexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
    u32 stride = sizeof(BBDebugVertex);
    u32 offset = 0;
    ctx->IASetVertexBuffers(0, 1, mBBDebugVertBuf.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(rp.inputLayout);
    ctx->VSSetShader(rp.vertexShader, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, mBBDebugConstantBuf.GetAddressOf());

    ctx->RSSetState(renderContext.rasterizerState.Get());

    ctx->PSSetShader(rp.pixelShader, nullptr, 0);
    std::array srvs = {
      mImposterTextureView.Get(),
      mImposterDepthTexView.Get(),
    };
    ctx->PSSetShaderResources(0, srvs.size(), srvs.data());
    ctx->PSSetSamplers(0, 1, mImposterSamplerState.GetAddressOf());
    ctx->OMSetRenderTargets(1, &targetView, renderContext.depthStencilView.Get());

    if (recordDrawTime)
    {
      ctx->End(mDrawImposterCubeStart[testFrameCount].Get());
    }
    ctx->DrawIndexed(mBBDebugIndexCount, 0, 0);
    if (recordDrawTime)
    {
      ctx->End(mDrawImposterCubeEnd[testFrameCount].Get());
    }

    std::array<ID3D11ShaderResourceView *, 2> np = {nullptr, nullptr};
    ctx->PSSetShaderResources(0, np.size(), np.data());
    annotation->EndEvent();
  };

  auto RenderToImposterTexture = [&]() {
    annotation->BeginEvent(L"RenderToImposterTexture");

    D3D11_MAPPED_SUBRESOURCE mapped{};
    dx::ThrowIfFailed(
      ctx->Map(mImposterTargetCB.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
    auto *sceneData = reinterpret_cast<SceneData *>(mapped.pData);

    glm::vec3 cameraPos      = arcball.eye() - sceneOrigin;
    glm::vec3 cameraTarget   = arcball.dir();
    glm::vec3 cameraUp       = arcball.up();
    glm::mat4 imposterCamera = glm::lookAt(cameraPos, cameraTarget, cameraUp);

#if 0
    sceneData->viewProjection =
      projection * camera * glm::translate(glm::dmat4(1.0), modelPos); // rotation * t;
#else
    sceneData->viewProjection =
      glm::mat4{projection}
      * imposterCamera; // * glm::translate(glm::mat4(1.0), glm::vec3{modelPos}); // rotation * t;
#endif

    ctx->Unmap(mImposterTargetCB.Get(), 0);

    RenderProgram        rp           = shaderWatcher.GetRenderProgram(mShadersHandle);
    ID3D11DeviceContext *ctx          = renderContext.DeviceContext();
    f32                  clearColor[] = {0.0, 0.0, 0.0, 1.0};
    ctx->ClearRenderTargetView(mImposterTarget.Get(), clearColor);
    ctx->ClearDepthStencilView(mImposterDepthView.Get(), D3D11_CLEAR_DEPTH, 1.0, 0);

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetIndexBuffer(mIndexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
    u32 stride = sizeof(VertexFormat);
    u32 offset = 0;
    ctx->IASetVertexBuffers(0, 1, mVertBuf.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(rp.inputLayout);
    ctx->VSSetShader(rp.vertexShader, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, mImposterTargetCB.GetAddressOf());

    ctx->RSSetState(renderContext.rasterizerState.Get());

    ctx->PSSetShader(rp.pixelShader, nullptr, 0);
    ctx->OMSetRenderTargets(1, mImposterTarget.GetAddressOf(), mImposterDepthView.Get());
    if (recordDrawTime)
    {
      ctx->End(mImposterToTexStart[testFrameCount].Get());
    }
    for (u32 i = 0; i < mDraws.size(); i++)
    {
      const DrawOffsets draw = mDraws[i];
      ctx->VSSetConstantBuffers(1, 1, mModelConstants[i].GetAddressOf());
      ctx->DrawIndexed(draw.indexCount, draw.startIndex, draw.baseVertex);
    }
    if (recordDrawTime)
    {
      ctx->End(mImposterToTexEnd[testFrameCount].Get());
    }
    ID3D11RenderTargetView *np = nullptr;
    ctx->OMSetRenderTargets(1, &np, nullptr);
    annotation->EndEvent();
  };

  static bool drawModelChk        = false;
  static bool drawBBDebugChk      = false;
  static bool drawQuadDebugChk    = false;
  static bool DrawTexturedQuadChk = true;

  ImGui::Checkbox("drawModelChk", &drawModelChk);
  ImGui::Checkbox("drawBBDebugChk", &drawBBDebugChk);
  ImGui::Checkbox("drawQuadDebugChk", &drawQuadDebugChk);
  ImGui::Checkbox("DrawTexturedQuadChk", &DrawTexturedQuadChk);

  if (drawModelChk)
    DrawModel();
  if (drawBBDebugChk)
    DrawBoundingBoxOutline();
  glm::vec3 screenCenter = camera * glm::vec4{0, 0, 0, 1};
  screenCenter = glm::translate(glm::mat4(1.0), -screenCenter) * glm::vec4{screenCenter, 1.0};
  ImGui::Text("camera space center: (%f, %f, %f)", screenCenter.x, screenCenter.y, screenCenter.z);
  if (recordDrawTime)
  {

    renderContext.DeviceContext()->Begin(mGpuDisjointQueries[testFrameCount].Get());
  }
  if (DrawTexturedQuadChk)
  {
    RenderToImposterTexture();
    // DrawTexturedQuad();
    DrawImposterCube();
  }
  if (recordDrawTime)
  {

    renderContext.DeviceContext()->End(mGpuDisjointQueries[testFrameCount].Get());
  }
}
Parallax::Timing Parallax::GetTimingData(ID3D11DeviceContext3 *ctx)
{
  Parallax::Timing times;
  for (size_t i = 0; i < mGpuDisjointQueries.size(); i++)
  {
    dx::WaitForQueryToBeReady(ctx, mGpuDisjointQueries[i].Get());

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
    ctx->GetData(mGpuDisjointQueries[i].Get(), &disjoint, sizeof(disjoint), 0);

    u64 imposterToTexStart{}, imposterToTexEnd{};
    u64 cubeStart{}, cubeEnd{};

    ctx->GetData(mImposterToTexStart[i].Get(), &imposterToTexStart, sizeof(u64), 0);
    ctx->GetData(mImposterToTexEnd[i].Get(), &imposterToTexEnd, sizeof(u64), 0);
    ctx->GetData(mDrawImposterCubeStart[i].Get(), &cubeStart, sizeof(u64), 0);
    ctx->GetData(mDrawImposterCubeEnd[i].Get(), &cubeEnd, sizeof(u64), 0);

    double imposterToTexTime = static_cast<double>(imposterToTexEnd - imposterToTexStart)
                               / static_cast<double>(disjoint.Frequency) * 1000.0;
    double cubeTime =
      static_cast<double>(cubeEnd - cubeStart) / static_cast<double>(disjoint.Frequency) * 1000.0;
    times.imposterToTex.push_back(imposterToTexTime);
    times.imposterCube.push_back(cubeTime);
  }
  return times;
}

} // namespace methods
