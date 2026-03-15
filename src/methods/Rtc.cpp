#include "Rtc.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace methods
{

struct SceneData
{
  glm::mat4 mvpRtc;
};

struct RtcVertex
{
  glm::vec3 pos{};
  glm::vec3 normal{};
  glm::vec2 textureCoord{};
};

using VertexFormat = RtcVertex;

Rtc::Rtc(ID3D11Device3 *device, ShaderWatcher &shaderWatcher) :
    mShadersHandle{shaderWatcher.RegisterShader(
      VERT_PATH,
      PIXEL_PATH,
      {
        {
          .SemanticName         = "POSITION",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(VertexFormat, pos),
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
    mDevice{device}
{
}

void Rtc::SetScene(const Scene &scene)
{
  mDraws        = {};
  mConstantBufs = {};
  mCenterEyes   = {};
  std::vector<VertexFormat> vertices;
  std::vector<u32>          indices;

  auto GetCenter = [](const std::vector<ModelVertex> &vertices) {
    glm::dvec3 minPos{
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max()};
    glm::dvec3 maxPos{
      std::numeric_limits<float>::min(),
      std::numeric_limits<float>::min(),
      std::numeric_limits<float>::min()};

    for (auto &vertex : vertices)
    {
      minPos = glm::min(minPos, glm::dvec3{vertex.position});
      maxPos = glm::max(maxPos, glm::dvec3{vertex.position});
    }

    return (minPos + maxPos) / 2.0;
  };

  u32 vertexStart = 0;
  u32 startIndex  = 0;
  for (size_t i = 0; i < scene.model.parts.size(); i++)
  {
    const ModelMesh &mesh = scene.model.parts[i];
    for (auto &vertex : mesh.vertices)
    {
      VertexFormat v{};
      v.pos          = vertex.position;
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

    const glm::dvec3 centerEye = GetCenter(mesh.vertices);
    mCenterEyes.emplace_back(centerEye);
    mConstantBufs.emplace_back(dx::CreateConstantBuffer<SceneData>(mDevice, nullptr));
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
}

void Rtc::Update(
  ID3D11DeviceContext3 *ctx,
  const glm::dmat4     &projection,
  const glm::dmat4     &camera,
  const glm::dvec3     &modelPos)
{
  for (size_t i = 0; i < mCenterEyes.size(); i++)
  {
    glm::dmat4       mvpRtc    = glm::translate(camera, modelPos);
    const glm::dvec3 centerEye = mvpRtc * glm::dvec4{mCenterEyes[i], 1.0};
    mvpRtc[3]                  = glm::dvec4{centerEye, 1.0};
    D3D11_MAPPED_SUBRESOURCE mapped{};
    dx::ThrowIfFailed(
      ctx->Map(mConstantBufs[i].Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
    SceneData *data = reinterpret_cast<SceneData *>(mapped.pData);
    data->mvpRtc    = glm::mat4{projection * mvpRtc};
    ctx->Unmap(mConstantBufs[i].Get(), 0);
  }
}

void Rtc::Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher)
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

  ctx->RSSetState(renderContext.rasterizerState.Get());

  ctx->PSSetShader(rp.pixelShader, nullptr, 0);
  ctx->OMSetRenderTargets(
    1,
    renderContext.backbufferRTV.GetAddressOf(),
    renderContext.depthStencilView.Get());
  for (u32 i = 0; i < mDraws.size(); i++)
  {
    const DrawOffsets draw = mDraws[i];
    ctx->VSSetConstantBuffers(0, 1, mConstantBufs[i].GetAddressOf());
    ctx->DrawIndexed(draw.indexCount, draw.startIndex, draw.baseVertex);
  }
}

} // namespace methods