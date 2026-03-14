float __fmul_rn(float a, float b)
{
  return a * b;
}

struct SS
{
  float hs;
  float ls;
};

SS add_knuth_and_dekker1(float xs, float ys)
{
  float ws;
  SS z;

  z.hs = xs + ys;
  ws = z.hs - xs;
  z.ls = ys - ws;

  return z;
}

SS add_knuth_and_dekker2(float xs, float ys)
{
  float ws, vs, z1s, z2s;
  SS z;

  z.hs = xs + ys;
  ws = z.hs - xs;
  z1s = ys - ws;
  vs = z.hs - ws;
  z2s = vs - xs;
  z.ls = z1s - z2s;

  return z;
}

SS dsadd_full(SS a, SS b)
{
  float rs;
  SS z, c;

  z = add_knuth_and_dekker2(a.hs, b.hs);
  rs = a.ls + b.ls + z.ls;
  c = add_knuth_and_dekker1(z.hs, rs);

  return c;
}

SS dssub_full(SS a, SS b)
{
  SS c;
  float t1 = a.hs - b.hs;
  float e = t1 - a.hs;
  float t2 = ((-b.hs - e) + (a.hs - (t1 - e))) + a.ls - b.ls;
  c = add_knuth_and_dekker1(t1, t2);

  return c;
}

SS dsmul_full(SS a, SS b)
{
  SS sa, sb, c, t, d;
  float cona, conb, c2;

  cona = a.hs * 8193.0f;
  conb = b.hs * 8193.0f;
  sa.hs = cona - (cona - a.hs);
  sb.hs = conb - (conb - b.hs);
  sa.ls = a.hs - sa.hs;
  sb.ls = b.hs - sb.hs;
  c.hs = __fmul_rn(a.hs, b.hs);
  c.ls = (((sa.hs * sb.hs - c.hs) + sa.hs * sb.ls) + sa.ls * sb.hs) + sa.ls * sb.ls;

  c2 = __fmul_rn(a.hs, b.ls) + __fmul_rn(a.ls, b.hs);
  t = add_knuth_and_dekker2(c.hs, c2);
  t.ls += c.ls + __fmul_rn(a.ls, b.ls);

  d = add_knuth_and_dekker1(t.hs, t.ls);

  return d;
}

//SS dsadd_F1(SS a, SS b)
//{
//  SS c;
//
//  c = add_knuth_and_dekker2(a.hs, b.hs);
//  c.ls += a.ls + b.ls;
//
//  return c;
//}

SS dsadd_F23(SS a, SS b)
{
  SS c;

  c = add_knuth_and_dekker1(a.hs, b.hs);
  c.ls = c.ls + a.ls + b.ls;

  return c;
}

//SS dssub_F1(SS a, SS b)
//{
//  SS c;
//
//  c.hs = a.hs - b.hs;
//  float e = c.hs - a.hs;
//  c.ls = ((-b.hs - e) + (a.hs - (c.hs - e))) + a.ls - b.ls;
//  return c;
//}

SS dssub_F23(SS a, SS b)
{
  SS c;

  c.hs = a.hs - b.hs;
  float e = c.hs - a.hs;
  c.ls = (-b.hs - e) + a.ls - b.ls;

  return c;
}

//SS dsmul_F12(SS a, SS b)
//{
//  SS sa, sb, c, t, d;
//  float c2;
//  sa.hs = __int_as_float(__float_as_int(a.hs) & 0xfffff000);
//  sb.hs = __int_as_float(__float_as_int(b.hs) & 0xfffff000);
//  sa.ls = a.hs - sa.hs;
//  sb.ls = b.hs - sb.hs;
//  c.hs = __fmul_rn(a.hs, b.hs);
//  c.ls = (((sa.hs * sb.hs - c.hs) + sa.hs * sb.ls) + sa.ls * sb.hs) + sa.ls * sb.ls;
//  c2 = __fmul_rn(a.hs, b.ls) + __fmul_rn(a.ls, b.hs);
//  c.ls = c.ls + c2 + __fmul_rn(a.ls, b.ls);
//  d = add_knuth_and_dekker1(c.hs, c.ls);
//
//  return d;
//}

