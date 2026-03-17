#include "Parallax.hpp"

#include <array>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

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
          .Format               = DXGI_FORMAT_R32G32B32_UINT,
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
          .Format               = DXGI_FORMAT_R32G32B32_UINT,
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
    mBBDebugConstantBuf{dx::CreateConstantBuffer<SceneData>(device, nullptr)}
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
    PerMeshData bbDebugData{
      .transform =
        glm::scale(scene.model.transforms[i], scene.model.parts[i].boundingBox.GetScale()),
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
  data2->modelView = cameraProjection * glm::translate(glm::identity<glm::dmat4>(), modelPos);
  ctx->Unmap(mBBDebugConstantBuf.Get(), 0);

  const auto DrawModel = [&]() {
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
  };

  const auto DrawBBDebug = [&]() {
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
  };

  const auto GetBoundingQuad = [&]() {
    std::array<glm::vec2, 8> clipSpace;
    glm::mat4                clipSpaceToPixelCoords = glm::identity<glm::mat4>();

    clipSpaceToPixelCoords[0][0] = width / 2.0;
    clipSpaceToPixelCoords[3][0] = width / 2.0;
    clipSpaceToPixelCoords[1][1] = height / 2.0;
    clipSpaceToPixelCoords[3][1] = height / 2.0;

    for (size_t i = 0; i < mScene.model.parts.size(); i++)
    {
      const glm::dmat4 mvp = cameraProjection * glm::dmat4{data.modelView} * mBBTransforms[i];
      for (size_t vertIndex = 0; vertIndex < sBBVertices.size(); vertIndex++)
      {
        const glm::dvec4 position            = glm::dvec4{sBBVertices[vertIndex].position, 1.0};
        const glm::vec4  transformedPosition = clipSpaceToPixelCoords * glm::vec4{mvp * position};
        clipSpace[vertIndex] = glm::vec2{transformedPosition} / transformedPosition.w;
      }
      // Note: we don't care if the transformed points go beyond the clipping planes, since we just
      // want the width and height for the raster texture

      glm::vec2 pMin{std::numeric_limits<f32>::max()};
      glm::vec2 pMax{std::numeric_limits<f32>::min()};
      for (const auto &p : clipSpace)
      {
        pMin = glm::min(pMin, p);
        pMax = glm::min(pMax, p);
      }

      const glm::vec2 dim = pMax - pMin;
    }
  };

  DrawModel();
  DrawBBDebug();
  GetBoundingQuad();
}

} // namespace methods
