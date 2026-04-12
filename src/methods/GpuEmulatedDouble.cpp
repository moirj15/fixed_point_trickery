#include "GpuEmulatedDouble.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace methods
{

void DoubleToED(double d, float &high, float &low)
{
  high = static_cast<float>(d);
  low  = static_cast<float>(d - high);
}

struct EmulatedDoubleVec3
{
  glm::vec3 high;
  glm::vec3 low;
};

EmulatedDoubleVec3 DoubleToED(const glm::dvec3 &d)
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

EmulatedDoubleMat4 DoubleToED(const glm::dmat4 &d)
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

struct SceneData
{
  EmulatedDoubleMat4 viewProjection;
  EmulatedDoubleMat4 model;
};

struct PerMeshData
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

GpuEmulatedDoubleMethod::GpuEmulatedDoubleMethod(
  ID3D11Device3 *device,
  ShaderWatcher &shaderWatcher) :
    mShadersHandle{shaderWatcher.RegisterShader(
      VERT_PATH,
      PIXEL_PATH,
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
    mVertBuf{},
    mIndexBuf{},
    mConstantBuf{dx::CreateConstantBuffer<SceneData>(device, nullptr)},
    mDevice{device}
{
}

void GpuEmulatedDoubleMethod::SetScene(const Scene &scene)
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

      v.pos          = DoubleToED(glm::dvec3{vertex.position});
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
      .transform = DoubleToED(scene.model.transforms[i]),
    };
    mModelConstants.emplace_back(dx::CreateConstantBuffer<PerMeshData>(mDevice, &data));
  }
}

void GpuEmulatedDoubleMethod::Update(
  ID3D11DeviceContext3 *ctx,
  const glm::dmat4     &cameraProjection,
  const glm::dvec3     &modelPos)
{
  D3D11_MAPPED_SUBRESOURCE mapped{};
  dx::ThrowIfFailed(ctx->Map(mConstantBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
  SceneData data{
    .viewProjection = DoubleToED(cameraProjection),
    .model          = DoubleToED(glm::translate(glm::identity<glm::dmat4>(), modelPos)),
  };
  memcpy(mapped.pData, (void *)&data, sizeof(SceneData));
  ctx->Unmap(mConstantBuf.Get(), 0);
}

void GpuEmulatedDoubleMethod::Draw(
  dx::RenderContext      &renderContext,
  ShaderWatcher          &shaderWatcher,
  ID3D11RenderTargetView *targetView)
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
  ctx->OMSetRenderTargets(1, &targetView, renderContext.depthStencilView.Get());
  for (u32 i = 0; i < mDraws.size(); i++)
  {
    const DrawOffsets draw = mDraws[i];
    ctx->VSSetConstantBuffers(1, 1, mModelConstants[i].GetAddressOf());
    ctx->DrawIndexed(draw.indexCount, draw.startIndex, draw.baseVertex);
  }
}

} // namespace methods