SS dsmul_F3(SS a, SS b)
{
  SS sa, sb, t, d;
  //sa.hs = __int_as_float(__float_as_int(a.hs) & 0xfffff000);
  //sb.hs = __int_as_float(__float_as_int(b.hs) & 0xfffff000);
  sa.hs = asfloat(asint(a.hs) & 0xfffff000);
  sb.hs = asfloat(asint(b.hs) & 0xfffff000);
  sa.ls = (a.hs - sa.hs) + a.ls;
  sb.ls = (b.hs - sb.hs) + b.ls;
  t.hs = sa.hs * sb.hs;
  t.ls = sa.hs * sb.ls + sb.hs * sa.ls;
  d = add_knuth_and_dekker1(t.hs, t.ls);
  d.ls = d.ls + sa.ls * sb.ls;

  return d;
}

SS dsmul(SS a, SS b)
{
  return dsmul_F3(a, b);
}

SS dsadd(SS a, SS b)
{
  return dsadd_F23(a, b);
}

struct SceneData
{
  float4x4 mvpHigh;
  float4x4 mvpLow;
};

cbuffer Constants : register(b0)
{
  SceneData sceneData;
};

struct PerMesh
{
  float4x4 transformHigh;
  float4x4 transformLow;
};

cbuffer Constants : register(b1)
{
  PerMesh perMesh;
};

struct Vertex
{
  float3 posHigh : POSITION_HIGH;
  float3 posLow : POSITION_LOW;
  float3 normal : NORMAL;
  float2 textureCoord : TEXCOORD;
};

struct VSOut
{
  float4 pos : SV_Position;
  float3 color : COLOR;
};

struct EmulatedDouble3
{
  float3 high;
  float3 low;
};

struct EmulatedDouble4
{
  float4 high;
  float4 low;
};

struct EmulatedDouble4x4
{
  float4x4 high;
  float4x4 low;
};

EmulatedDouble4x4 EDMul(EmulatedDouble4x4 a, EmulatedDouble4x4 b)
{
  EmulatedDouble4x4 p;

  for (uint i = 0; i < 4; i++)
  {
    for (uint j = 0; j < 4; j++)
    {
      SS sum = { 0.0, 0.0 };
      for (uint k = 0; k < 4; k++)
      {
        SS as =
        {
          a.high[i][k],
          a.low[i][k],
        };

        SS bs =
        {
          b.high[k][j],
          b.low[k][j],
        };
        sum = dsadd(sum, dsmul(as, bs));
      }
      p.high[i][j] = sum.hs;
      p.low[i][j] = sum.ls;
    }
  }
  return p;
}

EmulatedDouble4 EDMul(EmulatedDouble4x4 a, EmulatedDouble4 b)
{
  EmulatedDouble4 p;
  for (uint i = 0; i < 4; i++)
  {
    SS sum = { 0.0, 0.0 };
    for (uint j = 0; j < 4; j++)
    {
      SS as = { a.high[i][j], a.low[i][j] };
      SS ab = { b.high[j], b.low[j] };
      sum = dsadd(sum, dsmul(as, ab));
    }
    p.high[i] = sum.hs;
    p.low[i] = sum.ls;
  }
  return p;
}

VSOut VSMain(Vertex vertex)
{
  VSOut ret = (VSOut) 0;

  EmulatedDouble4 pos =
  {
    float4(vertex.posHigh, 1.0),
    float4(vertex.posLow, 0.0),
  };

  EmulatedDouble4x4 mvp =
  {
    sceneData.mvpHigh,
    sceneData.mvpLow,
  };

  EmulatedDouble4x4 transform =
  {
    perMesh.transformHigh,
    perMesh.transformLow,
  };

  EmulatedDouble4x4 finalMVP = EDMul(mvp, transform);
  EmulatedDouble4 transformedPos = EDMul(finalMVP, pos);

  ret.pos = transformedPos.high + transformedPos.low;

  //ret.pos = (float4) mul(mvp, double4(pos, 1.0));
  ret.color = (vertex.normal + 1.0) / 2.0;
  return ret;
}

float4 PSMain(VSOut vsOut) : SV_TARGET
{
  return float4(vsOut.color, 1.0);
}
