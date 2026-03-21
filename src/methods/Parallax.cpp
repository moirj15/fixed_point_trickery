#include "Parallax.hpp"

#include <array>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>

namespace methods
{

struct SceneData
{
  glm::mat4 modelView;
};

struct PerMeshData
{
  glm::mat4 transform{};
};

using VertexFormat = ModelVertex;

struct BBDebugVertex
{
  glm::vec3 position;
  glm::vec3 color;
};

struct TexturedQuadVertex
{
  glm::vec3 position;
  glm::vec2 texCoord;
};

struct TextureQuadCB
{
  glm::mat4 mvp;
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
          .AlignedByteOffset    = offsetof(VertexFormat, position),
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
    mBBDebugConstantBuf{dx::CreateConstantBuffer<SceneData>(device, nullptr)},
    mQuadDebugShadersHandle{shaderWatcher.RegisterShader(
      QUAD_DEBUG_VERT_PATH,
      QUAD_DEBUG_PIXEL_PATH,
      {
        {
          .SemanticName         = "POSITION",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32A32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = 0,
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
      })},
    mTexQuadShaderHandle{shaderWatcher.RegisterShader(
      TEXTURED_QUAD_VERT_PATH,
      TEXTURED_QUAD_PIXEL_PATH,
      {
        {
          .SemanticName         = "POSITION",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32A32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = 0,
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "TEXCOORD",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = 0,
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        }, // todo rest of init
      })},
    mTexQuadConstantBuf{dx::CreateConstantBuffer<TextureQuadCB>(device, nullptr)}
{
  mBBDebugIndexCount = sBBIndices.size();

  mBBDebugVertBuf = dx::CreateVertexBuffer<BBDebugVertex>(
    mDevice,
    sBBVertices.size(),
    {sBBVertices.begin(), sBBVertices.end()});
  mBBDebugIndexBuf =
    dx::CreateIndexBuffer<u32>(mDevice, sBBIndices.size(), {sBBIndices.begin(), sBBIndices.end()});

  CD3D11_RASTERIZER_DESC rsDesc{CD3D11_DEFAULT{}};
  rsDesc.FrontCounterClockwise = true;
  rsDesc.FillMode              = D3D11_FILL_WIREFRAME;

  dx::ThrowIfFailed(mDevice->CreateRasterizerState(&rsDesc, mBBDebugRSState.GetAddressOf()));

  mQuadVertBuf = dx::CreateVertexBuffer<glm::vec4>(mDevice, 4, {}, true);

  std::array<u32, 6> quadIndices = {0, 1, 2, 2, 1, 3};

  mQuadIndexBuf = dx::CreateIndexBuffer<u32>(mDevice, 6, quadIndices);

  D3D11_TEXTURE2D_DESC quadTextureDesc = {
    .Width     = 1920,
    .Height    = 1080,
    .MipLevels = 1,
    .ArraySize = 1,
    .Format    = DXGI_FORMAT_R8G8B8A8_UNORM,
    // Multi sampling here
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
    device->CreateTexture2D(&quadTextureDesc, nullptr, mQuadTexture.GetAddressOf()));

  D3D11_SHADER_RESOURCE_VIEW_DESC quadViewDesc = {
    .Format        = quadTextureDesc.Format,
    .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
    .Texture2D =
      {
        .MostDetailedMip = 0,
        .MipLevels       = 1,
      },
  };

  dx::ThrowIfFailed(
    device->CreateShaderResourceView(mQuadTexture.Get(), &quadViewDesc, mQuadView.GetAddressOf()));

  dx::ThrowIfFailed(
    device->CreateRenderTargetView(mQuadTexture.Get(), nullptr, mQuadTarget.GetAddressOf()));

  D3D11_TEXTURE2D_DESC quadDepthDesc = {
    .Width     = 1920,
    .Height    = 1080,
    .MipLevels = 1,
    .ArraySize = 1,
    .Format    = DXGI_FORMAT_D24_UNORM_S8_UINT,
    // Multi sampling here
    .SampleDesc =
      {
        .Count   = 1,
        .Quality = 0,
      },
    .Usage          = D3D11_USAGE_DEFAULT,
    .BindFlags      = D3D11_BIND_DEPTH_STENCIL,
    .CPUAccessFlags = 0,
    .MiscFlags      = 0,
  };

  dx::ThrowIfFailed(device->CreateTexture2D(&quadDepthDesc, 0, mQuadDepth.GetAddressOf()));
  dx::ThrowIfFailed(
    device->CreateDepthStencilView(mQuadDepth.Get(), 0, mQuadDepthView.GetAddressOf()));

  CD3D11_SAMPLER_DESC samplerDesc{CD3D11_DEFAULT{}};
  dx::ThrowIfFailed(device->CreateSamplerState(&samplerDesc, mTexQuadSamplerState.GetAddressOf()));

  std::array<TexturedQuadVertex, 4> texQuadVerts = {
    TexturedQuadVertex{{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    TexturedQuadVertex{{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    TexturedQuadVertex{{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    TexturedQuadVertex{{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
  };
  mTexturedQuadVertBuf = dx::CreateVertexBuffer<TexturedQuadVertex>(mDevice, 4, texQuadVerts);
}

void Parallax::SetScene(const Scene &scene)
{
  mDraws                 = {};
  mModelConstants        = {};
  mBBDebugModelConstants = {};
  mBBTransforms          = {};
  mScene                 = scene;
  std::vector<VertexFormat> vertices;
  std::vector<u32>          indices;

  u32 vertexStart = 0;
  u32 startIndex  = 0;
  for (auto &mesh : scene.model.parts)
  {
    for (auto &vertex : mesh.vertices)
    {
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
  mVertBuf   = dx::CreateVertexBuffer<VertexFormat>(
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
    PerMeshData        bbDebugData{
             .transform = glm::translate(
        glm::scale(scene.model.transforms[i], bb.GetScale()),
        glm::vec3{bb.max + bb.min} / 2.0f),
    };
    mBBTransforms.emplace_back(bbDebugData.transform);
    mModelConstants.emplace_back(dx::CreateConstantBuffer<PerMeshData>(mDevice, &data));
    mBBDebugModelConstants.emplace_back(
      dx::CreateConstantBuffer<PerMeshData>(mDevice, &bbDebugData));
  }
}

void Parallax::Draw(
  u32                   width,
  u32                   height,
  ID3D11DeviceContext3 *ctx,
  const glm::dmat4     &cameraProjection,
  const glm::dvec3     &modelPos,
  dx::RenderContext    &renderContext,
  ShaderWatcher        &shaderWatcher)
{
  auto *annotation = renderContext.annotation.Get();

  D3D11_MAPPED_SUBRESOURCE mapped{};
  dx::ThrowIfFailed(ctx->Map(mConstantBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
  SceneData data{
    .modelView = cameraProjection * glm::translate(glm::identity<glm::dmat4>(), modelPos),
  };
  memcpy(mapped.pData, (void *)&data, sizeof(SceneData));
  ctx->Unmap(mConstantBuf.Get(), 0);

  D3D11_MAPPED_SUBRESOURCE mapped2{};
  dx::ThrowIfFailed(
    ctx->Map(mBBDebugConstantBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped2));
  SceneData *data2 = reinterpret_cast<SceneData *>(mapped2.pData);
  data2->modelView = cameraProjection; // * glm::translate(glm::identity<glm::dmat4>(), modelPos);
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
    ctx->OMSetRenderTargets(
      1,
      renderContext.backbufferRTV.GetAddressOf(),
      renderContext.depthStencilView.Get());
    for (u32 i = 0; i < mDraws.size(); i++)
    {
      const DrawOffsets draw = mDraws[i];
      ctx->VSSetConstantBuffers(1, 1, mModelConstants[i].GetAddressOf());
      ctx->DrawIndexed(draw.indexCount, draw.startIndex, draw.baseVertex);
    }
    annotation->EndEvent();
  };

  const auto DrawBBDebug = [&]() {
    annotation->BeginEvent(L"DrawBBDebug");
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
    ctx->OMSetRenderTargets(
      1,
      renderContext.backbufferRTV.GetAddressOf(),
      renderContext.depthStencilView.Get());
    for (u32 i = 0; i < mDraws.size(); i++)
    {
      const DrawOffsets draw = mDraws[i];
      ctx->VSSetConstantBuffers(1, 1, mBBDebugModelConstants[i].GetAddressOf());
      ctx->DrawIndexed(mBBDebugIndexCount, 0, 0);
    }
    annotation->EndEvent();
  };

  const auto DrawQuadDebug = [&]() {
    annotation->BeginEvent(L"DrawQuadDebug");
    RenderProgram        rp  = shaderWatcher.GetRenderProgram(mQuadDebugShadersHandle);
    ID3D11DeviceContext *ctx = renderContext.DeviceContext();

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetIndexBuffer(mQuadIndexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
    u32 stride = sizeof(glm::vec4);
    u32 offset = 0;
    ctx->IASetVertexBuffers(0, 1, mQuadVertBuf.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(rp.inputLayout);
    ctx->VSSetShader(rp.vertexShader, nullptr, 0);
    // ctx->VSSetConstantBuffers(0, 1, mBBDebugConstantBuf.GetAddressOf());

    ctx->RSSetState(mBBDebugRSState.Get());

    ctx->PSSetShader(rp.pixelShader, nullptr, 0);
    ctx->OMSetRenderTargets(
      1,
      renderContext.backbufferRTV.GetAddressOf(),
      renderContext.depthStencilView.Get());
    for (u32 i = 0; i < mDraws.size(); i++)
    {
      const DrawOffsets draw = mDraws[i];
      // ctx->VSSetConstantBuffers(1, 1, mBBDebugModelConstants[i].GetAddressOf());
      ctx->DrawIndexed(6, 0, 0);
    }
    annotation->EndEvent();
  };

  const auto GetBoundingQuad = [&]() {
    std::array<glm::vec2, 8> clipSpace;
    std::array<glm::vec3, 8> transformedVerts;
    glm::mat4                clipSpaceToPixelCoords = glm::identity<glm::mat4>();

    clipSpaceToPixelCoords[0][0] = width / 2.0;
    clipSpaceToPixelCoords[3][0] = width / 2.0;
    clipSpaceToPixelCoords[1][1] = height / 2.0;
    clipSpaceToPixelCoords[3][1] = height / 2.0;
    const glm::dmat4 model       = glm::translate(glm::identity<glm::dmat4>(), modelPos);

    glm::vec2 pMin{std::numeric_limits<f32>::max()};
    glm::vec2 pMax{std::numeric_limits<f32>::min()};
    for (size_t i = 0; i < mScene.model.parts.size(); i++)
    {
      const glm::dmat4 mvp = cameraProjection * glm::dmat4{data.modelView} * mBBTransforms[i];
      for (size_t vertIndex = 0; vertIndex < sBBVertices.size(); vertIndex++)
      {
        const glm::dvec4 position = glm::dvec4{sBBVertices[vertIndex].position, 1.0};
        const glm::vec4  transformedPosition =
          glm::vec4{glm::dmat4{data.modelView} * mBBTransforms[i] * position};

        transformedVerts[vertIndex] = model * mBBTransforms[i] * glm::vec4{position};
        clipSpace[vertIndex] =
          glm::vec2{clipSpaceToPixelCoords * transformedPosition} / transformedPosition.w;
      }

      glm::vec2 cMin{std::numeric_limits<f32>::max()};
      glm::vec2 cMax{std::numeric_limits<f32>::min()};
      for (const auto &p : transformedVerts)
      {
        cMin = glm::min(cMin, glm::vec2{p});
        cMax = glm::max(cMax, glm::vec2{p});
      }

#if 0
      D3D11_MAPPED_SUBRESOURCE mapped{};
      dx::ThrowIfFailed(ctx->Map(mQuadVertBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
      std::span verts = {reinterpret_cast<glm::vec4 *>(mapped.pData), 4};
      verts[0]        = /*glm::mat4{cameraProjection} **/ glm::vec4{cMin.x, cMax.y, 0.0f, 1.0};
      verts[1]        = /*glm::mat4{cameraProjection} **/ glm::vec4{cMin.x, cMin.y, 0.0f, 1.0};
      verts[2]        = /*glm::mat4{cameraProjection} **/ glm::vec4{cMax.x, cMax.y, 0.0f, 1.0};
      verts[3]        = /*glm::mat4{cameraProjection} **/ glm::vec4{cMax.x, cMin.y, 0.0f, 1.0};
      ctx->Unmap(mQuadVertBuf.Get(), 0);
#endif
      // Note: we don't care if the transformed points go beyond the clipping planes, since we
      // just want the width and height for the raster texture

      for (const auto &p : clipSpace)
      {
        pMin = glm::min(pMin, p);
        pMax = glm::max(pMax, p);
      }

      D3D11_MAPPED_SUBRESOURCE mapped{};
      dx::ThrowIfFailed(ctx->Map(mQuadVertBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
      std::span verts = {reinterpret_cast<glm::vec4 *>(mapped.pData), 4};
      verts[0] = glm::vec4{(pMin.x / width) * 2.0 - 1.0, (pMax.y / height) * 2.0 - 1.0, 0.0f, 1.0};
      verts[1] = glm::vec4{(pMin.x / width) * 2.0 - 1.0, (pMin.y / height) * 2.0 - 1.0, 0.0f, 1.0};
      verts[2] = glm::vec4{(pMax.x / width) * 2.0 - 1.0, (pMax.y / height) * 2.0 - 1.0, 0.0f, 1.0};
      verts[3] = glm::vec4{(pMax.x / width) * 2.0 - 1.0, (pMin.y / height) * 2.0 - 1.0, 0.0f, 1.0};
      ctx->Unmap(mQuadVertBuf.Get(), 0);
    }
    const glm::vec2 dim = pMax - pMin;
    return glm::uvec2{dim};
  };

  auto RenderToQuad = [&](glm::uvec2 size) {
    annotation->BeginEvent(L"RenderToQuad");
    RenderProgram        rp           = shaderWatcher.GetRenderProgram(mShadersHandle);
    ID3D11DeviceContext *ctx          = renderContext.DeviceContext();
    f32                  clearColor[] = {0.5, 0.5, 0.5, 1.0};
    ctx->ClearRenderTargetView(mQuadTarget.Get(), clearColor);
    ctx->ClearDepthStencilView(mQuadDepthView.Get(), D3D11_CLEAR_DEPTH, 1.0, 0);

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
    ctx->OMSetRenderTargets(1, mQuadTarget.GetAddressOf(), mQuadDepthView.Get());
    for (u32 i = 0; i < mDraws.size(); i++)
    {
      const DrawOffsets draw = mDraws[i];
      ctx->VSSetConstantBuffers(1, 1, mModelConstants[i].GetAddressOf());
      ctx->DrawIndexed(draw.indexCount, draw.startIndex, draw.baseVertex);
    }
    ID3D11RenderTargetView *np = nullptr;
    ctx->OMSetRenderTargets(1, &np, nullptr);
    // ctx->OMSetRenderTargets(
    //   1,
    //   renderContext.backbufferRTV.GetAddressOf(),
    //   renderContext.depthStencilView.Get());
    annotation->EndEvent();
  };

  auto DrawTexturedQuad = [&]() {
    annotation->BeginEvent(L"DrawTexturedQuad");
    D3D11_MAPPED_SUBRESOURCE mapped{};
    dx::ThrowIfFailed(
      ctx->Map(mTexQuadConstantBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
    TextureQuadCB *data = reinterpret_cast<TextureQuadCB *>(mapped.pData);
    data->mvp =
      cameraProjection * glm::translate(glm::identity<glm::dmat4>(), mScene.model.position);
    ctx->Unmap(mTexQuadConstantBuf.Get(), 0);

    RenderProgram        rp  = shaderWatcher.GetRenderProgram(mTexQuadShaderHandle);
    ID3D11DeviceContext *ctx = renderContext.DeviceContext();

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetIndexBuffer(mQuadIndexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
    u32 stride = sizeof(TexturedQuadVertex);
    u32 offset = 0;
    ctx->IASetVertexBuffers(0, 1, mTexturedQuadVertBuf.GetAddressOf(), &stride, &offset);
    ctx->IASetInputLayout(rp.inputLayout);
    ctx->VSSetShader(rp.vertexShader, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, mTexQuadConstantBuf.GetAddressOf());

    ctx->RSSetState(renderContext.rasterizerState.Get());

    ctx->PSSetShader(rp.pixelShader, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, mQuadView.GetAddressOf());
    ctx->PSSetSamplers(0, 1, mTexQuadSamplerState.GetAddressOf());
    ctx->OMSetRenderTargets(
      1,
      renderContext.backbufferRTV.GetAddressOf(),
      renderContext.depthStencilView.Get());
    ctx->DrawIndexed(6, 0, 0);
    ID3D11ShaderResourceView *np = nullptr;
    ctx->PSSetShaderResources(0, 1, &np);
    annotation->EndEvent();
  };

  // DrawModel();
  // DrawBBDebug();
  const glm::uvec2 size = GetBoundingQuad();
  ImGui::Text("Quad Size: (%d, %d)", size.x, size.y);
  // Need to have logic for when the quad is larger than the screen.
  // When that occurs, clamp the texture size to the screen size, but still render to the quad
  DrawQuadDebug();
  RenderToQuad(size);
  DrawTexturedQuad();
}

} // namespace methods
