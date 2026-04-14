#include "RteGpu.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace methods
{

struct SceneData
{
  glm::mat4 viewProjection;
  glm::vec4 cameraPosHigh;
  glm::vec4 cameraPosLow;
  glm::vec4 worldHigh;
  glm::vec4 worldLow;
};

struct PerMeshData
{
  glm::dmat4 transform{};
};

struct RteGpuVertex
{
  glm::vec3 low{};
  glm::vec3 high{};
  glm::vec3 normal{};
  glm::vec2 textureCoord{};
};

using VertexFormat = RteGpuVertex;

RteGpuMethod::RteGpuMethod(ID3D11Device3 *device, ShaderWatcher &shaderWatcher) :
    mShadersHandle{shaderWatcher.RegisterShader(
      VERT_PATH,
      PIXEL_PATH,
      {
        {
          .SemanticName         = "POSITION_LOW",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_UINT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(VertexFormat, low),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "POSITION_HIGH",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_UINT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(VertexFormat, high),
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
    mDevice{device}
{
}

static void ToSplitDouble(const glm::dvec3 &v, glm::vec3 &lo, glm::vec3 &hi)
{
  const double SHORT_DOUBLE = 65536.0;
  for (u32 i = 0; i < 3; i++)
  {
    if (v[i] >= 0.0)
    {
      double doubleHigh = std::floor(v[i] / SHORT_DOUBLE) * SHORT_DOUBLE;
      hi[i]             = static_cast<float>(doubleHigh);
      lo[i]             = static_cast<float>(v[i] - doubleHigh);
    }
    else
    {
      double doubleHigh = std::floor(-v[i] / SHORT_DOUBLE) * SHORT_DOUBLE;
      hi[i]             = static_cast<float>(-doubleHigh);
      lo[i]             = static_cast<float>(v[i] + doubleHigh);
    }
  }
}

void RteGpuMethod::SetScene(const Scene &scene)
{
  mDraws          = {};
  mModelConstants = {};
  std::vector<VertexFormat> vertices;
  std::vector<u32>          indices;

  u32 vertexStart = 0;
  u32 startIndex  = 0;
  for (auto &mesh : scene.model.parts)
  {
    for (auto &vertex : mesh.vertices)
    {
      VertexFormat v{};

      ToSplitDouble(vertex.position, v.low, v.high);
      v.normal       = vertex.normal;
      v.textureCoord = vertex.textureCoord;
      vertices.push_back(v);
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
    mModelConstants.emplace_back(dx::CreateConstantBuffer<PerMeshData>(mDevice, &data));
  }
}

void RteGpuMethod::Update(
  ID3D11DeviceContext3 *ctx,
  const glm::dmat4     &projection,
  const glm::dmat4     &camera,
  const glm::dvec3     &cameraPos,
  const glm::dvec3     &modelPos)
{
  D3D11_MAPPED_SUBRESOURCE mapped{};
  dx::ThrowIfFailed(ctx->Map(mConstantBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
  auto modelView = glm::inverse(camera) * glm::translate(glm::identity<glm::dmat4>(), modelPos);
  // clang-format off
  glm::dmat4 mv = {
    modelView[0][0],modelView[1][0],modelView[2][0], 0.0,
    modelView[0][1],modelView[1][1],modelView[2][1], 0.0,
    modelView[0][2],modelView[1][2],modelView[2][2], 0.0,
    modelView[0][3],modelView[1][3],modelView[2][3],modelView[3][3],
  };
  // clang-format on
  SceneData data{
    .viewProjection = projection * mv,
  };
  glm::vec3 cHigh, cLow;
  ToSplitDouble(cameraPos, cHigh, cLow);
  glm::vec3 wHigh, wLow;
  ToSplitDouble(modelPos, wHigh, wLow);
  data.cameraPosHigh = {cHigh, 1.0};
  data.cameraPosLow  = {cLow, 1.0};
  data.worldHigh     = {wHigh, 1.0};
  data.worldLow      = {wLow, 1.0};
  memcpy(mapped.pData, (void *)&data, sizeof(SceneData));
  ctx->Unmap(mConstantBuf.Get(), 0);
}

void RteGpuMethod::Draw(
  dx::RenderContext      &renderContext,
  ShaderWatcher          &shaderWatcher,
  ID3D11RenderTargetView *targetView,
  bool                    recordDrawTime,
  u32                     testFrameCount)
{
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
  if (recordDrawTime)
  {
    renderContext.DeviceContext()->Begin(mGpuDisjointQueries[testFrameCount].Get());
    renderContext.DeviceContext()->End(mGpuStarts[testFrameCount].Get());
  }
  for (u32 i = 0; i < mDraws.size(); i++)
  {
    const DrawOffsets draw = mDraws[i];
    ctx->VSSetConstantBuffers(1, 1, mModelConstants[i].GetAddressOf());
    ctx->DrawIndexed(draw.indexCount, draw.startIndex, draw.baseVertex);
  }
  if (recordDrawTime)
  {
    renderContext.DeviceContext()->End(mGpuEnds[testFrameCount].Get());
    renderContext.DeviceContext()->End(mGpuDisjointQueries[testFrameCount].Get());
  }
}

std::vector<double> RteGpuMethod::GetTimingData(ID3D11DeviceContext3 *ctx)
{
  std::vector<double> times;
  for (size_t i = 0; i < mGpuDisjointQueries.size(); i++)
  {
    dx::WaitForQueryToBeReady(ctx, mGpuDisjointQueries[i].Get());

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
    ctx->GetData(mGpuDisjointQueries[i].Get(), &disjoint, sizeof(disjoint), 0);

    u64 start{}, end{};
    ctx->GetData(mGpuStarts[i].Get(), &start, sizeof(u64), 0);
    ctx->GetData(mGpuEnds[i].Get(), &end, sizeof(u64), 0);

    double time =
      static_cast<double>(end - start) / static_cast<double>(disjoint.Frequency) * 1000.0;
    times.push_back(time);
  }
  return times;
}

} // namespace methods